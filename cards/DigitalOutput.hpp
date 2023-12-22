#pragma once
#include <ESPMegaPRO.h>
#include <Adafruit_PWMServoDriver.h>
class DigitalOutputCard
{
public:
    // Instantiate the card with the specified address
    DigitalOutputCard(uint8_t address);
    // Instantiate the card with the specified position on the dip switch
    DigitalOutputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4);
    // Initialize the card
    void begin();
    // Set the output to the specified state
    void digitalWrite(uint8_t pin, uint8_t value);
    // Set the output to the specified pwm value
    void analogWrite(uint8_t pin, uint8_t value);
private:
    Adafruit_PWMServoDriver pwm;
    uint8_t address;
};