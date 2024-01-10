#pragma once
#include <driver/rmt.h>

/**
 * @brief Class for sending IR signals
 * 
 * @warning This class takes one of the RMT channels, The ESP32 has 8 RMT channels, so you can use 7 IRBlaster objects at the same time (or 6 if you use the IRReceiver class)
*/
class IRBlaster
{
    public:
        IRBlaster(const uint8_t pin, rmt_channel_t channel);
        IRBlaster(const uint8_t pin);
        ~IRBlaster();
        void send(const uint16_t *data, const size_t size);
    private:
        rmt_channel_t channel;
};