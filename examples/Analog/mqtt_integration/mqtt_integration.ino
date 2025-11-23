/**
 * @file mqtt_integration.ino
 * @brief Complete MQTT integration example for ESPMegaPRO Analog Card
 *
 * This example demonstrates full IoT integration with:
 * - Remote DAC control via MQTT
 * - Automatic ADC value publishing
 * - Request/response patterns
 * - Custom ADC conversion callbacks
 * - Status reporting
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Analog Expansion Card
 * - Network connection (WiFi/Ethernet)
 * - MQTT broker (e.g., Mosquitto)
 *
 * Features Demonstrated:
 * - ESPMegaIoT integration
 * - MQTT publish/subscribe
 * - Remote sensor monitoring
 * - Remote actuator control
 * - Callback system
 * - Auto-discovery
 *
 * MQTT Topics:
 * Subscribe (Control):
 *   - <base>/dac/<00-03>/set/state
 *   - <base>/dac/<00-03>/set/value
 *   - <base>/adc/<00-07>/set/conversion_interval
 *   - <base>/adc/<00-07>/set/conversion_enabled
 *   - <base>/requeststate
 *
 * Publish (Status):
 *   - <base>/dac/<00-03>/state
 *   - <base>/dac/<00-03>/value
 *   - <base>/adc/<00-07>/value
 *
 * @author Siwat INC
 * @version 1.0
 * @date 2025-11-23
 */

#include <ESPMegaPRO.h>
#include <ESPMegaIoT.hpp>
#include <AnalogCard.hpp>

// Create instances
AnalogCard analogCard;
ESPMegaIoT iot;

// Network Configuration
// Note: Configure these in your ESPMegaPRO network settings
// This example assumes network is already configured

// MQTT Configuration
const char* MQTT_SERVER = "mqtt.local";  // Your MQTT broker address
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "";              // Leave empty if no authentication
const char* MQTT_PASSWORD = "";
const char* MQTT_BASE_TOPIC = "espmega/analog";

// ADC Configuration
const float ADC_REFERENCE = 6.144;
const float ADC_MAX = 32768.0;

// Application state
bool systemReady = false;
unsigned long lastStatusPrint = 0;
const unsigned long STATUS_PRINT_INTERVAL = 10000; // Print status every 10s

/**
 * @brief Convert ADC raw value to voltage
 */
float adcToVoltage(uint16_t rawValue) {
    return (rawValue * ADC_REFERENCE) / ADC_MAX;
}

/**
 * @brief Callback for ADC conversions
 * This is called whenever an ADC value is read and published
 */
void onADCConversion(uint8_t pin, uint16_t value) {
    float voltage = adcToVoltage(value);
    Serial.printf("[ADC Callback] Channel %d: Raw=%d, Voltage=%.3fV\n",
                  pin, value, voltage);

    // Example: Check for specific conditions
    if (pin == 0 && voltage > 5.0) {
        Serial.println("[WARNING] ADC 0 voltage exceeds 5V!");
    }

    // Example: Trigger actions based on sensor readings
    if (pin == 1) {
        // If ADC 1 (e.g., temperature) exceeds threshold, control DAC 0 (e.g., fan)
        if (voltage > 2.5) {
            analogCard.setDACState(0, true);
            Serial.println("[AUTO] Enabled DAC 0 (cooling activated)");
        } else if (voltage < 2.0) {
            analogCard.setDACState(0, false);
            Serial.println("[AUTO] Disabled DAC 0 (cooling deactivated)");
        }
    }
}

/**
 * @brief Callback for DAC changes
 * This is called whenever a DAC state or value changes
 */
void onDACChange(uint8_t pin, bool state, uint16_t value) {
    float voltage = (value / 4095.0) * 3.3;
    Serial.printf("[DAC Callback] Channel %d: State=%s, Value=%d (%.3fV)\n",
                  pin, state ? "ON" : "OFF", value, voltage);
}

/**
 * @brief Print system status
 */
