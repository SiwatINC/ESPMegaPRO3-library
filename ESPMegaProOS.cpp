#include <ESPMegaProOS.hpp>

// Reserve FRAM address 0 - 1000 for ESPMegaPRO Internal Use
// (34 Bytes) Address 0-33 for Built-in Digital Output Card
// (266 Bytes) Address 34-300 for ESPMegaPRO IoT Module

/**
 * @brief Create a new ESPMegaPRO object
 *
 * @warning Only one ESPMegaPRO object can be created, creating more than one will result in undefined behavior
 */
ESPMegaPRO::ESPMegaPRO()
{
}

/**
 * @brief Initializes the ESPMegaPRO object.
 *
 * This function initializes the ESPMegaPRO object and all of its components.
 * It also initializes the built-in Digital Input and Digital Output cards.
 *
 * @return True if the initialization is successful, false otherwise.
 */
bool ESPMegaPRO::begin()
{
    Wire.begin(14, 33);
    fram.begin(FRAM_ADDRESS);
    Serial.begin(115200);
    this->installCard(1, &outputs);
    outputs.bindFRAM(&fram, 0);
    uint8_t outputPinMap[16] = {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};
    outputs.loadPinMap(outputPinMap);
    outputs.loadFromFRAM();
    outputs.setAutoSaveToFRAM(true);
    if (!this->installCard(0, &inputs))
    {
        ESP_LOGE("ESPMegaPRO", "Failed to initialize inputs");
        ESP_LOGE("ESPMegaPRO", "Is this an ESPMegaPRO device?");
        return false;
    }
    uint8_t inputPinMap[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 15, 14, 13, 12};
    inputs.loadPinMap(inputPinMap);
    return true;
}

/**
 * @brief The main loop for the ESPMegaPRO object.
 *
 * @note This function must be called in the main loop of the program.
 *
 * It will call the loop() function of all installed expansion cards, the ESPMegaIoT module, and the internal display.
 *
 */
void ESPMegaPRO::loop()
{
    inputs.loop();
    outputs.loop();
    for (int i = 0; i < 255; i++)
    {
        if (cardInstalled[i])
        {
            cards[i]->loop();
        }
    }
    if (iotEnabled)
    {
        iot->loop();
    }
    if (internalDisplayEnabled)
    {
        display->loop();
    }
    if (webServerEnabled)
    {
        webServer->loop();
    }
}

/**
 * @brief Installs an expansion card to the specified slot.
 *
 * @note This function automatically initializes the expansion card.
 *
 * @param slot The slot to install the card to.
 * @param card Pointer to the ExpansionCard object.
 *
 * @return True if the installation is successful, false otherwise.
 */
bool ESPMegaPRO::installCard(uint8_t slot, ExpansionCard *card)
{
    if (slot > 255)
        return false;
    if (cardInstalled[slot])
    {
        ESP_LOGE("ESPMegaPRO", "Card already installed at slot %d", slot);
        return false;
    }
    if (!card->begin())
    {
        ESP_LOGE("ESPMegaPRO", "Failed to initialize card at slot %d", slot);
        return false;
    }
    cards[slot] = card;
    cardInstalled[slot] = true;
    cardCount++;
    return true;
}

/**
 * @brief Updates the internal RTC from NTP.
 *
 * @note Network must be connected before calling this function (see ESPMegaPRO.ESPMegaIoT::connectNetwork()).
 *
 * @return True if the update is successful, false otherwise.
 */
bool ESPMegaPRO::updateTimeFromNTP()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        rtctime_t rtctime = this->getTime();
        if (rtctime.hours != timeinfo.tm_hour || rtctime.minutes != timeinfo.tm_min ||
            rtctime.seconds != timeinfo.tm_sec || rtctime.day != timeinfo.tm_mday ||
            rtctime.month != timeinfo.tm_mon + 1 || rtctime.year != timeinfo.tm_year + 1900)
        {
            this->setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                          timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
        }
        return true;
    }
    return false;
}

/**
 * @brief Gets the current time from the internal RTC.
 *
 * @return The current time as a rtctime_t struct.
 */
rtctime_t ESPMegaPRO::getTime()
{
    tmElements_t timeElement;
    RTC.read(timeElement);
    rtctime_t time;
    time.hours = timeElement.Hour;
    time.minutes = timeElement.Minute;
    time.seconds = timeElement.Second;
    time.day = timeElement.Day;
    time.month = timeElement.Month;
    time.year = timeElement.Year + 1970;
    return time;
}

