#pragma once
#include <Arduino.h>

// описание класса
class SensorManager {
  public:
    // список членов, доступных в программе
    SensorManager(byte pin, bool state=false, bool flag_inverted=false); // конструктор
    void toggle(); // метод для переключения состояния сенсора
    void set_state(bool state); // метод для установки состояния сенсора
    bool get_state(); // метод для получения состояния сенсора
    bool get_value(); // метод для получения значения сенсора
    void set_sensor_flag(bool state); // метод для установки флага сенсора
    bool get_sensor_flag(); // метод для получения флага сенсора
  private:
    // список членов для использования внутри класса
    byte _pin;  // номер пина
    bool _state; // состояние сенсора
    bool _sensor_flag = true; // флаг сенсора
    bool _flag_inverted;
};