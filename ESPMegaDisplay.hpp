#pragma once
#include <Arduino.h>
#include <map>

#define DISPLAY_FETCH_TIMEOUT 100 // ms
#define DISPLAY_FETCH_RETRY_COUNT 5

/**
 * @brief The ESPMegaDisplay class is a class for controlling the ESPMegaDisplay.
 * 
 * @note The ESPMegaDisplay is a UART controlled display.
 * @note Connect the Display's TX pin to the ESPMega's RX pin and the Display's RX pin to the ESPMega's TX pin.
 */
class ESPMegaDisplay
{
    public:
        ESPMegaDisplay(HardwareSerial *displayAdapter);
        void begin();
        void loop();
        void reset();
        void setBrightness(int value);
        void setVolume(int value);
        void jumpToPage(int page);
        void setString(const char* component, const char* value);
        void setNumber(const char* component, int value);
        const char* getString(const char* component);
        bool getStringToBuffer(const char* component, char* buffer, uint8_t buffer_size);
        uint32_t getNumber(const char* component);
        uint16_t registerTouchCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback);
        void unregisterTouchCallback(uint16_t handle);
        uint16_t registerPageChangeCallback(std::function<void(uint8_t)> callback);
        void unregisterPageChangeCallback(uint16_t handle);
        uint16_t registerPayloadCallback(std::function<void(uint8_t, uint8_t*, uint8_t)> callback);
        void unregisterPayloadCallback(uint16_t handle);
        
    protected:
        uint8_t currentPage;
        uint8_t rx_buffer_index;
        char rx_buffer[256];
        char tx_buffer[256];
        bool recieveSerialCommand();
        bool recieveSerialCommand(bool process);
        void processSerialCommand();
        void processTouchPayload();
        void processPageReportPayload();
        void sendStopBytes();
        void sendCommand(char* command);
        bool payloadIsValid();
        bool waitForValidPayload(uint32_t timeout);
        HardwareSerial *displayAdapter;
        std::map<uint16_t, std::function<void(uint8_t, uint8_t, uint8_t)>> touch_callbacks;
        std::map<uint16_t, std::function<void(uint8_t)>> page_change_callbacks;
        std::map<uint16_t, std::function<void(uint8_t, uint8_t*, uint8_t)>> payload_callbacks;
};