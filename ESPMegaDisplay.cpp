#include <ESPMegaDisplay.hpp>

/**
 * @brief Receives and processes serial commands from the display adapter.
 * @param process Flag indicating whether to process the received commands.
 * @return True if data is received, false otherwise.
 */
bool ESPMegaDisplay::recieveSerialCommand(bool process)
{
    bool dataRecieved = false;
    // Read the serial buffer if available
    while (displayAdapter->available())
    {
        rx_buffer[rx_buffer_index] = displayAdapter->read();
        rx_buffer_index++;
        // Check for overflow
        if (rx_buffer_index >= 256)
        {
            rx_buffer_index = 0;
        }
        if (process)
            this->processSerialCommand();
        dataRecieved = true;
    }
    return dataRecieved;
}

/**
 * @brief Receives and processes serial commands from the display adapter.
 * @return True if data is received, false otherwise.
 */
bool ESPMegaDisplay::recieveSerialCommand()
{
    return recieveSerialCommand(true);
}

/**
 * @brief Processes the received serial command.
 * @note This function interacts directly with the rx_buffer.
 */
void ESPMegaDisplay::processSerialCommand()
{
    // Check if the rx buffer ended with stop bytes (0xFF 0xFF 0xFF)
    if (!payloadIsValid())
        return;
    // The rx buffer ended with stop bytes
    // Check if the rx buffer is a push payload
    // The payload type is the first byte of the payload
    // 0x00 is invalid instruction
    // 0x01 is success
    // 0x65 is a touch event
    // 0x66 is a page report event
    if (rx_buffer[0] == 0x65)
    {
        processTouchPayload();
    }
    else if (rx_buffer[0] == 0x66)
    {
        processPageReportPayload();
    }
    this->rx_buffer_index = 0;
}

/**
 * @brief Processes the touch event payload.
 * @note This function interacts directly with the rx_buffer.
 */
void ESPMegaDisplay::processTouchPayload()
{
    if (rx_buffer_index != 7)
        return;
    // The second byte of the payload is the page number
    uint8_t page = rx_buffer[1];
    // The third byte of the payload is the component id
    uint8_t component = rx_buffer[2];
    // The fourth byte of the payload is the event
    uint8_t event = rx_buffer[3];
    // 0x01 is press, 0x00 is release
    ESP_LOGV("ESPMegaDisplay", "Touch event: page=%d, component=%d, event=%d", page, component, event);
    for (auto const &callback : touch_callbacks)
    {
        callback.second(page, component, event);
    }
}

/**
 * @brief Processes the page report event payload.
 * @note This function interacts directly with the rx_buffer.
 */
void ESPMegaDisplay::processPageReportPayload()
{
    if (rx_buffer_index != 5)
        return;
    // The second byte of the payload is the page number
    // Check if the page number is different from the current page
    // If it is different, we call the page change callbacks
    if (rx_buffer[1] == this->currentPage)
        return;
    this->currentPage = rx_buffer[1];
    ESP_LOGV("ESPMegaDisplay", "Page change event: page=%d", this->currentPage);
    for (auto const &callback : page_change_callbacks)
    {
        callback.second(this->currentPage);
    }
}

/**
 * @brief Sends stop bytes to the display adapter.
 */
void ESPMegaDisplay::sendStopBytes()
{
    displayAdapter->write(0xFF);
    displayAdapter->write(0xFF);
    displayAdapter->write(0xFF);
}

/**
 * @brief Sends a command to the display adapter.
 * @param command The command to send.
 */
void ESPMegaDisplay::sendCommand(char *command)
{
    displayAdapter->print(command);
    sendStopBytes();
}

/**
 * @brief Jumps to the specified page on the display.
 * @param page The page number to jump to.
 */
void ESPMegaDisplay::jumpToPage(int page)
{
    this->displayAdapter->print("page ");
    this->displayAdapter->print(page);
    sendStopBytes();
}

/**
 * @brief Sets the value of a number component on the display.
 * @param component The component name.
 * @param value The value to set.
 */
void ESPMegaDisplay::setNumber(const char *component, int value)
{
    this->displayAdapter->print(component);
    this->displayAdapter->print("=");
    this->displayAdapter->print(value);
    sendStopBytes();
}

/**
 * @brief Sets the value of a string component on the display.
 * @param component The component name.
 * @param value The value to set.
 */
void ESPMegaDisplay::setString(const char *component, const char *value)
{
    this->displayAdapter->print(component);
    this->displayAdapter->print("=\"");
    this->displayAdapter->print(value);
    this->displayAdapter->print("\"");
    sendStopBytes();
}

/**
 * @brief Gets the value of a number component from the display.
 * @warning This function is blocking.
 * @warning If the display does not respond or is not connected, this function will block for up to DISPLAY_FETCH_RETRY_COUNT * DISPLAY_FETCH_TIMEOUT milliseconds.
 * @param component The component name.
 * @return The value of the number component.
 */
