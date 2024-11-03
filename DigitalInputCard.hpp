#pragma once
#include <ExpansionCard.hpp>
#include <PCF8574.h>
#include <map>

// Card Type
#define CARD_TYPE_DIGITAL_INPUT 0x01

/**
 * @brief ESPMegaPRO Digital Input Card
 * 
 * This class represents the ESPMegaPRO Digital Input Card.
 * It allows you to read the state of the digital inputs from the ESPMegaPRO Digital Input Card.
 * It also allows you to register callback functions to be called when a pin changes.
 * The callback function also support debouncing.
 * 
 */
class DigitalInputCard : public ExpansionCard {
    public:
        // Instantiate the card with the specified address
        DigitalInputCard(uint8_t address_a, uint8_t address_b);
        // Instantiate the card with the specified position on the dip switch
        DigitalInputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5);
        // Initialize the card
        bool begin();
        // Refresh and Read the input from the specified pin, always refresh the input buffers
        bool digitalRead(uint8_t pin);
        // Read the input from the specified pin, also refresh the input buffers if refresh is true
        bool digitalRead(uint8_t pin, bool refresh);
        // Preform a loop to refresh the input buffers
        void loop();
        // Get the input buffer for bank A
        uint8_t getInputBufferA();
        // Get the input buffer for bank B
        uint8_t getInputBufferB();
        // Set the debounce time for the specified pin
        void setDebounceTime(uint8_t pin, uint32_t debounceTime);
        // Register a callback function to be called when a pin changes
        uint8_t registerCallback(std::function<void(uint8_t, bool)> callback);
        // Unregister the callback function
        void unregisterCallback(uint8_t handler);
        // Load a new pin map
        void loadPinMap(uint8_t pinMap[16]);
        // Preload previousInputBuffer and inputBuffer
        void preloadInputBuffer();
        // Status of card
        bool getStatus();
        // Get type of card
        uint8_t getType();
    private:
        bool initOk = false;
        PCF8574 inputBankA;
        PCF8574 inputBankB;
        uint8_t address_a;
        uint8_t address_b;
        uint8_t inputBufferA;
        uint8_t inputBufferB;
        uint8_t previousInputBufferA;
        uint8_t previousInputBufferB;
        uint32_t debounceTime[16];
        uint32_t lastDebounceTime[16];
        bool pinChanged[16];
        // A map of the physical pin to the virtual pin
        uint8_t pinMap[16];
        // A map of the virtual pin to the physical pin
        uint8_t virtualPinMap[16];
        uint8_t callbacks_handler_index = 0;
        std::map<uint8_t, std::function<void(uint8_t, bool)>> callbacks;
        void refreshInputBankA();
        void refreshInputBankB();
        void handlePinChange(int pin, uint8_t& currentBuffer, uint8_t& previousBuffer);
};