#include <ESPMegaProOS.hpp>
#include "esp_sntp.h"

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
    // Detect GPIO2 Reset
    gpio_num_t buttonPin = GPIO_NUM_2;
    gpio_pad_select_gpio(buttonPin);
    gpio_set_direction(buttonPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(buttonPin, GPIO_PULLUP_ONLY);
    if (gpio_get_level(buttonPin) == 0)
    {
        ESP_LOGW("ESPMegaPRO", "GPIO2 is low, if this condition persists for 5 more seconds, the device will factory reset");
        bool shouldReset = true;
        for (int i = 0; i < 50; i++)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            if (gpio_get_level(buttonPin) == 1)
            {
                ESP_LOGI("ESPMegaPRO", "Reset condition cleared, Continuing boot process");
                shouldReset = false;
                break;
            }
        }
        if (shouldReset)
        {
            ESP_LOGW("ESPMegaPRO", "Reset condition met, Factory Resetting device");
            for (int i = 0; i < fram.getSizeBytes(); i++)
            {
                if (i % 1024 == 0)
                    ESP_LOGV("ESPMegaPRO", "Clearing FRAM Address %d", i);
                fram.write8(i, 0);
            }
            ESP_LOGI("ESPMegaPRO", "Factory Reset Complete");
            ESP_LOGI("ESPMegaPRO", "Rebooting device");
            esp_restart();
        }
    }
    // Recovery Mode
    recovery.bindFRAM(&fram, 600);
    recovery.begin();
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
    recovery.loop();
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
        static int64_t lastNTPUpdate = (esp_timer_get_time() / 1000) - NTP_UPDATE_INTERVAL_MS + NTP_INITIAL_SYNC_DELAY_MS;
        if ((esp_timer_get_time() / 1000) - lastNTPUpdate > NTP_UPDATE_INTERVAL_MS)
        {
            ESP_LOGV("ESPMegaPRO", "Updating time from NTP");
            lastNTPUpdate = esp_timer_get_time() / 1000;
            this->updateTimeFromNTP();
        }
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
    uint32_t start = esp_timer_get_time() / 1000;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (!(timeinfo.tm_year > (2016 - 1900)))
    {
        ESP_LOGI("ESPMegaPRO", "NTP is not ready yet!");
        return false;   
    }
    rtctime_t rtctime = this->getTime();
    if (rtctime.hours != timeinfo.tm_hour || rtctime.minutes != timeinfo.tm_min ||
        rtctime.seconds != timeinfo.tm_sec || rtctime.day != timeinfo.tm_mday ||
        rtctime.month != timeinfo.tm_mon + 1 || rtctime.year != timeinfo.tm_year + 1900)
    {
        this->setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                      timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    }
    ESP_LOGV("ESPMegaPRO", "Time updated from NTP: %s", asctime(&timeinfo));
    return true;
}

/**
 * @brief Sets the timezone for the internal RTC.
 * 
 * @note This function takes POSIX timezone strings (e.g. "EST5EDT,M3.2.0,M11.1.0").
*/
void ESPMegaPRO::setTimezone(const char* offset)
{
    setenv("TZ", offset, 1);
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
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, this->iot->getMqttConfig()->mqtt_server);
    sntp_setservername(1, "pool.ntp.org");
    sntp_init();
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