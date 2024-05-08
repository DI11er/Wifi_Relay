#include "voltage.h"

static int number_measurements =  60;

float get_voltage(int analog_pin) {
  /* Измерение напряжение на аналоговом пине */
  float voltage = 0.0;
  for (int i = 0; i < number_measurements; i++) {
    voltage += analogRead(analog_pin) * (15.4 / 1024.0);
    delay(10);
  }
  
  voltage /= number_measurements;

  if (voltage < 0.25) { voltage = 0; }
  else if (voltage > 5.5) { voltage *= RATIO1; } 
  else if ( voltage < 5.5) { voltage *= RATIO2; } 

  return voltage;
}