#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ArduinoJson.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include <Hash.h>

#include "voltage.h"

#include <SoftwareSerial.h>

ESP8266WiFiMulti WiFiMulti;
SocketIOclient socketIO;

#define ssid "ASUS_5"                                  // Название WiFi
#define password "Admin1512"                     // Пароль от WiFi

#define address_server "213.155.193.31"                 // Адрес сервера
#define port_server 4145                              // Порт сервера

IPAddress local_IP(192, 168, 1, 66);                  // Устанавливаем статический IP адрес
IPAddress gateway(192, 168, 1, 5);                    // Устанавливаем IP адрес шлюза
IPAddress subnet(255, 255, 255, 0);                   // Устанавливаем маску сети

#define RELAY1_PIN D1
#define RELAY2_PIN D2
#define RELAY_GSM_PIN D3
#define SENSOR1_PIN D5
#define SENSOR2_PIN D6
#define RELAY_SENSOR_PIN D4

#define GSM_RX 13 // D7
#define GSM_TX 15 // D8

#define update_relay 1 // минуты
#define update_sensor 20 // секунды

#define U1_voltage_level 3.6

String ip;

bool relay1 = true;
bool relay2 = true;

bool sensor1 = false;
bool sensor2 = false;

bool sensor1_flag = true;
bool sensor2_flag = true;

bool sensor_voltage1_flag = true;

float U1_voltage = 0.0;

int countTry = 0;                                     // Количество неудачных попыток авторизации
String _response = "";                                // Переменная для хранения ответов модуля
String result = "";                                   // Переменная для хранения вводимых данных через DTMF
const String pass = "1923";                           // Пароль для авторизации
bool flag_pre_auth = true;                            // Флаг авторизации

bool flag_init_gsm = true;
bool flag_server_life = false;

SoftwareSerial mySerial(GSM_RX, GSM_TX, false);               // Программный UART GSM модуля

// раздефайнить или задефайнить для использования
#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif



void setup() {
  pinMode(A0, INPUT);
  pinMode(SENSOR1_PIN, INPUT);
  pinMode(SENSOR2_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);                                           // Меняем режим работы пина на вывод сигнала
  pinMode(RELAY2_PIN, OUTPUT);                                           // Меняем режим работы пина на вывод сигнала
  pinMode(RELAY_GSM_PIN, OUTPUT);  
  pinMode(RELAY_SENSOR_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, relay1);                                           
  digitalWrite(RELAY2_PIN, relay2);                                    
  digitalWrite(RELAY_GSM_PIN, true);  
  digitalWrite(RELAY_SENSOR_PIN, true);               // Выключаем реле сенсеров


  U1_voltage = get_voltage(0);

  #ifdef DEBUG_ENABLE
    Serial.begin(9600);

    while (!Serial) {}

    DEBUG_PRINT(F("Connect! - Conection"));
  #endif

  init_websocketio_clint();

}

void loop() {
  socketIO.loop();
  handler_update_data_timer();
  handler_sensor_timer();

  if (!flag_init_gsm) {
    handler_gsm();
  }

  #ifdef DEBUG_ENABLE
    handler_serials();
  #endif
}


/* GSM */

