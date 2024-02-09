#include <RemoteVariable.hpp>

RemoteVariable::RemoteVariable() {
    this->topic = nullptr;
    this->value = nullptr;
}
RemoteVariable::~RemoteVariable() {
    delete this->topic;
    delete this->value;
}
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

void RemoteVariable::begin(size_t size, const char* topic, ESPMegaIoT* iot) {
    this->begin(size, topic, iot, false, nullptr);
}

void RemoteVariable::subscribe() {
    ESP_LOGD("RemoteVariable", "Subscribing to %s", this->topic);
    this->iot->subscribe(this->topic);
    if(this->useValueRequest) {
        ESP_LOGD("RemoteVariable", "Subscribing to %s", this->valueRequestTopic);
        this->requestValue();
    }
}
char* RemoteVariable::getValue() {
    return this->value;
}

void RemoteVariable::mqtt_callback(char* topic, char* payload) {
    if (strcmp(topic, this->topic) == 0) {
        ESP_LOGD("RemoteVariable", "Received MQTT message from %s", topic);
        strcpy(this->value, payload);
    }
}

void RemoteVariable::requestValue() {
    if(!this->useValueRequest)
        return;
    ESP_LOGD("RemoteVariable", "Sending request to %s", this->valueRequestTopic);
    this->iot->publish(this->valueRequestTopic, "request");
}

void RemoteVariable::enableSetValue(const char* setValueTopic) {
    this->useSetValue = true;
    this->setValueTopic = setValueTopic;
}

void RemoteVariable::setValue(const char* value) {
    if(!this->useSetValue)
        return;
    this->iot->publish(this->setValueTopic, value);
}