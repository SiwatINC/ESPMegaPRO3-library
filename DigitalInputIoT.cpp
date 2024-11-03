#include <DigitalInputIoT.hpp>


/**
 * @brief Initializes the DigitalInputIoT object.
 * 
 * This function sets the necessary parameters for the DigitalInputIoT object, such as the card ID, expansion card, MQTT client, and base topic.
 * It also enables the publishing of digital input values and registers a callback function for handling value changes.
 * 
 * @note Although this function can be called in the main program, it is recommended to use ESPMegaIoT::registerCard() to automatically manage the instantiation and initialization of this component.
 * 
 * @param card_id The ID of the card.
 * @param card Pointer to the DigitalInputCard object.
 * @param mqtt Pointer to the PubSubClient object.
 * @param base_topic The base topic for MQTT communication.
 * @return True if the initialization is successful, false otherwise.
 */
bool DigitalInputIoT::begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic) {
    this->card = (DigitalInputCard *)card;
    if(!this->card->getStatus()) {
        return false;
    }
    this->card_id = card_id;
    this->mqtt = mqtt;
    this->base_topic = base_topic;
    this->setDigitalInputsPublishEnabled(true);
    this->card->registerCallback(std::bind(&DigitalInputIoT::handleValueChange, this, std::placeholders::_1, std::placeholders::_2));
    return true;

}

/**
 * @brief Subscribes to the MQTT topics for the DigitalInputIoT component.
 */
void DigitalInputIoT::subscribe() {
    this->subscribeRelative(PUBLISH_ENABLE_TOPIC);
    this->subscribeRelative(INPUT_REQUEST_STATE_TOPIC);
}

/**
 * @brief Handles MQTT messages for the DigitalInputIoT component.
 * 
 * @param topic The trimmed topic of the MQTT message.
 * @param payload The null-terminated payload of the MQTT message.
 */
void DigitalInputIoT::handleMqttMessage(char *topic, char *payload) {
    if (!strcmp(topic, INPUT_REQUEST_STATE_TOPIC)) {
        this->publishDigitalInputs();
    }
    // payload is char '0' or '1'
    else if (!strcmp(topic, PUBLISH_ENABLE_TOPIC)) {
        if (payload[0] == '1') {
            this->setDigitalInputsPublishEnabled(true);
        } else {
            this->setDigitalInputsPublishEnabled(false);
        }
    }

}

/**
 * @brief Publish all digital inputs to the MQTT broker.
 */
void DigitalInputIoT::publishDigitalInputs() {
    if (!this->digital_inputs_publish_enabled) {
        return;
    }
    for (int i = 0; i < 16; i++) {
        this->publishDigitalInput(i);
    }
}

/**
 * @brief Set if the digital inputs should be published to the MQTT broker.
 * 
 * @param enabled True if the digital inputs should be published, false otherwise.
 */
void DigitalInputIoT::setDigitalInputsPublishEnabled(bool enabled) {
    this->digital_inputs_publish_enabled = enabled;
    if (enabled) {
        this->publishDigitalInputs();
    }
}

/**
 * @brief Handles a value change for a digital input.
 * 
 * @note This function is registered as a callback function for the DigitalInputCard object.
 * 
 * @param pin The pin that changed.
 * @param value The new value of the pin.
 */
void DigitalInputIoT::handleValueChange(uint8_t pin, uint8_t value) {
    if (this->digital_inputs_publish_enabled) {
        this->publishDigitalInput(pin);
    }

}

/**
 * @brief Publish all inputs to the MQTT Broker
 * 
 * @note This function is overriden from the IoTComponent class and is called by ESPMegaIoT.
 * 
 * @param pin The pin to publish.
 */
void DigitalInputIoT::publishReport() {
    this->publishDigitalInputs();
}
uint8_t DigitalInputIoT::getType() {
    return CARD_TYPE_DIGITAL_INPUT;
}

/**
 * @brief Publish a digital input to the MQTT broker.
 * 
 * @param pin The pin to publish.
 */
void DigitalInputIoT::publishDigitalInput(uint8_t pin) {
    char topic[20] = {0};
    char payload[20] = {0};
    topic[0] = pin/10 + '0';
    topic[1] = pin%10 + '0';
    topic[2] = '\0';
    payload[0] = this->card->digitalRead(pin, false) + '0';
    payload[1] = '\0';
    this->publishRelative(topic, payload);
}