// Отдельная функция для настройки gsm модуля
void init_gsm_model() {
  DEBUG_PRINT("[GSM DEBUG] -> Start Init");
  mySerial.begin(9600);

  byte i = 0;

  while(!mySerial) {}

  do {
    _response = sendATCommand(F("AT"), true);  // Включение DTMF (тонального набора)
    _response.trim();                       // Убираем пробельные символы в начале и конце
    i++;
  } while (_response != "OK" && i <= 5);              // Не пускать дальше, пока модем не вернет ОК
  

  i = 0;
  do {
    _response = sendATCommand(F("AT+CREG?"), true);                   // Проверка регистрации в сети
    _response.trim();                                                 // Убираем пробельные символы в начале и конце
    delay(100);
    i++;
  } while (_response.substring(_response.indexOf(F(",")) + 1, _response.length()).toInt() != 1 && i <= 5);


  i = 0;
  do {
    _response = sendATCommand(F("AT+DDET=1"), true);  // Включение DTMF (тонального набора)
    _response.trim();                       // Убираем пробельные символы в начале и конце
    i++;
  } while (_response != "OK" && i <= 5);              // Не пускать дальше, пока модем не вернет ОК

  i = 0;
  do {
    _response = sendATCommand(F("AT+CLIP=1"), true);  // Включаем АОН
    _response.trim();                       // Убираем пробельные символы в начале и конце
    i++;
  } while (_response != "OK" && i <= 5);              // Не пускать дальше, пока модем не вернет ОК

  i = 0;
  do {
    _response = sendATCommand(F("AT+CMGF=1"), true);         // Включить TextMode для SMS
    _response.trim();                       // Убираем пробельные символы в начале и конце
    i++;
  } while (_response != "OK" && i <= 5);              // Не пускать дальше, пока модем не вернет ОК


  if (i >= 6) {DEBUG_PRINT("[GSM DEBUG] -> Not Connect");}
  else {DEBUG_PRINT("[GSM DEBUG] -> Connect");}
  
}
 
// Отдельная функция для обработки данных поступаемых из UART
void handler_serials() {
  #ifdef DEBUG_ENABLE
    if (Serial.available()) {             // Ожидаем команды по Serial...
      mySerial.write(Serial.read());    // ...и отправляем полученную команду модему
    }
    if (mySerial.available()) { 
      Serial.write(mySerial.read()); 
    }
  #endif
}

// Отдельная функция для обработки данных поступаемых от GSM
void handler_gsm() {
  if (mySerial.available())   {                 // Если модем, что-то отправил...
    _response = waitResponse();                 // Получаем ответ от модема для анализа
    DEBUG_PRINT(_response);                  // Если нужно выводим в монитор порта

    int index = -1;
    do  {                                       // Перебираем построчно каждый пришедший ответ
      index = _response.indexOf("\r\n");        // Получаем идекс переноса строки
      String submsg = "";
      if (index > -1) {                         // Если перенос строки есть, значит
        submsg = _response.substring(0, index); // Получаем первую строку
        _response = _response.substring(index + 2); // И убираем её из пачки
      }
      else {                                    // Если больше переносов нет
        submsg = _response;                     // Последняя строка - это все, что осталось от пачки
        _response = "";                         // Пачку обнуляем
      }
      submsg.trim();                            // Убираем пробельные символы справа и слева
      if (submsg != "") {                       // Если строка значимая (не пустая), то распознаем уже её
        DEBUG_PRINT("submessage: " + submsg);
        if (submsg.startsWith("+DTMF:")) {      // Если ответ начинается с "+DTMF:" тогда:
          String symbol = submsg.substring(7, 8);  // Выдергиваем символ с 7 позиции длиной 1 (по 8)
          processingDTMF(symbol);               // Логику выносим для удобства в отдельную функцию
        }

        if (submsg.startsWith("RING")) {         // Есть входящий вызов
          int phoneindex = _response.indexOf("+CLIP: \""); // Есть ли информация об определении номера, если да, то phoneindex>-1
          String innerPhone = "";                   // Переменная для хранения определенного номера
          if (phoneindex >= 0) {                    // Если информация была найдена
            phoneindex += 8;                        // Парсим строку и ...
            innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // ...получаем номер
            DEBUG_PRINT("Number: " + innerPhone); // Выводим номер в монитор порта
          }
          // Проверяем, чтобы длина номера была больше 6 цифр, и номер должен быть в списке
          if (innerPhone.length() >= 7) {
            sendATCommand(F("ATA"), true);        // Если да, то отвечаем на вызов
          }
          else {
            sendATCommand(F("ATH"), true);        // Если нет, то отклоняем вызов
          }
        }

        if (submsg.startsWith(F("NO CARRIER"))) {                // Завершение звонка
          sendATCommand(F("ATH"), true); 
        }
        if (submsg.startsWith("+CMGS:")) {       // Пришло сообщение об отправке SMS
          int index = submsg.lastIndexOf("\r\n");// Находим последний перенос строки, перед статусом
          String result = submsg.substring(index + 2, submsg.length()); // Получаем статус
          result.trim();                            // Убираем пробельные символы в начале/конце

          if (result == "OK") {                     // Если результат ОК - все нормально
            DEBUG_PRINT(F("Message was sent. OK"));
          }
          else {                                    // Если нет, нужно повторить отправку
            DEBUG_PRINT(F("Message was not sent. Error"));
          }
        }
      }
    } while (index > -1);                       // Пока индекс переноса строки действителен
  }
}

