#include <ClimateIoT.hpp>


ClimateIoT::ClimateIoT() {

}

/**
 * @brief Destructor for the ClimateIoT class.
 */
ClimateIoT::~ClimateIoT() {
    // Destructor implementation
}

/**
 * @brief Initializes the ClimateIoT component.
 * 
 * This function sets the MQTT client, base topic, card ID, and card pointer.
 * It also registers the sensor and air conditioner update callbacks.
 * 
 * @param card_id The ID of the expansion card.
 * @param card A pointer to the ExpansionCard object.
 * @param mqtt A pointer to the PubSubClient object.
 * @param base_topic The base topic for MQTT communication.
 * @return True if the initialization is successful, false otherwise.
 */
bool ClimateIoT::begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic) {
    this->mqtt = mqtt;
    this->base_topic = base_topic;
    this->card_id = card_id;
    this->card = (ClimateCard *)card;
    // Reister Callbacks
    auto bindedSensorCallback = std::bind(&ClimateIoT::handleSensorUpdate, this, std::placeholders::_1, std::placeholders::_2);
    this->card->registerSensorCallback(bindedSensorCallback);
    auto bindedAirConditionerCallback = std::bind(&ClimateIoT::handleAirConditionerUpdate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->card->registerChangeCallback(bindedAirConditionerCallback);
    ESP_LOGD("ClimateIoT", "Climate IoT Component initialized");
    return true;
}

/**
 * @brief Handles MQTT messages for the ClimateIoT component.
 * 
 * @param topic The trimmed topic of the MQTT message.
 * @param payload The null-terminated payload of the MQTT message.
 */
void ClimateIoT::handleMqttMessage(char *topic, char *payload) {
    uint8_t topic_length = strlen(topic);
    if (this->processSetTemperatureMessage(topic, payload, topic_length))
        return;
    if (this->processSetModeMessage(topic, payload, topic_length))
        return;
    if (this->processSetFanSpeedMessage(topic, payload, topic_length))
        return;
    if (this->processRequestStateMessage(topic, payload, topic_length))
        return;
}

/**
 * @brief Publishes the temperature of the air conditioner to the MQTT broker.
 */
void ClimateIoT::publishClimateTemperature() {
    char payload[5];
    itoa(this->card->getTemperature(), payload, 10);
    this->publishRelative(AC_TEMPERATURE_REPORT_TOPIC, payload);
}

/**
 * @brief Publishes the mode of the air conditioner to the MQTT broker.
 */
void ClimateIoT::publishClimateMode() {
    this->publishRelative(AC_MODE_REPORT_TOPIC, this->card->getModeName());
}

/**
 * @brief Publishes the fan speed of the air conditioner to the MQTT broker.
 */
void ClimateIoT::publishClimateFanSpeed() {
    this->publishRelative(AC_FAN_SPEED_REPORT_TOPIC, this->card->getFanSpeedName());
}

/**
 * @brief Publishes the temperature and humidity of the room to the MQTT broker.
 */
void ClimateIoT::publishSensor() {
    this->publishRoomTemperature();
    this->publishHumidity();
}

/**
 * @brief Publishes the climate data (temperature, mode, fan speed) to the MQTT broker.
 */
void ClimateIoT::publishClimate() {
    this->publishClimateTemperature();
    this->publishClimateMode();
    this->publishClimateFanSpeed();
}

/**
 * @brief Publishes the room temperature to the MQTT broker.
 */
void ClimateIoT::publishRoomTemperature() {
    if (this->card->getSensorType() == AC_SENSOR_TYPE_NONE ) {
        return;
    }
    char payload[5];
    itoa(this->card->getRoomTemperature(), payload, 10);
    this->publishRelative(AC_ROOM_TEMPERATURE_REPORT_TOPIC, payload);
}

/**
 * @brief Publishes the humidity of the room to the MQTT broker.
 */
void ClimateIoT::publishHumidity() {
    if (this->card->getSensorType() == AC_SENSOR_TYPE_DHT22) {
        char payload[5];
        itoa(this->card->getHumidity(), payload, 10);
        this->publishRelative(AC_HUMIDITY_REPORT_TOPIC, payload);
    }
}

/**
 * @brief Handle Air Conditioner state change.
 * 
 * @note This function is called by the underlying ClimateCard object and is not meant to be called manually.
 * 
 * @param temperature Temperature of the air conditioner
 * @param mode Mode of the air conditioner
 * @param fan_speed Fan speed of the air conditioner 
 */
void ClimateIoT::handleStateChange(uint8_t temperature, uint8_t mode, uint8_t fan_speed) {
    this->publishClimate();
}

/**
 * @brief Publishes the climate and sensor data to the MQTT broker.
 */
void ClimateIoT::publishReport() {
    this->publishClimate();
    this->publishSensor();
}

/**
 * @brief Subscribes to MQTT topics.
 */
void ClimateIoT::subscribe() {
    ESP_LOGD("ClimateIoT", " topics");
    this->subscribeRelative(AC_TEMPERATURE_SET_TOPIC);
    this->subscribeRelative(AC_MODE_SET_TOPIC);
    this->subscribeRelative(AC_FAN_SPEED_SET_TOPIC);
    ESP_LOGD("ClimateIoT", "Subscribed to topics");
}

/**
 * @brief The loop function for the ClimateIoT component.
 * 
 * @note This function does nothing.
 */
void ClimateIoT::loop() {
    
}

/**
 * @brief Returns the type of the expansion card.
 * 
 * @return The type of the expansion card.
 */
uint8_t ClimateIoT::getType() {
    return CARD_TYPE_CLIMATE;
}

/**
 * @brief Processes the set temperature MQTT message.
 * 
 * @param topic The trimmed topic of the MQTT message.
 * @param payload The null-terminated payload of the MQTT message.
 * @param topic_length The length of the topic.
 * @return True if the message is processed, false otherwise.
 */
bool ClimateIoT::processSetTemperatureMessage(char *topic, char *payload, uint8_t topic_length) {
    if (!strcmp(topic, AC_TEMPERATURE_SET_TOPIC)) {
        uint8_t temperature = atoi(payload);
        this->card->setTemperature(temperature);
        return true;
    }
    return false;
}

/**
 * @brief Processes the set mode MQTT message.
 * 
 * @param topic The trimmed topic of the MQTT message.
 * @param payload The null-terminated payload of the MQTT message.
 * @param topic_length The length of the topic.
 * @return True if the message is processed, false otherwise.
 */
bool ClimateIoT::processSetModeMessage(char *topic, char *payload, uint8_t topic_length) {
    if (!strcmp(topic, AC_MODE_SET_TOPIC)) {
        this->card->setModeByName(payload);
        return true;
    }
    return false;
}

/**
 * @brief Processes the set fan speed MQTT message.
 * 
 * @param topic The trimmed topic of the MQTT message.
 * @param payload The null-terminated payload of the MQTT message.
 * @param topic_length The length of the topic.
 * @return True if the message is processed, false otherwise.
 */
bool ClimateIoT::processSetFanSpeedMessage(char *topic, char *payload, uint8_t topic_length) {
    if (!strcmp(topic, AC_FAN_SPEED_SET_TOPIC)) {
        this->card->setFanSpeedByName(payload);
    }
    return false;
}

/**
 * @brief Processes the request state MQTT message.
 * 
 * @param topic The trimmed topic of the MQTT message.
 * @param payload The null-terminated payload of the MQTT message.
 * @param topic_length The length of the topic.
 * @return True if the message is processed, false otherwise.
 */
bool ClimateIoT::processRequestStateMessage(char *topic, char *payload, uint8_t topic_length) {
    if (!strcmp(topic, AC_REQUEST_STATE_TOPIC)) {
        this->publishReport();
        return true;
    }
    return false;
}

/**
 * @brief This function is a callback function registered with the Climate card to be called when the sensor data is updated.
 * 
 * @param temperature The room temperature.
 * @param humidity The room humidity.
 * 
 * @note The temperature and humidity are not used in this function but are required by the ClimateCard class to match the signature of the callback function.
 */
void ClimateIoT::handleSensorUpdate(float temperature, float humidity) {
    this->publishSensor();
}

/**
 * @brief This function is a callback function registered with the Climate card to be called when the air conditioner state is updated.
 * 
 * @param mode The mode of the air conditioner.
 * @param fan_speed The fan speed of the air conditioner.
 * @param temperature The temperature of the air conditioner.
 */
void ClimateIoT::handleAirConditionerUpdate(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    this->publishClimate();
}