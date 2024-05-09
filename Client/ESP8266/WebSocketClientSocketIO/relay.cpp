#include "relay.h" // влючаем заголовок обязательно

// реализация методов
RelayManager::RelayManager(byte pin, bool state) {
  _pin = pin;
  
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, state);
} // конструктор

void RelayManager::toggle() { 
  _state = !_state; 
  digitalWrite(_pin, _state);
}

void RelayManager::set_state(bool state) {
  _state = state;
  digitalWrite(_pin, _state);
}

void RelayManager::restart() {
  _state = false;
  digitalWrite(_pin, _state);
  _state = true;
  delay(10000);
  digitalWrite(_pin, _state);  
}

bool RelayManager::get_state() { return digitalRead(_pin); }