#include <IRReceiver.hpp>
#include <functional>

volatile unsigned int IRReceiver::irBufferPtr = 0;
volatile unsigned int IRReceiver::irBuffer[1000];
uint8_t IRReceiver::pin;

void IRReceiver::begin(uint8_t pin) {
    IRReceiver::pin = pin;
    IRReceiver::irBufferPtr = 0;
}

void IRReceiver::start_long_receive() {
    irBufferPtr = 0;
    attachInterrupt(digitalPinToInterrupt(pin), IRReceiver::handleInterrupt, CHANGE);
}

ir_data_t IRReceiver::end_long_receive() {
    detachInterrupt(digitalPinToInterrupt(pin));
    // The data in the array is the time between each transition, so we need to convert it to the time of each transition
    ir_data_t data;
    const size_t size = irBufferPtr-1;
    data.data = (unsigned int*)calloc(size, sizeof(unsigned int));
    if (data.data == nullptr) {
        data.size = 0;
        return data;
    }
    // The data in the array is the time between each transition, so we need to convert it to the time of each transition
    for (size_t i = 1; i <= size; i++) {
        data.data[i-1] = irBuffer[i] - irBuffer[i-1];
    }
    //memcpy(data.data, (const unsigned int*)irBuffer, (size)*sizeof(unsigned int));
    Serial.println("Done");
    data.size = irBufferPtr;
    return data;
}

void IRAM_ATTR IRReceiver::handleInterrupt() {
    if (irBufferPtr > 1000) return; //full buffer = bad data
    irBuffer[irBufferPtr++] = micros(); //just continually record the time-stamp of signal transitions  
}