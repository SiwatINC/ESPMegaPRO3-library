#include <InternalDisplay.hpp>

/**
 * @brief Initialize the Internal Display
 * 
 * @note You should not call this function directly, instead use ESPMegaIoT::enableInternalDisplay()
 * 
 * @param iot The ESPMegaIoT object
 * @param getRtcTime A function that returns the current time
 */
void InternalDisplay::begin(ESPMegaIoT *iot, std::function<rtctime_t()> getRtcTime)
{
    this->iot = iot;
    this->getRtcTime = getRtcTime;
    this->mqttConfig = this->iot->getMqttConfig();
    this->networkConfig = this->iot->getNetworkConfig();
    // Register callbacks
    auto bindedPageChangeCallback = std::bind(&InternalDisplay::handlePageChange, this, std::placeholders::_1);
    this->registerPageChangeCallback(bindedPageChangeCallback);
    auto bindedTouchCallback = std::bind(&InternalDisplay::handleTouch, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->registerTouchCallback(bindedTouchCallback);
    // Initialize the display
    this->displayAdapter->begin(115200);
    this->displayAdapter->setTimeout(100);
    this->displayAdapter->flush();
    this->reset();
    delay(500);
    this->jumpToPage(1);
}

/**
 * @brief The main loop of the Internal Display
 * 
 * @note You should not call this function directly, instead use ESPMega::loop()
 */
void InternalDisplay::loop()
{
    // Keep reading the Serial Adapter
    this->recieveSerialCommand();

    // Refresh the top bar every 5 seconds
    static uint32_t lastTopBarRefresh;
    if (millis() - lastTopBarRefresh > INTERNAL_DISPLAY_TOP_BAR_REFRESH_INTERVAL)
    {
        this->updateStatusIcons(this->iot->networkConnected(), this->iot->mqttConnected());
        lastTopBarRefresh = millis();
    }
    // Refresh the clock every 10 seconds
    static uint32_t lastClockRefresh;
    if (millis() - lastClockRefresh > INTERNAL_DISPLAY_CLOCK_REFRESH_INTERVAL)
    {
        this->updateClock();
        lastClockRefresh = millis();
    }
}

/**
 * @brief Update the display in response to a change in the input state
 * 
 * @param pin The pin that changed
 * @param state The new state of the pin
 */
void InternalDisplay::handleInputStateChange(uint8_t pin, bool state)
{
    // If the input card is binded to the display and the current page is the input page
    // then update the respective input component
    if (this->inputCard == nullptr || this->currentPage != INTERNAL_DISPLAY_INPUT_PAGE)
        return;
    // Update the input state
    this->setInputMarker(pin, state);
}

/**
 * @brief Update the display in response to a change in the PWM state
 * 
 * @param pin The pin that changed
 * @param state The new state of the pin
 * @param value The new value of the pin
 */
void InternalDisplay::handlePwmStateChange(uint8_t pin, bool state, uint16_t value)
{
    // If the output card is binded to the display and the current page is the output page
    // then update the respective output component
    if (this->outputCard == nullptr)
        return;
    if(this->currentPage == INTERNAL_DISPLAY_OUTPUT_PAGE) {
    // Update the output state
    this->setOutputBar(pin, value);
    this->setOutputStateColor(pin, state);
    }
    // Refresh the PWM Adjustment page if the current page is the PWM Adjustment page and the pin is the same
    else if (this->currentPage == INTERNAL_DISPLAY_PWM_ADJUSTMENT_PAGE && this->pmwAdjustmentPin == pin)
    {
        this->refreshPWMAdjustment();
    }
}

/**
 * @brief Update the display in response to page change
 * 
 * @param page The new page
 */
void InternalDisplay::handlePageChange(uint8_t page)
{
    // Refresh the page
    this->refreshPage(page);
}

/**
 * @brief Save the network config to the FRAM
 */
void InternalDisplay::saveNetworkConfig()
{
    // The network config page have the following components:
    // ip_set -> a text input to set the ip address
    // netmask_set -> a text input to set the netmask
    // gateway_set -> a text input to set the gateway
    // dns_set -> a text input to set the dns
    // host_set -> a text input to set the hostname

    // Save the ip address
    IPAddress ip;
    // 000.000.000.000, 16 characters, 3 dots, 3 characters per octet, 1 null terminator
    char ip_buffer[30];
    if(!this->getStringToBuffer("ip_set.txt", ip_buffer, 30))
    {
        return;
    }

    // Validate the ip address
    if (!ip.fromString(ip_buffer)) {
        return;
    }
    
    // Save the netmask
    IPAddress netmask;
    if (!this->getStringToBuffer("netmask_set.txt", ip_buffer, 30)) {
        return;
    }
    // Validate the netmask
    if (!netmask.fromString(ip_buffer)) {
        return;
    }
    
    // Save the gateway
    IPAddress gateway;
    if (!this->getStringToBuffer("gateway_set.txt", ip_buffer, 30)) {
        return;
    }
    // Validate the gateway
    if (!gateway.fromString(ip_buffer)) {
        return;
    }

    // Save the dns
    IPAddress dns;
    if(!this->getStringToBuffer("dns_set.txt", ip_buffer, 30))
        return;
    // Validate the dns
    if (!dns.fromString(ip_buffer))
        return;
    
    // Save the hostname
    if(!this->getStringToBuffer("host_set.txt", this->networkConfig->hostname, 32))
        return;

    // Write the ip address, netmask and gateway to the network config
    this->networkConfig->ip = ip;
    this->networkConfig->subnet = netmask;
    this->networkConfig->gateway = gateway;
    this->networkConfig->dns1 = dns;
    this->networkConfig->dns2 = dns;
    this->networkConfig->useStaticIp = true;
    this->networkConfig->useWifi = false;
    this->networkConfig->wifiUseAuth = false;
    this->iot->saveNetworkConfig();
    ESP.restart();
}

/**
 * @brief Save the MQTT config to the FRAM
 */
void InternalDisplay::saveMQTTConfig()
{
    // The MQTT config page have the following components:
    // mqttsv_set -> a text input to set the mqtt server
    // port_set -> a text input to set the mqtt port
    // use_auth -> a checkbox to enable/disable mqtt authentication
    // user_set -> a text input to set the mqtt username
    // password_set -> a text input to set the mqtt password
    // topic_set -> a text input to set the mqtt base topic
    
    // Send the stop bytes to flush the serial buffer
    this->sendStopBytes();

    // Save the mqtt server
    if(!this->getStringToBuffer("mqttsv_set.txt", this->mqttConfig->mqtt_server, 16))
        return;
    
    // Save the mqtt port
    this->mqttConfig->mqtt_port = this->getNumber("port_set.val");
    
    // Save the mqtt username
    if(!this->getStringToBuffer("user_set.txt", this->mqttConfig->mqtt_user, 16))
        return;
    
    // Save the mqtt password
    if(!this->getStringToBuffer("password_set.txt", this->mqttConfig->mqtt_password, 16))
        return;
    
    // Save the mqtt base topic
    if(!this->getStringToBuffer("topic_set.txt", this->mqttConfig->base_topic, 16))
        return;
    
    // Save the mqtt use auth
    uint8_t use_auth = this->getNumber("use_auth.val");
    this->mqttConfig->mqtt_useauth = use_auth  == 1 ? true : false;
    this->iot->saveMqttConfig();
    ESP.restart();
}

/**
 * @brief Update the status icons on the Internal Display's top bar
 * 
 * @param networkStatus The network status
 * @param mqttStatus The MQTT status
 */
void InternalDisplay::updateStatusIcons(bool networkStatus, bool mqttStatus)
{
    this->setNumber("server.pic", mqttStatus ? PIC_MQTT_CONNECTED : PIC_MQTT_DISCONNECTED);
    this->setNumber("lan.pic", networkStatus ? PIC_LAN_CONNECTED : PIC_LAN_DISCONNECTED);
}

/**
 * @brief Update the clock on the Internal Display's top bar
 */
void InternalDisplay::updateClock()
{
    rtctime_t time = this->getRtcTime();
    this->displayAdapter->printf("time.txt=\"%02d:%02d %s\"", time.hours % 12, time.minutes, time.hours / 12 ? "PM" : "AM");
    this->sendStopBytes();
}

/**
 * @brief Send data to display element on the current page
 */
void InternalDisplay::refreshPage()
{
    this->refreshPage(this->currentPage);
}

/**
 * @brief Send data to display element on the specified page
 * 
 * @note The current page must be the specified page
 * 
 * @param page The page to refresh
 */
void InternalDisplay::refreshPage(uint8_t page)
{
    switch (page)
    {
    case INTERNAL_DISPLAY_DASHBOARD_PAGE:
        this->refreshDashboard();
        break;
    case INTERNAL_DISPLAY_INPUT_PAGE:
        if (this->inputCard == nullptr)
        {
            this->jumpToPage(INTERNAL_DISPLAY_INPUT_NULL_PTR_PAGE);
            break;
        }
        this->refreshInput();
        break;
    case INTERNAL_DISPLAY_OUTPUT_PAGE:
        if (this->outputCard == nullptr)
        {
            this->jumpToPage(INTERNAL_DISPLAY_OUTPUT_NULL_PTR_PAGE);
            break;
        }
        this->refreshOutput();
        break;
    case INTERNAL_DISPLAY_AC_PAGE:
        if (this->climateCard == nullptr)
        {
            this->jumpToPage(INTERNAL_DISPLAY_CLIMATE_NULL_PTR_PAGE);
            break;
        }
        this->refreshAC();
        break;
    case INTERNAL_DISPLAY_PWM_ADJUSTMENT_PAGE:
        this->refreshPWMAdjustment();
        break;
    case INTERNAL_DISPLAY_NETWORK_CONFIG_PAGE:
        this->refreshNetworkConfig();
        break;
    case INTERNAL_DISPLAY_MQTT_CONFIG_PAGE:
        this->refreshMQTTConfig();
        break;
    default:
        break;
    }
}

/**
 * @brief Send data to display element on the dashboard page
 */
void InternalDisplay::refreshDashboard()
{
    // The dashboard have the following components:
    // 1. Hostname
    // 2. IP Address
    // 3. MQTT Server with port
    // 4. MQTT Connection status
    this->setString("hostname.txt", this->networkConfig->hostname);
    // Construct the IP address string
    static char ip_address[25];
    sprintf(ip_address, "%d.%d.%d.%d", this->networkConfig->ip[0], this->networkConfig->ip[1], this->networkConfig->ip[2], this->networkConfig->ip[3]);
    this->setString("ip_address.txt", ip_address);
    // Send the MQTT server and port
    this->displayAdapter->print("server_address.txt=\"");
    this->displayAdapter->print(this->mqttConfig->mqtt_server);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Send the MQTT connection status
    this->setString("status_txt.txt", this->iot->mqttConnected() ? MSG_MQTT_CONNECTED : MSG_MQTT_DISCONNECTED);
}

/**
 * @brief Send data to display element on the input page
 */
void InternalDisplay::refreshInput()
{
    for (uint8_t i = 0; i < 16; i++)
    {
        this->setInputMarker(i, this->inputCard->digitalRead(i, false));
    }
}

/**
 * @brief Send data to display element on the output page
 */
void InternalDisplay::refreshOutput()
{
    for (uint8_t i = 0; i < 16; i++)
    {
        this->setOutputBar(i, this->outputCard->getValue(i));
        this->setOutputStateColor(i, this->outputCard->getState(i));
    }
}

/**
 * @brief Send data to display element on the AC page
 */
void InternalDisplay::refreshAC()
{
    this->displayAdapter->print("temp.txt=\"");
    this->displayAdapter->print(this->climateCard->getTemperature());
    this->displayAdapter->print("C\"");
    this->sendStopBytes();
    this->displayAdapter->print("fan_auto.pic=");
    this->displayAdapter->print(this->climateCard->getFanSpeed() == AC_FAN_SPEED_AUTO ? PIC_AC_FAN_SPEED_AUTO_ACTIVE : PIC_AC_FAN_SPEED_AUTO_INACTIVE);
    this->sendStopBytes();
    this->displayAdapter->print("fan_low.pic=");
    this->displayAdapter->print(this->climateCard->getFanSpeed() == AC_FAN_SPEED_LOW ? PIC_AC_FAN_SPEED_LOW_ACTIVE : PIC_AC_FAN_SPEED_LOW_INACTIVE);
    this->sendStopBytes();
    this->displayAdapter->print("fan_mid.pic=");
    this->displayAdapter->print(this->climateCard->getFanSpeed() == AC_FAN_SPEED_MEDIUM ? PIC_AC_FAN_SPEED_MEDIUM_ACTIVE : PIC_AC_FAN_SPEED_MEDIUM_INACTIVE);
    this->sendStopBytes();
    this->displayAdapter->print("fan_high.pic=");
    this->displayAdapter->print(this->climateCard->getFanSpeed() == AC_FAN_SPEED_HIGH ? PIC_AC_FAN_SPEED_HIGH_ACTIVE : PIC_AC_FAN_SPEED_HIGH_INACTIVE);
    this->sendStopBytes();
    this->displayAdapter->print("mode_off.pic=");
    this->displayAdapter->print(this->climateCard->getMode() == AC_MODE_OFF ? PIC_AC_MODE_OFF_ACTIVE : PIC_AC_MODE_OFF_INACTIVE);
    this->sendStopBytes();
    this->displayAdapter->print("mode_fan.pic=");
    this->displayAdapter->print(this->climateCard->getMode() == AC_MODE_FAN_ONLY ? PIC_AC_MODE_FAN_ACTIVE : PIC_AC_MODE_FAN_INACTIVE);
    this->sendStopBytes();
    this->displayAdapter->print("mode_cool.pic=");
    this->displayAdapter->print(this->climateCard->getMode() == AC_MODE_COOL ? PIC_AC_MODE_COOL_ACTIVE : PIC_AC_MODE_COOL_INACTIVE);
    this->sendStopBytes();
    if (this->climateCard->getSensorType() == AC_SENSOR_TYPE_DHT22)
    {
        this->displayAdapter->print("roomtemp.txt=\"");
        this->displayAdapter->print(this->climateCard->getRoomTemperature());
        this->displayAdapter->print("C\"");
        this->sendStopBytes();
        this->displayAdapter->print("roomhumid.txt=\"");
        this->displayAdapter->print(this->climateCard->getHumidity());
        this->displayAdapter->print("%\"");
        this->sendStopBytes();
    }
    else if (this->climateCard->getSensorType() == AC_SENSOR_TYPE_DS18B20)
    {
        this->displayAdapter->print("roomtemp.txt=\"");
        this->displayAdapter->print(this->climateCard->getRoomTemperature());
        this->displayAdapter->print("C\"");
        this->sendStopBytes();
        this->setString("roomhumid.txt", "N/A");
    }
    else
    {
        this->setString("roomtemp.txt", "N/A");
        this->setString("roomhumid.txt", "N/A");
    }
}

/**
 * @brief Set the PWM status output bar value (Fullness of the bar)
 * 
 * @param pin The pin of the PWM
 * @param value The value of the PWM (0 - 4095)
 */
void InternalDisplay::setOutputBar(uint8_t pin, uint16_t value)
{
    // Write the value to the output bar
    this->displayAdapter->print("j");
    this->displayAdapter->print(pin);
    this->displayAdapter->print(".val=");
    this->displayAdapter->print((int)(value * 100 / 4095));
    this->sendStopBytes();
}

/**
 * @brief Set the PWM status output bar color to match the PWM state
 * 
 * @param pin The pin of the PWM
 * @param state The state of the PWM
 */
void InternalDisplay::setOutputStateColor(uint8_t pin, bool state)
{
    this->displayAdapter->print("j");
    this->displayAdapter->print(pin);
    this->displayAdapter->print(".ppic=");
    this->displayAdapter->print(state ? PIC_PWM_BAR_ON : PIC_PWM_BAR_OFF);
    this->sendStopBytes();
}

/**
 * @brief Set Input Marker to match the input state
 * 
 * @param pin The pin of the input
 * @param state The state of the input
 */
void InternalDisplay::setInputMarker(uint8_t pin, bool state)
{
    this->displayAdapter->print("I");
    this->displayAdapter->print(pin);
    this->displayAdapter->print(".val=");
    this->displayAdapter->print(state ? 1 : 0);
    this->sendStopBytes();
}

/**
 * @brief Create a new Internal Display object
 * 
 * @param displayAdapter The HardwareSerial object that is connected to the display
 */
InternalDisplay::InternalDisplay(HardwareSerial *displayAdapter) : ESPMegaDisplay(displayAdapter)
{
    this->currentPage = INTERNAL_DISPLAY_DASHBOARD_PAGE;
    this->iot = nullptr;
    this->inputCard = nullptr;
    this->outputCard = nullptr;
    this->climateCard = nullptr;
    this->pmwAdjustmentPin = 0;
}

/**
 * @brief Set the input card to be be shown on the input page
 * 
 * @param inputCard The input card object to be shown
 */
void InternalDisplay::bindInputCard(DigitalInputCard *inputCard)
{
    // Check if the input card is already binded
    // If it is, then unbind it first
    if (this->inputCard != nullptr)
        this->unbindInputCard();
    this->inputCard = inputCard;
    auto bindedInputStateChangeCallback =
        std::bind(&InternalDisplay::handleInputStateChange, this,
                  std::placeholders::_1, std::placeholders::_2);
    this->bindedInputCardCallbackHandler =
        this->inputCard->registerCallback(bindedInputStateChangeCallback);
}

/**
 * @brief Unbind the input card from the display
 */
void InternalDisplay::unbindInputCard()
{
    if (this->inputCard == nullptr)
        return;
    this->inputCard->unregisterCallback(this->bindedInputCardCallbackHandler);
    this->inputCard = nullptr;
}

/**
 * @brief Set the output card to be be shown on the output page
 * 
 * @param outputCard The output card object to be shown
 */
void InternalDisplay::bindOutputCard(DigitalOutputCard *outputCard)
{
    // Check if the output card is already binded
    // If it is, then unbind it first
    if (this->outputCard != nullptr)
        this->unbindOutputCard();
    this->outputCard = outputCard;
    auto bindedPwmStateChangeCallback = std::bind(&InternalDisplay::handlePwmStateChange, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->bindedOutputCardCallbackHandler =
        this->outputCard->registerChangeCallback(bindedPwmStateChangeCallback);
}

/**
 * @brief Unbind the output card from the display
 */
void InternalDisplay::unbindOutputCard()
{
    if (this->outputCard == nullptr)
        return;
    this->outputCard->unregisterChangeCallback(this->bindedOutputCardCallbackHandler);
    this->outputCard = nullptr;
}

/**
 * @brief Set the climate card to be be shown on the AC page
 * 
 * This assume that your ClimeateCard has the mode and fan speed names in the following order:
 * mode: [off, fan_only, cool]
 * fan_speed: [auto, low, medium, high]
 * 
 * @param climateCard The climate card object to be shown
 */
void InternalDisplay::bindClimateCard(ClimateCard *climateCard)
{   
    // Check if the climate card is already binded
    // If it is, then unbind it first
    if (this->climateCard != nullptr)
        this->unbindClimateCard();
    this->climateCard = climateCard;
    auto bindedACStateChangeCallback = std::bind(&InternalDisplay::handleACStateChange, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->bindedClimateCardCallbackHandler =
        this->climateCard->registerChangeCallback(bindedACStateChangeCallback);
}

/**
 * @brief Unbind the climate card from the display
 */
void InternalDisplay::unbindClimateCard()
{
    if (this->climateCard == nullptr)
        return;
    this->climateCard->unregisterChangeCallback(this->bindedClimateCardCallbackHandler);
    this->climateCard = nullptr;
}

/**
 * @brief Send data to display element on the PWM Adjustment page
 */
void InternalDisplay::refreshPWMAdjustment()
{
    // The PWM Adjustment page have the following components:
    // pwm_value -> a slider to adjust the PWM value
    // pwm_state -> a button to toggle the PWM state
    // pwm_id -> a text to show the PWM pin

    // Refresh the PWM pin
    this->refreshPWMAdjustmentId();
    // Refresh the PWM value
    this->refreshPWMAdjustmentSlider();
    // Refresh the PWM state
    this->refreshPWMAdjustmentState();
}

/**
 * @brief Send the PWM pin id to the display on the PWM Adjustment page
 */
void InternalDisplay::refreshPWMAdjustmentId()
{
    // Send the PWM pin
    this->displayAdapter->print("pwm_id.txt=\"P");
    this->displayAdapter->print(pmwAdjustmentPin);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
}

/**
 * @brief Send the PWM value to the display on the PWM Adjustment page
 */
void InternalDisplay::refreshPWMAdjustmentSlider()
{
    // Send the PWM value
    this->displayAdapter->print("pwm_value.val=");
    this->displayAdapter->print(this->outputCard->getValue(this->pmwAdjustmentPin));
    this->sendStopBytes();
}

/**
 * @brief Send the PWM state to the display on the PWM Adjustment page
 */
void InternalDisplay::refreshPWMAdjustmentState()
{
    // Send the PWM state
    this->displayAdapter->print("pwm_state.txt=\"");
    this->displayAdapter->print(this->outputCard->getState(this->pmwAdjustmentPin) ? MSG_PWM_ADJUSTMENT_STATE_ON : MSG_PWM_ADJUSTMENT_STATE_OFF);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
}

/**
 * @brief Handle the touch event on the display
 * 
 * @param page The page that the touch event occured
 * @param component The component that the touch event occured
 * @param type The type of the touch event
 */
void InternalDisplay::handleTouch(uint8_t page, uint8_t component, uint8_t type)
{
    // Switch based on the page
    switch (page)
    {
    case INTERNAL_DISPLAY_AC_PAGE:
        this->handleACTouch(type, component);
        break;
    case INTERNAL_DISPLAY_PWM_ADJUSTMENT_PAGE:
        this->handlePWMAdjustmentTouch(type, component);
        break;
    case INTERNAL_DISPLAY_NETWORK_CONFIG_PAGE:
        if (type == TOUCH_TYPE_RELEASE && component == 7)
            this->saveNetworkConfig();
        break;
    case INTERNAL_DISPLAY_MQTT_CONFIG_PAGE:
        if (type == TOUCH_TYPE_RELEASE && component == 2)
            this->saveMQTTConfig();
        break;
    default:
        break;
    }
}

/**
 * @brief Handle the touch event on the AC page
 * 
 * @param type The type of the touch event
 * @param component The component that the touch event occured
 */
void InternalDisplay::handleACTouch(uint8_t type, uint8_t component)
{
    // b1 [component 18] -> inclement AC temperature by 1
    // b0 [component 17] -> declement AC temperature by 1
    // fan_auto [component 4] -> set the fan speed to auto
    // fan_low [component 5] -> set the fan speed to low
    // fan_med [component 6] -> set the fan speed to medium
    // fan_high [component 7] -> set the fan speed to high
    // mode_off [component 10] -> set the mode to off
    // mode_fan [component 9] -> set the mode to fan only
    // mode_cool [component 8] -> set the mode to cool

    // For b0 and b1, if the type is not release then return
    // For other components, if the type is not press then return
    if ((component == 17 || component == 18) && type != TOUCH_TYPE_RELEASE)
        return;
    if ((component != 17 && component != 18) && type != TOUCH_TYPE_PRESS)
        return;

    // Switch based on the component
    switch (component)
    {
    case 17:
        // Decrement the temperature
        this->climateCard->setTemperature(this->climateCard->getTemperature() - 1);
        break;
    case 18:
        // Increment the temperature
        this->climateCard->setTemperature(this->climateCard->getTemperature() + 1);
        break;
    case 4:
        // Set the fan speed to auto
        this->climateCard->setFanSpeed(AC_FAN_SPEED_AUTO);
        break;
    case 5:
        // Set the fan speed to low
        this->climateCard->setFanSpeed(AC_FAN_SPEED_LOW);
        break;
    case 6:
        // Set the fan speed to medium
        this->climateCard->setFanSpeed(AC_FAN_SPEED_MEDIUM);
        break;
    case 7:
        // Set the fan speed to high
        this->climateCard->setFanSpeed(AC_FAN_SPEED_HIGH);
        break;
    case 10:
        // Set the mode to off
        this->climateCard->setMode(AC_MODE_OFF);
        break;
    case 9:
        // Set the mode to fan only
        this->climateCard->setMode(AC_MODE_FAN_ONLY);
        break;
    case 8:
        // Set the mode to cool
        this->climateCard->setMode(AC_MODE_COOL);
        break;
    default:
        break;
    }
}

/**
 * @brief Handle the touch event on the PWM Adjustment page
 * 
 * @param type The type of the touch event
 * @param component The component that the touch event occured
 */
void InternalDisplay::handlePWMAdjustmentTouch(uint8_t type, uint8_t component)
{
    // b0 [component 5] -> decrement the PWM id if its greater than 0, else set it to 15
    // b1  [component 6] -> increment the PWM id if its less than 15, else set it to 0
    // pwm_state [component 4] -> toggle the PWM state
    // pwm_value [component 1] -> set the PWM value based on the slider value
    // If the type is not release then return
    if (type != TOUCH_TYPE_RELEASE)
        return;
    uint16_t val = 0;
    // switch based on the component
    switch (component)
    {
    case 5:
        // Decrement the PWM id
        this->pmwAdjustmentPin = this->pmwAdjustmentPin > 0 ? this->pmwAdjustmentPin - 1 : 15;
        this->refreshPWMAdjustment();
        break;
    case 6:
        // Increment the PWM id
        this->pmwAdjustmentPin = this->pmwAdjustmentPin < 15 ? this->pmwAdjustmentPin + 1 : 0;
        this->refreshPWMAdjustment();
        break;
    case 4:
        // Toggle the PWM state
        this->outputCard->setState(this->pmwAdjustmentPin, !this->outputCard->getState(this->pmwAdjustmentPin));
        this->refreshPWMAdjustmentState();
        break;
    case 1:
        // Set the PWM value
        val = (uint16_t)this->getNumber("pwm_value.val");
        this->outputCard->setValue(this->pmwAdjustmentPin, val);
        break;
    default:
        break;
    }
}

/**
 * @brief Send data to display element on the network config page
 */
void InternalDisplay::refreshNetworkConfig()
{
    // The network config page have the following components:
    // ip_set -> a text input to set the ip address
    // netmask_set -> a text input to set the netmask
    // gateway_set -> a text input to set the gateway
    // dns_set -> a text input to set the dns
    // host_set -> a text input to set the hostname

    // Refresh the ip address
    this->displayAdapter->print("ip_set.txt=\"");
    this->sendIpToDisplay(this->networkConfig->ip);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the netmask
    this->displayAdapter->print("netmask_set.txt=\"");
    this->sendIpToDisplay(this->networkConfig->subnet);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the gateway
    this->displayAdapter->print("gateway_set.txt=\"");
    this->sendIpToDisplay(this->networkConfig->gateway);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the dns
    this->displayAdapter->print("dns_set.txt=\"");
    this->sendIpToDisplay(this->networkConfig->dns1);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the hostname
    this->displayAdapter->print("host_set.txt=\"");
    this->displayAdapter->print(this->networkConfig->hostname);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
}

/**
 * @brief Send data to display element on the mqtt config page
 */
void InternalDisplay::refreshMQTTConfig()
{
    // The MQTT config page have the following components:
    // mqttsv_set -> a text input to set the mqtt server
    // port_set -> a text input to set the mqtt port
    // use_auth -> a checkbox to enable/disable mqtt authentication
    // user_set -> a text input to set the mqtt username
    // password_set -> a text input to set the mqtt password
    // topic_set -> a text input to set the mqtt base topic

    // Refresh the mqtt server
    this->displayAdapter->print("mqttsv_set.txt=\"");
    this->displayAdapter->print(this->mqttConfig->mqtt_server);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the mqtt port
    this->displayAdapter->print("port_set.val=");
    this->displayAdapter->print(this->mqttConfig->mqtt_port);
    this->sendStopBytes();
    // Refresh the mqtt username
    this->displayAdapter->print("user_set.txt=\"");
    this->displayAdapter->print(this->mqttConfig->mqtt_user);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the mqtt password
    this->displayAdapter->print("password_set.txt=\"");
    this->displayAdapter->print(this->mqttConfig->mqtt_password);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the mqtt base topic
    this->displayAdapter->print("topic_set.txt=\"");
    this->displayAdapter->print(this->mqttConfig->base_topic);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
    // Refresh the mqtt use auth
    this->displayAdapter->print("use_auth.val=");
    this->displayAdapter->print(this->mqttConfig->mqtt_useauth ? 1 : 0);
    this->sendStopBytes();
}

/**
 * @brief Write an ip address to the display
 * 
 * @note This function only writes the ip address to the display, you need to send the prefix and suffix yourself
 * 
 * @param ip The ip address to send
 */
void InternalDisplay::sendIpToDisplay(IPAddress ip)
{
    // Send the ip address
    this->displayAdapter->print(ip[0]);
    this->displayAdapter->print(".");
    this->displayAdapter->print(ip[1]);
    this->displayAdapter->print(".");
    this->displayAdapter->print(ip[2]);
    this->displayAdapter->print(".");
    this->displayAdapter->print(ip[3]);
}

/**
 * @brief Handle the AC state change
 * 
 * @note This function is registered as a callback to the ClimateCard
 * 
 * @param mode The new mode
 * @param fan_speed The new fan speed
 * @param temperature The new temperature
 */
void InternalDisplay::handleACStateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature)
{
    // If the climate card is binded to the display and the current page is the AC page
    // then update the respective AC component
    if (this->climateCard == nullptr || this->currentPage != INTERNAL_DISPLAY_AC_PAGE)
        return;
    this->sendStopBytes();
    // Update the AC state
    this->refreshAC();
}

/**
 * @brief Set the boot status text
 * 
 * @param text The text to set
 */
void InternalDisplay::setBootStatus(const char *text) {
    this->displayAdapter->print("boot_state.txt=\"");
    this->displayAdapter->print(text);
    this->displayAdapter->print("\"");
    this->sendStopBytes();
}