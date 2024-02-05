#pragma once
#include <ESPMegaRTU.hpp>
#include <DigitalOutputCard.hpp>

class DigitalOutputRTU : public ESPMegaRTU
{
public:
    DigitalOutputRTU();
    ~DigitalOutputRTU();
    void begin(char *remoteBaseTopic, uint8_t remote_card_slot, ESPMegaIoT *iot);
    void subscribe();
    void digitalWrite(uint8_t pin, bool state);
    void analogWrite(uint8_t pin, uint16_t value);
    void setState(uint8_t pin, bool state);
    void setValue(uint8_t pin, uint16_t value);
    bool getState(uint8_t pin);
    uint16_t getValue(uint8_t pin);
    uint8_t registerChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback);
    void unregisterChangeCallback(uint8_t handler);

private:
    void mqttCallback(char *topic, char *payload);
    ESPMegaIoT *iot;
    size_t size;
    uint16_t callbackCount;
    std::map<uint8_t, std::function<void(uint8_t, bool, uint16_t)>> change_callbacks;
    bool outputStateBuffer[16];
    uint16_t outputValueBuffer[16];
};