void printSystemStatus() {
    Serial.println("\n========================================");
    Serial.println("         SYSTEM STATUS REPORT");
    Serial.println("========================================");
    Serial.printf("Uptime: %lu ms (%.2f hours)\n",
                  millis(), millis() / 3600000.0);
    Serial.println();

    // IoT Status
    Serial.println("IoT Status:");
    Serial.printf("  MQTT Connected: %s\n", iot.mqttConnected() ? "YES" : "NO");
    if (iot.mqttConnected()) {
        Serial.printf("  Base Topic: %s\n", MQTT_BASE_TOPIC);
    }
    Serial.println();

    // DAC Status
    Serial.println("DAC Status:");
    for (uint8_t i = 0; i < 4; i++) {
        bool state = analogCard.getDACState(i);
        uint16_t value = analogCard.getDACValue(i);
        float voltage = (value / 4095.0) * 3.3;

        Serial.printf("  DAC %d: %s  Value=%4d (%.3fV)\n",
                      i, state ? "ON " : "OFF", value, voltage);
    }
    Serial.println();

    // ADC Status (current readings)
    Serial.println("ADC Status:");
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t raw = analogCard.analogRead(i);
        float voltage = adcToVoltage(raw);

        Serial.printf("  ADC %d: Raw=%-5d  Voltage=%6.3fV\n", i, raw, voltage);
    }
    Serial.println("========================================\n");
}

/**
 * @brief Setup function
 */
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("  ESPMegaPRO Analog Card - MQTT Demo");
    Serial.println("========================================\n");

    // Initialize Analog Card
    Serial.print("Initializing Analog Card... ");
    if (!analogCard.begin()) {
        Serial.println("FAILED!");
        Serial.println("ERROR: Cannot continue without Analog Card");
        while (1) delay(1000);
    }
    Serial.println("SUCCESS!");

    // Register DAC change callback (before IoT to catch all changes)
    analogCard.registerDACChangeCallback(onDACChange);
    Serial.println("DAC change callback registered");

    // Initialize IoT system
    Serial.println("\nInitializing IoT system...");
    Serial.println("Note: Ensure network and MQTT settings are configured");

    // Begin IoT
    iot.begin();

    // Register the Analog Card with IoT system
    // This automatically creates an AnalogIoT component
    Serial.print("Registering Analog Card with IoT... ");
    iot.registerCard(0, &analogCard);
    Serial.println("SUCCESS!");

    // Get the AnalogIoT component for advanced configuration
    AnalogIoT* analogIoT = (AnalogIoT*)iot.getCard(0);

    if (analogIoT != nullptr) {
        Serial.println("\nConfiguring AnalogIoT component...");

        // Configure ADC publishing
        // Enable publishing for channels 0-3, disable for 4-7
        for (uint8_t i = 0; i < 4; i++) {
            analogIoT->setADCConversionEnabled(i, true);
            analogIoT->setADCConversionInterval(i, 2000); // Every 2 seconds
            Serial.printf("  ADC %d: Publishing enabled (2s interval)\n", i);
        }

        for (uint8_t i = 4; i < 8; i++) {
            analogIoT->setADCConversionEnabled(i, false);
            Serial.printf("  ADC %d: Publishing disabled\n", i);
        }

        // Register ADC conversion callback
        analogIoT->registerADCConversionCallback(onADCConversion);
        Serial.println("  ADC conversion callback registered");

        Serial.println("\nAnalogIoT configuration complete!");
    } else {
        Serial.println("WARNING: Could not get AnalogIoT component");
    }

    Serial.println("\n========================================");
    Serial.println("Setup complete! System ready.");
    Serial.println("========================================");

    Serial.println("\nMQTT Command Examples:");
    Serial.println("----------------------------------------");
    Serial.println("Set DAC 0 to 2048 (1.65V):");
    Serial.printf("  mosquitto_pub -h %s -t '%s/dac/00/set/value' -m '2048'\n",
                  MQTT_SERVER, MQTT_BASE_TOPIC);
    Serial.println("\nEnable DAC 0:");
    Serial.printf("  mosquitto_pub -h %s -t '%s/dac/00/set/state' -m '1'\n",
                  MQTT_SERVER, MQTT_BASE_TOPIC);
    Serial.println("\nRequest current state:");
    Serial.printf("  mosquitto_pub -h %s -t '%s/requeststate' -m '1'\n",
                  MQTT_SERVER, MQTT_BASE_TOPIC);
    Serial.println("\nSubscribe to ADC 0 readings:");
    Serial.printf("  mosquitto_sub -h %s -t '%s/adc/00/value'\n",
                  MQTT_SERVER, MQTT_BASE_TOPIC);
    Serial.println("\nEnable ADC 5 publishing (1s interval):");
    Serial.printf("  mosquitto_pub -h %s -t '%s/adc/05/set/conversion_enabled' -m '1'\n",
                  MQTT_SERVER, MQTT_BASE_TOPIC);
    Serial.printf("  mosquitto_pub -h %s -t '%s/adc/05/set/conversion_interval' -m '1000'\n",
                  MQTT_SERVER, MQTT_BASE_TOPIC);
    Serial.println("========================================\n");

    systemReady = true;
    lastStatusPrint = millis();
}