// Отдельная функция для обработки логики DTMF
void processingDTMF(String symbol) {
  DEBUG_PRINT("Key: " + symbol);             // Выводим в Serial для контроля, что ничего не потерялось
  if (!flag_pre_auth) {
    if (symbol == "#") {
      bool correct;                             // Для оптимизации кода, переменная корректности команды
      correct = handler_command(result);
      if (!correct) DEBUG_PRINT("Incorrect command: " + result); // Если команда некорректна, выводим сообщение
      result = "";                                       // После каждой решетки сбрасываем вводимую комбинацию
    }
    else {
      result += symbol;                               // Если нет, добавляем в конец
    }
  }
  else {
   auth(symbol);
  }
}

// Отдельная функция для обработки комманд DTMF
bool handler_command(String command) {
  if (command == "111") {
    sendATCommand(F("AT+VTS=\"1,4\""), true);
    return true;
  }

  return false;
}

// Отдельная функция для обработки авторизации
void auth(String symbol) {
  if (countTry < 3) {                                 // Если 3 неудачных попытки, перестаем реагировать на нажатия
    DEBUG_PRINT((String)"[GSM DEBUG AUTH]" + result);
    if (symbol == "#") {
      bool correct = false;  
      if (result == pass) {                           // Введенная строка совпадает с заданным паролем
        DEBUG_PRINT("The correct password is entered: " + result); // Информируем о корректном вводе пароля
        countTry = 0;                                 // Обнуляем счетчик неудачных попыток ввода
        flag_pre_auth = false;
        sendATCommand(F("AT+VTS=\"1,4\""), true);
      }
      else {
        countTry += 1;                                // Увеличиваем счетчик неудачных попыток на 1
        DEBUG_PRINT(F("Incorrect password"));         // Неверный пароль
        DEBUG_PRINT("Counter:" + (String)countTry);// Количество неверных попыток
      }
      result = "";                                    // После каждой решетки сбрасываем вводимую комбинацию 
    }
    else {
      result += symbol;
    }
  }
  else {
    sendATCommand(F("ATH"), true);        // Если количество неудачных папыток привысило 3, то отклоняем вызов
  }
}

// Отдельная функция для отправки комманд в GSM
String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  DEBUG_PRINT("[GSM DEBUG] -> " + cmd);                          // Дублируем команду в монитор порта
  mySerial.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse();                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {  // Убираем из ответа дублирующуюся команду
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    DEBUG_PRINT("[GSM DEBUG] -> " + _resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}

// Отдельная функция для получения ответа от GSM
String waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!mySerial.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (mySerial.available()) {                     // Если есть, что считывать...
    _resp = mySerial.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    DEBUG_PRINT((String)"[GSM DEBUG] -> " + "Timeout...");               // ... оповещаем об этом и...
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}

// Отдельная функция для отправки sms
void sendSMS(String phone, String message) {
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
}

/* GSM */




/* WebSocketClientSocketIO */

// Инициализация клиента websocketio
void init_websocketio_clint() {
  /* Функция для настройки подключения WebSocketIO Clienta */
  WiFi.disconnect();

  WiFi.config(local_IP, gateway, subnet);

  // disable AP
  if(WiFi.getMode() & WIFI_AP) {
      WiFi.softAPdisconnect(true);
  }
  
  WiFiMulti.addAP(ssid, password);

  while(WiFiMulti.run() != WL_CONNECTED) {
      DEBUG_PRINT(F("."));
      delay(100);
  }

  ip = WiFi.localIP().toString();
  // server address, port and URL
  socketIO.begin(address_server, port_server, "/socket.io/?EIO=4");

  // event handler
  socketIO.onEvent(socketIOEvent);
}

// Обработчик событий
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  /* Функция обробатывающая события WebSocketIO */
  switch(type) {
    case sIOtype_DISCONNECT: {
      DEBUG_PRINT(F("[WebSocketIO DEBUG] -> DISCONNECT"));
      if (flag_init_gsm) {
        flag_init_gsm = false;
        digitalWrite(RELAY_GSM_PIN, false); 
        delay(10000); 
        init_gsm_model();
      }

      if (flag_server_life) {
        flag_server_life = false;
      }
      break;
    }
    case sIOtype_CONNECT: {
      DEBUG_PRINT(F("[WebSocketIO DEBUG] -> CONNECT"));
      socketIO.send(sIOtype_CONNECT, "/");
      delay(100);
      join_room_event();
      delay(100);
      get_relay_parameters();
      delay(100);

      flag_server_life = true;
      flag_init_gsm = true; 

      digitalWrite(RELAY_GSM_PIN, true);  

      break;
    }
    case sIOtype_EVENT: {
      handler_commands(payload, length);
      break;
    }
  }
}

