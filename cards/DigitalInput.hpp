#pragma once
#include <ESPMegaPRO.h>
#include <PCF8574.h>

class DigitalInputCard {
    public:
        // Instantiate the card with the specified address
        DigitalInputCard(uint8_t address_a, uint8_t address_b);
        // Instantiate the card with the specified position on the dip switch
        DigitalInputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5);
        // Initialize the card
        void begin();
        // Read the input from the specified pin
        uint8_t digitalRead(uint8_t pin);
        // Register a callback function to be called when a pin changes
        void registerCallback(void (*callback)(void));
}