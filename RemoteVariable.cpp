#include <RemoteVariable.hpp>

RemoteVariable::RemoteVariable() {
    this->topic = nullptr;
    this->value = nullptr;
}
RemoteVariable::~RemoteVariable() {
    delete this->topic;
    delete this->value;
}
void RemoteVariable::begin(size_t size, char* topic, ESPMegaIoT* iot, bool useValueRequest, char* valueRequestTopic) {
    this->iot = iot;
    this->size = size;
    topic = (char*)calloc(size, sizeof(char));
    value = (char*)calloc(size, sizeof(char));
    this->valueRequestTopic = nullptr;
    auto bindedMqttCallback = std::bind(&RemoteVariable::mqtt_callback, this, std::placeholders::_1, std::placeholders::_2);
    this->iot->registerMqttCallback(bindedMqttCallback);
    auto bindedSubscribe = std::bind(&RemoteVariable::subscribe, this);
    this->iot->registerSubscribeCallback(bindedSubscribe);
}

void RemoteVariable::begin(size_t size, char* topic, ESPMegaIoT* iot) {
    this->begin(size, topic, iot, false, nullptr);
}

void RemoteVariable::subscribe() {
    this->iot->subscribe(this->topic);
    if(this->useValueRequest) {
        this->requestValue();
    }
}
char* RemoteVariable::getValue() {
    return this->getValue();
}

void RemoteVariable::mqtt_callback(char* topic, char* payload) {
    if (strcmp(topic, this->topic) == 0) {
        strcpy(this->value, payload);
    }
}

void RemoteVariable::requestValue() {
    if(!this->useValueRequest)
        return;
    this->iot->publish(this->valueRequestTopic, "request");
}

void RemoteVariable::enableSetValue(char* setValueTopic) {
    this->useSetValue = true;
    this->setValueTopic = setValueTopic;
}

void RemoteVariable::setValue(char* value) {
    if(!this->useSetValue)
        return;
    this->iot->publish(this->setValueTopic, value);
}