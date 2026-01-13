/**
 * @file full_integration.ino
 * @brief Complete example showcasing all InternalDisplay features
 *
 * This comprehensive example demonstrates:
 * - Full display integration with all card types
 * - Climate control with temperature sensors
 * - Web server with display OTA updates
 * - Custom touch event handling
 * - Advanced MQTT integration
 * - Scheduled automation
 * - State persistence
 * - Error handling and recovery
 *
 * Hardware Requirements:
 * - ESPMegaPRO board
 * - Internal display connected to Serial port
 * - Climate card with IR LED
 * - DHT22 or DS18B20 temperature sensor (optional)
 * - Ethernet connection
 * - SD card (optional, for display firmware updates)
 *
 * Features:
 * - Complete system monitoring via display
 * - Remote control via MQTT
 * - Web interface for configuration and OTA
 * - Automatic climate control based on temperature
 * - Input-triggered automation
 * - Comprehensive error handling
 *
 * @author ESPMegaPRO Team
 * @date 2025-11-23
 */

#include <ESPMegaProOS.hpp>
#include <InternalDisplay.hpp>
#include <ClimateCard.hpp>
#include <ESPMegaDisplayOTA.hpp>
#include <ETH.h>

// ============================================
// Configuration Constants
// ============================================

// Card slot assignments
#define CLIMATE_CARD_SLOT 2
#define INPUT_CARD_SLOT 0
#define OUTPUT_CARD_SLOT 1

// FRAM addresses
#define CLIMATE_FRAM_ADDRESS 1001
#define WEBSERVER_FRAM_ADDRESS 100

// Climate configuration
#define IR_SENSOR_PIN 14
#define TEMP_SENSOR_TYPE AC_SENSOR_TYPE_DHT22
#define TEMP_SENSOR_PIN 5

// Web server configuration
#define WEB_SERVER_PORT 80

// Automation thresholds
#define TEMP_HOT_THRESHOLD 28.0      // Start cooling above this temp
#define TEMP_WARM_THRESHOLD 26.0     // Use fan only above this temp
#define TEMP_COOL_THRESHOLD 24.0     // Turn off below this temp
#define TEMP_CHECK_INTERVAL 60000    // Check every 60 seconds

// Status reporting
#define STATUS_REPORT_INTERVAL 30000 // Report every 30 seconds

// Enable/disable features
#define ENABLE_WEB_SERVER true
#define ENABLE_DISPLAY_OTA true
#define ENABLE_AUTO_CLIMATE true
#define ENABLE_INPUT_AUTOMATION true
#define ENABLE_STATUS_REPORTING true

// ============================================
// IR Code Database
// ============================================

/**
 * Replace these with your actual IR codes.
 * Capture from your AC remote using an IR receiver.
 */
const uint16_t irCode[3][4][15][1] = {0};

const char *mode_names[] = {"off", "fan_only", "cool"};
const char *fan_speed_names[] = {"auto", "low", "medium", "high"};

size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t **codePtr) {
    if (temperature < 16) temperature = 16;
    if (temperature > 30) temperature = 30;
    uint8_t tempIndex = temperature - 16;
    *codePtr = &(irCode[mode][fan_speed][tempIndex][0]);
    return sizeof(irCode[mode][fan_speed][tempIndex]) / sizeof(uint16_t);
}

AirConditioner ac = {
    .max_temperature = 30,
    .min_temperature = 16,
    .modes = 3,
    .mode_names = mode_names,
    .fan_speeds = 4,
    .fan_speed_names = fan_speed_names,
    .getInfraredCode = &getInfraredCode
};

// ============================================
// Global Objects
// ============================================

ESPMegaPRO espmega = ESPMegaPRO();
ClimateCard climateCard = ClimateCard(IR_SENSOR_PIN, ac);

#if ENABLE_DISPLAY_OTA
ESPMegaDisplayOTA displayOta;
#endif

// ============================================
// State Variables
// ============================================

struct SystemState {
    bool networkConnected = false;
    bool mqttConnected = false;
    bool autoClimateEnabled = ENABLE_AUTO_CLIMATE;
    bool autoInputEnabled = ENABLE_INPUT_AUTOMATION;
    unsigned long uptimeSeconds = 0;
    float lastRoomTemp = 0.0;
    float lastHumidity = 0.0;
    uint8_t lastClimateMode = 0;
} state;

