/**
 * @file AnalogCard.cpp
 * @brief Implementation of the AnalogCard class.
 */

#include <AnalogCard.hpp>
#include "esp_log.h"

/**
 * @brief Default constructor for the AnalogCard class.
 */
AnalogCard::AnalogCard() : dac0(DAC0_ADDRESS),
                           dac1(DAC1_ADDRESS),
                           dac2(DAC2_ADDRESS),
                           dac3(DAC3_ADDRESS),
                           analogInputBankA(),
                           analogInputBankB(),
                           dac_change_callbacks()
{
    this->handler_count = 0;
}

/**
 * @brief Writes a value to the specified DAC pin.
 * @param pin The DAC pin to write to.
 * @param value The value to write.
 */
void AnalogCard::dacWrite(uint8_t pin, uint16_t value)
{
    ESP_LOGV("AnalogCard", "DAC Write: %d, %d", pin, value);
    this->setDACState(pin, value > 0);
    this->setDACValue(pin, value);
}

/**
 * @brief Sets the state of the specified DAC pin.
 * @param pin The DAC pin to set the state of.
 * @param state The state to set (true = on, false = off).
 */
void AnalogCard::setDACState(uint8_t pin, bool state)
{
    ESP_LOGD("AnalogCard", "Setting DAC state: %d, %d", pin, state);
    this->dac_state[pin] = state;
    this->sendDataToDAC(pin, this->dac_value[pin] * state);
    for (const auto& callback : this->dac_change_callbacks)
    {
        callback.second(pin, state, this->dac_value[pin]);
    }
}

/**
 * @brief Sets the value of the specified DAC pin.
 * @param pin The DAC pin to set the value of.
 * @param value The value to set.
 */
void AnalogCard::setDACValue(uint8_t pin, uint16_t value)
{
    ESP_LOGD("AnalogCard", "Setting DAC value: %d, %d", pin, value);
    this->dac_value[pin] = value;
    this->sendDataToDAC(pin, value * this->dac_state[pin]);
    for (const auto& callback : this->dac_change_callbacks)
    {
        callback.second(pin, this->dac_state[pin], value);
    }
}

/**
 * @brief Gets the value of the specified DAC pin.
 * @param pin The DAC pin to get the value of.
 * @return The value of the DAC pin.
 */
uint16_t AnalogCard::getDACValue(uint8_t pin)
{
    return this->dac_value[pin];
}

/**
 * @brief Gets the state of the specified DAC pin.
 * @param pin The DAC pin to get the state of.
 * @return The state of the DAC pin (true = on, false = off).
 */
bool AnalogCard::getDACState(uint8_t pin)
{
    return this->dac_state[pin];
}

/**
 * @brief Sends data to the specified DAC pin.
 * @param pin The DAC pin to send data to.
 * @param value The data to send.
 * @note This function does not call the DAC change callbacks.
 */
void AnalogCard::sendDataToDAC(uint8_t pin, uint16_t value)
{
    switch (pin)
    {
    case 0:
        this->dac0.writeDAC(value);
        break;
    case 1:
        this->dac1.writeDAC(value);
        break;
    case 2:
        this->dac2.writeDAC(value);
        break;
    case 3:
        this->dac3.writeDAC(value);
        break;
    }
}

/**
 * @brief Reads the value from the specified analog pin.
 * @param pin The analog pin to read from.
 * @return The value read from the analog pin.
 */
uint16_t AnalogCard::analogRead(uint8_t pin)
{
    if (pin >= 0 && pin <= 3)
    {
        return this->analogInputBankA.readADC_SingleEnded(pin);
    }
    else if (pin >= 4 && pin <= 7)
    {
        return this->analogInputBankB.readADC_SingleEnded(pin - 4);
    }
    return 65535;
}

/**
 * @brief Initializes the AnalogCard.
 * @return True if initialization is successful, false otherwise.
 */
bool AnalogCard::begin()
{
    if (!this->dac0.begin())
    {
        ESP_LOGE("AnalogCard", "Card Analog ERROR: Failed to install DAC0");
        return false;
    }
    if (!this->dac1.begin())
    {
        ESP_LOGE("AnalogCard", "Card Analog ERROR: Failed to install DAC1");
        return false;
    }
    if (!this->dac2.begin())
    {
        ESP_LOGE("AnalogCard", "Card Analog ERROR: Failed to install DAC2");
        return false;
    }
    if (!this->dac3.begin())
    {
        ESP_LOGE("AnalogCard", "Card Analog ERROR: Failed to install DAC3");
        return false;
    }
    if (!this->analogInputBankA.begin())
    {
        ESP_LOGE("AnalogCard", "Card Analog ERROR: Failed to install analog input bank A");
        return false;
    }
    if (!this->analogInputBankB.begin())
    {
        ESP_LOGE("AnalogCard", "Card Analog ERROR: Failed to install analog input bank B");
        return false;
    }
    return true;
}

/**
 * @brief The main loop of the AnalogCard.
 * @note This function does nothing.
 */
void AnalogCard::loop()
{
}

/**
 * @brief Gets the type of the AnalogCard.
 * @return The type of the AnalogCard.
 */
uint8_t AnalogCard::getType()
{
    return CARD_TYPE_ANALOG;
}

/**
 * @brief Registers a callback function to be called when the state or value of a DAC pin changes.
 * @param callback The callback function to register.
 * @return The handler ID of the registered callback.
 */
uint8_t AnalogCard::registerDACChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback)
{
    this->dac_change_callbacks[this->handler_count] = callback;
    return this->handler_count++;
}

/**
 * @brief Unregisters a previously registered DAC change callback.
 * @param handler The handler ID of the callback to unregister.
 */
void AnalogCard::unregisterDACChangeCallback(uint8_t handler)
{
    this->dac_change_callbacks.erase(handler);
}