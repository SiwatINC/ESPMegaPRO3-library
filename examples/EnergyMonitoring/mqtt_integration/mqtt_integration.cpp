/**
 * MQTT Integration for Current Transformer Monitoring
 *
 * This example demonstrates complete IoT integration for energy monitoring
 * using MQTT. Features include:
 * - Automatic MQTT publishing of current, power, and energy
 * - Remote energy reset and set commands
 * - Home Assistant auto-discovery support
 * - Network configuration with WiFi/Ethernet
 * - FRAM persistence for energy data
 * - Remote monitoring and control
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board with FRAM
 * - Analog Card
 * - Current Transformer (e.g., SCT-013-000)
 * - Network connection (WiFi or Ethernet)
 *
 * MQTT Topics (assuming base_topic = "home/espmega"):
 *   Published:
 *     - home/espmega/ct/0/current  - Current measurement (A)
 *     - home/espmega/ct/0/power    - Power consumption (W)
 *     - home/espmega/ct/0/energy   - Accumulated energy (Wh)
 *
 *   Subscribed:
 *     - home/espmega/ct/0/requeststate  - Request status update
 *     - home/espmega/ct/0/energy/set    - Set energy value
 *     - home/espmega/ct/0/energy/reset  - Reset energy to zero
 *
 * Author: SIWAT INC
 * Date: 2024
 */

#include <ESPMegaIoT.hpp>
#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>
#include <CurrentTransformerIoT.hpp>
#include <FRAM.h>

// =========================
// Configuration
// =========================

// Network Configuration
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const bool USE_WIFI = true;  // Set to false for Ethernet

// MQTT Configuration
const char* MQTT_SERVER = "192.168.1.100";  // Your MQTT broker IP
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "espmega";          // Leave empty if no auth
const char* MQTT_PASSWORD = "password";     // Leave empty if no auth
const bool MQTT_USE_AUTH = true;            // Set to false for no auth
const char* BASE_TOPIC = "home/espmega";

// CT Configuration
const float LINE_VOLTAGE = 230.0;
const uint32_t CT_FRAM_ADDRESS = 1000;

// Card ID for IoT system
const uint8_t CT_CARD_ID = 0;

// =========================
// Global Instances
// =========================

extern FRAM ESPMega_FRAM;

ESPMegaIoT iot;
AnalogCard analogCard;
float lineVoltage = LINE_VOLTAGE;

// ADC to Current conversion for SCT-013-000
auto adcToCurrent = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;
    const float SCALE_FACTOR = 0.0488;
    return (adc - ADC_ZERO) * SCALE_FACTOR;
};

CurrentTransformerCard ct(
    &analogCard,
    0,              // Analog input A0
    &lineVoltage,
    adcToCurrent,
    1000            // Sample every 1 second
);

CurrentTransformerIoT ctIot;

