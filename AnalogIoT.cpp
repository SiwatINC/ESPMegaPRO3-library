/**
 * @file AnalogIoT.cpp
 * @brief Implementation of the AnalogIoT class.
 *
 * This file contains the implementation of the AnalogIoT class, which provides functionality for handling analog input and output operations in an IoT system.
 * The class allows for setting the state and value of digital-to-analog converters (DACs), as well as reading the value of analog-to-digital converters (ADCs).
 * It also supports publishing the state and value of DACs and ADCs over MQTT.
 */
#include <AnalogIoT.hpp>


/**
 * @brief Default constructor for the AnalogIoT class.
 * 
 * This constructor initializes the AnalogIoT object and sets up the ADC conversion callbacks.
 */
AnalogIoT::AnalogIoT() : adc_conversion_callbacks() {
    for (uint8_t i = 0; i < 8; i++) {
        adc_publish_enabled[i] = false;
        adc_conversion_interval[i] = 1000;
    }
    this->adc_conversion_callback_index = 0;
}

/**
 * @brief Default destructor for the AnalogIoT class.
 */
AnalogIoT::~AnalogIoT() {
    this->adc_conversion_callbacks.clear();
}

/**
 * @brief Initializes the AnalogIoT object.
 * @param card_id The ID of the card.
 * @param card A pointer to the card object.
 * @param mqtt A pointer to the MQTT client object.
 * @param base_topic The base MQTT topic.
 * @return True if the initialization was successful, false otherwise.
 * @note This function can be called from the main program but it is recommended to use ESPMegaIoT to initialize the IoT Components.
 * This function initializes the AnalogIoT object and registers the callbacks for handling DAC changes.
 */
bool AnalogIoT::begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic) {
    this->mqtt = mqtt;
    this->base_topic = base_topic;
    this->card = (AnalogCard*)card;
    this-> card_id = card_id;
    this->dac_set_state_length = strlen(DAC_SET_STATE_TOPIC);
    this->dac_set_value_length = strlen(DAC_SET_VALUE_TOPIC);
    this->dac_state_length = strlen(DAC_STATE_TOPIC);
    this->dac_value_length = strlen(DAC_VALUE_TOPIC);
    this->request_state_length = strlen(REQUEST_STATE_TOPIC);
    this->dac_publish_enable_length = strlen(DAC_PUBLISH_ENABLE_TOPIC);
    // Register callbacks
    auto bindedCallback =  std::bind(&AnalogIoT::handleDACChange, this, std::placeholders::_1, std::placeholders::_2);
    this->card->registerDACChangeCallback(bindedCallback);
    return true;
}

/**
 * @brief Publishes the state of all DACs.
 * @note This function is called when a request state message is received.
 */
void AnalogIoT::handleMqttMessage(char *topic, char *payload){ 
    uint8_t topic_length = strlen(topic);
    if(this-> processDACSetStateMessage(topic, payload, topic_length)) return;
    if(this-> processDACSetValueMessage(topic, payload, topic_length)) return;
    if(this-> processRequestStateMessage(topic, payload, topic_length)) return;
    if(this-> processADCSetConversionIntervalMessage(topic, payload, topic_length)) return;
    if(this-> processADCSetConversionEnabledMessage(topic, payload, topic_length)) return;
}

/**
 * @brief Publishes the state of all DACs.
 */
void AnalogIoT::publishADCs() {
    for (uint8_t i = 0; i < 8; i++) {
        this->publishADC(i);
    }
}

/**
 * @brief Publishes the state of a DAC.
 * @param pin The pin of the DAC.
 */
void AnalogIoT::publishADC(uint8_t pin) {
    if (this->adc_publish_enabled[pin]) {
        uint16_t value = this->card->analogRead(pin);
        char *topic = new char[15];
        sprintf(topic, "adc/%02d/value", pin);
        char *payload = new char[10];
        sprintf(payload, "%d", value);
        this->publishRelative(topic, payload);
        delete[] topic;
        delete[] payload;
        // Call all callbacks
        for (auto& callback : this->adc_conversion_callbacks) {
            callback.second(pin, value);
        }
    }
}

/**
 * @brief Sets the interval at which the state of all DACs is published.
 * @param interval The interval in milliseconds.
 */
void AnalogIoT::setADCsPublishInterval(uint32_t interval) {
    for (uint8_t i = 0; i < 8; i++) {
        adc_conversion_interval[i] = interval;
    }   
}

/**
 * @brief Sets whether the state of all DACs is published.
 * @param enabled True if the state of all DACs should be published, false otherwise.
 */
void AnalogIoT::setADCsPublishEnabled(bool enabled) {
    for (uint8_t i = 0; i < 8; i++) {
        adc_publish_enabled[i] = enabled;
    }
}

/**
 * @brief Registers a callback for handling ADC conversions.
 * @param callback The callback function.
 * @return The handler of the callback.
 */
uint8_t AnalogIoT::registerADCConversionCallback(std::function<void(uint8_t, uint16_t)> callback) {
    this->adc_conversion_callbacks[this->adc_conversion_callback_index] = callback;
    return this->adc_conversion_callback_index++;
}