// Обработчик команд
void handler_commands(uint8_t * payload, size_t length) {
  /* Обработчик комман */
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) { 
    return; 
  }
  String eventName = doc[0];
    
  if (ip == doc[1]["ip"]) {
    String chanel_name = doc[1]["data"]["channel"]["name"];
    String action = doc[1]["data"]["channel"]["action"];

    if (eventName == "control_sensor") {
      handler_sensor(chanel_name, action);
    } 
    
    if (eventName == "control_relay") {
      handler_relay(chanel_name, action);
    }
    
    get_relay_parameters();

    if (eventName == "control_voltage") {
      handler_voltage(chanel_name, action);
    }
  }
}

// Обработчик сенсеров
void handler_sensor(String chanel_name, String action) {
  /* Обработчик сенсеров */
  if (chanel_name == "S1") {
    if (action == "toggle") {
      sensor1 = !sensor1;
    }
  } 
  
  if (chanel_name == "S2") {
    if (action == "toggle") {
      sensor2 = !sensor2;
    } 
  }

  if (sensor1 || sensor2) {
    digitalWrite(RELAY_SENSOR_PIN, false);
  } else {
    digitalWrite(RELAY_SENSOR_PIN, true); 
  }
}

// Обработчик реле
void handler_relay(String chanel_name, String action) {
  /* Обработчик реле */
  if (chanel_name == "R1") {
    if (action == "toggle") {
      relay1 = !relay1;  
      digitalWrite(RELAY1_PIN, relay1);
    } 
    if (action == "restart") {
      relay1 = false;
      digitalWrite(RELAY1_PIN, LOW);
      get_relay_parameters();
      relay1 = true;
      delay(10000);
      digitalWrite(RELAY1_PIN, HIGH);  
    }
  } 
  if (chanel_name == "R2") {
    if (action == "toggle") {
      relay2 = !relay2;
      digitalWrite(RELAY2_PIN, relay2); 
    } 
    if (action == "restart") {
      relay2 = false;
      digitalWrite(RELAY2_PIN, LOW);
      get_relay_parameters();
      relay2 = true;
      delay(10000);
      digitalWrite(RELAY2_PIN, HIGH);  
    }
  }
}

// Обработчик напряжений
void handler_voltage(String chanel_name, String action) {
  /* Обработчик напряжений */
  if (chanel_name == "U1") {
    if (action == "measure") {
      U1_voltage = get_voltage(0);
    }
  }
}

// Обработчик занимающийся отправкой данных на сервер
void handler_update_data_timer() {
  static uint32_t tmr;

  if(millis() - tmr > (update_relay * 60 * 1000)) {
    tmr = millis();
    U1_voltage = get_voltage(0);
    voltage_monitoring();
    get_relay_parameters();
  }
}