// Expansion cards array for IoT system
ExpansionCard* cards[1];

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("========================================");
    Serial.println("  MQTT Energy Monitor Integration");
    Serial.println("========================================");
    Serial.println();

    // Initialize FRAM
    Serial.print("Initializing FRAM...");
    if (!ESPMega_FRAM.begin()) {
        Serial.println(" FAILED!");
        Serial.println("WARNING: FRAM not available, energy will not persist");
    } else {
        Serial.println(" OK");
    }

    // Initialize Analog Card
    Serial.print("Initializing Analog Card...");
    if (!analogCard.begin()) {
        Serial.println(" FAILED!");
        Serial.println("FATAL ERROR: Cannot start without Analog Card");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" OK");

    // Initialize CT Card
    Serial.print("Initializing Current Transformer Card...");
    if (!ct.begin()) {
        Serial.println(" FAILED!");
        Serial.println("FATAL ERROR: CT initialization failed");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" OK");

    // Bind FRAM and load energy
    if (ESPMega_FRAM.begin()) {
        ct.bindFRAM(&ESPMega_FRAM, CT_FRAM_ADDRESS);
        ct.loadEnergy();
        Serial.printf("Loaded energy from FRAM: %.3f Wh\n", ct.getEnergy());

        // Enable auto-save for MQTT applications
        ct.setEnergyAutoSave(false);  // Manual save for better control
    }

    // Setup expansion cards array
    cards[0] = &ct;

    // Configure IoT system
    Serial.println();
    Serial.println("Configuring IoT System...");

    // Initialize IoT
    iot.intr_begin(cards);

    // Configure network
    NetworkConfig* netConfig = iot.getNetworkConfig();
    netConfig->useWifi = USE_WIFI;
    if (USE_WIFI) {
        netConfig->wifiUseAuth = true;
        strncpy(netConfig->ssid, WIFI_SSID, 32);
        strncpy(netConfig->password, WIFI_PASSWORD, 32);
    }
    strncpy(netConfig->hostname, "espmega-energy", 32);

    // Configure MQTT
    MqttConfig* mqttConfig = iot.getMqttConfig();
    strncpy(mqttConfig->mqtt_server, MQTT_SERVER, 32);
    mqttConfig->mqtt_port = MQTT_PORT;
    mqttConfig->mqtt_useauth = MQTT_USE_AUTH;
    if (MQTT_USE_AUTH) {
        strncpy(mqttConfig->mqtt_user, MQTT_USER, 32);
        strncpy(mqttConfig->mqtt_password, MQTT_PASSWORD, 32);
    }
    strncpy(mqttConfig->base_topic, BASE_TOPIC, 32);

    // Connect to network
    Serial.println();
    if (USE_WIFI) {
        Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
        iot.connectToWifi(WIFI_SSID, WIFI_PASSWORD);

        // Wait for WiFi connection
        int attempts = 0;
        while (!iot.wifiConnected() && attempts < 30) {
            Serial.print(".");
            delay(1000);
            attempts++;
        }

        if (iot.wifiConnected()) {
            Serial.println(" Connected!");
        } else {
            Serial.println(" FAILED!");
            Serial.println("Could not connect to WiFi. Check credentials.");
        }
    } else {
        Serial.println("Starting Ethernet...");
        iot.ethernetBegin();
        delay(2000);  // Wait for Ethernet
        Serial.println("Ethernet started");
    }

    // Initialize CT IoT component
    Serial.println();
    Serial.print("Initializing CT IoT component...");
    if (!ctIot.begin(CT_CARD_ID, &ct, iot.getMqttClient(), (char*)BASE_TOPIC)) {
        Serial.println(" FAILED!");
    } else {
        Serial.println(" OK");
    }

    // Subscribe to MQTT topics
    ctIot.subscribe();

    // Register card with IoT system
    iot.registerCard(CT_CARD_ID);

    Serial.println();
    Serial.println("========================================");
    Serial.println("System Configuration:");
    Serial.println("========================================");
    Serial.printf("Network:      %s\n", USE_WIFI ? "WiFi" : "Ethernet");
    if (USE_WIFI) {
        Serial.printf("SSID:         %s\n", WIFI_SSID);
    }
    Serial.printf("MQTT Broker:  %s:%d\n", MQTT_SERVER, MQTT_PORT);
    Serial.printf("MQTT Auth:    %s\n", MQTT_USE_AUTH ? "Enabled" : "Disabled");
    Serial.printf("Base Topic:   %s\n", BASE_TOPIC);
    Serial.printf("CT Card ID:   %d\n", CT_CARD_ID);
    Serial.println();
    Serial.println("MQTT Topics:");
    Serial.printf("  Current:  %s/ct/%d/current\n", BASE_TOPIC, CT_CARD_ID);
    Serial.printf("  Power:    %s/ct/%d/power\n", BASE_TOPIC, CT_CARD_ID);
    Serial.printf("  Energy:   %s/ct/%d/energy\n", BASE_TOPIC, CT_CARD_ID);
    Serial.println();
    Serial.printf("  Set:      %s/ct/%d/energy/set\n", BASE_TOPIC, CT_CARD_ID);
    Serial.printf("  Reset:    %s/ct/%d/energy/reset\n", BASE_TOPIC, CT_CARD_ID);
    Serial.printf("  Request:  %s/ct/%d/requeststate\n", BASE_TOPIC, CT_CARD_ID);
    Serial.println("========================================");
    Serial.println();

    delay(2000);
    Serial.println("Monitoring started...");
    Serial.println();
}

void loop() {
    // Update CT card (measurements)
    ct.loop();

    // Update IoT system (MQTT, network)
    iot.loop();

    // Print status to serial
    printStatus();

    // Periodic FRAM save (every 10 Wh to reduce wear)
    static double lastSavedEnergy = 0;
    double currentEnergy = ct.getEnergy();
    if (currentEnergy - lastSavedEnergy >= 10.0) {
        ct.saveEnergy();
        lastSavedEnergy = currentEnergy;
        Serial.println();
        Serial.printf(">>> Energy saved to FRAM: %.3f Wh\n", currentEnergy);
        Serial.println();
    }

    delay(100);
}

/**
 * Print current status to serial monitor
 */
void printStatus() {
    static unsigned long lastPrint = 0;

    if (millis() - lastPrint >= 5000) {
        // Network status
        String networkStatus = "Disconnected";
        if (USE_WIFI && iot.wifiConnected()) {
            networkStatus = "WiFi Connected";
        } else if (!USE_WIFI) {
            networkStatus = "Ethernet";
        }

        // MQTT status
        String mqttStatus = iot.getMqttClient()->connected() ? "Connected" : "Disconnected";

        // Measurements
        float current = ct.getCurrent();
        float power = ct.getPower();
        double energy = ct.getEnergy();

        Serial.printf("[Net: %s | MQTT: %s] Current: %.2f A | Power: %.2f W | Energy: %.3f Wh\n",
            networkStatus.c_str(),
            mqttStatus.c_str(),
            current,
            power,
            energy
        );

        lastPrint = millis();
    }
}

