#pragma once
#include <ESPMegaPRO.h>
#include <Adafruit_ADS1X15.h>
#include <MCP4725.h>

#define ANALOG_INPUT_BANK_A_ADDRESS 0x48
#define ANALOG_INPUT_BANK_B_ADDRESS 0x49
#define DAC0_ADDRESS 0x60
#define DAC1_ADDRESS 0x61
#define DAC2_ADDRESS 0x62
#define DAC3_ADDRESS 0x63

class AnalogCard {
    public:
        AnalogCard();
        void dacWrite(uint8_t pin);
        uint16_t analogRead(uint8_t pin);
        void begin();
    private:
        MCP4725 dac0;
        MCP4725 dac1;
        MCP4725 dac2;
        MCP4725 dac3;
        Adafruit_ADS1115 analogInputBankA;
        Adafruit_ADS1115 analogInputBankB;
};