uint32_t ESPMegaDisplay::getNumber(const char *component)
{
    // We might be in the middle of a serial command
    // We reset the rx buffer index to 0 to ensure that we don't read the wrong data
    this->rx_buffer_index = 0;
    uint32_t start = millis();
    // Send the get command
    this->displayAdapter->print("get ");
    this->displayAdapter->print(component);
    sendStopBytes();
    // Try to get a valid payload DISPLAY_FETCH_RETRY_COUNT times
    // Wait for the response
    bool validPayload = false;
    for (int i = 0; i < DISPLAY_FETCH_RETRY_COUNT; i++)
    {
        if (!waitForValidPayload(DISPLAY_FETCH_TIMEOUT))
        {
            ESP_LOGE("ESPMegaDisplay", "Timeout while waiting for display response");
            return 0;
        }
        if (rx_buffer[0] != 0x71)
        {
            ESP_LOGI("ESPMegaDisplay", "Invalid payload type: %d, retrying.", rx_buffer[0]);
            // The rx buffer is invalid, reset the rx buffer index
            rx_buffer_index = 0;
            continue;
        }
        else
        {
            validPayload = true;
            break;
        }
    }
    if (!validPayload)
    {
        ESP_LOGE("ESPMegaDisplay", "Invalid payload type: %d, max retry excceded.", rx_buffer[0]);
        return 0;
    }

    // The rx buffer is valid
    // The expected payload is type 0x71

    // The 2nd to 5th byte of the payload is the value
    // It's a 4 byte 32-bit value in little endian order.
    // Convert the 4 bytes to a 32-bit value
    uint32_t value = 0;
    value |= rx_buffer[1];
    value |= rx_buffer[2] << 8;
    value |= rx_buffer[3] << 16;
    value |= rx_buffer[4] << 24;
    return value;
}

/**
 * @brief Gets the value of a string component from the display.
 * @warning This function is blocking.
 * @warning If the display does not respond or is not connected, this function will block for up to DISPLAY_FETCH_RETRY_COUNT * DISPLAY_FETCH_TIMEOUT milliseconds.
 * @param component The component name.
 * @return The value of the string component.
 * @note The returned char array must be freed after use.
 */
const char *ESPMegaDisplay::getString(const char *component)
{
    // We might be in the middle of a serial command
    // We reset the rx buffer index to 0 to ensure that we don't read the wrong data
    this->rx_buffer_index = 0;
    uint32_t start = millis();
    // Send the get command
    this->displayAdapter->print("get ");
    this->displayAdapter->print(component);
    sendStopBytes();
    // Wait for the response
    // Try to get a valid payload DISPLAY_FETCH_RETRY_COUNT times
    // Wait for the response
    bool validPayload = false;
    for (int i = 0; i < DISPLAY_FETCH_RETRY_COUNT; i++)
    {
        if (!waitForValidPayload(DISPLAY_FETCH_TIMEOUT))
        {
            ESP_LOGE("ESPMegaDisplay", "Timeout while waiting for display response");
            return nullptr;
        }
        if (rx_buffer[0] != 0x70)
        {
            ESP_LOGI("ESPMegaDisplay", "Invalid payload type: %d, retrying.", rx_buffer[0]);
            // The rx buffer is invalid, reset the rx buffer index
            rx_buffer_index = 0;
            continue;
        }
        else
        {
            validPayload = true;
            break;
        }
    }
    if (!validPayload)
    {
        ESP_LOGE("ESPMegaDisplay", "Invalid payload type: %d, max retry excceded.", rx_buffer[0]);
        return nullptr;
    }
    // The 2nd bytes onwards before the stop bytes is the string
    // The length of the string is the length of the payload minus 4
    uint8_t length = rx_buffer_index - 4;
    // First we malloc a char array with the length of the string
    char *value = (char *)malloc(length + 1);
    // Copy the string from the rx buffer to the char array
    memcpy(value, rx_buffer + 1, length);
    // Add the null terminator
    value[length] = '\0';
    // Return the char array
    return value;
}

/**
 * @brief Gets the value of a number component from the display and stores it in a buffer.
 * @warning This function is blocking.
 * @warning If the display does not respond or is not connected, this function will block for up to DISPLAY_FETCH_RETRY_COUNT * DISPLAY_FETCH_TIMEOUT milliseconds.
 * @param component The component name.
 * @param buffer The buffer to store the value.
 * @param buffer_size The size of the buffer.
 * @return True if the value is successfully stored in the buffer, false otherwise.
 */