/*
 * ========================================
 * Home Assistant Configuration
 * ========================================
 *
 * Add this to your configuration.yaml:
 *
 * mqtt:
 *   sensor:
 *     # Current Sensor
 *     - name: "Energy Monitor Current"
 *       state_topic: "home/espmega/ct/0/current"
 *       unit_of_measurement: "A"
 *       device_class: current
 *       state_class: measurement
 *       icon: mdi:current-ac
 *
 *     # Power Sensor
 *     - name: "Energy Monitor Power"
 *       state_topic: "home/espmega/ct/0/power"
 *       unit_of_measurement: "W"
 *       device_class: power
 *       state_class: measurement
 *       icon: mdi:flash
 *
 *     # Energy Sensor
 *     - name: "Energy Monitor Energy"
 *       state_topic: "home/espmega/ct/0/energy"
 *       unit_of_measurement: "Wh"
 *       device_class: energy
 *       state_class: total_increasing
 *       icon: mdi:lightning-bolt
 *
 *   # Reset Energy Button
 *   button:
 *     - name: "Reset Energy Monitor"
 *       command_topic: "home/espmega/ct/0/energy/reset"
 *       icon: mdi:restore
 *
 *   # Set Energy Number Input
 *   number:
 *     - name: "Set Energy Monitor Value"
 *       command_topic: "home/espmega/ct/0/energy/set"
 *       state_topic: "home/espmega/ct/0/energy"
 *       min: 0
 *       max: 999999
 *       step: 1
 *       unit_of_measurement: "Wh"
 *
 * # Utility Meter for Daily/Monthly Tracking
 * utility_meter:
 *   energy_monitor_daily:
 *     source: sensor.energy_monitor_energy
 *     cycle: daily
 *
 *   energy_monitor_monthly:
 *     source: sensor.energy_monitor_energy
 *     cycle: monthly
 *
 * # Template Sensor for kWh conversion
 * template:
 *   - sensor:
 *       - name: "Energy Monitor kWh"
 *         unit_of_measurement: "kWh"
 *         state: >
 *           {{ (states('sensor.energy_monitor_energy') | float / 1000) | round(3) }}
 *
 *       - name: "Energy Monitor Cost"
 *         unit_of_measurement: "USD"
 *         state: >
 *           {{ (states('sensor.energy_monitor_kwh') | float * 0.12) | round(2) }}
 *
 * ========================================
 * MQTT Command Examples
 * ========================================
 *
 * Using mosquitto_pub command line tool:
 *
 * # Request current state
 * mosquitto_pub -h 192.168.1.100 -t "home/espmega/ct/0/requeststate" -m ""
 *
 * # Reset energy counter
 * mosquitto_pub -h 192.168.1.100 -t "home/espmega/ct/0/energy/reset" -m ""
 *
 * # Set energy to specific value (e.g., 1000 Wh)
 * mosquitto_pub -h 192.168.1.100 -t "home/espmega/ct/0/energy/set" -m "1000.0"
 *
 * # Subscribe to all energy monitor topics
 * mosquitto_sub -h 192.168.1.100 -t "home/espmega/ct/0/#" -v
 *
 * ========================================
 * Node-RED Flow Example
 * ========================================
 *
 * [MQTT In] → [Function: Parse] → [InfluxDB Out]
 *                                → [Dashboard Gauge]
 *                                → [Notification (if > threshold)]
 *
 * Function node code:
 *
 * // Extract location and measurement
 * var parts = msg.topic.split('/');
 * var measurement = parts[parts.length - 1];
 * var value = parseFloat(msg.payload);
 *
 * // Create InfluxDB point
 * msg.payload = {
 *     measurement: measurement,
 *     fields: {
 *         value: value
 *     },
 *     tags: {
 *         location: "main_circuit",
 *         device: "espmega"
 *     }
 * };
 *
 * return msg;
 *
 * ========================================
 * Troubleshooting
 * ========================================
 *
 * 1. MQTT not connecting:
 *    - Check MQTT broker IP and port
 *    - Verify credentials if auth is enabled
 *    - Check firewall settings
 *    - Test with mosquitto_pub/sub
 *
 * 2. No data being published:
 *    - Check network connection
 *    - Verify MQTT broker is running
 *    - Check base topic configuration
 *    - Monitor serial output for errors
 *
 * 3. Energy not persisting:
 *    - Check FRAM initialization
 *    - Verify FRAM address not in use
 *    - Check auto-save settings
 *    - Monitor save events in serial
 *
 * 4. WiFi connection issues:
 *    - Verify SSID and password
 *    - Check signal strength
 *    - Try static IP if DHCP fails
 *    - Check router settings
 *
 * ========================================
 * Advanced Features
 * ========================================
 *
 * 1. Multiple CT Monitoring:
 *    - See multiple_ct.ino example
 *    - Each CT gets unique card_id
 *    - Topics: home/espmega/ct/0/, ct/1/, etc.
 *
 * 2. Custom Publishing Rate:
 *    - Modify conversionInterval for sampling
 *    - Add rate limiting to reduce MQTT traffic
 *    - Balance between responsiveness and bandwidth
 *
 * 3. Security:
 *    - Use MQTT over TLS (port 8883)
 *    - Implement strong passwords
 *    - Consider VPN for remote access
 *    - Regular firmware updates
 */
