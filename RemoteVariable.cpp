#include <RemoteVariable.hpp>

/**
 * @brief Construct a new Remote Variable object
 * This constructor does not take any arguments.
 * Please use the begin method to initialize the object.
*/
RemoteVariable::RemoteVariable() {
    this->topic = nullptr;
    this->value = nullptr;
}

/**
 * @brief Destroy the Remote Variable object
 * This destructor does not take any arguments.
 * It will free the memory allocated for the topic and value.
*/
RemoteVariable::~RemoteVariable() {
    free(this->value);
}

/**
 * @brief Initialize the RemoteVariable object with value request
 * 
 * This method is used to initialize the RemoteVariable object.
 * 
 * @param size The maximum size of the variable in bytes
 * @param topic The topic that the variable is published to
 * @param iot The ESPMegaIoT object
 * @param useValueRequest Whether to use value request
 * @param valueRequestTopic The topic to request value
 * 
 * @note Because the value is null terminated, the size should be the desired text length + 1
 * @warning This use dynamic memory allocation, so it is important to call the destructor when the object is no longer needed
*/
void RemoteVariable::begin(size_t size, const char* topic, ESPMegaIoT* iot, bool useValueRequest, const char* valueRequestTopic) {
    this->iot = iot;
    this->size = size;
    this->topic = topic;
    this->useValueRequest = useValueRequest;
    this->valueRequestTopic = valueRequestTopic;
    value = (char*)calloc(size, sizeof(char));
    auto bindedMqttCallback = std::bind(&RemoteVariable::mqtt_callback, this, std::placeholders::_1, std::placeholders::_2);
    this->iot->registerMqttCallback(bindedMqttCallback);
    auto bindedSubscribe = std::bind(&RemoteVariable::subscribe, this);
    this->iot->registerSubscribeCallback(bindedSubscribe);
    this->subscribe();
}

/**
 * @brief Initialize the RemoteVariable object
 * 
 * This method is used to initialize the RemoteVariable object.
 * 
 * @param size The maximum size of the variable in bytes
 * @param topic The topic that the variable is published to
 * @param iot The ESPMegaIoT object
 * 
 * @note Because the value is null terminated, the size should be the desired text length + 1
 * @warning This use dynamic memory allocation, so it is important to call the destructor when the object is no longer needed
*/
void RemoteVariable::begin(size_t size, const char* topic, ESPMegaIoT* iot) {
    this->begin(size, topic, iot, false, nullptr);
}

/**
 * @brief Subscribe to the topic
 * This method is used internally to subscribe to the topic from iot core
*/
void RemoteVariable::subscribe() {
    ESP_LOGD("RemoteVariable", "Subscribing to %s", this->topic);
    this->iot->subscribe(this->topic);
    if(this->useValueRequest) {
        ESP_LOGD("RemoteVariable", "Subscribing to %s", this->valueRequestTopic);
        this->requestValue();
    }
}

/**
 * @brief Get the value of the variable
 * 
 * This method is used to get the value of the variable.
 * 
 * @return char* The null terminated string of the value
*/
char* RemoteVariable::getValue() {
    return this->value;
}

/**
 * @brief The MQTT callback
 * This method is binded to the MQTT callback in iot core
*/
void RemoteVariable::mqtt_callback(char* topic, char* payload) {
    if (strcmp(topic, this->topic) == 0) {
        ESP_LOGD("RemoteVariable", "Received MQTT message from %s", topic);
        strcpy(this->value, payload);
        for (auto& callback : this->valueChangeCallback) {
            callback.second(this->value);
        }
    }
}

/**
 * @brief Request the value of the variable
 * This method request a value update from the device that has the variable
 * @note This method is only functional if the useValueRequest is set to true
*/
void RemoteVariable::requestValue() {
    if(!this->useValueRequest)
        return;
    ESP_LOGD("RemoteVariable", "Sending request to %s", this->valueRequestTopic);
    this->iot->publish(this->valueRequestTopic, "request");
}

/**
 * @brief Enable the set value feature
 * This method is used to enable the set value feature
 * @param setValueTopic The topic to send the value to
*/
void RemoteVariable::enableSetValue(const char* setValueTopic) {
    this->useSetValue = true;
    this->setValueTopic = setValueTopic;
}

/**
 * @brief Set the value of the variable
 * This method is used to set the value of the variable
 * @param value The value to set
 * @note This method is only functional if the enableSetValue is already called
*/
void RemoteVariable::setValue(const char* value) {
    if(!this->useSetValue)
        return;
    this->iot->publish(this->setValueTopic, value);
}

/**
 * @brief Register a callback for value change
 * This method will be called when the value of the variable changes on the remote device
 * @param callback The callback function
 * @return uint8_t The handler for the callback
*/
uint8_t RemoteVariable::registerCallback(std::function<void(char*)> callback) {
    this->valueChangeCallback[this->valueChangeCallbackCount] = callback;
    return valueChangeCallbackCount++;
}

/**
 * @brief Unregister a callback
 * This method is used to unregister a callback
 * @param handler The handler of the callback
*/
void RemoteVariable::unregisterCallback(uint8_t handler) {
    this->valueChangeCallback.erase(handler);
}