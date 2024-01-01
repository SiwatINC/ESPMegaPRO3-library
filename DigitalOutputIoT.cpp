#include <DigitalOutputIoT.hpp>

/**
 * @brief Create a new DigitalOutputIoT object
 */
DigitalOutputIoT::DigitalOutputIoT()
{
    this->state_report_topic = new char[10];
    this->value_report_topic = new char[10];
    this->digital_outputs_publish_enabled = true;
}

/**
 * @brief Destroy the DigitalOutputIoT object
 */
DigitalOutputIoT::~DigitalOutputIoT()
{
    delete[] this->state_report_topic;
    delete[] this->value_report_topic;
}

/**
 * @brief Initialize the DigitalOutputIoT object
 *
 * @note ALthough this function can be called inside the main program, it is recommended to use ESPMegaPRO::installCard() instead
 *
 * @param card_id The id of the card
 * @param card The card object
 * @param mqtt The PubSubClient object
 * @param base_topic The base topic of the card
 * @return True if the initialization is successful, false otherwise
 */
bool DigitalOutputIoT::begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic)
{
    ESP_LOGD("DigitalOutputIoT", "Beginning DigitalOutputIoT");
    this->mqtt = mqtt;
    this->base_topic = base_topic;
    this->card = (DigitalOutputCard *)card;
    this->card_id = card_id;
    this->set_state_length = strlen(SET_STATE_TOPIC);
    this->set_value_length = strlen(SET_VALUE_TOPIC);
    this->state_length = strlen(STATE_TOPIC);
    this->value_length = strlen(VALUE_TOPIC);
    this->request_state_length = strlen(REQUEST_STATE_TOPIC);
    this->publish_enable_length = strlen(PUBLISH_ENABLE_TOPIC);
    strcpy(this->state_report_topic, "00/state");
    strcpy(this->value_report_topic, "00/value");
    ESP_LOGV("DigitalOutputIoT", "Registering callbacks inside DigitalOutputIoT::begin");
    // Register callbacks
    auto bindedCallback = std::bind(&DigitalOutputIoT::handleValueChange, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->card->registerChangeCallback(bindedCallback);
    ESP_LOGV("DigitalOutputIoT", "DigitalOutputIoT::begin complete");
    return true;
}

/**
 * @brief Handle the MQTT messages for the DigitalOutputIoT card
 *
 * @param topic The topic of the message
 * @param payload The payload of the message
 */
void DigitalOutputIoT::handleMqttMessage(char *topic, char *payload)
{
    // Protocol for digital output card
    // Note that pin is always 2 characters long and padded with 0 if necessary
    // Set pin state topic: <pin>/set/state payload: 0/1
    // Set pin pwm topic: <pin>/set/value payload: 0-4095
    // Publish pin state topic: <pin>/state payload: 0/1
    // Publish pin pwm topic: <pin>/value payload: 0-4095
    // Publish all topic: requeststate payload: N/A
    // Enable/disable publish topic: publish_enable payload: 0/1
    uint8_t topic_length = strlen(topic);
    if (this->processSetStateMessage(topic, payload, topic_length))
        return;
    if (this->processSetValueMessage(topic, payload, topic_length))
        return;
    if (this->processRequestStateMessage(topic, payload, topic_length))
        return;
}

/**
 * @brief Process a set state message
 *
 * @param topic The null terminated topic
 * @param payload The null terminated payload
 * @param topic_length The length of the topic
 * @return True if the message is a set state message, false otherwise
 */
bool DigitalOutputIoT::processSetStateMessage(char *topic, char *payload, uint8_t topic_length)
{
    // Check if the topic is a set state topic
    // The correct format is <pin>/set/state
    // This mean that the topic must end with /set/state
    // Check if the 3rd character is /
    if (topic[2] != '/')
    {
        return false;
    }
    // The topic must be set_state_length + 2 characters long
    if (topic_length != set_state_length + 2)
    {
        return false;
    }
    // Check if the topic ends with /set/state
    if (!strncmp(topic + 2, SET_STATE_TOPIC, set_state_length))
    {
        // Get the pin number
        uint8_t pin = (topic[0] - '0') * 10 + (topic[1] - '0');
        // Get the state
        bool state = false;
        char state_char = payload[0];
        if (state_char == '0')
        {
            state = false;
        }
        else if (state_char == '1')
        {
            state = true;
        }
        else
        {
            return false;
        }
        // Set the state
        card->setState(pin, state);
        return true;
    }
    return false;
}

/**
 * @brief Process a set value message
 *
 * @param topic The null terminated topic
 * @param payload The null terminated payload
 * @param topic_length The length of the topic
 * @return True if the message is a set value message, false otherwise
 */
bool DigitalOutputIoT::processSetValueMessage(char *topic, char *payload, uint8_t topic_length)
{
    // Check if the topic is a set value topic
    // The correct format is <pin>/set/value
    // This mean that the topic must end with /set/value
    // Check if the 3rd character is /
    if (topic[2] != '/')
    {
        return false;
    }
    // The topic must be set_value_length + 2 characters long
    if (topic_length != set_value_length + 2)
    {
        return false;
    }
    // Check if the topic ends with /set/value
    if (!strncmp(topic + 2, SET_VALUE_TOPIC, set_value_length))
    {
        // Get the pin number
        uint8_t pin = (topic[0] - '0') * 10 + (topic[1] - '0');
        // Get the value
        uint16_t value = atoi(payload);
        // Set the value
        card->setValue(pin, value);
        return true;
    }
    return false;
}

/**
 * @brief Process a request state message
 *
 * @param topic The null terminated topic
 * @param payload The null terminated payload
 * @param topic_length The length of the topic
 * @return True if the message is a request state message, false otherwise
 */
bool DigitalOutputIoT::processRequestStateMessage(char *topic, char *payload, uint8_t topic_length)
{
    // Check if the topic is a request state topic
    // The correct format is requeststate
    // This mean that the topic must be request_state_length characters long
    // The topic must be request_state_length characters long
    if (topic_length != request_state_length)
    {
        return false;
    }
    // Check if the topic is requeststate
    if (!strncmp(topic, REQUEST_STATE_TOPIC, request_state_length))
    {
        // Publish the state of all pins
        publishDigitalOutputs();
        return true;
    }
    return false;
}

/**
 * @brief Publish the state of all digital outputs
 */
void DigitalOutputIoT::publishDigitalOutputs()
{
    if (!digital_outputs_publish_enabled)
        return;
    for (int i = 0; i < 16; i++)
    {
        publishDigitalOutput(i);
    }
}

/**
 * @brief Publish the state and value of the specified digital output
 *
 * @param pin The pin to publish
 */
void DigitalOutputIoT::publishDigitalOutput(uint8_t pin)
{
    publishDigitalOutputState(pin);
    publishDigitalOutputValue(pin);
}

/**
 * @brief Publish the state of the specified digital output
 *
 * @param pin The pin to publish
 */
void DigitalOutputIoT::publishDigitalOutputState(uint8_t pin)
{
    if (!digital_outputs_publish_enabled)
        return;
    state_report_topic[0] = pin / 10 + '0';
    state_report_topic[1] = pin % 10 + '0';
    publishRelative(state_report_topic, card->getState(pin) ? "1" : "0");
}

/**
 * @brief Publish the value of the specified digital output
 *
 * @param pin The pin to publish
 */
void DigitalOutputIoT::publishDigitalOutputValue(uint8_t pin)
{
    if (!digital_outputs_publish_enabled)
        return;
    value_report_topic[0] = pin / 10 + '0';
    value_report_topic[1] = pin % 10 + '0';
    char payload[5];
    sprintf(payload, "%d", card->getValue(pin));
    publishRelative(value_report_topic, payload);
}

/**
 * @brief Enable/disable publishing of digital outputs
 *
 * @param enabled True to enable publishing, false to disable publishing
 */
void DigitalOutputIoT::setDigitalOutputsPublishEnabled(bool enabled)
{
    digital_outputs_publish_enabled = enabled;
}

/**
 * @brief Handle the value change of a pin
 *
 * @note This function is registered as a callback function with the DigitalOutputCard object
 *
 * @param pin The pin that changed
 * @param state The new state of the pin
 * @param value The new value of the pin
 */
void DigitalOutputIoT::handleValueChange(uint8_t pin, bool state, uint16_t value)
{
    publishDigitalOutput(pin);
}

/**
 * @brief Publish all digital outputs
 *
 * @note This function is called by the ESPMegaIoT object
 */
void DigitalOutputIoT::publishReport()
{
    publishDigitalOutputs();
}

/**
 * @brief Get the type of the IoT component
 *
 * @return The type of the IoT component
 */
uint8_t DigitalOutputIoT::getType()
{
    return CARD_TYPE_DIGITAL_OUTPUT;
}

/**
 * @brief Subscribe to the MQTT topics used by the DigitalOutputIoT object
 */
void DigitalOutputIoT::subscribe()
{
    char topic[20];
    // Subscribe to all set state topics
    for (int i = 0; i < 16; i++)
    {
        sprintf(topic, "%02d/set/state", i);
        subscribeRelative(topic);
    }
    // Subscribe to all set value topics
    for (int i = 0; i < 16; i++)
    {
        sprintf(topic, "%02d/set/value", i);
        subscribeRelative(topic);
    }
    // Subscribe to request state topic
    subscribeRelative(REQUEST_STATE_TOPIC);
    // Subscribe to publish enable topic
    subscribeRelative(PUBLISH_ENABLE_TOPIC);
}

/**
 * @brief The main loop for the DigitalOutputIoT object
 * 
 * @note This function is not used, it is only here to implement the IoTComponent interface
 */
void DigitalOutputIoT::loop()
{
}