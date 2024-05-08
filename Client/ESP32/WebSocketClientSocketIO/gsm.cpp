#include "gsm.h"
// реализация методов
GsmManager::GsmManager(byte pin_tx, byte pin_rx, unsigned long baud):_myPort(pin_tx, pin_rx) {
  _baud = baud;
}; // конструктор

// Отдельная функция для настройки gsm модуля
void GsmManager::init_gsm_model() {
  // DEBUG_PRINT("[GSM DEBUG] -> Start Init");
    _myPort.begin(_baud);


  while(!_myPort) {}

  do {
    _response = sendATCommand(F("AT"), true);  // Включение DTMF (тонального набора)
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК
  
  do {
    _response = sendATCommand(F("AT+CREG?"), true);                   // Проверка регистрации в сети
    _response.trim();                                                 // Убираем пробельные символы в начале и конце
    delay(100);
  } while (_response.substring(_response.indexOf(F(",")) + 1, _response.length()).toInt() != 1);

  do {
    _response = sendATCommand(F("AT+DDET=1"), true);  // Включение DTMF (тонального набора)
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК

  do {
    _response = sendATCommand(F("AT+CLIP=1"), true);  // Включаем АОН
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК

  
  sendATCommand(F("AT+CLVL?"), true);          // Запрашиваем громкость динамика
  sendATCommand(F("AT+CMGF=1"), true);         // Включить TextMode для SMS

  // DEBUG_PRINT("[GSM DEBUG] -> Connect");
};

void GsmManager::drop_serial() {
  _myPort.end();
}

// Отдельная функция для обработки данных поступаемых от GSM
void GsmManager::handler_gsm() {
  if (_myPort.available())   {                 // Если модем, что-то отправил...
    _response = _waitResponse();                 // Получаем ответ от модема для анализа
    // DEBUG_PRINT(_response);                  // Если нужно выводим в монитор порта

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
        // DEBUG_PRINT("submessage: " + submsg);
        if (submsg.startsWith("+DTMF:")) {      // Если ответ начинается с "+DTMF:" тогда:
          String symbol = submsg.substring(7, 8);  // Выдергиваем символ с 7 позиции длиной 1 (по 8)
          _processingDTMF(symbol);               // Логику выносим для удобства в отдельную функцию
        }

        if (submsg.startsWith("RING")) {         // Есть входящий вызов
          int phoneindex = _response.indexOf("+CLIP: \""); // Есть ли информация об определении номера, если да, то phoneindex>-1
          String innerPhone = "";                   // Переменная для хранения определенного номера
          if (phoneindex >= 0) {                    // Если информация была найдена
            phoneindex += 8;                        // Парсим строку и ...
            innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // ...получаем номер
            // DEBUG_PRINT("Number: " + innerPhone); // Выводим номер в монитор порта
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
            // DEBUG_PRINT(F("Message was sent. OK"));
          }
          else {                                    // Если нет, нужно повторить отправку
            // DEBUG_PRINT(F("Message was not sent. Error"));
          }
        }
      }
    } while (index > -1);                       // Пока индекс переноса строки действителен
  }
}

// Отдельная функция для обработки логики DTMF
void GsmManager::_processingDTMF(String symbol) {
  // DEBUG_PRINT("Key: " + symbol);             // Выводим в Serial для контроля, что ничего не потерялось
  if (!_flag_pre_auth) {
    if (symbol == "#") {
      bool correct;                             // Для оптимизации кода, переменная корректности команды
      correct = _handler_command(_result);
      // if (!correct) DEBUG_PRINT("Incorrect command: " + result); // Если команда некорректна, выводим сообщение
      _result = "";                                       // После каждой решетки сбрасываем вводимую комбинацию
    }
    else {
      _result += symbol;                               // Если нет, добавляем в конец
    }
  }
  else {
   _auth(symbol);
  }
}

// Отдельная функция для обработки комманд DTMF
bool GsmManager::_handler_command(String command) {
  if (command == "111") {
    sendATCommand(F("AT+VTS=\"1,4\""), true);
    return true;
  }

  return false;
}

// Отдельная функция для обработки авторизации
void GsmManager::_auth(String symbol) {
  if (_countTry < 3) {                                 // Если 3 неудачных попытки, перестаем реагировать на нажатия
    // DEBUG_PRINT((String)"[GSM DEBUG AUTH]" + result);
    if (symbol == "#") {
      bool correct = false;  
      if (_result == _pass) {                           // Введенная строка совпадает с заданным паролем
        // DEBUG_PRINT("The correct password is entered: " + result); // Информируем о корректном вводе пароля
        _countTry = 0;                                 // Обнуляем счетчик неудачных попыток ввода
        _flag_pre_auth = false;
        sendATCommand(F("AT+VTS=\"1,4\""), true);
      }
      else {
        _countTry += 1;                                // Увеличиваем счетчик неудачных попыток на 1
        // DEBUG_PRINT(F("Incorrect password"));         // Неверный пароль
        // DEBUG_PRINT("Counter:" + (String)countTry);// Количество неверных попыток
      }
      _result = "";                                    // После каждой решетки сбрасываем вводимую комбинацию 
    }
    else {
      _result += symbol;
    }
  }
  else {
    sendATCommand(F("ATH"), true);        // Если количество неудачных папыток привысило 3, то отклоняем вызов
  }
}

// Отдельная функция для отправки комманд в GSM
String GsmManager::sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  // DEBUG_PRINT("[GSM DEBUG] -> " + cmd);                          // Дублируем команду в монитор порта
  _myPort.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = _waitResponse();                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {  // Убираем из ответа дублирующуюся команду
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    // DEBUG_PRINT("[GSM DEBUG] -> " + _resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}

// Отдельная функция для получения ответа от GSM
String GsmManager::_waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!_myPort.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (_myPort.available()) {                     // Если есть, что считывать...
    _resp = _myPort.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    // DEBUG_PRINT((String)"[GSM DEBUG] -> " + "Timeout...");               // ... оповещаем об этом и...
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}

// Отдельная функция для отправки sms
void GsmManager::sendSMS(String phone, String message) {
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
}
/* GSM */