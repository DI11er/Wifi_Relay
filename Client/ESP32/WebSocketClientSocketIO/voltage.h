#pragma once
#include <Arduino.h>

// описание класса
class VoltageManager {  // класс VoltageManager
  public:
    // список членов, доступных в программе
    VoltageManager(byte pin, long r1, long r2, float ratio1=1.0, float ratio2=1.0); // конструктор
    float get_voltage();  // метод для получения напряжения
    void set_ratio1(float ratio); // метод для установки коэффициента коррекции измеряемого напряжения
    void set_ratio2(float ratio); // метод для установки коэффициента коррекции измеряемого напряжения
  private:
    // список членов для использования внутри класса
    byte _pin;  // номер пина
    long _r1;  // верхнее плечо делителя
    long _r2;  // нижнее плечо делителя
    float _voltage; // переменная для хранения напряжения
    int _number_measurements = 60;  // количество выборок
    float _VREF = 3.318;  // опорное напряжение
    float _ratio1; // коэффициент коррекции измеряемого напряжения > 5.5 в
    float _ratio2; // коэффициент коррекции измеряемого напряжения < 5.5 в
};
