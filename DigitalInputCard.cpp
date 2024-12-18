#include <DigitalInputCard.hpp>

/**
 * @brief Create a new Digital Input Card object with the specified address
 * @note If you are using the ESPMegaI/O board, you should use the dip switch constructor
 *
 * @param address_a The ESPMegaI/O address of bank A
 * @param address_b The ESPMegaI/O address of bank B
 */
DigitalInputCard::DigitalInputCard(uint8_t address_a, uint8_t address_b) : callbacks()
{
    this->address_a = address_a;
    this->address_b = address_b;
    this->callbacks_handler_index = 0;
}

/**
 * @brief Create a new Digital Input Card object with the specified position on the dip switch
 *
 * @note The bit 0 are at the left of the dip switch
 *
 * @warning There are 6 switches on the dip switch, 3 for bank A and 3 for bank B, They should be unique for each bank accross all the cards
 *
 * @param bit0 The position of the first switch on the dip switch
 * @param bit1 The position of the second switch on the dip switch
 * @param bit2 The position of the third switch on the dip switch
 * @param bit3 The position of the fourth switch on the dip switch
 * @param bit4 The position of the fifth switch on the dip switch
 * @param bit5 The position of the sixth switch on the dip switch
 */
DigitalInputCard::DigitalInputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5)
{
    this->address_a = 0x20;
    this->address_b = 0x20;
    this->inputBufferA = 0;
    this->inputBufferB = 0;
    if (bit0)
        this->address_a += 1;
    if (bit1)
        this->address_a += 2;
    if (bit2)
        this->address_a += 4;
    if (bit3)
        this->address_b += 1;
    if (bit4)
        this->address_b += 2;
    if (bit5)
        this->address_b += 4;
}

/**
 * @brief Initialize the Digital Input Card
 *
 * @return True if the initialization is successful, false otherwise
 */
bool DigitalInputCard::begin()
{
    this->inputBankA = PCF8574(this->address_a);
    this->inputBankB = PCF8574(this->address_b);
    if (!this->inputBankA.begin())
    {
        ESP_LOGE("DigitalInputCard", "Input Card ERROR: Failed to install input bank A");
        this->initOk = false;
        return false;
    }
    if (!this->inputBankB.begin())
    {
        ESP_LOGE("DigitalInputCard", "Input Card ERROR: Failed to install input bank B");
        this->initOk = false;
        return false;
    }
    // Set the debounce time for all pins to 50ms
    for (int i = 0; i < 16; i++)
    {
        this->debounceTime[i] = 50;
        this->lastDebounceTime[i] = 0;
    }
    // Initialize the pin map to the default values
    for (int i = 0; i < 16; i++)
    {
        this->pinMap[i] = i;
        this->virtualPinMap[i] = i;
    }
    this->initOk = true;
    return true;
}

/**
 * @brief Read the input from the specified pin, always refresh the input buffers
 *
 * @param pin The pin to read from
 * @return True if the pin is HIGH, false if the pin is LOW
 */
bool DigitalInputCard::digitalRead(uint8_t pin)
{
    return this->digitalRead(pin, true);
}

/**
 * @brief Read the input from the specified pin, also refresh the input buffers if refresh is true
 *
 * @param pin The pin to read from
 * @param refresh If true, the input buffers will be refreshed before reading the pin
 * @return True if the pin is HIGH, false if the pin is LOW
 */
bool DigitalInputCard::digitalRead(uint8_t pin, bool refresh)
{
    if(!this->initOk)
    {
        ESP_LOGE("DigitalInputCard", "Input Card ERROR: Card not initialized");
        return false;
    }
    pin = pinMap[pin];
    // First check if the pin is in bank A or B
    if (pin >= 0 && pin <= 7)
    {
        // Refresh the input buffers if refresh is true
        if (refresh)
            refreshInputBankA();
        // Extract the bit from the buffer
        return ((inputBufferA >> (7 - pin)) & 1);
    }
    else if (pin >= 8 && pin <= 15)
    {
        // Refresh the input buffers if refresh is true
        if (refresh)
            refreshInputBankB();
        // Extract the bit from the buffer
        return ((inputBufferB >> (15 - pin)) & 1);
    }
    return false;
}

/**
 * @brief Check if the specified pin changed since the last call to this function
 *
 * @note This function compares the current input buffer with the previous input buffer to detect changes
 *
 * @param pin The pin to check
 * @param currentBuffer The current input buffer
 * @param previousBuffer The previous input buffer
 */
