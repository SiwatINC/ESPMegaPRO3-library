/**
 * @file mqtt_control.ino
 * @brief Complete MQTT-controlled climate system using ESPMegaPRO
 *
 * This example demonstrates full IoT integration of the ClimateCard with
 * MQTT for remote monitoring and control over network.
 *
 * Hardware Required:
 * - ESPMegaPRO board (or ESP32 with Ethernet/WiFi)
 * - IR LED connected to GPIO 14
 * - DHT22 sensor connected to GPIO 27
 * - Ethernet connection (or configure WiFi)
 *
 * Features Demonstrated:
 * - Full ESPMegaPRO integration
 * - MQTT publish/subscribe for remote control
 * - State persistence with FRAM
 * - Network configuration
 * - Automatic reconnection
 * - Home automation integration
 *
 * MQTT Topics (base: "home/climate"):
 *   Subscribe (Control):
 *     home/climate/card/2/set/temperature   - Set AC temperature
 *     home/climate/card/2/set/mode          - Set AC mode
 *     home/climate/card/2/set/fan_speed     - Set fan speed
 *     home/climate/card/2/requeststate      - Request status update
 *
 *   Publish (Status):
 *     home/climate/card/2/temperature       - Current AC temperature setting
 *     home/climate/card/2/mode              - Current AC mode
 *     home/climate/card/2/fan_speed         - Current fan speed
 *     home/climate/card/2/room_temperature  - Room temperature from sensor
 *     home/climate/card/2/humidity          - Room humidity
 *
 * Home Assistant Integration:
 *   See configuration example at end of file
 */

#include <ESPMegaProOS.hpp>
#include <ClimateCard.hpp>
#include <ETH.h>

// ============================================================================
// Configuration
// ============================================================================

// Network Configuration
#define USE_STATIC_IP true       // Set to false for DHCP
#define USE_ETHERNET true        // Set to false for WiFi

// Static IP settings (if USE_STATIC_IP is true)
#define IP_ADDRESS {192, 168, 1, 100}
#define GATEWAY {192, 168, 1, 1}
#define SUBNET {255, 255, 255, 0}
#define DNS1 {8, 8, 8, 8}
#define DNS2 {8, 8, 4, 4}
#define HOSTNAME "espmega-climate"

// WiFi settings (if USE_ETHERNET is false)
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// MQTT Configuration
#define MQTT_SERVER "192.168.1.10"  // MQTT broker IP address
#define MQTT_PORT 1883
#define MQTT_BASE_TOPIC "home/climate"
#define MQTT_USE_AUTH false

// MQTT Authentication (if MQTT_USE_AUTH is true)
#define MQTT_USERNAME "mqtt_user"
#define MQTT_PASSWORD "mqtt_pass"

// Climate Card Configuration
#define IR_TX_PIN 14
#define SENSOR_PIN 27
#define SENSOR_TYPE AC_SENSOR_TYPE_DHT22
#define CLIMATE_CARD_SLOT 2      // Card slot number (affects MQTT topics)
#define RMT_CHANNEL RMT_CHANNEL_0

// FRAM Configuration
#define FRAM_CLIMATE_ADDRESS 1001  // Starting address in FRAM (3 bytes needed)
#define FRAM_AUTO_SAVE true        // Auto-save state on changes

// Debug Options
// #define ENABLE_SERIAL_DEBUG
// #define ENABLE_MQTT_DEBUG

// ============================================================================
// IR Code Definitions
// ============================================================================

const uint16_t irCodes[3][4][15][67] = {
    // Mode 0: Off
    {{{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}}},
    // Mode 1: Cool
    {{{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}}},
    // Mode 2: Fan Only
    {{{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}}}
};

size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr) {
    *codePtr = irCodes[mode][fan_speed][temperature];
    return sizeof(irCodes[mode][fan_speed][temperature]) / sizeof(uint16_t);
}

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
// Global Objects
// ============================================================================

ESPMegaPRO espmega;
ClimateCard climateCard(IR_TX_PIN, myAC, SENSOR_TYPE, SENSOR_PIN, RMT_CHANNEL);

