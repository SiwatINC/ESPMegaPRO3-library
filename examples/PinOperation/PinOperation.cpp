#include "ESPMegaPRO.h"

void setup() {
    ESPMega_begin(); // Must be called first in setup
    
    ESPMega_analogWrite(5,2048) // Write PWM value of 2048 to pin 5
}

void loop() {
    ESPMega_loop(); //Must be called in loop

    bool pin9 = ESPMega_digitalRead(9) // Read Digital Input Pin 9
    ESPMega_digitalWrite(9,pin9) // Mirror Input Pin 9 to Output Pin 9
}