// функция для контроля напряжения
void voltage_monitoring() {
  DynamicJsonDocument doc(1024);

  doc[0] = "voltage_low_signal";

  doc[1]["ip"] = ip;
  
  if (U1_voltage < U1_voltage_level && sensor_voltage1_flag) {
    doc[1]["chanel"]["name"] = "U1";
    doc[1]["chanel"]["voltage"] = U1_voltage;
    sensor_voltage1_flag = false;
    String buffer;
    serializeJson(doc, buffer);

    DEBUG_PRINT(F("U1 Voltage low"));

    // Send event
    socketIO.sendEVENT(buffer);
  }
}

// Обработчик занимающийся опросом сенсеров
void handler_sensor_timer() {
  static uint32_t tmr;

  if(millis() - tmr > (update_sensor * 1000)) {
    tmr = millis();

    /* Функция для добавления пользователя в группу relay */
    DynamicJsonDocument doc(1024);

    doc[0] = "alarm_signal";

    doc[1]["ip"] = ip;
      
    if (sensor1 && digitalRead(SENSOR1_PIN) && sensor1_flag) {
      doc[1]["chanel"]["name"] = "S1";
      doc[1]["chanel"]["description"] = "Датчик открытия двери";
      sensor1_flag = false;
      String buffer;
      serializeJson(doc, buffer);

      DEBUG_PRINT(F("Sensor 1 Alarm"));

      // Send event
      socketIO.sendEVENT(buffer);
      get_relay_parameters();
    }
    
    if (sensor2 && !digitalRead(SENSOR2_PIN) && sensor2_flag) {
      doc[1]["chanel"]["name"] = "S2";
      doc[1]["chanel"]["description"] = "Датчик наклона";
      sensor2_flag = false;
      String buffer;
      serializeJson(doc, buffer);

      DEBUG_PRINT(F("Sensor 2 Alarm"));

      // Send event
      socketIO.sendEVENT(buffer);
      get_relay_parameters();
    } 
      
    if (digitalRead(SENSOR1_PIN) == LOW) {sensor1_flag = true;}
    if (digitalRead(SENSOR2_PIN) == HIGH) {sensor2_flag = true;}
  }
}

// Функция для отправки события о подключение к комнате на сервер
void join_room_event() {
  /* Функция для добавления пользователя в группу relay */
  DynamicJsonDocument doc(1024);

  doc[0] = "join_room";

  doc[1]["username"] = (String)"relay_" + ip;
  doc[1]["room"] = "relay";

  // JSON to String (serializion)
  String buffer;
  serializeJson(doc, buffer);

  // Send event
  socketIO.sendEVENT(buffer);
}

// Функция для отправки данных на сервер
void get_relay_parameters() {
  if (!flag_server_life) { return; }
  /* Формирования JSON для оправки данных на сервер через WebSocketIO */
  DynamicJsonDocument doc(1024);
  
  /* Событие */
  doc[0] = "relay_parameters";

  doc[1]["data"]["ip_relay"] = ip;

  doc[1]["data"]["voltage_info"][0]["name"] = "U1";
  doc[1]["data"]["voltage_info"][0]["voltage"] = U1_voltage;

  doc[1]["data"]["relay_info"][0]["name"] = "R1";
  doc[1]["data"]["relay_info"][0]["state"] = relay1;
  doc[1]["data"]["relay_info"][1]["name"] = "R2";
  doc[1]["data"]["relay_info"][1]["state"] = relay2;

  doc[1]["data"]["sensor_info"][0]["name"] = "S1";
  doc[1]["data"]["sensor_info"][0]["state_sensor"] = sensor1;
  doc[1]["data"]["sensor_info"][1]["name"] = "S2";
  doc[1]["data"]["sensor_info"][1]["state_sensor"] = sensor2;

  if (sensor1) {
    doc[1]["data"]["sensor_info"][0]["alarm_signal"] = digitalRead(SENSOR1_PIN);
  }
  if (sensor2) {
    doc[1]["data"]["sensor_info"][1]["alarm_signal"] = !digitalRead(SENSOR2_PIN);
  }
  
  String buffer;
  serializeJson(doc, buffer);

  // Send event
  socketIO.sendEVENT(buffer);

  // Print JSON for debugging
  DEBUG_PRINT((String)"[WebSocketIO DEBUG] -> " + buffer);
}

/* WebSocketClientSocketIO */


