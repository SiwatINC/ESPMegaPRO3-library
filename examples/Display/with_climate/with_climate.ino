/**
 * @file with_climate.ino
 * @brief Example of using InternalDisplay with Climate/AC control
 *
 * This example demonstrates:
 * - Complete display setup with input, output, and climate cards
 * - Air conditioner control via display
 * - Temperature sensor integration (DHT22 or DS18B20)
 * - Climate state persistence in FRAM
 * - IR code transmission for AC control
 *
 * Hardware Requirements:
 * - ESPMegaPRO board
 * - Internal display connected to Serial port
 * - Climate card in slot 2 (or your chosen slot)
 * - IR LED for AC control
 * - Optional: DHT22 or DS18B20 temperature sensor
 * - Ethernet connection
 *
 * The display will show:
 * - Dashboard with system status
 * - Input/Output pages
 * - AC control page with temperature, mode, and fan speed
 * - Configuration pages
 *
 * @author ESPMegaPRO Team
 * @date 2025-11-23
 */

#include <ESPMegaProOS.hpp>
#include <InternalDisplay.hpp>
#include <ClimateCard.hpp>
#include <ETH.h>

// ============================================
// Configuration
// ============================================

// Climate card slot number
#define CLIMATE_CARD_SLOT 2

// FRAM address for climate card state storage
#define CLIMATE_FRAM_ADDRESS 1001

// IR sensor pin (on the climate card)
#define IR_SENSOR_PIN 14

// ============================================
// IR Code Database
// ============================================

/**
 * @brief IR timing codes for AC control
 *
 * This is a 4D array structured as:
 * irCode[mode][fan_speed][temperature][timing_index]
 *
 * Replace the {0} with actual IR timing codes for your AC unit.
 * You can capture these codes using an IR receiver and the
 * ESPMegaPRO IR learning mode.
 *
 * Temperature range: 16-30°C (15 values, index = temp - 16)
 * Modes: 0=off, 1=fan_only, 2=cool
 * Fan speeds: 0=auto, 1=low, 2=medium, 3=high
 */
const uint16_t irCode[3][4][15][1] = {0};

// Mode names (must be in this order for InternalDisplay)
const char *mode_names[] = {
    "off",       // Mode 0
    "fan_only",  // Mode 1
    "cool"       // Mode 2
};

// Fan speed names (must be in this order for InternalDisplay)
const char *fan_speed_names[] = {
    "auto",      // Fan speed 0
    "low",       // Fan speed 1
    "medium",    // Fan speed 2
    "high"       // Fan speed 3
};

/**
 * @brief Get IR code for specific AC settings
 *
 * This function is called by the ClimateCard when AC settings change.
 * It returns a pointer to the IR timing array for the requested settings.
 *
 * @param mode AC mode (0=off, 1=fan, 2=cool)
 * @param fan_speed Fan speed (0=auto, 1=low, 2=medium, 3=high)
 * @param temperature Target temperature (16-30)
 * @param codePtr Pointer to store the code array address
 * @return Size of the IR code array
 */
size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t **codePtr) {
    // Validate temperature range
    if (temperature < 16) temperature = 16;
    if (temperature > 30) temperature = 30;

    // Convert temperature to array index (16°C = index 0)
    uint8_t tempIndex = temperature - 16;

    // Set the code pointer to the appropriate IR timing array
    *codePtr = &(irCode[mode][fan_speed][tempIndex][0]);

    // Return the size of the code array
    return sizeof(irCode[mode][fan_speed][tempIndex]) / sizeof(uint16_t);
}

// ============================================
// Air Conditioner Configuration
// ============================================

/**
 * @brief Air conditioner specification
 *
 * This structure defines the capabilities of your AC unit.
 * Adjust the temperature range to match your specific model.
 */
AirConditioner ac = {
    .max_temperature = 30,              // Maximum temperature setting
    .min_temperature = 16,              // Minimum temperature setting
    .modes = 3,                         // Number of modes (off, fan, cool)
    .mode_names = mode_names,           // Mode name array
    .fan_speeds = 4,                    // Number of fan speeds
    .fan_speed_names = fan_speed_names, // Fan speed name array
    .getInfraredCode = &getInfraredCode // IR code getter function
};

// ============================================
// Global Objects
// ============================================

// Create ESPMegaPRO object
ESPMegaPRO espmega = ESPMegaPRO();

// Create climate card
ClimateCard climateCard = ClimateCard(IR_SENSOR_PIN, ac);

// ============================================
// Callback Functions
// ============================================

/**
 * @brief Input change callback
 * @param pin Input pin number
 * @param value New state
 */
void onInputChange(uint8_t pin, uint8_t value) {
    Serial.print("Input ");
    Serial.print(pin);
    Serial.print(" changed to: ");
    Serial.println(value ? "HIGH" : "LOW");
}

/**
 * @brief Climate state change callback
 *
 * This function is called when AC settings change, either from
 * the display, MQTT, or programmatically.
 *
 * @param mode New mode
 * @param fan_speed New fan speed
 * @param temperature New temperature
 */
void onClimateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.println("=== Climate State Changed ===");
    Serial.print("Mode: ");
    Serial.println(mode_names[mode]);
    Serial.print("Fan Speed: ");
    Serial.println(fan_speed_names[fan_speed]);
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    Serial.println("============================");
}

/**
 * @brief MQTT message callback
 * @param topic MQTT topic
 * @param payload Message payload
 */
void onMqttMessage(char *topic, char *payload) {
    Serial.print("MQTT: ");
    Serial.print(topic);
    Serial.print(" = ");
    Serial.println(payload);
}

// ============================================
// Setup Function
// ============================================

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    Serial.println("==========================================");
    Serial.println("ESPMegaPRO Display with Climate Control");
    Serial.println("==========================================");

    // ========================================
    // Step 1: Initialize ESPMegaPRO
    // ========================================
    Serial.println("Initializing ESPMegaPRO...");
    espmega.begin();

    // ========================================
    // Step 2: Configure IoT Module
    // ========================================
    Serial.println("Configuring IoT module...");
    espmega.enableIotModule();

    ETH.begin();
    espmega.iot->bindEthernetInterface(&ETH);

    espmega.iot->loadNetworkConfig();
    espmega.iot->connectNetwork();

    espmega.iot->loadMqttConfig();
    espmega.iot->connectToMqtt();
    espmega.iot->registerMqttCallback(onMqttMessage);

    // Register input and output cards with MQTT
    espmega.iot->registerCard(0);  // Input card
    espmega.iot->registerCard(1);  // Output card

    // ========================================
    // Step 3: Install and Configure Climate Card
    // ========================================
    Serial.println("Installing climate card...");

    // Install climate card in the specified slot
    espmega.installCard(CLIMATE_CARD_SLOT, &climateCard);

    // Bind climate card to FRAM for state persistence
    Serial.println("Binding climate card to FRAM...");
    climateCard.bindFRAM(&espmega.fram, CLIMATE_FRAM_ADDRESS);

    // Load saved state from FRAM
    Serial.println("Loading climate state from FRAM...");
    climateCard.loadStateFromFRAM();

    // Enable automatic state saving to FRAM
    // This saves the state whenever it changes
    Serial.println("Enabling FRAM auto-save...");
    climateCard.setFRAMAutoSave(true);

    // Register climate change callback
    auto climateCallback = std::bind(&onClimateChange,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3);
    climateCard.registerChangeCallback(climateCallback);

    // Register climate card with MQTT for remote control
    Serial.println("Registering climate card with MQTT...");
    espmega.iot->registerCard(CLIMATE_CARD_SLOT);

    // ========================================
    // Step 4: Optional - Configure Temperature Sensor
    // ========================================

    // Option A: DHT22 sensor
    // Provides both temperature and humidity
    Serial.println("Configuring DHT22 sensor...");
    climateCard.bindSensor(AC_SENSOR_TYPE_DHT22, 5);  // DHT22 on pin 5

    // Option B: DS18B20 sensor
    // Provides temperature only
    // climateCard.bindSensor(AC_SENSOR_TYPE_DS18B20, 6);  // DS18B20 on pin 6

    // Option C: No sensor
    // climateCard.bindSensor(AC_SENSOR_TYPE_NONE, 0);

    // ========================================
    // Step 5: Enable and Configure Display
    // ========================================
    Serial.println("Enabling internal display...");
    espmega.enableInternalDisplay(&Serial);

    // Bind all cards to display
    Serial.println("Binding cards to display...");
    espmega.display->bindInputCard(&espmega.inputs);
    espmega.display->bindOutputCard(&espmega.outputs);
    espmega.display->bindClimateCard(&climateCard);

    // ========================================
    // Step 6: Register Input Callback
    // ========================================
    espmega.inputs.registerCallback(onInputChange);

    // ========================================
    // Initialization Complete
    // ========================================
    Serial.println("==========================================");
    Serial.println("Initialization complete!");
    Serial.println("==========================================");
    Serial.println();
    Serial.println("Display pages:");
    Serial.println("  Page 1: Dashboard");
    Serial.println("  Page 2: Digital Inputs");
    Serial.println("  Page 3: PWM Outputs");
    Serial.println("  Page 4: Air Conditioner Control");
    Serial.println("  Page 5: PWM Adjustment");
    Serial.println("  Page 6: Network Configuration");
    Serial.println("  Page 11: MQTT Configuration");
    Serial.println();
    Serial.println("Current AC settings:");
    Serial.print("  Mode: ");
    Serial.println(mode_names[climateCard.getMode()]);
    Serial.print("  Fan Speed: ");
    Serial.println(fan_speed_names[climateCard.getFanSpeed()]);
    Serial.print("  Temperature: ");
    Serial.print(climateCard.getTemperature());
    Serial.println("°C");
    Serial.println("==========================================");
}

// ============================================
// Main Loop
// ============================================