/**
 * @brief Sets the current time of the internal RTC.
 *
 * @param hours The hours.
 * @param minutes The minutes.
 * @param seconds The seconds.
 * @param day The day.
 * @param month The month.
 * @param year The year.
 */
void ESPMegaPRO::setTime(int hours, int minutes, int seconds, int day, int month, int year)
{
    tmElements_t timeElement;
    timeElement.Hour = hours;
    timeElement.Minute = minutes;
    timeElement.Second = seconds;
    timeElement.Day = day;
    timeElement.Month = month;
    timeElement.Year = year - 1970;
    RTC.write(timeElement);
}

/**
 * @brief Enables, Instanitates, and Initializes the ESPMegaIoT module.
 *
 * @note This function must be called before using the ESPMegaIoT module.
 */
void ESPMegaPRO::enableIotModule()
{
    if (iotEnabled)
        return;
    this->iot = new ESPMegaIoT();
    this->iot->bindFRAM(&fram);
    this->iot->intr_begin(cards);
    iotEnabled = true;
}

/**
 * @brief Gets the expansion card installed at the specified slot.
 *
 * @param slot The slot to get the card from.
 *
 * @return Pointer to the ExpansionCard object, or nullptr if no card is installed at the specified slot.
 */
ExpansionCard *ESPMegaPRO::getCard(uint8_t slot)
{
    if (slot > 255)
        return nullptr;
    if (!cardInstalled[slot])
        return nullptr;
    return cards[slot];
}

/**
 * @brief Enables, Instanitates, and Initializes the internal display.
 *
 * @note &Serial is used for the internal display on ESPMegaPRO R3.
 * @note This function can only be called if the ESPMegaIoT module is enabled.
 * @note This function must be called before using the internal display.
 *
 * @param serial Pointer to the HardwareSerial object to use for the internal display (Serial for ESPMegaPRO R3).
 */
void ESPMegaPRO::enableInternalDisplay(HardwareSerial *serial)
{
    if (internalDisplayEnabled)
        return;
    if (!iotEnabled)
    {
        ESP_LOGE("ESPMegaPRO", "Cannot enable internal display without IoT module enabled");
        return;
    }
    ESP_LOGD("ESPMegaPRO", "Enabling Internal Display");
    display = new InternalDisplay(serial);
    ESP_LOGD("ESPMegaPRO", "Binding Internal Display to IoT Module");
    auto bindedGetTime = std::bind(&ESPMegaPRO::getTime, this);
    ESP_LOGD("ESPMegaPRO", "Binding Internal Display to Input/Output Cards");
    display->bindInputCard(&inputs);
    display->bindOutputCard(&outputs);
    display->begin(this->iot, bindedGetTime);
    internalDisplayEnabled = true;
    ESP_LOGD("ESPMegaPRO", "Internal Display Enabled");
}

/**
 * @brief Dumps the contents of the internal FRAM to the serial port.
 *
 * @param start The starting address.
 * @param end The ending address.
 */
void ESPMegaPRO::dumpFRAMtoSerial(uint16_t start, uint16_t end)
{
    for (int i = start; i <= end; i++)
    {
        if (i % 16 == 0)
        {
            Serial.printf("\n%03d: ", i);
        }
        Serial.printf("%03d ", this->fram.read8(i));
    }
}

/**
 * @brief Dumps the contents of the internal FRAM to the serial port in ASCII.
 *
 * @param start The starting address.
 * @param end The ending address.
 */
void ESPMegaPRO::dumpFRAMtoSerialASCII(uint16_t start, uint16_t end)
{
    for (int i = 0; i < 500; i++)
    {
        Serial.printf("%d: %c\n", i, this->fram.read8(i));
    }
}

/**
 * @brief Enables the internal web server.
 *
 * @note This function can only be called if the ESPMegaIoT module is enabled.
 * @note This function can only be called once.
 *
 * @param port The port to use for the web server.
 */
void ESPMegaPRO::enableWebServer(uint16_t port)
{
    if (!iotEnabled)
    {
        ESP_LOGE("ESPMegaPRO", "Cannot enable web server without IoT module enabled");
        return;
    }
    if (webServerEnabled)
    {
        ESP_LOGE("ESPMegaPRO", "Web server already enabled");
        return;
    }
    webServer = new ESPMegaWebServer(port, this->iot);
    webServer->bindFRAM(&fram);
    webServer->begin();
    webServerEnabled = true;
}