void DigitalInputCard::handlePinChange(int pin, uint8_t &currentBuffer, uint8_t &previousBuffer)
{
    // Get the index of the pin in the pin map
    uint8_t virtualPin = virtualPinMap[pin];
    if (pin < 8)
    {
        // Handle Bank A
        if (((previousBuffer >> (7 - pin)) & 1) != ((currentBuffer >> (7 - pin)) & 1))
        {
            // When the pin first change, store the time
            if (!pinChanged[virtualPin])
            {
                ESP_LOGD("DigitalInputCard", "Pin %d changed", virtualPin);
                pinChanged[virtualPin] = true;
                lastDebounceTime[virtualPin] = millis();
            }
            else
            {
                ESP_LOGD("DigitalInputCard", "Pin %d (%d>%d) debounce time: %d", virtualPin, ((previousBuffer >> (7 - pin)) & 1), ((currentBuffer >> (7 - pin)) & 1), millis() - lastDebounceTime[virtualPin]);
                // If the pin was already changed, check if the debounce time has passed
                if ((millis() - lastDebounceTime[virtualPin]) > debounceTime[virtualPin])
                {
                    ESP_LOGD("DigitalInputCard", "Pin %d triggered", virtualPin);
                    // Call the callback function
                    for (auto const &callback : callbacks)
                    {
                        callback.second(virtualPin, ((currentBuffer >> (7 - pin)) & 1));
                    }
                    // Store the previous buffer at the specified pin (bitwise operation)
                    // new value : (currentBuffer >> (7 - pin)) & 1)
                    previousBuffer = (previousBuffer & ~(1 << (7 - pin))) | (((currentBuffer >> (7 - pin)) & 1) << (7 - pin));
                    // Reset the pin changed flag
                    pinChanged[virtualPin] = false;
                }
            }
        }
        else
        {
            // Pin bounce back to previous state, reset the debounce time
            lastDebounceTime[virtualPin] = millis();
            pinChanged[virtualPin] = false;
        }
    }
    else
    {
        // Handle Bank B
        if (((previousBuffer >> (15 - pin)) & 1) != ((currentBuffer >> (15 - pin)) & 1))
        {
            // When the pin first change, store the time
            if (!pinChanged[virtualPin])
            {
                ESP_LOGD("DigitalInputCard", "Pin %d changed", virtualPin);
                pinChanged[virtualPin] = true;
                lastDebounceTime[virtualPin] = millis();
            }
            else
            {
                ESP_LOGD("DigitalInputCard", "Pin %d (%d>%d) debounce time: %d", virtualPin, ((previousBuffer >> (15 - pin)) & 1), ((currentBuffer >> (15 - pin)) & 1), millis() - lastDebounceTime[virtualPin]);
                // If the pin was already changed, check if the debounce time has passed
                if ((millis() - lastDebounceTime[virtualPin]) > debounceTime[virtualPin])
                {
                    ESP_LOGD("DigitalInputCard", "Pin %d triggered", virtualPin);
                    // Call the callback function
                    for (auto const &callback : callbacks)
                    {
                        callback.second(virtualPin, ((currentBuffer >> (15 - pin)) & 1));
                    }
                    // Store the previous buffer at the specified pin (bitwise operation)
                    // new value : (currentBuffer >> (15 - pin)) & 1)
                    previousBuffer = (previousBuffer & ~(1 << (15 - pin))) | (((currentBuffer >> (15 - pin)) & 1) << (15 - pin));
                    // Reset the pin changed flag
                    pinChanged[virtualPin] = false;
                }
            }
        }
        else
        {
            // Pin bounce back to previous state, reset the debounce time
            lastDebounceTime[virtualPin] = millis();
            pinChanged[virtualPin] = false;
        }
    }
}

/**
 * @brief A loop to refresh the input buffers and check for pin changes
 *
 * @note Although this function can be called in the main loop, it is recommended install the card in ESPMega to automatically manage the loop
 */
// Preform a loop to refresh the input buffers
void DigitalInputCard::loop()
{
    // Refresh the input buffers
    refreshInputBankA();
    refreshInputBankB();
    // Iterate over all pins and check if they changed
    for (int i = 0; i < 16; i++)
    {
        // Check which bank the pin is in
        if (i < 8)
        {
            handlePinChange(i, inputBufferA, previousInputBufferA);
        }
        else if (i >= 8 && i <= 15)
        {
            handlePinChange(i, inputBufferB, previousInputBufferB);
        }
    }
}

