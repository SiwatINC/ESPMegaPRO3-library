#include <DigitalInputRTU.hpp>

DigitalInputRTU::DigitalInputRTU()
{
    this->iot = nullptr;
    this->size = 0;
    this->callbackCount = 0;
    for (int i = 0; i < 16; i++)
        this->inputBuffer[i] = false;
}

DigitalInputRTU::~DigitalInputRTU()
{

}

void DigitalInputRTU::begin(char *remoteBaseTopic, uint8_t remote_card_slot, ESPMegaIoT *iot)
{
    this->remoteBaseTopic = remoteBaseTopic;
    this->remoteBaseTopicLength = strlen(remoteBaseTopic);
    this->cardSlot = remote_card_slot;
    this->iot = iot;
}

void DigitalInputRTU::subscribe()
{
    // Subscribe to all pins
    for(uint8_t i = 0; i < 16; i++)
    {
        char* topic = (char*)calloc(this->remoteBaseTopicLength + 6, sizeof(char));
        sprintf(topic, "%s/%02d/%02d", this->remoteBaseTopic, this->cardSlot, i);
        this->iot->subscribe(topic);
        free(topic);
    }
}

bool DigitalInputRTU::digitalRead(uint8_t pin)
{
    return this->inputBuffer[pin];
}

uint8_t DigitalInputRTU::registerCallback(std::function<void(uint8_t, bool)> callback)
{
    this->callbacks[callbackCount] = callback;
    return callbackCount++;
}

void DigitalInputRTU::unregisterCallback(uint8_t handler)
{
    this->callbacks.erase(handler);
}

void DigitalInputRTU::mqttCallback(char *topic, char *payload)
{
    // First trim base topic from topic
    char* topicWithoutBase = topic + this->remoteBaseTopicLength;
    // Then check if topic is in format /cardSlot/pin
    // Example trimmed topic: /01/00
    // Card slot range from 00 to 99
    // Pin range from 00 to 15
    // Note that both card and pin are padded with 0 if they are less than 10
    if(strlen(topicWithoutBase) != 6)
        return;
    if(topicWithoutBase[0] != '/')
        return;
    if(topicWithoutBase[3] != '/')
        return;
    // Extract card slot and pin
    uint8_t cardSlot = (topicWithoutBase[1] - '0') * 10 + (topicWithoutBase[2] - '0');
    uint8_t pin = (topicWithoutBase[4] - '0') * 10 + (topicWithoutBase[5] - '0');
    // Check if card slot is correct
    if(cardSlot != this->cardSlot)
        return;
    // Extract value
    bool value = strcmp(payload, "1") == 0;
    // Update input buffer
    this->inputBuffer[pin] = value;
    // Call callbacks
    for(auto const& callback : this->callbacks)
    {
        callback.second(pin, value);
    }
}
