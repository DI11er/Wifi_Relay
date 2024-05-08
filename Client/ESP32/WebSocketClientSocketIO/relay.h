#pragma once
#include <Arduino.h>

// описание класса
class RelayManager {  // класс RelayManager
  public:
    // список членов, доступных в программе
    RelayManager(byte pin, bool state=true); // конструктор
    void toggle(); // метод для переключения состояния сенсера
    void set_state(bool state); // метод для установки состояния сенсера
    void restart();
    bool get_state(); // метод для проверки состояния реле
  private:
    // список членов для использования внутри класса
    byte _pin;  // номер пина
    bool _state; // состояние сенсера
};