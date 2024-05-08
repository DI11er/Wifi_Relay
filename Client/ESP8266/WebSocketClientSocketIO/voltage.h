#include "Arduino.h"

// -> C = U(мультиметр) / U(выходное)
#define RATIO1 0.97 // > 5.5
#define RATIO2 0.98 // < 5.5


float get_voltage(int analog_pin);