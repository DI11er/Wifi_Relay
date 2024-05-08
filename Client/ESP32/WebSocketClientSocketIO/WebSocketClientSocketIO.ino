#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include "voltage.h"
#include "sensor.h"
#include "relay.h"
#include "gsm.h"

WiFiMulti WiFiMulti;
SocketIOclient socketIO;

#define GSM_RX 6                                      // RX - модуля к TX - GSM модуля
#define GSM_TX 7                                      // TX - модуля к RX - GSM модуля

GsmManager GSM(GSM_TX, GSM_RX, 9600);                 // Экземпляр gsm

RelayManager R1(3, true);                             // Экземпляр реле 1
RelayManager R2(4, true);                             // Экземпляр реле 2
RelayManager RELAY_GSM_PIN(8, true);                  // Экземпляр реле питания модуля GSM
RelayManager RELAY_SENSOR_PIN(21, true);              // Экземпляр реле питания сенсоров

SensorManager S1(10, false, false);                   // Экземпляр сенсора 1
SensorManager S2(20 , false);                         // Экземпляр сенсора 2

VoltageManager U1(0, 400000, 100000, 0.92, 0.92);     // Экземпляр сенсора питания

#define ssid ""                                       // Название WiFi
#define password ""                                   // Пароль от WiFi

#define address_server ""                             // Адрес сервера
#define port_server 4145                              // Порт сервера

IPAddress local_IP(192, 168, 6, 37);                  // Устанавливаем статический IP адрес
IPAddress gateway(192, 168, 6, 99);                   // Устанавливаем IP адрес шлюза
IPAddress subnet(255, 255, 255, 0);                   // Устанавливаем маску сети
         
#define update_relay_data 1                           // Время опроса данных и отправки на сервер в минутах
#define update_sensor_state 20                        // Время опроса состояния сенсеров в секундах

#define U1_voltage_level 3.5                          // Пороговый уровень напряжения

String ip;                                            // Переменная для хранения ip-адреса устройства

bool sensor_voltage1_flag = true;                     // Флаг для контроля напряжения

float U1_voltage = 0.0;                               // Переменная для хранения напряжения на канале U1

bool flag_init_gsm = true;                            // Флаг инициализации GSM

bool flag_state_connection = false;                   // Флаг наличия подключения к серверу


// для включения отладки нужно раскоментировать переменную DEBUG_ENABLE
#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif


void setup() {
  // устанавливаем разрешение АЦП на 12 bits (0-4096)
  analogReadResolution(12);
  U1_voltage = U1.get_voltage();

  #ifdef DEBUG_ENABLE
    Serial.begin(115200);

    while (!Serial) {}

    DEBUG_PRINT(F("DEBUG_MODE -> enable"));
  #endif

  init_websocketio_clint();

}

void loop() {
  socketIO.loop();

  if (flag_state_connection) {
    handler_update_data_timer();
    handler_sensor_timer();
  }

  if (!flag_init_gsm) {
    GSM.handler_gsm();
  }
}

/* WebSocketClientSocketIO */
// Инициализация клиента websocketio
void init_websocketio_clint() {
  /* Функция для настройки подключения WebSocketIO Clienta */
  WiFi.disconnect();

  WiFi.config(local_IP, gateway, subnet);

  // отключение точки доступа
  if(WiFi.getMode() & WIFI_AP) {
      WiFi.softAPdisconnect(true);
  }
  
  WiFiMulti.addAP(ssid, password);

  while(WiFiMulti.run() != WL_CONNECTED) {
      DEBUG_PRINT(F("."));
      delay(100);
  }

  ip = WiFi.localIP().toString();
  // адрес сервера, порт и URL
  socketIO.begin(address_server, port_server, "/socket.io/?EIO=4");

  // назначение обработчика событий
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
        RELAY_GSM_PIN.set_state(false);       // Включение GSM модема 
        delay(10000); 
        GSM.init_gsm_model();
      }

      if (flag_state_connection) {
        flag_state_connection = false;
      }
      break;
    }
    case sIOtype_CONNECT: {
      DEBUG_PRINT(F("[WebSocketIO DEBUG] -> CONNECT"));
      socketIO.send(sIOtype_CONNECT, "/");
      delay(500);
      join_room_event();
      delay(500);
      get_relay_parameters();
      delay(500);

      flag_state_connection = true;
      flag_init_gsm = true; 

      RELAY_GSM_PIN.set_state(true);       // Отключение GSM модема
      GSM.drop_serial();
      break;
    }
    case sIOtype_EVENT: {
      handler_commands(payload, length);  // Обработчик событий
      break;
    }
  }
}

// Обработчик команд
void handler_commands(uint8_t * payload, size_t length) {
  JsonDocument doc;
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
  if (chanel_name == "S1") {
    if (action == "toggle") {
      S1.toggle();
    }
  } 
  if (chanel_name == "S2") {
    if (action == "toggle") {
      S2.toggle();
    } 
  }

  if (S1.get_state() || S2.get_state()) {
    RELAY_SENSOR_PIN.set_state(false);
  } else {
    RELAY_SENSOR_PIN.set_state(true);
  }
}

