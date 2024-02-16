/**
 * @file ClimateCard.cpp
 * @brief Implementation file for the ClimateCard class.
 *
 * This file contains the implementation of the ClimateCard class, which represents a climate control card.
 * The ClimateCard class provides methods for controlling an air conditioner, reading temperature and humidity
 * from sensors, and saving and loading the state to/from FRAM memory.
 */
#include <ClimateCard.hpp>

/**
 * @brief Construct a new ClimateCard object.
 *
 * @note RMT channel must be universally unique, this means that you can't use the same channel for multiple cards.
 * 
 * @param ir_pin The GPIO pin number of the IR transmitter.
 * @param ac The AirConditioner object that represents the air conditioner.
 * @param sensor_type The type of the sensor connected to the card.
 * @param sensor_pin The GPIO pin number of the sensor.
 * @param channel The RMT channel to use for IR transmission.
 */
ClimateCard::ClimateCard(uint8_t ir_pin, AirConditioner ac, uint8_t sensor_type, uint8_t sensor_pin, rmt_channel_t channel) : ir_blaster(ir_pin, channel)
{
    this->ir_pin = ir_pin;
    this->ac = ac;
    this->sensor_type = sensor_type;
    this->sensor_pin = sensor_pin;
    // Initialize Pointers
    this->dht = nullptr;
    this->ds18b20 = nullptr;
    this->fram = nullptr;
    // Initialize Variables
    this->fram_address = 0;
    this->fram_auto_save = false;
    this->state.ac_temperature = 0;
    this->state.ac_mode = 0;
    this->state.ac_fan_speed = 0;
    this->humidity = 0;
    this->room_temperature = 0;
    // Initialize state
    this->state.ac_temperature = 25;
    this->state.ac_mode = 0;
    this->state.ac_fan_speed = 0;
}

/**
 * @brief Construct a new ClimateCard object.
 *
 * @param ir_pin The GPIO pin number of the IR transmitter.
 * @param ac The AirConditioner object that represents the air conditioner.
 * @param channel The RMT channel to use for IR transmission.
 * 
 * @note RMT channel must be universally unique, this means that you can't use the same channel for multiple cards.
 * 
 * @note This constructor can be used when no sensor is connected to the card.
 */
ClimateCard::ClimateCard(uint8_t ir_pin, AirConditioner ac, rmt_channel_t channel) : ClimateCard(ir_pin, ac, AC_SENSOR_TYPE_NONE, 0, channel)
{
}

/**
 * @brief The destructor of the ClimateCard class.
 */
ClimateCard::~ClimateCard()
{
    delete dht;
    delete ds18b20;
}

/**
 * @brief Initialize the ClimateCard object.
 *
 * @return true if initialization was successful.
 * @return false if initialization failed.
 */
bool ClimateCard::begin()
{
    switch (sensor_type)
    {
    case AC_SENSOR_TYPE_DHT22:
        dht = new DHTNEW(sensor_pin);
        break;
    case AC_SENSOR_TYPE_DS18B20:
        OneWire oneWire(sensor_pin);
        ds18b20 = new DS18B20(&oneWire);
        break;
    }
    updateAirConditioner();
    return true;
}

/**
 * @brief Loop function of the ClimateCard class.
 * 
 * @note When this card is installed in an ESPMega, this function is called automatically by the ESPMega class.
 */
void ClimateCard::loop()
{
    static uint32_t last_sensor_update = 0;
    if (millis() - last_sensor_update >= AC_SENSOR_READ_INTERVAL)
    {
        last_sensor_update = millis();
        updateSensor();
    }
}

/**
 * @brief bind FRAM memory to the ClimateCard object at the specified address.
 * 
 * @note This function must be called before calling loadStateFromFRAM() or saveStateToFRAM().
 * @note This card takes up 3 bytes of FRAM memory.
 * 
 * @param fram The FRAM object.
 * @param fram_address The starting address of the card in FRAM memory.
 */
void ClimateCard::bindFRAM(FRAM *fram, uint16_t fram_address)
{
    this->fram = fram;
    this->fram_address = fram_address;
}

/**
 * @brief Set whether the state should be automatically saved to FRAM memory.
 * 
 * @note This function has no effect if bindFRAM() has not been called.
 * @param autoSave Whether the state should be automatically saved to FRAM memory.
 */
void ClimateCard::setFRAMAutoSave(bool autoSave)
{
    this->fram_auto_save = autoSave;
}

