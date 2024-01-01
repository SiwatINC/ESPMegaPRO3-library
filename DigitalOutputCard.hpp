#pragma once
#include <ExpansionCard.hpp>
#include <Adafruit_PWMServoDriver.h>
#include <FRAM.h>
#include <map>

// Protocol for digital output card
// Note that pin is always 2 characters long and padded with 0 if necessary
// Set pin state topic: <pin>/set/state payload: 0/1
// Set pin pwm topic: <pin>/set/value payload: 0-4095
// Publish pin state topic: <pin>/state payload: 0/1
// Publish pin pwm topic: <pin>/value payload: 0-4095
// Publish all topic: requeststate payload: N/A
// Enable/disable publish topic: publish_enable payload: 0/1

// MQTT Topics
#define SET_STATE_TOPIC "/set/state"
#define SET_VALUE_TOPIC "/set/value"
#define STATE_TOPIC "/state"
#define VALUE_TOPIC "/value"
#define REQUEST_STATE_TOPIC "requeststate"
#define PUBLISH_ENABLE_TOPIC "publish_enable"

// Card type
#define CARD_TYPE_DIGITAL_OUTPUT 0x00

/**
 * @brief The DigitalOutputCard class is a class for controlling the Digital Output Card.
 * 
 * This Digital Output Card has 16 digital outputs.
 * All outputs are PWM capable.
 * ALl outputs are 12V Push-Pull outputs.
 * Outputs is grouped in 4 groups of 4 outputs.(0-3, 4-7, 8-11, 12-15)
 * Each pin is capable of 0.6A, however each group's total current is limited to 1.2A.
 * 
 * @warning You should not instantiate this class directly, instead use ESPMegaIO's registerCard function.
 */
class DigitalOutputCard : public ExpansionCard
{
public:
    // Instantiate the card with the specified address
    DigitalOutputCard(uint8_t address);
    // Instantiate the card with the specified position on the dip switch
    DigitalOutputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4);
    // Initialize the card
    bool begin();
    // Dummy loop function
    void loop();
    // Set the output to the specified state
    // This function set both the state and the pwm value
    void digitalWrite(uint8_t pin, bool state);
    // Set the output to the specified pwm value
    // This function set both the state and the pwm value
    void analogWrite(uint8_t pin, uint16_t value);
    // Set the state of the specified pin
    void setState(uint8_t pin, bool state);
    // Set the pwm value of the specified pin
    void setValue(uint8_t pin, uint16_t value);
    // Get the state of the specified pin
    bool getState(uint8_t pin);
    // Get the pwm value of the specified pin
    uint16_t getValue(uint8_t pin);
    // Register a callback function that will be called when the state of a pin changes
    uint8_t registerChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback);
    // Unregister the callback function
    void unregisterChangeCallback(uint8_t handler);
    // Load a new pin map
    void loadPinMap(uint8_t pinMap[16]);  
    // Bind the fram object to the card
    // The Output card use the fram in this layout:
    // [2 bytes] 0-1 : state
    // [32 bytes] 2-33 : value
    void bindFRAM(FRAM *fram, uint16_t address);
    // Save the state and value to the fram
    void saveToFRAM();
    // Load the state and value from the fram
    void loadFromFRAM();
    // Set the auto save to fram flag
    void setAutoSaveToFRAM(bool autoSave);
    // Save a single pin value to fram
    void savePinValueToFRAM(uint8_t pin);
    // Save state to fram
    void saveStateToFRAM();
    // Save value to fram
    void saveValueToFRAM();
    // Get type of card
    uint8_t getType();
private:
    // FRAM address
    uint16_t framAddress;
    // FRAM is binded
    bool framBinded = false;
    // Auto save to fram
    bool framAutoSave = false;
    // The fram object pointer
    FRAM *fram;
    // The pwm driver
    Adafruit_PWMServoDriver pwm;
    // The address of the card
    uint8_t address;
    // The state of the card
    bool state_buffer[16];
    // The pwm value of the card
    uint16_t value_buffer[16];
    // The callback function
    uint8_t callbacks_handler_index = 0;
    std::map<uint8_t, std::function<void(uint8_t, bool, uint16_t)>> change_callbacks;
    // Physical pin to virtual pin map
    uint8_t pinMap[16];
    // Return 16 bit value representing all 16 channels
    uint16_t packStates();
    // Unpack the 16 bit value to the state buffer
    void unpackStates(uint16_t states);
    // Virtual pin to physical pin map
    uint8_t virtualPinMap[16];
};