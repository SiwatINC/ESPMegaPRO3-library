#pragma once
#include <Arduino.h>
#include <map>
#include <functional>

struct ir_data_t {
    unsigned int *data;
    size_t size;
};

/**
 * @brief Class for receiving IR signals
 * 
 * @warning Only one IRReceiver can be used at a time
*/
class IRReceiver {
    public:
        static void begin(uint8_t pin);
        static void start_long_receive();
        static ir_data_t end_long_receive();
    private:
        static uint8_t pin;
        static void IRAM_ATTR handleInterrupt();
        static volatile unsigned int irBuffer[1000]; //stores timings - volatile because changed by ISR
        static volatile unsigned int irBufferPtr; //Pointer thru irBuffer - volatile because changed by ISR
};