/**
 * @brief Main loop
 */
void loop() {
    // Process IoT (handles MQTT, network, etc.)
    iot.loop();

    // Process Analog Card
    analogCard.loop();

    // Print periodic status
    if (systemReady && millis() - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
        lastStatusPrint = millis();
        printSystemStatus();
    }

    // Optional: Add your application logic here
    // The callbacks will handle most of the work automatically
}

/**
 * MQTT Topic Structure:
 *
 * Base Topic: espmega/analog (configurable)
 *
 * DAC Topics:
 * ├── dac/00/set/state      [SUB] Set DAC 0 state (0=OFF, 1=ON)
 * ├── dac/00/set/value      [SUB] Set DAC 0 value (0-4095)
 * ├── dac/00/state          [PUB] DAC 0 current state
 * ├── dac/00/value          [PUB] DAC 0 current value
 * ├── dac/01/...            [Same for DAC 1]
 * ├── dac/02/...            [Same for DAC 2]
 * └── dac/03/...            [Same for DAC 3]
 *
 * ADC Topics:
 * ├── adc/00/value                    [PUB] ADC 0 reading
 * ├── adc/00/set/conversion_interval  [SUB] Set ADC 0 interval (ms)
 * ├── adc/00/set/conversion_enabled   [SUB] Enable ADC 0 (0=OFF, 1=ON)
 * ├── adc/01/...                      [Same for ADC 1]
 * ├── ... (through ADC 07)
 *
 * System Topics:
 * └── requeststate          [SUB] Request all states be published
 *
 * Configuration:
 *
 * 1. Network Setup:
 *    - Configure WiFi or Ethernet in ESPMegaPRO settings
 *    - Ensure network connectivity before MQTT
 *
 * 2. MQTT Broker:
 *    - Install mosquitto: apt-get install mosquitto mosquitto-clients
 *    - Start broker: systemctl start mosquitto
 *    - Test: mosquitto_sub -h localhost -t '#' -v
 *
 * 3. Firewall:
 *    - Allow port 1883 (MQTT)
 *    - For TLS: port 8883
 *
 * 4. Authentication (if required):
 *    - Set MQTT_USER and MQTT_PASSWORD
 *    - Configure broker with password file
 *
 * Advanced Features:
 *
 * 1. Custom Processing:
 *    - Modify onADCConversion() for custom sensor logic
 *    - Modify onDACChange() for state logging
 *
 * 2. Remote Control:
 *    - Use MQTT to control DACs from any networked device
 *    - Create dashboards with Node-RED, Home Assistant, etc.
 *
 * 3. Data Logging:
 *    - Subscribe to ADC topics and log to database
 *    - Use InfluxDB + Grafana for visualization
 *
 * 4. Alerts:
 *    - Monitor ADC values via MQTT
 *    - Trigger notifications on threshold violations
 *
 * Troubleshooting:
 *
 * 1. MQTT not connecting:
 *    - Check broker address and port
 *    - Verify network connectivity
 *    - Check broker logs
 *
 * 2. Topics not working:
 *    - Verify base topic matches configuration
 *    - Check topic format (case sensitive)
 *    - Ensure proper payload format (numeric)
 *
 * 3. ADC not publishing:
 *    - Check conversion_enabled is set to 1
 *    - Verify conversion_interval is reasonable
 *    - Check IoT loop is being called
 *
 * 4. Commands not taking effect:
 *    - Verify MQTT subscription is active
 *    - Check payload is valid number
 *    - Monitor serial output for errors
 */
