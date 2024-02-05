#pragma once
#include <ESPMegaRTU.hpp>

class DigitalInputRTU : public ESPMegaRTU {
    public:
        DigitalInputRTU();
        ~DigitalInputRTU();
        void begin(char* remoteBaseTopic, uint8_t remote_card_slot, ESPMegaIoT* iot);
        void subscribe();
        bool digitalRead(uint8_t pin);
        uint8_t registerCallback(std::function<void(uint8_t, bool)> callback);
        void unregisterCallback(uint8_t handler);
    private:
        void mqttCallback(char* topic, char* payload);
        ESPMegaIoT* iot;
        size_t size;
        uint16_t callbackCount;
        std::map<uint8_t, std::function<void(uint8_t, bool)>> callbacks;
        bool inputBuffer[16];
};