// ============================================================================
// Callback Functions
// ============================================================================

void onMqttMessage(char *topic, char *payload) {
    #ifdef ENABLE_MQTT_DEBUG
    Serial.printf("[MQTT RX] %s: %s\n", topic, payload);
    #endif
}

void onACStateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    #ifdef ENABLE_SERIAL_DEBUG
    Serial.printf("[AC] State: %s, %s, %d°C\n",
                  climateCard.getModeName(),
                  climateCard.getFanSpeedName(),
                  temperature);
    #endif
}

void onSensorUpdate(float temperature, float humidity) {
    #ifdef ENABLE_SERIAL_DEBUG
    Serial.printf("[SENSOR] Temp: %.1f°C, Humidity: %.1f%%\n",
                  temperature, humidity);
    #endif
}

// ============================================================================
// Network Configuration Functions
// ============================================================================

void configureNetwork() {
    Serial.println("Configuring network...");

    NetworkConfig netConfig;

    // IP configuration
    if (USE_STATIC_IP) {
        uint8_t ip[] = IP_ADDRESS;
        uint8_t gateway[] = GATEWAY;
        uint8_t subnet[] = SUBNET;
        uint8_t dns1[] = DNS1;
        uint8_t dns2[] = DNS2;

        memcpy(netConfig.ip, ip, 4);
        memcpy(netConfig.gateway, gateway, 4);
        memcpy(netConfig.subnet, subnet, 4);
        memcpy(netConfig.dns1, dns1, 4);
        memcpy(netConfig.dns2, dns2, 4);
        netConfig.useStaticIp = true;
    } else {
        netConfig.useStaticIp = false;
    }

    // Network type
    netConfig.useWifi = !USE_ETHERNET;

    // WiFi settings
    if (!USE_ETHERNET) {
        netConfig.wifiUseAuth = true;
        strcpy(netConfig.ssid, WIFI_SSID);
        strcpy(netConfig.password, WIFI_PASSWORD);
    }

    // Hostname
    strcpy(netConfig.hostname, HOSTNAME);

    // Apply configuration
    espmega.iot->setNetworkConfig(netConfig);

    Serial.println("Network configured:");
    Serial.printf("  Type: %s\n", USE_ETHERNET ? "Ethernet" : "WiFi");
    Serial.printf("  IP Mode: %s\n", USE_STATIC_IP ? "Static" : "DHCP");
    if (USE_STATIC_IP) {
        Serial.printf("  IP: %d.%d.%d.%d\n",
                      netConfig.ip[0], netConfig.ip[1],
                      netConfig.ip[2], netConfig.ip[3]);
    }
    Serial.printf("  Hostname: %s\n", HOSTNAME);
}

void configureMQTT() {
    Serial.println("Configuring MQTT...");

    MqttConfig mqttConfig;

    // Server and port
    strcpy(mqttConfig.mqtt_server, MQTT_SERVER);
    mqttConfig.mqtt_port = MQTT_PORT;

    // Base topic
    strcpy(mqttConfig.base_topic, MQTT_BASE_TOPIC);

    // Authentication
    mqttConfig.mqtt_useauth = MQTT_USE_AUTH;
    #if MQTT_USE_AUTH
    strcpy(mqttConfig.mqtt_user, MQTT_USERNAME);
    strcpy(mqttConfig.mqtt_password, MQTT_PASSWORD);
    #endif

    // Apply configuration
    espmega.iot->setMqttConfig(mqttConfig);

    Serial.println("MQTT configured:");
    Serial.printf("  Broker: %s:%d\n", MQTT_SERVER, MQTT_PORT);
    Serial.printf("  Base Topic: %s\n", MQTT_BASE_TOPIC);
    Serial.printf("  Authentication: %s\n", MQTT_USE_AUTH ? "Yes" : "No");
}

// ============================================================================
// Status Display
// ============================================================================