void loop() {
    // Main system loop
    espmega.loop();

    // ========================================
    // Optional: Custom Climate Control Logic
    // ========================================

    // Example 1: Auto mode based on room temperature
    // If DHT22 sensor is connected, automatically adjust AC
    static unsigned long lastAutoCheck = 0;
    if (millis() - lastAutoCheck > 60000) {  // Check every minute
        lastAutoCheck = millis();

        // Only if sensor is available and returns valid data
        if (climateCard.getSensorType() != AC_SENSOR_TYPE_NONE) {
            float roomTemp = climateCard.getRoomTemperature();

            // If room temp is valid (not 0 or error value)
            if (roomTemp > 0 && roomTemp < 50) {
                Serial.print("Room temperature: ");
                Serial.print(roomTemp);
                Serial.println("°C");

                // Example auto control logic:
                // If room is too hot, turn on AC to cool mode
                if (roomTemp > 28) {
                    Serial.println("Room too hot, enabling cooling");
                    climateCard.setMode(2);  // Cool mode
                    climateCard.setFanSpeed(1);  // Low fan
                    climateCard.setTemperature(24);  // Target 24°C
                }
                // If room is comfortable, use fan only
                else if (roomTemp > 26) {
                    Serial.println("Room warm, enabling fan");
                    climateCard.setMode(1);  // Fan only mode
                    climateCard.setFanSpeed(0);  // Auto fan
                }
                // If room is cool enough, turn off
                else if (roomTemp < 24) {
                    Serial.println("Room comfortable, turning off");
                    climateCard.setMode(0);  // Off
                }
            }
        }
    }

    // Example 2: Schedule-based control
    // Turn on AC at specific times
    static unsigned long lastScheduleCheck = 0;
    if (millis() - lastScheduleCheck > 60000) {  // Check every minute
        lastScheduleCheck = millis();

        // Get current time from RTC
        rtctime_t currentTime = espmega.rtc.getTime();

        // Example: Turn on AC at 2 PM (14:00)
        if (currentTime.hours == 14 && currentTime.minutes == 0) {
            Serial.println("Schedule trigger: Turning on AC");
            climateCard.setMode(2);  // Cool mode
            climateCard.setFanSpeed(0);  // Auto fan
            climateCard.setTemperature(24);  // 24°C
        }

        // Example: Turn off AC at 10 PM (22:00)
        if (currentTime.hours == 22 && currentTime.minutes == 0) {
            Serial.println("Schedule trigger: Turning off AC");
            climateCard.setMode(0);  // Off
        }
    }

    // Example 3: Status monitoring
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 30000) {  // Every 30 seconds
        lastStatusPrint = millis();

        Serial.println("--- System Status ---");
        Serial.print("Network: ");
        Serial.println(espmega.iot->networkConnected() ? "Connected" : "Disconnected");
        Serial.print("MQTT: ");
        Serial.println(espmega.iot->mqttConnected() ? "Connected" : "Disconnected");

        Serial.print("AC Mode: ");
        Serial.println(mode_names[climateCard.getMode()]);
        Serial.print("AC Fan: ");
        Serial.println(fan_speed_names[climateCard.getFanSpeed()]);
        Serial.print("AC Temp: ");
        Serial.print(climateCard.getTemperature());
        Serial.println("°C");

        if (climateCard.getSensorType() == AC_SENSOR_TYPE_DHT22) {
            Serial.print("Room Temp: ");
            Serial.print(climateCard.getRoomTemperature());
            Serial.println("°C");
            Serial.print("Humidity: ");
            Serial.print(climateCard.getHumidity());
            Serial.println("%");
        } else if (climateCard.getSensorType() == AC_SENSOR_TYPE_DS18B20) {
            Serial.print("Room Temp: ");
            Serial.print(climateCard.getRoomTemperature());
            Serial.println("°C");
        }

        Serial.println("--------------------");
    }
}

/**
 * Additional Notes:
 *
 * 1. IR Code Configuration:
 *    - Replace the irCode array with actual IR timing codes
 *    - Use an IR receiver to capture codes from your AC remote
 *    - Each AC brand/model has different codes
 *    - Codes are in microseconds timing format
 *
 * 2. Temperature Sensor:
 *    - DHT22: Temperature + Humidity
 *    - DS18B20: Temperature only
 *    - Can be omitted if not needed
 *    - Sensor readings shown on display page 4
 *
 * 3. FRAM Persistence:
 *    - AC state is automatically saved to FRAM
 *    - Restored on power-up
 *    - Address 1001 is used by default
 *    - Change if it conflicts with other data
 *
 * 4. MQTT Control:
 *    - AC can be controlled via MQTT
 *    - Topics: <base_topic>/ac/mode, fan, temperature
 *    - Publishes state changes automatically
 *
 * 5. Display Integration:
 *    - Page 4 shows AC control interface
 *    - Touch interface for all settings
 *    - Real-time sensor readings
 *    - Visual feedback for active settings
 *
 * 6. Extending This Example:
 *    - Add scheduling logic
 *    - Implement auto mode based on temperature
 *    - Add humidity control
 *    - Integrate with home automation
 *    - Add multiple AC units
 */