/**
 * @brief Unregisters a callback for handling ADC conversions.
 * @param handler The handler of the callback.
 */
void AnalogIoT::unregisterADCConversionCallback(uint8_t handler) {
    this->adc_conversion_callbacks.erase(handler);
}

/**
 * @brief Sets the interval at which the value of an ADC channel is read.
 * @param pin The pin of the ADC channel.
 * @param interval The interval in milliseconds.
 */
void AnalogIoT::setADCConversionInterval(uint8_t pin, uint16_t interval) {
    adc_conversion_interval[pin] = interval;
}

/**
 * @brief Enables or disables the periodic reading of the value of an ADC channel.
 * @param pin The pin of the ADC channel.
 * @param enabled True if the value of the ADC channel should be read, false otherwise.
 */
void AnalogIoT::setADCConversionEnabled(uint8_t pin, bool enabled) {
    adc_publish_enabled[pin] = enabled;
}

/**
 * @brief Processes a message received on the MQTT topic for setting the state of a DAC.
 * @param topic The topic of the message.
 * @param payload The payload of the message.
 * @param topic_length The length of the topic.
 * @note This function is not meant to be called from user code.
 * @return True if the message was processed, false otherwise.
 */
bool AnalogIoT::processADCSetConversionIntervalMessage(char *topic, char *payload, uint8_t topic_length) {
    // TODO: Process payload matching the criteria
    // Topic: adc/<%02d>/set/conversion_interval
    // The first 4 characters are "adc/"
    // The length of the topic must be 30 characters
    // The last 24 characters must be "/set/conversion_interval"
    // After all these conditions are met, the topic is valid
    // Extract the pin number from the topic
    if (topic_length != 30) {
        return false;
    }
    if (strncmp(topic, "adc/", 4)) {
        return false;
    }
    if (strncmp(topic + 26, "/set/conversion_interval", 24)) {
        return false;
    }
    uint8_t pin = (topic[4] - '0') * 10 + (topic[5] - '0');
    // Extract the payload
    uint16_t interval = atoi(payload);
    // Set the interval
    this->setADCConversionInterval(pin, interval);
    return true;
}

/**
 * @brief Processes a message received on the MQTT topic for setting the value of a DAC.
 * @param topic The topic of the message.
 * @param payload The payload of the message.
 * @param topic_length The length of the topic.
 * @note This function is not meant to be called from user code.
 * @return True if the message was processed, false otherwise.
 */
bool AnalogIoT::processADCSetConversionEnabledMessage(char *topic, char *payload, uint8_t topic_length) {
    // Topic: adc/<%02d>/set/conversion_enabled
    // The first 4 characters are "adc/"
    // The length of the topic must be 29 characters
    // The last 23 characters must be ""/set/conversion_enabled
    // After all these conditions are met, the topic is valid
    // Extract the pin number from the topic
    if (topic_length != 29) {
        return false;
    }
    if (strncmp(topic, "adc/", 4)) {
        return false;
    }
    if (strncmp(topic + 25, "/set/conversion_enabled", 23)) {
        return false;
    }
    uint8_t pin = (topic[4] - '0') * 10 + (topic[5] - '0'); 
    // Extract the payload
    bool enabled = atoi(payload);
    // Set conversion enabled
    this->setADCConversionEnabled(pin, enabled);
    return true;
}

/**
 * @brief Processes a message received on the MQTT topic for setting the state of a DAC.
 * @param topic The topic of the message.
 * @param payload The payload of the message.
 * @param topic_length The length of the topic.
 * @note This function is not meant to be called from user code.
 * @return True if the message was processed, false otherwise.
 */
bool AnalogIoT::processDACSetStateMessage(char *topic, char *payload, uint8_t topic_length) {
    // Topic: dac/<%02d>/set/state
    // The first 4 characters are "dac/"
    // The length of the topic must be 16 characters
    // The last 10 characters must be "/set/state"
    // After all these conditions are met, the topic is valid
    // Extract the pin number from the topic
    if (topic_length != 16) {
        return false;
    }
    if (strncmp(topic, "dac/", 4)) {
        return false;
    }
    if (strncmp(topic + 12, "/set/state", 10)) {
        return false;
    }
    uint8_t pin = (topic[4] - '0') * 10 + (topic[5] - '0');
    // Extract the payload
    bool state = atoi(payload);
    // Set the state
    this->card->setDACState(pin, state);
    return true;
}

/**
 * @brief Processes a message received on the MQTT topic for setting the value of a DAC.
 * @param topic The topic of the message.
 * @param payload The payload of the message.
 * @param topic_length The length of the topic.
 * @note This function is not meant to be called from user code.
 * @return True if the message was processed, false otherwise.
 */