// ============================================
// Callback Functions
// ============================================

/**
 * @brief Input change callback with automation logic
 */
void onInputChange(uint8_t pin, uint8_t value) {
    Serial.printf("Input %d: %s\n", pin, value ? "HIGH" : "LOW");

    #if ENABLE_INPUT_AUTOMATION
    if (state.autoInputEnabled) {
        // Example automation: Input 0 controls Output 0
        if (pin == 0) {
            espmega.outputs.setState(0, value);
            Serial.println("Auto: Mirrored input 0 to output 0");
        }

        // Example: Input 1 and 2 both HIGH triggers AC
        if (pin == 1 || pin == 2) {
            bool in1 = espmega.inputs.digitalRead(1);
            bool in2 = espmega.inputs.digitalRead(2);

            if (in1 && in2) {
                Serial.println("Auto: Inputs 1+2 HIGH, enabling AC");
                climateCard.setMode(2);  // Cool
                climateCard.setTemperature(24);
                climateCard.setFanSpeed(0);  // Auto
            }
        }

        // Example: Input 15 is emergency stop
        if (pin == 15 && value) {
            Serial.println("EMERGENCY STOP activated!");
            // Turn off all outputs
            for (int i = 0; i < 16; i++) {
                espmega.outputs.setState(i, false);
            }
            // Turn off AC
            climateCard.setMode(0);

            // Could also publish MQTT alert
            if (state.mqttConnected) {
                espmega.iot->publish("alarm/emergency", "Emergency stop activated");
            }
        }
    }
    #endif
}

/**
 * @brief Climate state change callback
 */
void onClimateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.println("=== Climate Changed ===");
    Serial.printf("Mode: %s\n", mode_names[mode]);
    Serial.printf("Fan: %s\n", fan_speed_names[fan_speed]);
    Serial.printf("Temp: %d°C\n", temperature);
    Serial.println("======================");

    state.lastClimateMode = mode;

    // Publish to MQTT if connected
    if (state.mqttConnected) {
        char payload[64];
        sprintf(payload, "{\"mode\":\"%s\",\"fan\":\"%s\",\"temp\":%d}",
                mode_names[mode], fan_speed_names[fan_speed], temperature);
        espmega.iot->publish("climate/state", payload);
    }
}

/**
 * @brief PWM output change callback
 */
void onOutputChange(uint8_t pin, bool state, uint16_t value) {
    Serial.printf("Output %d: %s (PWM: %d)\n", pin, state ? "ON" : "OFF", value);

    // Example: Log significant changes
    if (value > 3000 || value < 500) {
        Serial.printf("Notable PWM change on pin %d: %d\n", pin, value);
    }
}

/**
 * @brief MQTT message callback with command handling
 */
void onMqttMessage(char *topic, char *payload) {
    Serial.printf("MQTT: %s = %s\n", topic, payload);

    // Parse custom commands
    // Example topic structure: <base>/command/<type>

    // Extract command type from topic
    char *commandPtr = strstr(topic, "/command/");
    if (commandPtr != NULL) {
        commandPtr += 9;  // Skip "/command/"

        // Auto climate enable/disable
        if (strcmp(commandPtr, "auto_climate") == 0) {
            if (strcmp(payload, "on") == 0 || strcmp(payload, "1") == 0) {
                state.autoClimateEnabled = true;
                Serial.println("Auto climate enabled via MQTT");
            } else if (strcmp(payload, "off") == 0 || strcmp(payload, "0") == 0) {
                state.autoClimateEnabled = false;
                Serial.println("Auto climate disabled via MQTT");
            }
        }

        // Auto input enable/disable
        else if (strcmp(commandPtr, "auto_input") == 0) {
            if (strcmp(payload, "on") == 0 || strcmp(payload, "1") == 0) {
                state.autoInputEnabled = true;
                Serial.println("Auto input enabled via MQTT");
            } else if (strcmp(payload, "off") == 0 || strcmp(payload, "0") == 0) {
                state.autoInputEnabled = false;
                Serial.println("Auto input disabled via MQTT");
            }
        }

        // Display brightness control
        else if (strcmp(commandPtr, "brightness") == 0) {
            int brightness = atoi(payload);
            if (brightness >= 0 && brightness <= 100) {
                espmega.display->setBrightness(brightness);
                Serial.printf("Display brightness set to %d\n", brightness);
            }
        }

        // Display page control
        else if (strcmp(commandPtr, "page") == 0) {
            int page = atoi(payload);
            if (page >= 0 && page <= 15) {
                espmega.display->jumpToPage(page);
                Serial.printf("Display jumped to page %d\n", page);
            }
        }

        // System reboot
        else if (strcmp(commandPtr, "reboot") == 0) {
            Serial.println("Reboot requested via MQTT");
            delay(100);
            ESP.restart();
        }
    }
}

