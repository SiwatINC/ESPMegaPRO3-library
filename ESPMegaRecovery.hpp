#include <FRAM.h>
#include <ESPMegaWebServer.hpp>
#include <esp_log.h>
/**
 * @brief Recovery mode for ESPMega
 * Recovery mode is a mode that is entered when the device is in a bootloop
 * In this mode, the device will block all other tasks and wait for a new firmware to be uploaded over the air
 * To exit recovery mode, the device must be restarted or a new firmware must be uploaded
 * This is useful when the device is in a bootloop and the user can't access the device to upload a new firmware (ota bricking)
 * The device is considered to be in a bootloop when it is restarted before RECOVERY_WATCHDOG_TIMEOUT seconds for 5 consecutive times
 * The timer is started when the device's initialization is complete (enter loop)
 */

#define RECOVERY_WATCHDOG_TIMEOUT 15 // The time in seconds to wait for a restart before considering it a bootloop

class ESPMegaRecovery {
public:
    ESPMegaRecovery();
    void begin();
    void loop();
    void enterRecoveryMode(); // Enter recovery mode, Block all other tasks, start OTA server
    void bindFRAM(FRAM* fram, uint32_t address); // Bind the FRAM block to store the bootloop counter
    uint8_t getBootloopCounter(); // Get the bootloop counter
    void inclementBootloopCounter(); // Increment the bootloop counter
    void resetBootloopCounter(); // Reset the bootloop counter
    bool isRecoveryMode(); // Check if the device is in recovery mode
private:
    void getDeviceInfoHandler(AsyncWebServerRequest *request);
    void configHandler(AsyncWebServerRequest *request);
    FRAM* fram;
    uint32_t fram_address;
    uint8_t bootloop_counter;
    ESPMegaIoT* iot;
    ESPMegaWebServer* web_server;
    bool recovery_mode;
};