bool AnalogIoT::processDACSetValueMessage(char *topic, char *payload, uint8_t topic_length) {
    // Topic: dac/<%02d>/set/value
    // The first 4 characters are "dac/"
    // The length of the topic must be 16 characters
    // The last 10 characters must be "/set/value"
    // After all these conditions are met, the topic is valid
    // Extract the pin number from the topic
    if (topic_length != 16) {
        return false;
    }
    if (strncmp(topic, "dac/", 4)) {
        return false;
    }
    if (strncmp(topic + 12, "/set/value", 10)) {
        return false;
    }
    uint8_t pin = (topic[4] - '0') * 10 + (topic[5] - '0');
    // Extract the payload
    uint16_t value = atoi(payload);
    // Set the value
    this->card->setDACValue(pin, value);
    return true;
}

/**
 * @brief Processes a message received on the MQTT topic for requesting the state of all DACs.
 * @param topic The topic of the message.
 * @param payload The payload of the message.
 * @param topic_length The length of the topic.
 * @note This function is not meant to be called from user code.
 * @return True if the message was processed, false otherwise.
 */
bool AnalogIoT::processRequestStateMessage(char *topic, char *payload, uint8_t topic_length) {
    // Topic: requeststate
    // The length of the topic must be 12 characters
    // After all these conditions are met, the topic is valid
    if (topic_length != 12) {
        return false;
    }
    if (strncmp(topic, REQUEST_STATE_TOPIC, 12)) {
        return false;
    }
    // Publish the state of all DACs
    this->publishDACs();
    // Publish the state of all ADCs
    this->publishADCs();
    return false;
}

/**
 * @brief Subscribes to all MQTT topics used by the AnalogIoT object.
 * @note This function is called when the MQTT client connects.
 */
void AnalogIoT::subscribe() {
    // There are 4 DACs and 8 ADCs
    // DACs: dac/<%02d>/set/state, dac/<%02d>/set/value, dac/publish_enable
    // ADCs: adc/<%02d>/set/conversion_interval, adc/<%02d>/set/conversion_enabled

    // Subscribe to all set state topics
    char topic[20];
    for (uint8_t i = 0; i < 4; i++) {
        sprintf(topic, "dac/%02d/set/state", i);
        this->subscribeRelative(topic);
    }
    // Subscribe to all set value topics
    for (uint8_t i = 0; i < 4; i++) {
        sprintf(topic, "dac/%02d/set/value", i);
        this->subscribeRelative(topic);
    }
    // Subscribe to all set conversion interval topics
    for (uint8_t i = 0; i < 8; i++) {
        sprintf(topic, "adc/%02d/set/conversion_interval", i);
        this->subscribeRelative(topic);
    }
    // Subscribe to all set conversion enabled topics
    for (uint8_t i = 0; i < 8; i++) {
        sprintf(topic, "adc/%02d/set/conversion_enabled", i);
        this->subscribeRelative(topic);
    }
    // Subscribe to publish enable topic
    this->subscribeRelative("dac/publish_enable");
}
void AnalogIoT::loop() {
    // Iterate over all ADCs and publish if enabled and interval has passed
    uint32_t now = millis();
    for (uint8_t i = 0; i < 8; i++) {
        if (this->adc_publish_enabled[i] && now - this->last_adc_publish > this->adc_conversion_interval[i]) {
            this->publishADC(i);
            this->last_adc_publish = now;
        }
    }
}

/**
 * @brief Publishes the state of all DACs.
 */
void AnalogIoT::publishReport() {
    publishADCs();
    publishDACs();
}

/**
 * @brief Gets the type of the card.
 * @return The type of the card.
 */
uint8_t AnalogIoT::getType() {
    return CARD_TYPE_ANALOG;
}

/**
 * @brief Publishes the state of all DACs.
 */
void AnalogIoT::publishDACs() {
    for (uint8_t i = 0; i < 4; i++) {
        this->publishDAC(i);
    }
}

/**
 * @brief Publishes the state of a DAC.
 * @param pin The pin of the DAC.
 */
void AnalogIoT::publishDAC(uint8_t pin) {
    this->publishDACState(pin);
    this->publishDACValue(pin);
}

/**
 * @brief Publishes the state of a DAC.
 * @param pin The pin of the DAC.
 */
void AnalogIoT::publishDACState(uint8_t pin) {
    char *topic = new char[15];
    sprintf(topic, "dac/%02d/state", pin);
    char *payload = new char[2];
    sprintf(payload, "%d", this->card->getDACState(pin));
    this->publishRelative(topic, payload);
    delete[] topic;
    delete[] payload;
}

/**
 * @brief Publishes the value of a DAC.
 * @param pin The pin of the DAC.
 */
void AnalogIoT::publishDACValue(uint8_t pin) {
    char *topic = new char[15];
    sprintf(topic, "dac/%02d/value", pin);
    char *payload = new char[5];
    sprintf(payload, "%d", this->card->getDACValue(pin));
    this->publishRelative(topic, payload);
    delete[] topic;
    delete[] payload;
}

/**
 * @brief Publishes the state of a DAC.
 * @param pin The pin of the DAC.
 */
void AnalogIoT::handleDACChange(uint8_t pin, uint16_t value) {
    this->publishDAC(pin);
}