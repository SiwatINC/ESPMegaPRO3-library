#include <DigitalOutputCard.hpp>

/**
 * @brief Create a new Digital Output Card object with the specified address
 * 
 * @note If you are using the ESPMegaI/O board, you should use the dip switch constructor
 * 
 * @param address The ESPMegaI/O address of the card
 */
DigitalOutputCard::DigitalOutputCard(uint8_t address) : change_callbacks(){
    this->address = address;
    // load default pin map
    for (int i = 0; i < 16; i++) {
        this->pinMap[i] = i;
        this->virtualPinMap[i] = i;
    }
    this->framBinded = false;
    this->callbacks_handler_index = 0;
}

/**
 * @brief Create a new Digital Output Card object with the specified position on the dip switch
 * 
 * @note The bit 0 are at the left of the dip switch
 * 
 * @warning There are 5 switches on the dip switch, they should be unique accross all the cards
 * 
 * @param bit0 The position of the first switch on the dip switch
 * @param bit1 The position of the second switch on the dip switch
 * @param bit2 The position of the third switch on the dip switch
 * @param bit3 The position of the fourth switch on the dip switch
 * @param bit4 The position of the fifth switch on the dip switch
 */
DigitalOutputCard::DigitalOutputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4) :
    DigitalOutputCard(0x20+bit0+bit1*2+bit2*4+bit3*8+bit4*16)
{

    
}

/**
 * @brief Initialize the Digital Output Card
 * 
 * @note Although this function can be called inside the main program, it is recommended to use ESPMegaPRO::installCard() instead
 * 
 * @return True if the initialization is successful, false otherwise
 */
bool DigitalOutputCard::begin() {
    this->pwm = Adafruit_PWMServoDriver(this->address);
    this->pwm.begin();
    pwm.setOutputMode(true);
    // Output card don't send ack, we can't check if it's connected
    // so we just return true
    return true;
}

/**
 * @brief Write a digtal LOW or HIGH to the specified pin
 * 
 * @note This function set both the state and the pwm value of the pin
 * 
 * @param pin The pin to set the state
 * @param state The logic level to set the pin to
 */
void DigitalOutputCard::digitalWrite(uint8_t pin, bool state) {
    this->pwm.setPin(virtualPinMap[pin], state ? 4095 : 0);
    this->state_buffer[pin] = state;
    this->value_buffer[pin] = state ? 4095 : 0;
    if (this->framAutoSave) {
        this->saveStateToFRAM();
        this->savePinValueToFRAM(pin);
    }
    for (const auto& callback : change_callbacks)
    {
        callback.second(pin, state, state ? 4095 : 0);
    }
}

/**
 * @brief Write a pwm value to the specified pin
 * 
 * @note This function set both the state and the pwm value of the pin
 * 
 * @param pin The pin to set the pwm value
 * @param value The pwm value to set
 */
void DigitalOutputCard::analogWrite(uint8_t pin, uint16_t value) {
    // If value is greater than 4095, set it to 4095
    if (value > 4095) value = 4095;
    // Set the pwm value
    this->pwm.setPin(virtualPinMap[pin], value);
    if (this->framAutoSave) {
        this->saveStateToFRAM();
        this->savePinValueToFRAM(pin);
    }
    this->state_buffer[pin] = value > 0;
    this->value_buffer[pin] = value;
    for (const auto& callback : change_callbacks)
    {
        callback.second(pin, value > 0, value);
    }
}

/**
 * @brief The main loop for the Digital Output Card object.
 * 
 * @note This function is not used, it is only here to implement the ExpansionCard interface
 */
void DigitalOutputCard::loop() {
}

/**
 * @brief Get the state of the specified pin
 * 
 * @param pin The pin to get the state
 * @return The state of the pin
 */
bool DigitalOutputCard::getState(uint8_t pin) {
    return this->state_buffer[pin];
}

/**
 * @brief Get the pwm value of the specified pin
 * 
 * @param pin The pin to get the pwm value
 * @return The pwm value of the pin
 */
uint16_t DigitalOutputCard::getValue(uint8_t pin) {
    return this->value_buffer[pin];
}

/**
 * @brief Get the type of the card
 * 
 * @return The type of the card
 */
uint8_t DigitalOutputCard::getType() {
    return CARD_TYPE_DIGITAL_OUTPUT;
}

/**
 * @brief Set the state of the specified pin
 * 
 * @param pin The pin to set the state
 * @param state The state of the pin
 */
void DigitalOutputCard::setState(uint8_t pin, bool state) {
    this-> state_buffer[pin] = state;
    this->pwm.setPin(virtualPinMap[pin], state*value_buffer[pin]);
    if(this->framAutoSave) {
        this->saveStateToFRAM();
    }
    for(const auto& callback : change_callbacks) {
        callback.second(pin, state, value_buffer[pin]);
    }
}

