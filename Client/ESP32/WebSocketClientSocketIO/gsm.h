#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>



// описание класса
class GsmManager {
  public:
    // список членов, доступных в программе
    GsmManager(byte pin_tx, byte pin_rx, unsigned long baud); // конструктор
    void init_gsm_model();
    void handler_gsm();
    String sendATCommand(String cmd, bool waiting);
    void sendSMS(String phone, String message);
    void drop_serial();
  private:
    // список членов для использования внутри класса
    unsigned long _baud;
    int _countTry = 0;                                     // Количество неудачных попыток авторизации
    String _response = "";                                 // Переменная для хранения ответов модуля
    String _result = "";                                   // Переменная для хранения вводимых данных через DTMF
    const String _pass = "1923";                           // Пароль для авторизации
    bool _flag_pre_auth = true;                            // Флаг авторизации
    void _processingDTMF(String symbol);
    bool _handler_command(String command);
    void _auth(String symbol);
    String _waitResponse();
    SoftwareSerial _myPort;
};