/**
 * @brief Custom touch event handler
 *
 * This handles touch events that aren't handled by the built-in pages.
 * You can add custom buttons and functionality here.
 */
void onCustomTouch(uint8_t page, uint8_t component, uint8_t type) {
    // Only handle release events to avoid double-triggering
    if (type != 0x00) return;

    Serial.printf("Custom touch: page=%d, component=%d\n", page, component);

    // Example: Custom button on dashboard (page 1, component 20)
    if (page == 1 && component == 20) {
        Serial.println("Custom dashboard button pressed!");

        // Toggle auto climate mode
        state.autoClimateEnabled = !state.autoClimateEnabled;
        Serial.printf("Auto climate: %s\n", state.autoClimateEnabled ? "ON" : "OFF");

        // Update display text
        espmega.display->setString("auto_status.txt",
                                   state.autoClimateEnabled ? "Auto: ON" : "Auto: OFF");
    }

    // Example: Custom action on AC page (page 4, component 20)
    else if (page == 4 && component == 20) {
        Serial.println("Custom AC preset button pressed!");

        // Set AC to comfortable preset
        climateCard.setMode(2);  // Cool
        climateCard.setFanSpeed(1);  // Low
        climateCard.setTemperature(24);  // 24°C

        Serial.println("AC set to comfort preset");
    }
}

// ============================================
// Automation Functions
// ============================================

/**
 * @brief Automatic climate control based on temperature
 */
void autoClimateControl() {
    if (!state.autoClimateEnabled) return;
    if (climateCard.getSensorType() == AC_SENSOR_TYPE_NONE) return;

    float roomTemp = climateCard.getRoomTemperature();
    if (roomTemp <= 0 || roomTemp > 50) return;  // Invalid reading

    state.lastRoomTemp = roomTemp;

    // Get humidity if available
    if (climateCard.getSensorType() == AC_SENSOR_TYPE_DHT22) {
        state.lastHumidity = climateCard.getHumidity();
    }

    // Auto control logic
    uint8_t currentMode = climateCard.getMode();

    if (roomTemp > TEMP_HOT_THRESHOLD) {
        // Too hot - enable cooling
        if (currentMode != 2) {  // Not already in cool mode
            Serial.printf("Auto: Room hot (%.1f°C), enabling cooling\n", roomTemp);
            climateCard.setMode(2);  // Cool
            climateCard.setFanSpeed(1);  // Low fan
            climateCard.setTemperature(24);

            // Publish alert
            if (state.mqttConnected) {
                char msg[64];
                sprintf(msg, "Auto cooling enabled, room temp: %.1f°C", roomTemp);
                espmega.iot->publish("climate/auto", msg);
            }
        }
        // If very hot, increase fan speed
        else if (roomTemp > TEMP_HOT_THRESHOLD + 2) {
            if (climateCard.getFanSpeed() < 2) {
                Serial.println("Auto: Very hot, increasing fan speed");
                climateCard.setFanSpeed(2);  // Medium fan
            }
        }
    }
    else if (roomTemp > TEMP_WARM_THRESHOLD) {
        // Warm - use fan only
        if (currentMode == 0) {  // Currently off
            Serial.printf("Auto: Room warm (%.1f°C), enabling fan\n", roomTemp);
            climateCard.setMode(1);  // Fan only
            climateCard.setFanSpeed(0);  // Auto fan
        }
    }
    else if (roomTemp < TEMP_COOL_THRESHOLD) {
        // Cool enough - turn off
        if (currentMode != 0) {  // Currently on
            Serial.printf("Auto: Room cool (%.1f°C), turning off\n", roomTemp);
            climateCard.setMode(0);  // Off

            // Publish notification
            if (state.mqttConnected) {
                espmega.iot->publish("climate/auto", "Auto cooling disabled, room comfortable");
            }
        }
    }
}

