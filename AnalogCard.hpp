#pragma once
#include <ExpansionCard.hpp>
#include <Adafruit_ADS1X15.h>
#include <MCP4725.h>
#include <vector>
#include <map>

// Analog Card
#define CARD_TYPE_ANALOG 0x02

// Analog Card FRAM Address
#define ANALOG_INPUT_BANK_A_ADDRESS 0x48
#define ANALOG_INPUT_BANK_B_ADDRESS 0x49
#define DAC0_ADDRESS 0x60
#define DAC1_ADDRESS 0x61
#define DAC2_ADDRESS 0x62
#define DAC3_ADDRESS 0x63

/**
 * @brief This class represents the Analog Card.
 * 
 * The analog card has 8 analog inputs accross two banks, and 4 DAC outputs.
 * 
 * @note You do not need to specify the ESPMega I/O Address when creating an instance of this class as there can only be one Analog Card installed in the ESPMegaPRO board.
 * @warning There can only be one Analog Card installed in the ESPMegaPRO board.
 * 
 */
class AnalogCard : public ExpansionCard {
    public:
        AnalogCard();
        void dacWrite(uint8_t pin, uint16_t value);
        void sendDataToDAC(uint8_t pin, uint16_t value);
        uint16_t analogRead(uint8_t pin);
        bool begin();
        void loop();
        bool getDACState(uint8_t pin);
        uint16_t getDACValue(uint8_t pin);
        void setDACState(uint8_t pin, bool state);
        void setDACValue(uint8_t pin, uint16_t value);
        uint8_t registerDACChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback);
        void unregisterDACChangeCallback(uint8_t handler);
        uint8_t getType();
    private:
        uint8_t handler_count;
        // Map of handler IDs to callback functions
        std::map<uint8_t, std::function<void(uint8_t, bool, uint16_t)>> dac_change_callbacks;
        bool dac_state[4];
        uint16_t dac_value[4];
        MCP4725 dac0;
        MCP4725 dac1;
        MCP4725 dac2;
        MCP4725 dac3;
        Adafruit_ADS1115 analogInputBankA;
        Adafruit_ADS1115 analogInputBankB;
};