// Обработчик реле
void handler_relay(String chanel_name, String action) {
  if (chanel_name == "R1") {
    if (action == "toggle") {
      R1.toggle();
    } else if (action == "restart") {
      R1.restart();
    }
  }
  if (chanel_name == "R2") {
    if (action == "toggle") {
      R2.toggle();
    } else if (action == "restart") {
      R2.restart();
    }
  }
}

// Обработчик напряжений
void handler_voltage(String chanel_name, String action) {
  if (chanel_name == "U1") {
    if (action == "measure") {
      U1_voltage = U1.get_voltage();
    }
  }
}

// Обработчик занимающийся отправкой данных на сервер
void handler_update_data_timer() {
  static uint32_t tmr;

  if(millis() - tmr > (update_relay_data * 60 * 1000)) {
    tmr = millis();
    U1_voltage = U1.get_voltage();
    voltage_monitoring();
    get_relay_parameters();
  }
}

// функция для контроля напряжения
void voltage_monitoring() {
  JsonDocument doc;

  doc[0] = "voltage_low_signal";

  doc[1]["ip"] = ip;
  
  if (U1_voltage < U1_voltage_level && sensor_voltage1_flag) {
    doc[1]["chanel"]["name"] = "U1";
    doc[1]["chanel"]["voltage"] = U1_voltage;
    sensor_voltage1_flag = false;
    String buffer;
    serializeJson(doc, buffer);

    DEBUG_PRINT(F("U1 Voltage low"));

    // Отправка события
    socketIO.sendEVENT(buffer);
  }
}

// Обработчик, занимающийся опросом сенсеров
void handler_sensor_timer() {
  static uint32_t tmr;

  if(millis() - tmr > (update_sensor_state * 1000)) {
    tmr = millis();
    if (S1.get_value() || S2.get_value()) {
      JsonDocument doc;

      doc[0] = "alarm_signal";

      doc[1]["ip"] = ip;
      
      if (S1.get_state() && S1.get_value() && S1.get_sensor_flag()) {
        doc[1]["chanel"]["name"] = "S1";
        doc[1]["chanel"]["description"] = "Датчик открытия двери";
        S1.set_sensor_flag(false);
        String buffer;
        serializeJson(doc, buffer);

        DEBUG_PRINT(F("Sensor 1 Alarm"));

        // Отправка события
        socketIO.sendEVENT(buffer);
        get_relay_parameters();
      }
      if (S2.get_state() && S2.get_value() && S2.get_sensor_flag()) {
        doc[1]["chanel"]["name"] = "S2";
        doc[1]["chanel"]["description"] = "Датчик наклона";
        S2.set_sensor_flag(false);
        String buffer;
        serializeJson(doc, buffer);

        DEBUG_PRINT(F("Sensor 2 Alarm"));

        // Отправка события
        socketIO.sendEVENT(buffer);
        get_relay_parameters();
      }
    }

    if (!S1.get_value()) {S1.set_sensor_flag(true);}
    if (!S2.get_value()) {S2.set_sensor_flag(true);}

  }
}

// Функция для отправки события о подключение к комнате на сервер
void join_room_event() {
  /* Функция для добавления пользователя в группу relay */
  JsonDocument doc;

  doc[0] = "join_room";

  doc[1]["username"] = (String)"relay_" + ip;
  doc[1]["room"] = "relay";

  // JSON в строку (серилизация)
  String buffer;
  serializeJson(doc, buffer);

  // Отправка события
  socketIO.sendEVENT(buffer);
}

// Функция для отправки данных на сервер
void get_relay_parameters() {
  /* Формирования JSON для оправки данных на сервер через WebSocketIO */
  JsonDocument doc;
  
  doc[0] = "relay_parameters"; // Событие обновления данных

  doc[1]["data"]["ip_relay"] = ip;

  doc[1]["data"]["voltage_info"][0]["name"] = "U1";
  doc[1]["data"]["voltage_info"][0]["voltage"] = U1_voltage;

  doc[1]["data"]["relay_info"][0]["name"] = "R1";
  doc[1]["data"]["relay_info"][0]["state"] = R1.get_state();
  doc[1]["data"]["relay_info"][1]["name"] = "R2";
  doc[1]["data"]["relay_info"][1]["state"] = R2.get_state();

  doc[1]["data"]["sensor_info"][0]["name"] = "S1";
  doc[1]["data"]["sensor_info"][0]["state_sensor"] = S1.get_state();
  doc[1]["data"]["sensor_info"][1]["name"] = "S2";
  doc[1]["data"]["sensor_info"][1]["state_sensor"] = S2.get_state();

  if (S1.get_state()) {
    doc[1]["data"]["sensor_info"][0]["alarm_signal"] = S1.get_value();
  }
  if (S2.get_state()) {
    doc[1]["data"]["sensor_info"][1]["alarm_signal"] = S2.get_value();
  }
  
  // JSON в строку (серилизация)
  String buffer;
  serializeJson(doc, buffer);

  // Отправка собтия
  socketIO.sendEVENT(buffer);

  // Печать JSON для отладки
  DEBUG_PRINT((String)"[WebSocketIO DEBUG] -> " + buffer);
}
/* WebSocketClientSocketIO */


