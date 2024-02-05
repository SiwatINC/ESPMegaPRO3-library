#include <DigitalOutputRTU.hpp>
DigitalOutputRTU::DigitalOutputRTU()
{
    // constructor implementation
}

DigitalOutputRTU::~DigitalOutputRTU()
{
    // destructor implementation
}

void DigitalOutputRTU::begin(char *remoteBaseTopic, uint8_t remote_card_slot, ESPMegaIoT *iot)
{
    // begin implementation
    this->remoteBaseTopic = remoteBaseTopic;
    this->remoteBaseTopicLength = strlen(remoteBaseTopic);
    this->cardSlot = remote_card_slot;
    this->iot = iot;

}

void DigitalOutputRTU::subscribe()
{
    // Subscribe to all pins
    // State Topic format: <base_topic>/<card_slot>/<pin>/state
    // Value Topic format: <base_topic>/<card_slot>/<pin>/value
    char buffer[100];
    for (uint8_t i = 0; i < 16; i++)
    {
        sprintf(buffer, "%s/%02d/%02d/state", this->remoteBaseTopic, this->cardSlot, i);
        this->iot->subscribe(buffer);
        sprintf(buffer, "%s/%02d/%02d/value", this->remoteBaseTopic, this->cardSlot, i);
        this->iot->subscribe(buffer);
    }
}

void DigitalOutputRTU::digitalWrite(uint8_t pin, bool state)
{
    // Publish state to topic <base_topic>/<card_slot>/<pin>/set/state
    // Publish 4095 to topic <base_topic>/<card_slot>/<pin>/set/value
    char buffer[100];
    sprintf(buffer, "%s/%02d/%02d/set/state", this->remoteBaseTopic, this->cardSlot, pin);
    this->iot->publish(buffer, state ? "1" : "0");
    sprintf(buffer, "%s/%02d/%02d/set/value", this->remoteBaseTopic, this->cardSlot, pin);
    this->iot->publish(buffer, "4095");
}

void DigitalOutputRTU::analogWrite(uint8_t pin, uint16_t value)
{
    // Publish 1 to topic <base_topic>/<card_slot>/<pin>/set/state
    // Publish value to topic <base_topic>/<card_slot>/<pin>/set/value
    char buffer[100];
    sprintf(buffer, "%s/%02d/%02d/set/state", this->remoteBaseTopic, this->cardSlot, pin);
    this->iot->publish(buffer, "1");
    sprintf(buffer, "%s/%02d/%02d/set/value", this->remoteBaseTopic, this->cardSlot, pin);
    char valueBuffer[10];
    itoa(value, valueBuffer, 10);
    this->iot->publish(buffer, valueBuffer);
}

void DigitalOutputRTU::setState(uint8_t pin, bool state)
{
    // Publish state to topic <base_topic>/<card_slot>/<pin>/set/state
    char buffer[100];
    sprintf(buffer, "%s/%02d/%02d/set/state", this->remoteBaseTopic, this->cardSlot, pin);
    this->iot->publish(buffer, state ? "1" : "0");
}

void DigitalOutputRTU::setValue(uint8_t pin, uint16_t value)
{
    // Publish value to topic <base_topic>/<card_slot>/<pin>/set/value
    char buffer[100];
    sprintf(buffer, "%s/%02d/%02d/set/value", this->remoteBaseTopic, this->cardSlot, pin);
    char valueBuffer[10];
    itoa(value, valueBuffer, 10);
    this->iot->publish(buffer, valueBuffer);
}

bool DigitalOutputRTU::getState(uint8_t pin)
{
    return this->outputStateBuffer[pin];
}

uint16_t DigitalOutputRTU::getValue(uint8_t pin)
{
    return this->outputValueBuffer[pin];
}

uint8_t DigitalOutputRTU::registerChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback)
{
    this->change_callbacks[callbackCount] = callback;
    return callbackCount++;
}

void DigitalOutputRTU::unregisterChangeCallback(uint8_t handler)
{
    this->change_callbacks.erase(handler);
}

void DigitalOutputRTU::mqttCallback(char *topic, char *payload) {
    // First trim base topic from topic
    char* topicWithoutBase = topic + this->remoteBaseTopicLength;
    // Then check if topic is in format /<cardSlot>/<pin>/state or /<cardSlot>/<pin>/value
    // Example trimmed topic: /01/00/state, /01/00/value
    // Card slot range from 00 to 99
    // Pin range from 00 to 15
    // Note that both card and pin are padded with 0 if they are less than 10
    if(strlen(topicWithoutBase) != 12)
        return;
    if(topicWithoutBase[0] != '/')
        return;
    if(topicWithoutBase[3] != '/')
        return;
    if(topicWithoutBase[6] != '/')
        return;
    // Extract card slot and pin
    uint8_t cardSlot = (topicWithoutBase[1] - '0') * 10 + (topicWithoutBase[2] - '0');
    // Check if card slot is correct
    if(cardSlot != this->cardSlot)
        return;
    uint8_t pin = (topicWithoutBase[4] - '0') * 10 + (topicWithoutBase[5] - '0');
    // Extract value
    uint16_t value = atoi(payload);
    // Is it state or value?
    if(!strcmp(topicWithoutBase + 7, "state"))
    {
        // Update state buffer
        this->outputStateBuffer[pin] = strcmp(payload, "1") == 0;
        // Call callbacks
        for(auto const& callback : this->change_callbacks)
        {
            callback.second(pin, this->outputStateBuffer[pin], this->outputValueBuffer[pin]);
        }
    }
    else if(!strcmp(topicWithoutBase + 7, "value"))
    {
        // Update value buffer
        this->outputValueBuffer[pin] = value;
        // Call callbacks
        for(auto const& callback : this->change_callbacks)
        {
            callback.second(pin, this->outputStateBuffer[pin], this->outputValueBuffer[pin]);
        }
    }
}