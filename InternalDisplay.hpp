#pragma once
#include <ESPMegaDisplay.hpp>
#include <TimeStructure.hpp>
#include <ESPMegaIoT.hpp>
#include <DigitalInputCard.hpp>
#include <DigitalOutputCard.hpp>
#include <ClimateCard.hpp>

// Page IDs
#define INTERNAL_DISPLAY_BOOT_PAGE 0
#define INTERNAL_DISPLAY_DASHBOARD_PAGE 1
#define INTERNAL_DISPLAY_INPUT_PAGE 2
#define INTERNAL_DISPLAY_OUTPUT_PAGE 3
#define INTERNAL_DISPLAY_AC_PAGE 4
#define INTERNAL_DISPLAY_PWM_ADJUSTMENT_PAGE 5
#define INTERNAL_DISPLAY_NETWORK_CONFIG_PAGE 6
#define INTERNAL_DISPLAY_OTA_PAGE 9
#define INTERNAL_DISPLAY_CLIMATE_NULL_PTR_PAGE 10
#define INTERNAL_DISPLAY_MQTT_CONFIG_PAGE 11
#define INTERNAL_DISPLAY_INPUT_NULL_PTR_PAGE 12
#define INTERNAL_DISPLAY_OUTPUT_NULL_PTR_PAGE 13

// Picture IDs
#define PIC_LAN_DISCONNECTED 2
#define PIC_LAN_CONNECTED 3
#define PIC_MQTT_DISCONNECTED 4
#define PIC_MQTT_CONNECTED 5
#define PIC_PWM_BAR_ON 33
#define PIC_PWM_BAR_OFF 48
#define PIC_AC_MODE_OFF_ACTIVE 24
#define PIC_AC_MODE_OFF_INACTIVE 25
#define PIC_AC_MODE_FAN_ACTIVE 22
#define PIC_AC_MODE_FAN_INACTIVE 23
#define PIC_AC_MODE_COOL_ACTIVE 12
#define PIC_AC_MODE_COOL_INACTIVE 13
#define PIC_AC_FAN_SPEED_AUTO_ACTIVE 14
#define PIC_AC_FAN_SPEED_AUTO_INACTIVE 15
#define PIC_AC_FAN_SPEED_LOW_ACTIVE 18
#define PIC_AC_FAN_SPEED_LOW_INACTIVE 19
#define PIC_AC_FAN_SPEED_MEDIUM_ACTIVE 20
#define PIC_AC_FAN_SPEED_MEDIUM_INACTIVE 21
#define PIC_AC_FAN_SPEED_HIGH_ACTIVE 16
#define PIC_AC_FAN_SPEED_HIGH_INACTIVE 17

// AC Fan Speeds and Mode Position Assumptions
#define AC_FAN_SPEED_AUTO 0
#define AC_FAN_SPEED_LOW 1
#define AC_FAN_SPEED_MEDIUM 2
#define AC_FAN_SPEED_HIGH 3
#define AC_MODE_OFF 0
#define AC_MODE_FAN_ONLY 1
#define AC_MODE_COOL 2

// Messages
#define MSG_MQTT_CONNECTED "BMS Managed"
#define MSG_MQTT_DISCONNECTED "Standalone"
#define MSG_PWM_ADJUSTMENT_STATE_ON "ON"
#define MSG_PWM_ADJUSTMENT_STATE_OFF "OFF"

// Refresh Interval
#define INTERNAL_DISPLAY_CLOCK_REFRESH_INTERVAL 15000
#define INTERNAL_DISPLAY_TOP_BAR_REFRESH_INTERVAL 5000

// Touch Types
#define TOUCH_TYPE_PRESS 0x01
#define TOUCH_TYPE_RELEASE 0x0

/**
 * @brief The internal display of the ESPMegaPRO
 * 
 * This is the display that is installed on some ESPMegaPRO Chassis. It is a 3.5" TFT LCD with a resistive touch screen.
 * 
 * You can use this display to monitor the status of the ESPMegaPRO and also to control the various components of the
 * ESPMegaPRO. 
 * 
 * If you are using a custom display, you need to create a class that inherits from ESPMegaDisplay and implement the
 * methods in that class, you may refer to this class for reference.
 * 
 * @note This class is automatically instantiated by the ESPMegaPRO and can be accessed via the `display` variable.
 */
class InternalDisplay : public ESPMegaDisplay {
    public:
        InternalDisplay(HardwareSerial *displayAdapter);
        void begin(ESPMegaIoT *iot, std::function<rtctime_t()> getRtcTime);
        void loop();
        void bindInputCard(DigitalInputCard *inputCard);
        void bindOutputCard(DigitalOutputCard *outputCard);
        void bindClimateCard(ClimateCard *climateCard);
        void unbindInputCard();
        void unbindOutputCard();
        void unbindClimateCard();
        
    private:
        uint8_t bindedInputCardCallbackHandler;
        uint8_t bindedOutputCardCallbackHandler;
        uint8_t bindedClimateCardCallbackHandler;
        uint8_t bindedClimateCardSensorCallbackHandler;
        DigitalInputCard *inputCard;
        DigitalOutputCard *outputCard;
        ClimateCard *climateCard;
        void handleInputStateChange(uint8_t pin, bool state);
        void handlePwmStateChange(uint8_t pin, bool state, uint16_t value);
        void handlePageChange(uint8_t page);
        void setOutputBar(uint8_t pin, uint16_t value);
        void setOutputStateColor(uint8_t pin, bool state);
        void setInputMarker(uint8_t pin, bool state);
        void handleACStateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature);
        void saveNetworkConfig();
        void saveMQTTConfig();
        void updateStatusIcons(bool networkStatus, bool mqttStatus);
        void updateClock();
        void refreshPage();
        void refreshPage(uint8_t page);
        void refreshDashboard();
        void refreshInput();
        void refreshOutput();
        void refreshAC();
        void refreshPWMAdjustment();
        void refreshPWMAdjustmentSlider();
        void refreshPWMAdjustmentState();
        void refreshPWMAdjustmentId();
        void refreshNetworkConfig();
        void refreshMQTTConfig();
        void setBootStatus(const char* status);
        void sendIpToDisplay(IPAddress ip);
        uint8_t pmwAdjustmentPin;
        // Touch handlers
        void handleTouch(uint8_t page, uint8_t component, uint8_t type);
        void handlePWMAdjustmentTouch(uint8_t component, uint8_t type);
        void handleACTouch(uint8_t component, uint8_t type);
        MqttConfig *mqttConfig;
        NetworkConfig *networkConfig;
        // Pointers to various data
        ESPMegaIoT *iot;
        std::function<rtctime_t()> getRtcTime;
};