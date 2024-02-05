#pragma once
#include <ESPMegaIoT.hpp>

class ESPMegaRTU {
    public:
        ESPMegaRTU();
        ~ESPMegaRTU();
        virtual void subscribe();
        virtual void begin(char* remote_base_topic, uint8_t remote_card_slot, ESPMegaIoT* iot);
    protected:
        char* remoteBaseTopic;
        uint8_t remoteBaseTopicLength;
        uint8_t cardSlot;
        ESPMegaIoT* iot;
};