/**
 * @brief Save the state to FRAM memory.
 * @note This function has no effect if bindFRAM() has not been called.
 */
void ClimateCard::saveStateToFRAM()
{
    if (fram == nullptr)
        return;
    fram->write8(fram_address, state.ac_temperature);
    fram->write8(fram_address + 1, state.ac_mode);
    fram->write8(fram_address + 2, state.ac_fan_speed);
}

/**
 * @brief Load the state from FRAM memory.
 * 
 * @note This function has no effect if bindFRAM() has not been called.
 */
void ClimateCard::loadStateFromFRAM()
{
    if (fram == nullptr)
        return;
    if (state.ac_temperature > ac.max_temperature)
        state.ac_temperature = ac.max_temperature;
    else if (state.ac_temperature < ac.min_temperature)
        state.ac_temperature = ac.min_temperature;
    // If mode is out of range, set to 0
    if (state.ac_mode > ac.modes)
        state.ac_mode = 0;
    // If fan speed is out of range, set to 0
    if (state.ac_fan_speed > ac.fan_speeds)
        state.ac_fan_speed = 0;
    updateAirConditioner();
    for (const auto &callback : callbacks)
    {
        callback.second(this->state.ac_mode, this->state.ac_fan_speed, this->state.ac_temperature);
    }
}

/**
 * @brief Set the temperature of the air conditioner.
 * 
 * @param temperature The temperature to set.
 * @note If the temperature is out of range, it will be set to its respective maximum or minimum.
 */
void ClimateCard::setTemperature(uint8_t temperature)
{
    // If temperature is out of range, set to its respective maximum or minimum
    if (temperature > ac.max_temperature)
        temperature = ac.max_temperature;
    else if (temperature < ac.min_temperature)
        temperature = ac.min_temperature;
    this->state.ac_temperature = temperature;
    updateAirConditioner();
    if (fram_auto_save)
        saveStateToFRAM();
}

/**
 * @brief Set the mode of the air conditioner.
 * 
 * @note If the mode is out of range, it will be set to 0.
 * @param mode The mode to set.
 */
void ClimateCard::setMode(uint8_t mode)
{
    if (mode > ac.modes)
        mode = 0;
    this->state.ac_mode = mode;
    updateAirConditioner();
    if (fram_auto_save)
        saveStateToFRAM();
}

/**
 * @brief Get the name of the current mode.
 * @return The name of the current mode.
 */
char *ClimateCard::getModeName()
{
    return (char *)ac.mode_names[state.ac_mode];
}

/**
 * @brief Get the name of the current fan speed.
 * 
 * @return The name of the current fan speed.
 */
char *ClimateCard::getFanSpeedName()
{
    return (char *)ac.fan_speed_names[state.ac_fan_speed];
}

/**
 * @brief Set the fan speed of the air conditioner.
 * 
 * @note If the fan speed is out of range, it will be set to 0.
 * @param fan_speed The fan speed to set.
 */
void ClimateCard::setFanSpeed(uint8_t fan_speed)
{
    if (fan_speed > ac.fan_speeds)
        fan_speed = 0;
    this->state.ac_fan_speed = fan_speed;
    updateAirConditioner();
    if (fram_auto_save)
        saveStateToFRAM();
}

/**
 * @brief Set fan speed by name.
 * 
 * @param fan_speed_name The name of the fan speed to set.
 * @note If the fan speed is not found, the function will not do anything.
 */
void ClimateCard::setFanSpeedByName(const char *fan_speed_name)
{
    for (uint8_t i = 0; i < ac.fan_speeds; i++)
    {
        if (strcmp(fan_speed_name, ac.fan_speed_names[i]) == 0)
        {
            setFanSpeed(i);
            return;
        }
    }
}

/**
 * @brief Set mode by name.
 * 
 * @param mode_name The name of the mode to set.
 * @note If the mode is not found, the function will not do anything.
 */
void ClimateCard::setModeByName(const char *mode_name)
{
    for (uint8_t i = 0; i < ac.modes; i++)
    {
        if (strcmp(mode_name, ac.mode_names[i]) == 0)
        {
            setMode(i);
            return;
        }
    }
}

/**
 * @brief Register a callback function that will be called when the state of the air conditioner changes.
 * 
 * @param callback The callback function to register.
 * 
 * @return uint8_t The handler of the callback function.
 */
uint8_t ClimateCard::registerChangeCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback)
{
    callbacks[callbacks_handler_count] = callback;
    return callbacks_handler_count++;
}