/**
 * @brief Set the pwm value of the specified pin
 * 
 * @param pin The pin to set the pwm value
 * @param value The pwm value to set
 */
void DigitalOutputCard::setValue(uint8_t pin, uint16_t value) {
    // If value is greater than 4095, set it to 4095
    if (value > 4095) value = 4095;
    this-> value_buffer[pin] = value;
    this->pwm.setPin(virtualPinMap[pin], state_buffer[pin]*value);
    if (this->framAutoSave) {
        this->savePinValueToFRAM(pin);
    }
    for (const auto& callback : change_callbacks)
    {
        callback.second(pin, state_buffer[pin], value);
    }
}

/**
 * @brief Register a callback function for the specified pin
 * 
 * @param callback The callback function to be called, the first parameter is the pin, the second parameter is the state, the third parameter is the pwm value
 * @return The handler of the callback function
 */
uint8_t DigitalOutputCard::registerChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback) {
    this->change_callbacks[this->callbacks_handler_index] = callback;
    return this->callbacks_handler_index++;
}

/**
 * @brief Unregister a callback function
 * 
 * @param handler The handler of the callback function to be unregistered
 */
void DigitalOutputCard::unregisterChangeCallback(uint8_t handler) {
    this->change_callbacks.erase(handler);
}

/**
 * @brief Load a pin map
 * 
 * A pin map is an array of 16 elements that maps the physical pins to virtual pins
 * The virtual pins are the pins that are used in the callback functions and are used for all the functions in this class
 * The physical pins are the pins on the Output IC, This can be found on the schematic of the ESPMegaI/O board
 * This function is useful if you want to change the number identification of the pins to match your project needs
 * 
 * @param pinMap The pin map to load
 */
void DigitalOutputCard::loadPinMap(uint8_t pinMap[16]) {
    for(int i = 0; i < 16; i++) {
        this->pinMap[i] = pinMap[i];
        this->virtualPinMap[pinMap[i]] = i;
    }
}

/**
 * @brief Bind a FRAM to the card
 * 
 * @note The Output Card use 34 bytes of FRAM
 * 
 * @warning If the fram range overlap with another card, undefined behavior will occur
 * 
 * @param fram The FRAM to bind
 * @param address The address of the card in the FRAM
 */
void DigitalOutputCard::bindFRAM(FRAM *fram, uint16_t address) {
    this->fram = fram;
    this->framBinded = true;
    this->framAddress = address;
}

/**
 * @brief Pack the states of all the pins into a 16 bit integer
 * 
 * @return The packed states
 */
uint16_t DigitalOutputCard::packStates() {
    uint16_t packed = 0;
    for(int i = 0; i < 16; i++) {
        packed |= (state_buffer[i] << i);
    }
    return packed;
}

/**
 * @brief Unpack the states of all the pins from a 16 bit integer
 * 
 * @param states The packed states
 */
void DigitalOutputCard::unpackStates(uint16_t states) {
    for(int i = 0; i < 16; i++) {
        this->setState(i, (states >> i) & 1);
    }
}

/**
 * @brief Save the states and values of all the pins to the FRAM
 */
void DigitalOutputCard::saveToFRAM() {
    if(!framBinded) return;
    // Save the state
    uint16_t packed = packStates();
    this->fram->write16(framAddress, packed);
    // Save the value
    this->fram->write(framAddress+2, (uint8_t*)value_buffer, 32);
}

/**
 * @brief Load the states and values of all the pins from the FRAM
 */
void DigitalOutputCard::loadFromFRAM() {
    if(!framBinded) return;
    // Load the state
    uint16_t packed = this->fram->read16(framAddress);
    unpackStates(packed);
    // Load the value
    uint16_t value[16];
    this->fram->read(framAddress+2, (uint8_t*)value, 32);
    for(int i = 0; i < 16; i++) {
        this->setValue(i, value[i]);
    }
}

/**
 * @brief Set the auto save to FRAM
 * 
 * @param autoSave True to enable auto save, false to disable auto save
 */
void DigitalOutputCard::setAutoSaveToFRAM(bool autoSave) {
    this->framAutoSave = autoSave;
}

/**
 * @brief Save a single pin value to FRAM
 * 
 * @param pin The pin to save
 */
void DigitalOutputCard::savePinValueToFRAM(uint8_t pin) {
    if(!framBinded) return;
    this->fram->write(framAddress+2+pin*2, (uint8_t*)&value_buffer[pin], 2);
}

/**
 * @brief Save the states of all the pins to FRAM
 */
void DigitalOutputCard::saveStateToFRAM() {
    if(!framBinded) return;
    uint16_t packed = packStates();
    this->fram->write16(framAddress, packed);
}