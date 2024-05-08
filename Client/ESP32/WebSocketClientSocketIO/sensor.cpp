#include "sensor.h"  // подключаем заголовок обязательно

// реализация методов
SensorManager::SensorManager(byte pin, bool state, bool flag_inverted) {
  _pin = pin;
  _state = state;
  _flag_inverted = flag_inverted;

  pinMode(_pin, INPUT);
} // конструктор

void SensorManager::toggle() {_state = !_state; }

void SensorManager::set_state(bool state) {_state = state;}

bool SensorManager::get_state() {return _state; }

bool SensorManager::get_value() {
  if (_flag_inverted) { return !digitalRead(_pin); } 
  else { return digitalRead(_pin); }
}

void SensorManager::set_sensor_flag(bool state) {_sensor_flag = state;}

bool SensorManager::get_sensor_flag() {return _sensor_flag;}