bool ESPMegaDisplay::getStringToBuffer(const char *component, char *buffer, uint8_t buffer_size)
{
    // We might be in the middle of a serial command
    // We reset the rx buffer index to 0 to ensure that we don't read the wrong data
    this->rx_buffer_index = 0;
    uint32_t start = millis();
    // Send the get command
    this->displayAdapter->print("get ");
    this->displayAdapter->print(component);
    sendStopBytes();
    // Wait for the response
    // Try to get a valid payload DISPLAY_FETCH_RETRY_COUNT times
    // Wait for the response
    bool validPayload = false;
    for (int i = 0; i < DISPLAY_FETCH_RETRY_COUNT; i++)
    {
        if (!waitForValidPayload(DISPLAY_FETCH_TIMEOUT))
        {
            ESP_LOGE("ESPMegaDisplay", "Timeout while waiting for display response");
            this->sendStopBytes();
            return 0;
        }
        if (rx_buffer[0] != 0x70)
        {
            ESP_LOGI("ESPMegaDisplay", "Invalid payload type: %d, retrying.", rx_buffer[0]);
            this->sendStopBytes();
            // The rx buffer is invalid, reset the rx buffer index
            rx_buffer_index = 0;
            continue;
        }
        else
        {
            validPayload = true;
            break;
        }
    }
    if (!validPayload)
    {
        ESP_LOGE("ESPMegaDisplay", "Invalid payload type: %d, max retry excceded.", rx_buffer[0]);
        this->sendStopBytes();
        return 0;
    }

    // The 2nd bytes onwards before the stop bytes is the string
    // The length of the string is the length of the payload minus 4
    uint8_t length = rx_buffer_index - 4;
    // Check if the buffer is large enough to hold the string
    if (length > buffer_size)
    {
        ESP_LOGE("ESPMegaDisplay", "Buffer size too small");
        this->sendStopBytes();
        return false;
    }
    // Copy the string from the rx buffer to the char array
    memcpy(buffer, rx_buffer + 1, length);
    // Add the null terminator
    buffer[length] = '\0';
    return 1;
}

/**
 * @brief Waits for a valid payload from the display adapter.
 * @param timeout The timeout value in milliseconds.
 * @return True if a valid payload is received, false otherwise.
 */
bool ESPMegaDisplay::waitForValidPayload(uint32_t timeout)
{
    uint32_t start = millis();
    // If the payload is not valid, keep reading the serial buffer until timeout
    while (!this->payloadIsValid())
    {
        if (millis() - start > timeout)
        {
            return false;
        }
        recieveSerialCommand(false);
    }
    return true;
}

/**
 * @brief Checks if the received payload is valid.
 * @return True if the payload is valid, false otherwise.
 */
bool ESPMegaDisplay::payloadIsValid()
{
    // Check if the rx buffer ended with stop bytes (0xFF 0xFF 0xFF)
    if (rx_buffer_index < 3)
        return false;
    if (rx_buffer[rx_buffer_index - 1] != 0xFF)
        return false;
    if (rx_buffer[rx_buffer_index - 2] != 0xFF)
        return false;
    if (rx_buffer[rx_buffer_index - 3] != 0xFF)
        return false;
    return true;
}

/**
 * @brief Sets the brightness of the display.
 * @param value The brightness value.
 */
void ESPMegaDisplay::setBrightness(int value)
{
    this->displayAdapter->print("dim=");
    this->displayAdapter->print(value);
    sendStopBytes();
}

/**
 * @brief Sets the volume of the display.
 * @param value The volume value.
 */
void ESPMegaDisplay::setVolume(int value)
{
    this->displayAdapter->print("vol=");
    this->displayAdapter->print(value);
    sendStopBytes();
}

/**
 * @brief Restarts the display.
 */
void ESPMegaDisplay::reset()
{
    // First we send a stop bytes to clear the serial buffer
    // This ensures that the display is ready to receive the reset command
    sendStopBytes();
    this->displayAdapter->print("rest");
    sendStopBytes();
}

/**
 * @brief Constructor for the ESPMegaDisplay class.
 * @param displayAdapter The serial adapter connected to the display.
 */
ESPMegaDisplay::ESPMegaDisplay(HardwareSerial *displayAdapter)
{
    this->displayAdapter = displayAdapter;
    this->currentPage = 0;
    this->rx_buffer_index = 0;
}

/**
 * @brief Initializes the display.
 */
void ESPMegaDisplay::begin()
{
    this->displayAdapter->setTimeout(100);
    this->displayAdapter->flush();
    this->reset();
}

/**
 * @brief The main loop function of the display.
 */
void ESPMegaDisplay::loop()
{
    // Check if there is data in the serial buffer
    // If there is data, process the data
    recieveSerialCommand();
}

/**
 * @brief Registers a callback function for touch events.
 * @param callback The callback function.
 * @return The handle of the callback function.
 */
uint16_t ESPMegaDisplay::registerTouchCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback)
{
    uint16_t handle = touch_callbacks.size();
    touch_callbacks[handle] = callback;
    return handle;
}

/**
 * @brief Unregisters a callback function for touch events.
 * @param handle The handle of the callback function.
 */
void ESPMegaDisplay::unregisterTouchCallback(uint16_t handle)
{
    touch_callbacks.erase(handle);
}

/**
 * @brief Registers a callback function for page change events.
 * @param callback The callback function.
 * @return The handle of the callback function.
 */
uint16_t ESPMegaDisplay::registerPageChangeCallback(std::function<void(uint8_t)> callback)
{
    uint16_t handle = page_change_callbacks.size();
    page_change_callbacks[handle] = callback;
    return handle;
}

/**
 * @brief Unregisters a callback function for page change events.
 * @param handle The handle of the callback function.
 */
void ESPMegaDisplay::unregisterPageChangeCallback(uint16_t handle)
{
    page_change_callbacks.erase(handle);
}