/**
 * @brief Publish system status to MQTT
 */
void publishSystemStatus() {
    if (!state.mqttConnected) return;

    // Build status JSON
    char status[512];
    snprintf(status, sizeof(status),
             "{"
             "\"uptime\":%lu,"
             "\"network\":%s,"
             "\"mqtt\":%s,"
             "\"auto_climate\":%s,"
             "\"auto_input\":%s,"
             "\"room_temp\":%.1f,"
             "\"humidity\":%.1f,"
             "\"ac_mode\":\"%s\""
             "}",
             state.uptimeSeconds,
             state.networkConnected ? "true" : "false",
             state.mqttConnected ? "true" : "false",
             state.autoClimateEnabled ? "true" : "false",
             state.autoInputEnabled ? "true" : "false",
             state.lastRoomTemp,
             state.lastHumidity,
             mode_names[state.lastClimateMode]
    );

    espmega.iot->publish("status/system", status);
}

/**
 * @brief Print system status to serial
 */
void printSystemStatus() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║         SYSTEM STATUS REPORT           ║");
    Serial.println("╠════════════════════════════════════════╣");
    Serial.printf("║ Uptime:        %lu seconds\n", state.uptimeSeconds);
    Serial.printf("║ Network:       %s\n", state.networkConnected ? "Connected    " : "Disconnected ");
    Serial.printf("║ MQTT:          %s\n", state.mqttConnected ? "Connected    " : "Disconnected ");
    Serial.println("╠════════════════════════════════════════╣");
    Serial.printf("║ Auto Climate:  %s\n", state.autoClimateEnabled ? "Enabled      " : "Disabled     ");
    Serial.printf("║ Auto Input:    %s\n", state.autoInputEnabled ? "Enabled      " : "Disabled     ");
    Serial.println("╠════════════════════════════════════════╣");
    Serial.printf("║ Room Temp:     %.1f°C\n", state.lastRoomTemp);
    Serial.printf("║ Humidity:      %.1f%%\n", state.lastHumidity);
    Serial.printf("║ AC Mode:       %s\n", mode_names[state.lastClimateMode]);
    Serial.printf("║ AC Fan:        %s\n", fan_speed_names[climateCard.getFanSpeed()]);
    Serial.printf("║ AC Temp:       %d°C\n", climateCard.getTemperature());
    Serial.println("╚════════════════════════════════════════╝\n");
}

