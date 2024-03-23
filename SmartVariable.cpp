#include "SmartVariable.hpp"
SmartVariable::SmartVariable()
{
}

SmartVariable::~SmartVariable()
{
    if (this->value != nullptr)
        free(this->value);
}

void SmartVariable::begin(size_t size)
{
    this->value = (char *)calloc(size, sizeof(char));
    this->size = size;
}

void SmartVariable::enableIoT(ESPMegaIoT *iot, const char *topic)
{
    this->iot = iot;
    this->iotEnabled = true;
    this->topic = topic;
    ESP_LOGV("SmartVariable", "Binding MQTT Callback");
    auto bindedMqttCallback = std::bind(&SmartVariable::handleMqttCallback, this, std::placeholders::_1, std::placeholders::_2);
    this->iot->registerMqttCallback(bindedMqttCallback);
    ESP_LOGV("SmartVariable", "Binding MQTT Subscription");
    auto bindedMqttSubscription = std::bind(&SmartVariable::subscribeMqtt, this);
    this->iot->registerSubscribeCallback(bindedMqttSubscription);
    ESP_LOGI("SmartVariable", "Calling MQTT Subscribe");
    this->subscribeMqtt();
}

void SmartVariable::enableValueRequest(const char *valueRequestTopic)
{
    ESP_LOGD("SmartVariable", "Enabling Value Request");
    this->useValueRequest = true;
    this->valueRequestTopic = valueRequestTopic;
    this->subscribeMqtt();
}

void SmartVariable::setValue(const char *value)
{
    strncpy(this->value, value, this->size - 1);
    this->value[this->size - 1] = '\0';
    if (this->autoSave)
        this->saveValue();
    if (this->iotEnabled)
        this->publishValue();
}

char *SmartVariable::getValue()
{
    return this->value;
}

void SmartVariable::enableSetValue(const char *setValueTopic)
{
    this->setValueEnabled = true;
    this->setValueTopic = setValueTopic;
    this->subscribeMqtt();
}

void SmartVariable::publishValue()
{
    if (this->iotEnabled) {
        if (this->value == nullptr) {
            ESP_LOGE("SmartVariable", "Value is NULL");
            return;
        }
        if (this->topic == nullptr) {
            ESP_LOGE("SmartVariable", "Topic is NULL");
            return;
        }
        ESP_LOGV("SmartVariable", "Publishing Value: %s to %s", this->value, this->topic);
        this->iot->publish(this->topic, this->value);
    }
}

void SmartVariable::bindFRAM(FRAM *fram, uint32_t framAddress)
{
    this->bindFRAM(fram, framAddress, true);
}

void SmartVariable::bindFRAM(FRAM *fram, uint32_t framAddress, bool loadValue)
{
    this->framAddress = framAddress;
    this->fram = fram;
    if (loadValue)
        this->loadValue();
}

void SmartVariable::loadValue()
{
    this->fram->read(this->framAddress, (uint8_t *)this->value, this->size);
    this->setValue(this->value);
}

void SmartVariable::saveValue()
{
    this->fram->write(this->framAddress, (uint8_t *)this->value, this->size);
}

void SmartVariable::setValueAutoSave(bool autoSave)
{
    this->autoSave = autoSave;
}

uint16_t SmartVariable::registerCallback(std::function<void(char *)> callback)
{
    this->valueChangeCallbacks[this->currentHandlerId] = callback;
    return this->currentHandlerId++;
}

void SmartVariable::unregisterCallback(uint16_t handlerId)
{
    this->valueChangeCallbacks.erase(handlerId);
}

void SmartVariable::handleMqttCallback(char *topic, char *payload)
{
    if (!strcmp(topic, this->valueRequestTopic))
    {
        this->publishValue();
    }
    else if (!strcmp(topic, this->setValueTopic))
    {
        this->setValue(payload);
    }
}

void SmartVariable::subscribeMqtt()
{

    if (this->iotEnabled)
    {
        ESP_LOGV("SmartVariable", "IoT Enabled, running MQTT Subscribe");
        ESP_LOGV("SmartVariable", "Value Request: %d, Set Value: %d", this->useValueRequest, this->setValueEnabled);
        if (this->useValueRequest) {
            if (this->valueRequestTopic == nullptr) {
                ESP_LOGE("SmartVariable", "Value Request Topic is NULL");
                return;
            }
            ESP_LOGV("SmartVariable", "Subscribing to %s", this->valueRequestTopic);
            this->iot->subscribe(this->valueRequestTopic);
        }
        if (this->setValueEnabled) {
            if (this->setValueTopic == nullptr) {
                ESP_LOGE("SmartVariable", "Set Value Topic is NULL");
                return;
            }
            ESP_LOGV("SmartVariable", "Subscribing to %s", this->setValueTopic);
            this->iot->subscribe(this->setValueTopic);
        }
        ESP_LOGV("SmartVariable", "Publishing Value");
        this->publishValue();
    }
}

int32_t SmartVariable::getIntValue()
{
    return atoi(this->value);
}

void SmartVariable::setIntValue(int32_t value)
{
    itoa(value, this->value, 10);
    this->setValue(this->value);
}

double SmartVariable::getDoubleValue()
{
    return atof(this->value);
}

void SmartVariable::setDoubleValue(double value)
{
    dtostrf(value, 0, 2, this->value);
    this->setValue(this->value);
}
