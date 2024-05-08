#include "voltage.h"

// реализация методов
VoltageManager::VoltageManager(byte pin, long r1, long r2, float ratio1, float ratio2) {
  _pin = pin;
  
  _r1 = r1;
  _r2 = r2;

  _ratio1 = ratio1;
  _ratio2 = ratio2;

  pinMode(_pin, INPUT);
} // конструктор

float VoltageManager::get_voltage() {
  _voltage = 0.0;

  for (int i = 0; i < _number_measurements; i++) {
    _voltage += (float)(analogRead(_pin) * _VREF * ((_r1 + _r2) / _r2) / 4096);
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