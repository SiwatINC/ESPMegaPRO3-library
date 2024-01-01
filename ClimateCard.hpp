#pragma once
#include <ExpansionCard.hpp>
#include <driver/rmt.h>
#include <FRAM.h>
#include <OneWire.h>
#include <DS18B20.h>
#include <dhtnew.h>
#include <map>

#define RMT_TX_CHANNEL RMT_CHANNEL_0

#define CARD_TYPE_CLIMATE 0x03

#define AC_SENSOR_TYPE_NONE 0x00
#define AC_SENSOR_TYPE_DHT22 0x01
#define AC_SENSOR_TYPE_DS18B20 0x02

#define AC_SENSOR_READ_INTERVAL 5000
#define AC_SENSOR_READ_TIMEOUT 250

/**
 * @brief The struct is used to store the state of the air conditioner
 * 
 * @note This struct is stored in FRAM if it is used
 * @note This struct is 3 bytes long
 */
struct ClimateCardData {
    uint8_t ac_temperature; ///< Temperature of the air conditioner
    uint8_t ac_mode; ///< Mode of the air conditioner
    uint8_t ac_fan_speed;///< Fan speed of the air conditioner
};

/**
 * @brief This struct is used to store information about an air conditioner
 */
struct AirConditioner {
    uint8_t max_temperature; ///< Maximum temperature
    uint8_t min_temperature; ///< Minimum temperature
    uint8_t modes; ///< Number of modes
    const char **mode_names; ///< Names of modes in the form of an array of strings
    uint8_t fan_speeds; ///< Number of fan speeds
    const char **fan_speed_names; ///< Names of fan speeds in the form of an array of strings
    /**
     * @brief Function to get IR code
     * 
     * @param mode Mode of the air conditioner
     * @param fan_speed Fan speed of the air conditioner
     * @param temperature Temperature of the air conditioner
     * @param code Pointer to the IR code array
     * 
     * @return Size of the IR code array
     */
    size_t (*getInfraredCode)(uint8_t, uint8_t, uint8_t, const uint16_t**);
};
// This requires 3 bytes of FRAM

/**
 * @brief The ClimateCard class is a class for controlling an air conditioner
 * 
 * This class allows you to control an air conditioner using an IR LED.
 * It is meant to be used with the ESPMega Climate Card.
 * 
 * @note You can also use a DHT22 or DS18B20 temperature sensor to get the room temperature (and humidity if using a DHT22). Although, this is optional.
 * 
 */
class ClimateCard : public ExpansionCard {
    public:
        ClimateCard(uint8_t ir_pin, AirConditioner ac, uint8_t sensor_type, uint8_t sensor_pin);
        ClimateCard(uint8_t ir_pin, AirConditioner ac);
        ~ClimateCard();
        bool begin();
        void loop();
        void bindFRAM(FRAM *fram, uint16_t fram_address);
        void setFRAMAutoSave(bool autoSave);
        void saveStateToFRAM();
        void loadStateFromFRAM();
        void setTemperature(uint8_t temperature);
        uint8_t getTemperature();
        void setMode(uint8_t mode);
        void setModeByName(const char* mode_name);
        uint8_t getMode();
        char* getModeName();
        void setFanSpeed(uint8_t fan_speed);
        void setFanSpeedByName(const char* fan_speed_name);
        uint8_t getFanSpeed();
        char* getFanSpeedName();
        float getRoomTemperature();
        float getHumidity();
        uint8_t getSensorType();
        uint8_t registerChangeCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback);
        uint8_t registerSensorCallback(std::function<void(float, float)> callback);
        void unregisterChangeCallback(uint8_t handler);
        void unregisterSensorCallback(uint8_t handler);
        uint8_t getType();
    private:
        // Sensor objects
        // We use pointers here because we don't know which sensor will be used
        DHTNEW *dht;
        DS18B20 *ds18b20;
        // Callbacks
        uint8_t callbacks_handler_count = 0;
        uint8_t sensor_callbacks_handler_count = 0;
        std::map<uint8_t,std::function<void(uint8_t, uint8_t, uint8_t)>> callbacks;
        std::map<uint8_t,std::function<void(float, float)>> sensor_callbacks;
        // Update functions
        void updateSensor();
        void updateAirConditioner();
        // IR variables
        uint8_t ir_pin;
        // Air conditioner variables
        AirConditioner ac;
        ClimateCardData state;
        // Sensor variables
        uint8_t sensor_type;
        uint8_t sensor_pin;
        float humidity;
        float room_temperature;
        // FRAM variables
        FRAM *fram;
        uint16_t fram_address;
        bool fram_auto_save;
        uint16_t* getIrIndex(uint8_t mode, uint8_t fan_speed, uint8_t temperature);
};
