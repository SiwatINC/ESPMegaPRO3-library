#pragma once
#include <ESPMegaPRO.h>

// Instantiate the card with the specified address
DigitalInputCard::DigitalInputCard(uint8_t address_a, uint8_t address_b) {
    this->address_a = address_a;
    this->address_b = address_b;
}
// Instantiate the card with the specified position on the dip switch
// Bit 0,1,2 are for bank A
// Bit 3,4,5 are for bank B
DigitalInputCard::DigitalInputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5) {
    this->address_a = 0x20;
    this->address_b = 0x20;
    if (bit0) this->address_a += 1;
    if (bit1) this->address_a += 2;
    if (bit2) this->address_a += 4;
    if (bit3) this->address_b += 1;
    if (bit4) this->address_b += 2;
    if (bit5) this->address_b += 4;

}
// Initialize the card
void DigitalInputCard::begin() {
    this->inputBankA = PCF8574(this->address_a);
    this->inputBankB = PCF8574(this->address_b);
    this->inputBankA.begin();
    this->inputBankB.begin();
}
// Refresh and Read the input from the specified pin, always refresh the input buffers
uint8_t DigitalInputCard::digitalRead(uint8_t pin) {
    digitalRead(pin, true);
}
// Read the input from the specified pin, also refresh the input buffers if refresh is true
uint8_t DigitalInputCard::digitalRead(uint8_t pin, bool refresh) {
    // First check if the pin is in bank A or B
    if (pin >= 0 && pin <= 7) {
        // Refresh the input buffers if refresh is true
        if (refresh) refreshInputBankA();
        // Extract the bit from the buffer
        return ((inputBufferA >> (7 - pin)) & 1);
    } else if (pin >= 8 && pin <= 15) {
        // Refresh the input buffers if refresh is true
        if (refresh) refreshInputBankB();
        // Extract the bit from the buffer
        return ((inputBufferB >> (15 - pin)) & 1);
    }
}
// Preform a loop to refresh the input buffers
void DigitalInputCard::loop() {
    // Store the current input buffers
    uint8_t inputBufferA_old = inputBufferA;
    uint8_t inputBufferB_old = inputBufferB;
    // Refresh the input buffers
    refreshInputBankA();
    refreshInputBankB();
    // Iterate over all pins and check if they changed
    for (int i = 0; i < 16; i++) {
        // Check which bank the pin is in
        if (i<8) {
            // Check if the pin changed
            if (((inputBufferA_old >> (7 - i)) & 1) != ((inputBufferA >> (7 - i)) & 1)) {
                // Call the callback function if it is not null and pass the pin and the new value
                if (callback != NULL) callback(i, ((inputBufferA >> (7 - i)) & 1));
            }
        } else {
            // Check if the pin changed
            if (((inputBufferB_old >> (15 - i)) & 1) != ((inputBufferB >> (15 - i)) & 1)) {
                // Call the callback function if it is not null and pass the pin and the new value
                if (callback != NULL) callback(i, ((inputBufferB >> (15 - i)) & 1));
            }
        }
    }
}
// Get the input buffer for bank A
uint8_t DigitalInputCard::getInputBufferA() {
    return inputBufferA;
}
// Get the input buffer for bank B
uint8_t DigitalInputCard::getInputBufferB() {
    return inputBufferB;
}
// Register a callback function to be called when a pin changes
void DigitalInputCard::registerCallback(void (*callback)(void)) {
    this->callback = callback;
}

// Refresh the input buffer for bank A
void DigitalInputCard::refreshInputBankA() {
    inputBufferA = inputBankA.read8();
}
// Refresh the input buffer for bank B
void DigitalInputCard::refreshInputBankB() {
    inputBufferB = inputBankB.read8();
}