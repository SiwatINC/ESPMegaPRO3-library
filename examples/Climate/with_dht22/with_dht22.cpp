/**
 * @file with_dht22.ino
 * @brief ClimateCard example with DHT22 temperature and humidity sensor
 *
 * This example shows how to use the ClimateCard with a DHT22 sensor to monitor
 * room temperature and humidity while controlling an air conditioner.
 *
 * Hardware Required:
 * - ESP32 board
 * - IR LED connected to GPIO 14
 * - DHT22 sensor connected to GPIO 27
 * - Optional: IR LED driver circuit for better range
 *
 * Features Demonstrated:
 * - ClimateCard initialization with DHT22 sensor
 * - Reading room temperature and humidity
 * - AC control based on sensor readings
 * - Using sensor callback for real-time monitoring
 * - Responding to environmental changes
 *
 * Wiring:
 *   IR LED:
 *     GPIO 14 --> [330Ω] --> IR LED Anode --> GND
 *
 *   DHT22:
 *     GPIO 27 <--> DHT22 Data Pin
 *     3.3V or 5V --> DHT22 VCC
 *     GND --> DHT22 GND
 *
 * @note DHT22 readings update automatically every 5 seconds
 */

#include <ClimateCard.hpp>

// ============================================================================
// IR Code Definitions (same as basic_ac_control example)
// ============================================================================

// Placeholder IR codes - replace with your actual AC codes
const uint16_t irCodes[3][4][15][67] = {
    // Mode 0: Off
    {
        {{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}},
    },
    // Mode 1: Cool (simplified for example)
    {
        {{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}},
    },
    // Mode 2: Fan Only
    {
        {{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}},
    }
};

size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr) {
    *codePtr = irCodes[mode][fan_speed][temperature];
    return sizeof(irCodes[mode][fan_speed][temperature]) / sizeof(uint16_t);
}

// ============================================================================
// AC Definition
// ============================================================================

const char *mode_names[] = {"off", "cool", "fan_only"};
const char *fan_speed_names[] = {"auto", "low", "medium", "high"};

AirConditioner myAC = {
    .max_temperature = 30,
    .min_temperature = 16,
    .modes = 3,
    .mode_names = mode_names,
    .fan_speeds = 4,
    .fan_speed_names = fan_speed_names,
    .getInfraredCode = getInfraredCode
};

// ============================================================================
// Hardware Configuration
// ============================================================================

#define IR_TX_PIN 14         // GPIO pin for IR LED
#define DHT22_PIN 27         // GPIO pin for DHT22 sensor
#define RMT_CHANNEL RMT_CHANNEL_0

// Create ClimateCard instance with DHT22 sensor
ClimateCard ac(IR_TX_PIN, myAC, AC_SENSOR_TYPE_DHT22, DHT22_PIN, RMT_CHANNEL);

// ============================================================================
// Environmental Monitoring Variables
// ============================================================================

// Comfort zone settings
const float COMFORT_TEMP_MIN = 22.0;  // Minimum comfortable temperature
const float COMFORT_TEMP_MAX = 26.0;  // Maximum comfortable temperature
const float COMFORT_HUMIDITY_MAX = 70.0;  // Maximum comfortable humidity

// Sensor value history for averaging
#define HISTORY_SIZE 5
float tempHistory[HISTORY_SIZE] = {0};
float humidityHistory[HISTORY_SIZE] = {0};
uint8_t historyIndex = 0;
bool historyFull = false;

// ============================================================================
// Callback Functions
// ============================================================================

/**
 * Callback for sensor updates
 * Called automatically every 5 seconds when new sensor data is available
 */
void onSensorUpdate(float temperature, float humidity) {
    Serial.println("----------------------------------------");
    Serial.println("Sensor Reading Updated:");
    Serial.printf("  Temperature: %.1f°C\n", temperature);
    Serial.printf("  Humidity:    %.1f%% RH\n", humidity);

    // Store in history for averaging
    tempHistory[historyIndex] = temperature;
    humidityHistory[historyIndex] = humidity;
    historyIndex++;
    if (historyIndex >= HISTORY_SIZE) {
        historyIndex = 0;
        historyFull = true;
    }

    // Calculate averages
    float avgTemp = calculateAverage(tempHistory, historyFull ? HISTORY_SIZE : historyIndex);
    float avgHumidity = calculateAverage(humidityHistory, historyFull ? HISTORY_SIZE : historyIndex);

    Serial.printf("  Avg Temp:    %.1f°C (last %d readings)\n", avgTemp, historyFull ? HISTORY_SIZE : historyIndex);
    Serial.printf("  Avg Humidity: %.1f%% RH\n", avgHumidity);

    // Comfort analysis
    analyzeComfort(avgTemp, avgHumidity);

    Serial.println("----------------------------------------");
}

/**
 * Callback for AC state changes
 */
void onACStateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.println("========================================");
    Serial.println("AC State Changed:");
    Serial.printf("  Mode:        %s\n", ac.getModeName());
    Serial.printf("  Fan Speed:   %s\n", ac.getFanSpeedName());
    Serial.printf("  Temperature: %d°C\n", temperature);
    Serial.println("========================================");
}

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Calculate average of array values
 */