/**
 * @brief Get the input buffer for bank A (the first 8 pins)
 *
 * @return The input buffer for bank A where the first bit is the first pin and the last bit is the last pin
 */
uint8_t DigitalInputCard::getInputBufferA()
{
    // Rearrange the bits to match the pin map
    uint8_t inputBufferA_rearranged = 0;
    for (int i = 0; i < 8; i++)
    {
        inputBufferA_rearranged |= ((inputBufferA >> i) & 1) << (7 - i);
    }
    return inputBufferA_rearranged;
}

/**
 * @brief Get the input buffer for bank B (the last 8 pins)
 *
 * @return The input buffer for bank B where the first bit is the first pin and the last bit is the last pin
 */
uint8_t DigitalInputCard::getInputBufferB()
{
    // Rearrange the bits to match the pin map
    uint8_t inputBufferB_rearranged = 0;
    for (int i = 0; i < 8; i++)
    {
        inputBufferB_rearranged |= ((inputBufferB >> i) & 1) << (7 - i);
    }
    return inputBufferB_rearranged;
}

/**
 * @brief Register a callback function to be called when a pin changes
 *
 * @param callback The callback function to be called
 * @return The handler of the callback function
 */
uint8_t DigitalInputCard::registerCallback(std::function<void(uint8_t, bool)> callback)
{
    callbacks[this->callbacks_handler_index] = callback;
    return this->callbacks_handler_index++;
}

/**
 * @brief Read the input state from the input ic and store it in the input buffer for bank A
 */
void DigitalInputCard::refreshInputBankA()
{
    if(!this->initOk)
    {
        ESP_LOGE("DigitalInputCard", "Input Card ERROR: Card not initialized");
        return;
    }
    inputBufferA = inputBankA.read8();
}

/**
 * @brief Read the input state from the input ic and store it in the input buffer for bank B
 */
void DigitalInputCard::refreshInputBankB()
{
    if(!this->initOk)
    {
        ESP_LOGE("DigitalInputCard", "Input Card ERROR: Card not initialized");
        return;
    }
    inputBufferB = inputBankB.read8();
}

/**
 * @brief Set the debounce time for the specified pin
 *
 * Debounce is the time in milliseconds that the pin should be stable before the callback function is called
 * This is useful to prevent false triggers when the input is noisy
 * An example of this is when the input is connected to a mechanical switch
 *
 * @param pin The pin to set the debounce time for
 * @param debounceTime The debounce time in milliseconds
 */
void DigitalInputCard::setDebounceTime(uint8_t pin, uint32_t debounceTime)
{
    pin = pinMap[pin];
    this->debounceTime[pin] = debounceTime;
}

/**
 * @brief Unregister a callback function
 *
 * @param handler The handler of the callback function to unregister
 */
void DigitalInputCard::unregisterCallback(uint8_t handler)
{
    callbacks.erase(handler);
}

/**
 * @brief Load the pin map for the card
 *
 * A pin map is an array of 16 elements that maps the physical pins to virtual pins
 * The virtual pins are the pins that are used in the callback functions and are used for all the functions in this class
 * The physical pins are the pins on the Input IC, This can be found on the schematic of the ESPMegaI/O board
 * This function is useful if you want to change the number identification of the pins to match your project needs
 *
 * @param pinMap The pin map to load
 */
void DigitalInputCard::loadPinMap(uint8_t pinMap[16])
{
    for (int i = 0; i < 16; i++)
    {
        // Load the pin map (physical pin to virtual pin)
        this->pinMap[i] = pinMap[i];
        // Load the virtual pin map (virtual pin to physical pin)
        this->virtualPinMap[pinMap[i]] = i;
    }
}

/**
 * @brief Get the type of the card
 *
 * @return The type of the card
 */
uint8_t DigitalInputCard::getType()
{
    return CARD_TYPE_DIGITAL_INPUT;
}

/**
 * @brief Preload the previous input buffer and the input buffer
 *
 * @note This function is useful if you want to preload the input buffers with a run-time value
 */
void DigitalInputCard::preloadInputBuffer()
{
    refreshInputBankA();
    refreshInputBankB();
    previousInputBufferA = inputBufferA;
    previousInputBufferB = inputBufferB;
}

/**
 * @brief Get the status of the card
 *
 * @return True if the card is initialized, false otherwise
 */
bool DigitalInputCard::getStatus()
{
    return this->initOk;
}