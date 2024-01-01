#pragma once
#include <IoTComponent.hpp>
#include <ExpansionCard.hpp>
#include <ClimateCard.hpp>

// MQTT Topics
#define AC_MODE_REPORT_TOPIC "mode"
#define AC_MODE_SET_TOPIC "set/mode"
#define AC_TEMPERATURE_REPORT_TOPIC "temperature"
#define AC_TEMPERATURE_SET_TOPIC "set/temperature"
#define AC_FAN_SPEED_REPORT_TOPIC "fan_speed"
#define AC_FAN_SPEED_SET_TOPIC "set/fan_speed"
#define AC_ROOM_TEMPERATURE_REPORT_TOPIC "room_temperature"
#define AC_HUMIDITY_REPORT_TOPIC "humidity"
#define AC_REQUEST_STATE_TOPIC "request_state"

/**
 * @brief The ClimateIoT class is a class for connecting the Climate Card to the IoT module.
 * 
 * This function allows you to control the Climate Card using MQTT.
 * 
 * @warning You should not instantiate this class directly, instead use ESPMegaIoT's registerCard function.
 */
class ClimateIoT : public IoTComponent {
    public:
        ClimateIoT();
        ~ClimateIoT();
        bool begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic);
        void handleMqttMessage(char *topic, char *payload);
        void publishClimate();
        void publishClimateTemperature();
        void publishClimateMode();
        void publishClimateFanSpeed();
        void publishSensor();
        void publishRoomTemperature();
        void publishHumidity();
        void handleStateChange(uint8_t temperature, uint8_t mode, uint8_t fan_speed);
        void handleSensorUpdate(float temperature, float humidity);
        void handleAirConditionerUpdate(uint8_t mode, uint8_t fan_speed, uint8_t temperature);
        void publishReport();
        void subscribe();
        void loop();
        uint8_t getType();
    private:
        ClimateCard *card;
        bool processSetTemperatureMessage(char *topic, char *payload, uint8_t topic_length);
        bool processSetModeMessage(char *topic, char *payload, uint8_t topic_length);
        bool processSetFanSpeedMessage(char *topic, char *payload, uint8_t topic_length);
        bool processRequestStateMessage(char *topic, char *payload, uint8_t topic_length);
};