float calculateAverage(float* array, uint8_t count) {
    if (count == 0) return 0;

    float sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum += array[i];
    }
    return sum / count;
}

/**
 * Analyze environmental comfort and provide feedback
 */
void analyzeComfort(float temperature, float humidity) {
    Serial.println();
    Serial.println("Comfort Analysis:");

    // Temperature comfort
    if (temperature < COMFORT_TEMP_MIN) {
        Serial.printf("  Temperature: TOO COLD (%.1f°C < %.1f°C)\n", temperature, COMFORT_TEMP_MIN);
    } else if (temperature > COMFORT_TEMP_MAX) {
        Serial.printf("  Temperature: TOO HOT (%.1f°C > %.1f°C)\n", temperature, COMFORT_TEMP_MAX);
    } else {
        Serial.printf("  Temperature: COMFORTABLE (%.1f°C)\n", temperature);
    }

    // Humidity comfort
    if (humidity > COMFORT_HUMIDITY_MAX) {
        Serial.printf("  Humidity: TOO HUMID (%.1f%% > %.1f%%)\n", humidity, COMFORT_HUMIDITY_MAX);
    } else if (humidity < 30.0) {
        Serial.printf("  Humidity: TOO DRY (%.1f%% < 30%%)\n", humidity);
    } else {
        Serial.printf("  Humidity: COMFORTABLE (%.1f%%)\n", humidity);
    }

    // Heat index calculation (simplified)
    if (temperature > 27.0 && humidity > 40.0) {
        float heatIndex = temperature + (humidity / 100.0) * 5.0;  // Simplified formula
        Serial.printf("  Heat Index: %.1f°C (feels hotter due to humidity)\n", heatIndex);
    }
}

/**
 * Display current sensor readings on demand
 */
void displayCurrentReadings() {
    float temp = ac.getRoomTemperature();
    float humidity = ac.getHumidity();

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║     Current Environmental Data        ║");
    Serial.println("╠════════════════════════════════════════╣");
    Serial.printf("║  Room Temperature: %6.1f°C          ║\n", temp);
    Serial.printf("║  Room Humidity:    %6.1f%% RH        ║\n", humidity);
    Serial.println("╠════════════════════════════════════════╣");
    Serial.printf("║  AC Mode:          %-12s      ║\n", ac.getModeName());
    Serial.printf("║  AC Fan Speed:     %-12s      ║\n", ac.getFanSpeedName());
    Serial.printf("║  AC Temperature:   %6d°C          ║\n", ac.getTemperature());
    Serial.println("╚════════════════════════════════════════╝\n");
}

/**
 * Suggest AC adjustments based on current conditions
 */
void suggestACSettings() {
    float roomTemp = ac.getRoomTemperature();
    float humidity = ac.getHumidity();
    uint8_t currentACTemp = ac.getTemperature();

    if (roomTemp == 0) {
        Serial.println("No sensor data available yet for suggestions.");
        return;
    }

    Serial.println("\n┌─────────────────────────────────────┐");
    Serial.println("│  AC Setting Suggestions             │");
    Serial.println("└─────────────────────────────────────┘");

    // Temperature suggestions
    if (roomTemp > COMFORT_TEMP_MAX + 2.0) {
        Serial.println("  🔥 Room is quite hot!");
        Serial.println("  → Suggested: Cool mode, High fan");
        Serial.printf("  → Suggested temp: %d°C\n", (int)(COMFORT_TEMP_MIN + 1));
    } else if (roomTemp > COMFORT_TEMP_MAX) {
        Serial.println("  ☀️  Room is slightly warm");
        Serial.println("  → Suggested: Cool mode, Auto/Medium fan");
        Serial.printf("  → Suggested temp: %d°C\n", (int)COMFORT_TEMP_MIN);
    } else if (roomTemp < COMFORT_TEMP_MIN) {
        Serial.println("  ❄️  Room is cool - AC may not be needed");
        Serial.println("  → Suggested: Fan only or Off");
    } else {
        Serial.println("  ✓ Temperature is comfortable");
        if (ac.getMode() != 0) {
            Serial.println("  → You could turn off AC to save energy");
        }
    }

    // Humidity suggestions
    if (humidity > COMFORT_HUMIDITY_MAX) {
        Serial.println("  💧 High humidity detected");
        Serial.println("  → Running AC in Cool mode will help dehumidify");
    } else if (humidity < 30.0) {
        Serial.println("  🏜️  Low humidity - air is dry");
        Serial.println("  → Consider using a humidifier");
    }

    Serial.println();
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  ClimateCard with DHT22 Sensor Demo   ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    // Initialize ClimateCard
    Serial.println("Initializing ClimateCard with DHT22 sensor...");
    if (!ac.begin()) {
        Serial.println("ERROR: Failed to initialize ClimateCard!");
        while (1) delay(1000);
    }
    Serial.println("✓ ClimateCard initialized successfully!");
    Serial.println();

    // Verify sensor type
    Serial.print("Sensor Type: ");
    switch (ac.getSensorType()) {
        case AC_SENSOR_TYPE_DHT22:
            Serial.println("DHT22 (Temperature + Humidity)");
            break;
        case AC_SENSOR_TYPE_DS18B20:
            Serial.println("DS18B20 (Temperature only)");
            break;
        case AC_SENSOR_TYPE_NONE:
            Serial.println("None");
            Serial.println("WARNING: Expected DHT22 but no sensor configured!");
            break;
    }
    Serial.println();

    // Register callbacks
    Serial.println("Registering callbacks...");
    ac.registerSensorCallback(onSensorUpdate);
    ac.registerChangeCallback(onACStateChange);
    Serial.println("✓ Callbacks registered!");
    Serial.println();

    // Display comfort zone settings
    Serial.println("Comfort Zone Settings:");
    Serial.printf("  Temperature: %.1f°C - %.1f°C\n", COMFORT_TEMP_MIN, COMFORT_TEMP_MAX);
    Serial.printf("  Humidity: 30%% - %.1f%%\n", COMFORT_HUMIDITY_MAX);
    Serial.println();

    // Set initial AC state
    Serial.println("Setting initial AC state: Cool, Auto fan, 24°C");
    ac.setState(1, 0, 24);
    Serial.println();

    Serial.println("Waiting for first sensor reading...");
    Serial.println("(DHT22 readings update every 5 seconds)");
    Serial.println();
    Serial.println("Commands will be available shortly:");
    Serial.println("  - Environmental monitoring");
    Serial.println("  - AC setting suggestions");
    Serial.println("  - Automatic comfort adjustments");
    Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    // Update ClimateCard (reads sensor every 5 seconds)
    ac.loop();

    // Display comprehensive status every 30 seconds
    static unsigned long lastStatusDisplay = 0;
    if (millis() - lastStatusDisplay >= 30000) {
        lastStatusDisplay = millis();
        displayCurrentReadings();
        suggestACSettings();
    }

    // Example: Automatic comfort control
    // Uncomment to enable automatic AC adjustment based on room conditions
    /*
    static unsigned long lastAutoAdjust = 0;
    if (millis() - lastAutoAdjust >= 60000) {  // Check every 60 seconds
        lastAutoAdjust = millis();
        autoAdjustAC();
    }
    */
}