void displaySystemStatus() {
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║           Climate Control System Status          ║");
    Serial.println("╠════════════════════════════════════════════════════╣");

    // Network status
    Serial.printf("║  Network: %-40s ║\n",
                  espmega.iot->networkConnected() ? "CONNECTED" : "DISCONNECTED");

    if (espmega.iot->networkConnected()) {
        IPAddress ip = USE_ETHERNET ? ETH.localIP() : WiFi.localIP();
        char ipStr[16];
        sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        Serial.printf("║  IP Address: %-37s ║\n", ipStr);
    }

    // MQTT status
    Serial.printf("║  MQTT: %-43s ║\n",
                  espmega.iot->mqttConnected() ? "CONNECTED" : "DISCONNECTED");

    Serial.println("╠════════════════════════════════════════════════════╣");

    // Climate status
    Serial.printf("║  Room Temperature: %6.1f°C                       ║\n",
                  climateCard.getRoomTemperature());
    if (SENSOR_TYPE == AC_SENSOR_TYPE_DHT22) {
        Serial.printf("║  Room Humidity: %6.1f%%                          ║\n",
                      climateCard.getHumidity());
    }

    Serial.println("╠════════════════════════════════════════════════════╣");

    Serial.printf("║  AC Mode: %-40s ║\n", climateCard.getModeName());
    Serial.printf("║  AC Fan: %-41s ║\n", climateCard.getFanSpeedName());
    Serial.printf("║  AC Temperature: %6d°C                         ║\n",
                  climateCard.getTemperature());

    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

void displayMQTTTopics() {
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║              MQTT Topics Reference                ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  Control Topics (Subscribe):                      ║");
    Serial.printf("║    %s/card/%d/set/temperature      ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    Serial.printf("║    %s/card/%d/set/mode             ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    Serial.printf("║    %s/card/%d/set/fan_speed        ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    Serial.printf("║    %s/card/%d/requeststate         ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  Status Topics (Publish):                         ║");
    Serial.printf("║    %s/card/%d/temperature          ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    Serial.printf("║    %s/card/%d/mode                 ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    Serial.printf("║    %s/card/%d/fan_speed            ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    Serial.printf("║    %s/card/%d/room_temperature     ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    if (SENSOR_TYPE == AC_SENSOR_TYPE_DHT22) {
        Serial.printf("║    %s/card/%d/humidity             ║\n", MQTT_BASE_TOPIC, CLIMATE_CARD_SLOT);
    }
    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════╗");
    Serial.println("║      ESPMegaPRO Climate Control with MQTT         ║");
    Serial.println("╚════════════════════════════════════════════════════╝");
    Serial.println();

    // ========================================================================
    // Step 1: Initialize ESPMegaPRO
    // ========================================================================
    Serial.println("[1/8] Initializing ESPMegaPRO...");
    espmega.begin();
    Serial.println("      ✓ ESPMegaPRO initialized");

    // ========================================================================
    // Step 2: Enable IoT Module
    // ========================================================================
    Serial.println("[2/8] Enabling IoT module...");
    espmega.enableIotModule();
    Serial.println("      ✓ IoT module enabled");

    // ========================================================================
    // Step 3: Initialize Network Interface
    // ========================================================================
    Serial.println("[3/8] Initializing network interface...");
    if (USE_ETHERNET) {
        ETH.begin();
        espmega.iot->bindEthernetInterface(&ETH);
        Serial.println("      ✓ Ethernet interface initialized");
    } else {
        espmega.iot->bindWiFi(&WiFi);
        Serial.println("      ✓ WiFi interface initialized");
    }

    // ========================================================================
    // Step 4: Configure and Connect Network
    // ========================================================================
    Serial.println("[4/8] Configuring network...");
    configureNetwork();
    Serial.println("      Connecting to network...");
    espmega.iot->connectNetwork();

    // Wait for network connection
    int networkWaitCount = 0;
    while (!espmega.iot->networkConnected() && networkWaitCount < 30) {
        delay(1000);
        Serial.print(".");
        networkWaitCount++;
    }
    Serial.println();

    if (espmega.iot->networkConnected()) {
        Serial.println("      ✓ Network connected");
        IPAddress ip = USE_ETHERNET ? ETH.localIP() : WiFi.localIP();
        Serial.printf("      IP Address: %s\n", ip.toString().c_str());
    } else {
        Serial.println("      ✗ Network connection failed!");
        Serial.println("      Check network settings and restart");
    }

    // ========================================================================
    // Step 5: Configure and Connect MQTT
    // ========================================================================
    Serial.println("[5/8] Configuring MQTT...");
    configureMQTT();
    Serial.println("      Connecting to MQTT broker...");
    espmega.iot->connectToMqtt();

    // Wait for MQTT connection
    int mqttWaitCount = 0;
    while (!espmega.iot->mqttConnected() && mqttWaitCount < 20) {
        delay(1000);
        Serial.print(".");
        mqttWaitCount++;
    }
    Serial.println();

    if (espmega.iot->mqttConnected()) {
        Serial.println("      ✓ MQTT connected");
    } else {
        Serial.println("      ✗ MQTT connection failed!");
        Serial.println("      Check broker settings");
    }

    // Register MQTT callback
    espmega.iot->registerMqttCallback(onMqttMessage);

    // ========================================================================
    // Step 6: Initialize Climate Card
    // ========================================================================
    Serial.println("[6/8] Initializing ClimateCard...");
    if (!climateCard.begin()) {
        Serial.println("      ✗ ClimateCard initialization failed!");
    } else {
        Serial.println("      ✓ ClimateCard initialized");
    }

    // Register callbacks
    climateCard.registerSensorCallback(onSensorUpdate);
    climateCard.registerChangeCallback(onACStateChange);

    // ========================================================================
    // Step 7: Configure FRAM Persistence
    // ========================================================================
    Serial.println("[7/8] Configuring FRAM persistence...");
    climateCard.bindFRAM(&espmega.fram, FRAM_CLIMATE_ADDRESS);
    climateCard.loadStateFromFRAM();
    climateCard.setFRAMAutoSave(FRAM_AUTO_SAVE);
    Serial.printf("      ✓ FRAM configured (address %d)\n", FRAM_CLIMATE_ADDRESS);
    Serial.printf("      ✓ State loaded from FRAM\n");

    // ========================================================================
    // Step 8: Install Card and Register for IoT
    // ========================================================================
    Serial.println("[8/8] Installing card and registering for IoT...");
    espmega.installCard(CLIMATE_CARD_SLOT, &climateCard);
    Serial.printf("      ✓ Card installed in slot %d\n", CLIMATE_CARD_SLOT);

    espmega.iot->registerCard(CLIMATE_CARD_SLOT);
    Serial.println("      ✓ Card registered for MQTT");

    // ========================================================================
    // Initialization Complete
    // ========================================================================
    Serial.println();
    Serial.println("════════════════════════════════════════════════════");
    Serial.println("         Initialization Complete!                   ");
    Serial.println("════════════════════════════════════════════════════");
    Serial.println();

    displaySystemStatus();
    displayMQTTTopics();

    Serial.println("System is ready for MQTT control!");
    Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    // Update ESPMegaPRO (handles network, MQTT, cards, etc.)
    espmega.loop();

    // Periodic status display (every 5 minutes)
    static unsigned long lastStatusDisplay = 0;
    if (millis() - lastStatusDisplay >= 300000) {
        lastStatusDisplay = millis();
        displaySystemStatus();
    }

    // Monitor connection status
    static bool lastNetworkState = false;
    static bool lastMqttState = false;

    bool currentNetworkState = espmega.iot->networkConnected();
    bool currentMqttState = espmega.iot->mqttConnected();

    if (currentNetworkState != lastNetworkState) {
        lastNetworkState = currentNetworkState;
        Serial.printf("[NETWORK] %s\n", currentNetworkState ? "CONNECTED" : "DISCONNECTED");
        if (currentNetworkState) {
            IPAddress ip = USE_ETHERNET ? ETH.localIP() : WiFi.localIP();
            Serial.printf("[NETWORK] IP: %s\n", ip.toString().c_str());
        }
    }

    if (currentMqttState != lastMqttState) {
        lastMqttState = currentMqttState;
        Serial.printf("[MQTT] %s\n", currentMqttState ? "CONNECTED" : "DISCONNECTED");
    }
}

// ============================================================================
// MQTT Command Examples
// ============================================================================

/*
 * Using mosquitto_pub (command line):
 *
 * Set temperature to 24°C:
 *   mosquitto_pub -h 192.168.1.10 -t "home/climate/card/2/set/temperature" -m "24"
 *
 * Set mode to cool:
 *   mosquitto_pub -h 192.168.1.10 -t "home/climate/card/2/set/mode" -m "cool"
 *
 * Set fan speed to high:
 *   mosquitto_pub -h 192.168.1.10 -t "home/climate/card/2/set/fan_speed" -m "high"
 *
 * Request state update:
 *   mosquitto_pub -h 192.168.1.10 -t "home/climate/card/2/requeststate" -m ""
 *
 * Subscribe to all status updates:
 *   mosquitto_sub -h 192.168.1.10 -t "home/climate/card/2/#"
 */

// ============================================================================
// Home Assistant Configuration
// ============================================================================

/*
 * Add to your Home Assistant configuration.yaml:
 *
 * climate:
 *   - platform: mqtt
 *     name: "Living Room AC"
 *     modes:
 *       - "off"
 *       - "cool"
 *       - "fan_only"
 *     fan_modes:
 *       - "auto"
 *       - "low"
 *       - "medium"
 *       - "high"
 *     temperature_command_topic: "home/climate/card/2/set/temperature"
 *     temperature_state_topic: "home/climate/card/2/temperature"
 *     mode_command_topic: "home/climate/card/2/set/mode"
 *     mode_state_topic: "home/climate/card/2/mode"
 *     fan_mode_command_topic: "home/climate/card/2/set/fan_speed"
 *     fan_mode_state_topic: "home/climate/card/2/fan_speed"
 *     current_temperature_topic: "home/climate/card/2/room_temperature"
 *     min_temp: 16
 *     max_temp: 30
 *     temp_step: 1
 *
 * sensor:
 *   - platform: mqtt
 *     name: "Living Room Temperature"
 *     state_topic: "home/climate/card/2/room_temperature"
 *     unit_of_measurement: "°C"
 *     device_class: temperature
 *
 *   - platform: mqtt
 *     name: "Living Room Humidity"
 *     state_topic: "home/climate/card/2/humidity"
 *     unit_of_measurement: "%"
 *     device_class: humidity
 */

// ============================================================================
// Node-RED Flow Example
// ============================================================================

/*
 * Simple Node-RED flow for AC control:
 *
 * [Inject Node: 24] --> [Change: set topic to "home/climate/card/2/set/temperature"]
 *                   --> [MQTT Out]
 *
 * [MQTT In: "home/climate/card/2/#"] --> [Debug]
 *
 * Dashboard example:
 *
 * [UI Slider: 16-30] --> [Change: set topic] --> [MQTT Out]
 * [UI Dropdown: modes] --> [Change: set topic] --> [MQTT Out]
 * [MQTT In] --> [UI Text: display temp]
 */

// ============================================================================
// Troubleshooting
// ============================================================================

/*
 * Network won't connect:
 * - Check Ethernet cable or WiFi credentials
 * - Verify IP settings (try DHCP first)
 * - Check router/firewall settings
 *
 * MQTT won't connect:
 * - Verify broker IP and port
 * - Check authentication settings
 * - Test with mosquitto_pub/sub from another device
 * - Check broker logs
 *
 * AC not responding to MQTT:
 * - Verify topic names match configuration
 * - Check payload format (integers for temp, strings for mode/fan)
 * - Subscribe to topics to verify messages received
 * - Enable MQTT_DEBUG to see incoming messages
 *
 * State not persisting:
 * - Verify FRAM is working (check other FRAM examples)
 * - Ensure FRAM address doesn't conflict
 * - Verify FRAM_AUTO_SAVE is enabled
 */
