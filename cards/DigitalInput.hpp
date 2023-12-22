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
        // Refresh and Read the input from the specified pin, always refresh the input buffers
        uint8_t digitalRead(uint8_t pin);
        // Read the input from the specified pin, also refresh the input buffers if refresh is true
        uint8_t digitalRead(uint8_t pin, bool refresh);
        // Preform a loop to refresh the input buffers
        void loop();
        // Get the input buffer for bank A
        uint8_t getInputBufferA();
        // Get the input buffer for bank B
        uint8_t getInputBufferB();
        // Register a callback function to be called when a pin changes
        void registerCallback(void (*callback)(void));
    private:
        PCF8574 inputBankA;
        PCF8574 inputBankB;
        uint8_t address_a;
        uint8_t address_b;
        uint8_t inputBufferA;
        uint8_t inputBufferB;
        void (*callback)(void);
        void refreshInputBankA();
        void refreshInputBankB();
}