// ============================================================================
// Advanced Features (Optional)
// ============================================================================

/**
 * Automatically adjust AC based on room conditions
 * Uncomment the call in loop() to enable
 */
void autoAdjustAC() {
    float roomTemp = ac.getRoomTemperature();
    float humidity = ac.getHumidity();

    // Skip if no valid readings yet
    if (roomTemp == 0) return;

    Serial.println("\n>>> Auto-Adjust Check <<<");

    // Hot room - aggressive cooling
    if (roomTemp > COMFORT_TEMP_MAX + 3.0) {
        Serial.println("Room very hot - setting aggressive cooling");
        if (ac.getMode() != 1 || ac.getFanSpeed() != 3 || ac.getTemperature() != 20) {
            ac.setState(1, 3, 20);  // Cool, High fan, 20°C
        }
    }
    // Warm room - moderate cooling
    else if (roomTemp > COMFORT_TEMP_MAX) {
        Serial.println("Room warm - setting moderate cooling");
        if (ac.getMode() != 1 || ac.getFanSpeed() != 1) {
            uint8_t targetTemp = (uint8_t)(COMFORT_TEMP_MIN + 1);
            ac.setState(1, 1, targetTemp);  // Cool, Low fan
        }
    }
    // Comfortable - maintain or reduce
    else if (roomTemp >= COMFORT_TEMP_MIN && roomTemp <= COMFORT_TEMP_MAX) {
        Serial.println("Room comfortable - fan only mode");
        if (ac.getMode() != 2) {
            ac.setMode(2);  // Fan only
        }
    }
    // Cool room - turn off
    else {
        Serial.println("Room cool - turning off AC");
        if (ac.getMode() != 0) {
            ac.setMode(0);  // Off
        }
    }

    Serial.println();
}

// ============================================================================
// Additional Notes
// ============================================================================

/*
 * DHT22 Sensor Notes:
 *
 * - Reading interval: Automatic every 5 seconds (AC_SENSOR_READ_INTERVAL)
 * - Don't poll more frequently than 2 seconds (sensor limitation)
 * - Temperature accuracy: ±0.5°C
 * - Humidity accuracy: ±2% RH
 * - Operating range: -40°C to 80°C, 0-100% RH
 *
 * Troubleshooting:
 *
 * - If readings are 0: Check wiring, verify GPIO pin number
 * - If readings are erratic: Add 10kΩ pullup resistor to data line
 * - If no readings: Try different power supply (3.3V vs 5V)
 * - Verify DHT22 vs DHT11: This example requires DHT22
 *
 * Extending This Example:
 *
 * - Add web interface for remote monitoring
 * - Store temperature history to FRAM or SD card
 * - Send alerts when conditions go out of range
 * - Implement PID control for precise temperature maintenance
 * - Add LCD display for local status indication
 * - Integrate with home automation (MQTT, HTTP API)
 */