// ============================================
// Setup Function
// ============================================

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("╔══════════════════════════════════════════════════╗");
    Serial.println("║   ESPMegaPRO Full Integration Example           ║");
    Serial.println("║   Complete Display and Climate Control          ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    Serial.println();

    // ========================================
    // Initialize ESPMegaPRO
    // ========================================
    Serial.println("→ Initializing ESPMegaPRO core...");
    espmega.begin();

    // ========================================
    // Configure IoT Module
    // ========================================
    Serial.println("→ Configuring IoT module...");
    espmega.enableIotModule();

    Serial.println("→ Starting Ethernet...");
    ETH.begin();
    espmega.iot->bindEthernetInterface(&ETH);

    Serial.println("→ Loading network configuration...");
    espmega.iot->loadNetworkConfig();

    Serial.println("→ Connecting to network...");
    espmega.iot->connectNetwork();

    // Wait a moment for network to stabilize
    delay(2000);
    state.networkConnected = espmega.iot->networkConnected();

    Serial.println("→ Loading MQTT configuration...");
    espmega.iot->loadMqttConfig();

    Serial.println("→ Connecting to MQTT...");
    espmega.iot->connectToMqtt();

    delay(1000);
    state.mqttConnected = espmega.iot->mqttConnected();

    espmega.iot->registerMqttCallback(onMqttMessage);

    // ========================================
    // Install and Configure Climate Card
    // ========================================
    Serial.println("→ Installing climate card...");
    espmega.installCard(CLIMATE_CARD_SLOT, &climateCard);

    Serial.println("→ Binding climate card to FRAM...");
    climateCard.bindFRAM(&espmega.fram, CLIMATE_FRAM_ADDRESS);
    climateCard.loadStateFromFRAM();
    climateCard.setFRAMAutoSave(true);

    // Register climate callback
    auto climateCallback = std::bind(&onClimateChange,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3);
    climateCard.registerChangeCallback(climateCallback);

    #if TEMP_SENSOR_TYPE != AC_SENSOR_TYPE_NONE
    Serial.println("→ Configuring temperature sensor...");
    climateCard.bindSensor(TEMP_SENSOR_TYPE, TEMP_SENSOR_PIN);
    #endif

    // ========================================
    // Register Cards with MQTT
    // ========================================
    Serial.println("→ Registering cards with MQTT...");
    espmega.iot->registerCard(INPUT_CARD_SLOT);
    espmega.iot->registerCard(OUTPUT_CARD_SLOT);
    espmega.iot->registerCard(CLIMATE_CARD_SLOT);

    // ========================================
    // Enable Display
    // ========================================
    Serial.println("→ Enabling internal display...");
    espmega.enableInternalDisplay(&Serial);

    Serial.println("→ Binding cards to display...");
    espmega.display->bindInputCard(&espmega.inputs);
    espmega.display->bindOutputCard(&espmega.outputs);
    espmega.display->bindClimateCard(&climateCard);

    // Register custom touch handler
    Serial.println("→ Registering custom touch handler...");
    espmega.display->registerTouchCallback(onCustomTouch);

    // ========================================
    // Enable Web Server
    // ========================================
    #if ENABLE_WEB_SERVER
    Serial.println("→ Enabling web server...");
    espmega.enableWebServer(WEB_SERVER_PORT);

    // Set default credentials (remove in production!)
    espmega.webServer->setWebUsername("admin");
    espmega.webServer->setWebPassword("admin");
    espmega.webServer->saveCredentialsToFRAM();

    #if ENABLE_DISPLAY_OTA
    Serial.println("→ Enabling display OTA updates...");
    displayOta.begin("/display", espmega.display, espmega.webServer);
    Serial.println("   Display OTA available at: http://<ip>/display/index.html");
    #endif
    #endif

    // ========================================
    // Register Callbacks
    // ========================================
    Serial.println("→ Registering input callback...");
    espmega.inputs.registerCallback(onInputChange);

    Serial.println("→ Registering output callback...");
    auto outputCallback = std::bind(&onOutputChange,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3);
    espmega.outputs.registerChangeCallback(outputCallback);

    // ========================================
    // Initialization Complete
    // ========================================
    Serial.println();
    Serial.println("╔══════════════════════════════════════════════════╗");
    Serial.println("║             INITIALIZATION COMPLETE              ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    Serial.println();

    // Print network info
    if (state.networkConnected) {
        Serial.print("Network IP: ");
        Serial.println(ETH.localIP());
        Serial.print("Web Server: http://");
        Serial.print(ETH.localIP());
        Serial.println("/");
        #if ENABLE_DISPLAY_OTA
        Serial.print("Display OTA: http://");
        Serial.print(ETH.localIP());
        Serial.println("/display/index.html");
        #endif
    }

    Serial.println();
    Serial.println("Available display pages:");
    Serial.println("  1  - Dashboard");
    Serial.println("  2  - Digital Inputs");
    Serial.println("  3  - PWM Outputs");
    Serial.println("  4  - Air Conditioner");
    Serial.println("  5  - PWM Adjustment");
    Serial.println("  6  - Network Config");
    Serial.println("  11 - MQTT Config");
    Serial.println();

    Serial.println("Features enabled:");
    Serial.printf("  Auto Climate Control: %s\n", ENABLE_AUTO_CLIMATE ? "YES" : "NO");
    Serial.printf("  Input Automation: %s\n", ENABLE_INPUT_AUTOMATION ? "YES" : "NO");
    Serial.printf("  Web Server: %s\n", ENABLE_WEB_SERVER ? "YES" : "NO");
    Serial.printf("  Display OTA: %s\n", ENABLE_DISPLAY_OTA ? "YES" : "NO");
    Serial.printf("  Status Reporting: %s\n", ENABLE_STATUS_REPORTING ? "YES" : "NO");
    Serial.println();

    Serial.println("System ready!");
    Serial.println("══════════════════════════════════════════════════");
    Serial.println();
}

// ============================================
// Main Loop
// ============================================

void loop() {
    // Main system loop
    espmega.loop();

    // Update uptime
    state.uptimeSeconds = millis() / 1000;

    // Update connection states
    state.networkConnected = espmega.iot->networkConnected();
    state.mqttConnected = espmega.iot->mqttConnected();

    // ========================================
    // Auto Climate Control
    // ========================================
    #if ENABLE_AUTO_CLIMATE
    static unsigned long lastTempCheck = 0;
    if (millis() - lastTempCheck > TEMP_CHECK_INTERVAL) {
        lastTempCheck = millis();
        autoClimateControl();
    }
    #endif

    // ========================================
    // Status Reporting
    // ========================================
    #if ENABLE_STATUS_REPORTING
    static unsigned long lastStatusReport = 0;
    if (millis() - lastStatusReport > STATUS_REPORT_INTERVAL) {
        lastStatusReport = millis();
        printSystemStatus();
        publishSystemStatus();
    }
    #endif

    // ========================================
    // Error Recovery
    // ========================================

    // Auto-reconnect MQTT if disconnected
    static unsigned long lastMqttReconnect = 0;
    if (!state.mqttConnected && millis() - lastMqttReconnect > 30000) {
        lastMqttReconnect = millis();
        Serial.println("Attempting MQTT reconnection...");
        espmega.iot->connectToMqtt();
    }

    // Monitor network status
    static bool wasNetworkConnected = false;
    if (state.networkConnected != wasNetworkConnected) {
        wasNetworkConnected = state.networkConnected;
        if (state.networkConnected) {
            Serial.println("✓ Network connected");
        } else {
            Serial.println("✗ Network disconnected");
        }
    }

    // Monitor MQTT status
    static bool wasMqttConnected = false;
    if (state.mqttConnected != wasMqttConnected) {
        wasMqttConnected = state.mqttConnected;
        if (state.mqttConnected) {
            Serial.println("✓ MQTT connected");
            // Publish online status
            espmega.iot->publish("status/online", "true");
        } else {
            Serial.println("✗ MQTT disconnected");
        }
    }
}

/**
 * ============================================
 * ADDITIONAL NOTES AND CUSTOMIZATION TIPS
 * ============================================
 *
 * 1. Configuration:
 *    - Adjust thresholds at the top of the file
 *    - Enable/disable features with #define flags
 *    - Customize automation logic in callbacks
 *
 * 2. Display Customization:
 *    - Edit display layout in Nextion Editor
 *    - Add custom components and buttons
 *    - Handle custom touches in onCustomTouch()
 *
 * 3. MQTT Topics:
 *    Auto-registered topics:
 *    - <base>/input/<pin>/state
 *    - <base>/output/<pin>/state
 *    - <base>/climate/mode
 *    - <base>/climate/fan
 *    - <base>/climate/temperature
 *
 *    Custom topics:
 *    - <base>/command/auto_climate
 *    - <base>/command/auto_input
 *    - <base>/command/brightness
 *    - <base>/command/page
 *    - <base>/command/reboot
 *    - <base>/status/system
 *    - <base>/status/online
 *    - <base>/climate/auto
 *
 * 4. Automation Ideas:
 *    - Schedule-based climate control
 *    - Humidity-based dehumidifier control
 *    - Input pattern recognition
 *    - Output sequencing
 *    - Energy saving modes
 *    - Vacation mode
 *    - Integration with home automation
 *
 * 5. Web Interface:
 *    - Access at http://<ip>/
 *    - Default credentials: admin/admin
 *    - Change in production!
 *    - Display OTA at /display/index.html
 *
 * 6. Error Handling:
 *    - Automatic MQTT reconnection
 *    - Network status monitoring
 *    - Sensor error detection
 *    - Graceful degradation
 *
 * 7. Performance Optimization:
 *    - Adjust interval constants
 *    - Reduce MQTT publish frequency
 *    - Optimize serial communication
 *    - Use static variables efficiently
 *
 * 8. Security:
 *    - Change default web credentials
 *    - Use MQTT authentication
 *    - Consider TLS for MQTT
 *    - Validate all inputs
 *
 * 9. Debugging:
 *    - Monitor serial output
 *    - Check MQTT topics
 *    - Use status reporting
 *    - Add custom log messages
 *
 * 10. Extending This Example:
 *     - Add more cards (analog, etc.)
 *     - Implement data logging
 *     - Add LCD display support
 *     - Create custom web pages
 *     - Integrate with cloud services
 *     - Add voice control
 *     - Implement scenes/presets
 */
