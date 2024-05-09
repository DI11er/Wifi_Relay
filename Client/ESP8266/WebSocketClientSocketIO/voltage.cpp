#include "voltage.h"

// реализация методов
VoltageManager::VoltageManager(byte pin, float ratio1, float ratio2) {
  _pin = pin;

  _ratio1 = ratio1;
  _ratio2 = ratio2;

  pinMode(_pin, INPUT);
} // конструктор

float VoltageManager::get_voltage() {
  _voltage = 0.0;

  for (int i = 0; i < _number_measurements; i++) {
    _voltage += (float)analogRead(_pin) * (_VREF / 1024);
    delay(10);
  }

  _voltage /= _number_measurements;

  if (_voltage < 0.25) { _voltage = 0; }
  else if (_voltage > 5.5) { _voltage *= _ratio1; } 
  else if (_voltage < 5.5) { _voltage *= _ratio2; } 

  return _voltage;
}

void VoltageManager::set_ratio1(float ratio) {_ratio1 = ratio;}

void VoltageManager::set_ratio2(float ratio) {_ratio2 = ratio;}