/**
 * @brief Get the type of the card.
 * 
 * @return The handler of the callback function.
 */
uint8_t ClimateCard::getType()
{
    return CARD_TYPE_CLIMATE;
}

/**
 * @brief update environmental sensor data.
 * 
 * @note This function is called automatically by the loop() function.
 * @note This function has no effect if no sensor is connected to the card.
 * @note This function also calls the sensor callbacks.
 */
void ClimateCard::updateSensor()
{
    if (sensor_type == AC_SENSOR_TYPE_NONE)
        return;
    // Read sensor data and update variables
    switch (sensor_type)
    {
    case AC_SENSOR_TYPE_DHT22:
        if (millis() - dht->lastRead() < AC_SENSOR_READ_INTERVAL)
            return;
        dht->read();
        room_temperature = dht->getTemperature();
        humidity = dht->getHumidity();
        break;
    case AC_SENSOR_TYPE_DS18B20:
        ds18b20->requestTemperatures();
        uint32_t start = millis();
        while (!ds18b20->isConversionComplete())
        {
            if (millis() - start >= AC_SENSOR_READ_TIMEOUT)
            {
                return;
            }
        }
        room_temperature = ds18b20->getTempC();
        break;
    }
    for (const auto &callback : sensor_callbacks)
    {
        callback.second(room_temperature, humidity);
    }
}

/**
 * @brief Update the air conditioner state to match the state of the card.
 * 
 * @warning This function is not working yet.
 */
void ClimateCard::updateAirConditioner()
{
    const uint16_t* ir_code_ptr = nullptr;
    size_t itemCount = (*(this->ac.getInfraredCode))(this->state.ac_mode, this->state.ac_fan_speed, this->state.ac_temperature-this->ac.min_temperature, &ir_code_ptr);

    if (ir_code_ptr == nullptr)
        return;

    ir_blaster.send(ir_code_ptr, itemCount);

    // Publish state
    for (const auto &callback : callbacks)
    {
        callback.second(this->state.ac_mode, this->state.ac_fan_speed, this->state.ac_temperature);
    }
}

/**
 * @brief Get the type of the sensor connected to the card.
 * 
 * @return The type of the sensor connected to the card.
 */
uint8_t ClimateCard::getSensorType()
{
    return sensor_type;
}

/**
 * @brief Get the room temperature in degrees Celsius.
 * 
 * @return The room temperature.
 */
float ClimateCard::getRoomTemperature()
{
    return room_temperature;
}

/**
 * @brief Get the humidity in percent.
 * 
 * @return The humidity.
 */
float ClimateCard::getHumidity()
{
    return humidity;
}

/**
 * @brief Get the temperature of the air conditioner.
 * 
 * @return The temperature of the air conditioner.
 */
uint8_t ClimateCard::getTemperature()
{
    return state.ac_temperature;
}

/**
 * @brief Get the mode of the air conditioner.
 * 
 * @return The mode of the air conditioner.
 */
uint8_t ClimateCard::getMode()
{
    return state.ac_mode;
}

/**
 * @brief Get the fan speed of the air conditioner.
 * 
 * @return The fan speed of the air conditioner.
 */
uint8_t ClimateCard::getFanSpeed()
{
    return state.ac_fan_speed;
}

/**
 * @brief Register a callback function that will be called when the sensor data changes.
 * 
 * @param callback The callback function to register.
 * 
 * @return The handler of the callback function
 */
uint8_t ClimateCard::registerSensorCallback(std::function<void(float, float)> callback)
{
    sensor_callbacks[sensor_callbacks_handler_count] = callback;
    return sensor_callbacks_handler_count++;
}

/**
 * @brief Unregister a callback function.
 * 
 * @param handler The handler of the callback function to unregister.
 */
void ClimateCard::unregisterChangeCallback(uint8_t handler)
{
    callbacks.erase(handler);
}

/**
 * @brief Unregister a sensor callback function.
 * 
 * @param handler The handler of the callback function to unregister.
 */
void ClimateCard::unregisterSensorCallback(uint8_t handler)
{
    sensor_callbacks.erase(handler);
}

void ClimateCard::setState(uint8_t mode, uint8_t fan_speed, uint8_t temperature)
{
    this->state.ac_mode = mode;
    this->state.ac_fan_speed = fan_speed;
    this->state.ac_temperature = temperature;
    updateAirConditioner();
    if (fram_auto_save)
        saveStateToFRAM();
}