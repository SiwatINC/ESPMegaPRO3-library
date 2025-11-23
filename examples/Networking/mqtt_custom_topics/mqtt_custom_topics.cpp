/**
 * ESPMegaPRO MQTT Custom Topics Example
 *
 * This example demonstrates advanced MQTT topic usage with custom namespaces,
 * wildcards, and complex message handling.
 *
 * Features demonstrated:
 * - Custom topic namespaces (sensors, controls, status, config)
 * - Wildcard subscriptions (+ and #)
 * - JSON message publishing
 * - Multiple callback handlers
 * - Topic-based routing
 * - Configuration over MQTT
 * - Sensor data publishing
 *
 * Topic Structure:
 *   {base_topic}/sensors/temperature
 *   {base_topic}/sensors/humidity
 *   {base_topic}/sensors/all (JSON)
 *   {base_topic}/controls/led
 *   {base_topic}/controls/interval
 *   {base_topic}/status/system (JSON)
 *   {base_topic}/config/set
 *   {base_topic}/config/get
 *
 * Hardware Requirements:
 * - ESPMegaPRO R3 board
 * - Network connection
 * - MQTT broker
 *
 * Created: 2024
 * Author: SIWAT SYSTEM
 */

#include <ESPMegaPRO.h>
#include <ETH.h>
#include <ArduinoJson.h>

// Create ESPMegaPRO instance
ESPMegaPRO espmega;

// ===== MQTT CONFIGURATION =====
const char* MQTT_SERVER = "192.168.1.5";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_BASE_TOPIC = "home/espmega";

// ===== NETWORK CONFIGURATION =====
const char* hostname = "espmega-custom-01";
const IPAddress static_ip(192, 168, 1, 100);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns1(1, 1, 1, 1);

// ===== APPLICATION VARIABLES =====
// Simulated sensor values
float temperature = 23.5;
float humidity = 45.2;
uint32_t pressure = 1013;

// Control variables
bool ledState = false;
uint32_t publishInterval = 10000;  // Configurable publish interval

// Configuration
struct Config {
    bool enableSensors;
    bool enableControls;
    uint32_t sensorInterval;
    char location[32];
} config = {
    true,      // enableSensors
    true,      // enableControls
    10000,     // sensorInterval (10 seconds)
    "Living Room"  // location
};

// Timing
unsigned long lastSensorPublish = 0;
unsigned long lastStatusPublish = 0;
const unsigned long STATUS_INTERVAL = 30000;  // 30 seconds

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("========================================");
    Serial.println("ESPMegaPRO MQTT Custom Topics Example");
    Serial.println("========================================");
    Serial.println();

    // Initialize ESPMegaPRO
    espmega.begin();

    // Setup network
    setupNetwork();

    // Setup MQTT
    setupMQTT();

    Serial.println();
    Serial.println("========================================");
    Serial.println("Setup Complete");
    Serial.println("========================================");
    printTopicMap();
}

void loop() {
    espmega.loop();

    // Publish sensor data
    if (millis() - lastSensorPublish > config.sensorInterval) {
        lastSensorPublish = millis();

        if (espmega.iot.mqttConnected() && config.enableSensors) {
            publishSensorData();
        }
    }

    // Publish system status
    if (millis() - lastStatusPublish > STATUS_INTERVAL) {
        lastStatusPublish = millis();

        if (espmega.iot.mqttConnected()) {
            publishSystemStatus();
        }
    }

    // Simulate sensor value changes
    updateSimulatedSensors();
}

/**
 * Setup network connection
 */
void setupNetwork() {
    Serial.println("[NETWORK] Configuring network...");

    NetworkConfig netConfig;
    netConfig.ip = static_ip;
    netConfig.gateway = gateway;
    netConfig.subnet = subnet;
    netConfig.dns1 = dns1;
    strcpy(netConfig.hostname, hostname);
    netConfig.useStaticIp = true;
    netConfig.useWifi = false;

    espmega.iot.setNetworkConfig(netConfig);
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.connectNetwork();

    // Wait for connection
    unsigned long startTime = millis();
    while (!espmega.iot.networkConnected() && (millis() - startTime < 30000)) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (espmega.iot.networkConnected()) {
        Serial.println("[NETWORK] Connected!");
        Serial.print("IP: ");
        Serial.println(espmega.iot.getIp());
    }
}

/**
 * Setup MQTT connection and callbacks
 */
void setupMQTT() {
    Serial.println("[MQTT] Configuring MQTT...");

    MqttConfig mqttConfig;
    strcpy(mqttConfig.mqtt_server, MQTT_SERVER);
    mqttConfig.mqtt_port = MQTT_PORT;
    mqttConfig.mqtt_useauth = false;
    strcpy(mqttConfig.base_topic, MQTT_BASE_TOPIC);

    espmega.iot.setMqttConfig(mqttConfig);

    // Setup callbacks
    setupCallbacks();

    // Connect
    espmega.iot.connectToMqtt();

    // Wait for connection
    unsigned long startTime = millis();
    while (!espmega.iot.mqttConnected() && (millis() - startTime < 10000)) {
        delay(500);
        Serial.print(".");
        espmega.loop();
    }
    Serial.println();

    if (espmega.iot.mqttConnected()) {
        Serial.println("[MQTT] Connected!");
    }
}

/**
 * Setup MQTT callback handlers
 */
void setupCallbacks() {
    // Register relative callback for topic routing
    espmega.iot.registerRelativeMqttCallback([](char* topic, char* payload) {
        Serial.print("[MQTT RX] ");
        Serial.print(topic);
        Serial.print(" = ");
        Serial.println(payload);

        // Route to appropriate handler based on topic namespace
        if (strncmp(topic, "controls/", 9) == 0) {
            handleControlsTopic(topic + 9, payload);
        }
        else if (strncmp(topic, "config/", 7) == 0) {
            handleConfigTopic(topic + 7, payload);
        }
        else if (strncmp(topic, "sensors/", 8) == 0) {
            handleSensorsTopic(topic + 8, payload);
        }
    });

    // Register subscribe callback
    espmega.iot.registerSubscribeCallback([]() {
        Serial.println("[MQTT] Subscribing to custom topics...");

        // Subscribe to all controls
        espmega.iot.subscribeRelative("controls/#");

        // Subscribe to all config
        espmega.iot.subscribeRelative("config/#");

        // Subscribe to sensor requests
        espmega.iot.subscribeRelative("sensors/request");

        Serial.println("  - controls/#");
        Serial.println("  - config/#");
        Serial.println("  - sensors/request");
    });
}

/**
 * Handle controls/* topics
 */
void handleControlsTopic(char* subtopic, char* payload) {
    Serial.print("[CONTROLS] ");
    Serial.print(subtopic);
    Serial.print(" = ");
    Serial.println(payload);

    if (strcmp(subtopic, "led") == 0) {
        // Control LED
        if (strcmp(payload, "on") == 0 || strcmp(payload, "1") == 0) {
            ledState = true;
            Serial.println("LED turned ON");
        } else if (strcmp(payload, "off") == 0 || strcmp(payload, "0") == 0) {
            ledState = false;
            Serial.println("LED turned OFF");
        } else if (strcmp(payload, "toggle") == 0) {
            ledState = !ledState;
            Serial.println(ledState ? "LED toggled ON" : "LED toggled OFF");
        }

        // Publish new state
        espmega.iot.publishRelative("controls/led/state", ledState ? "on" : "off");
    }
    else if (strcmp(subtopic, "interval") == 0) {
        // Set publish interval
        uint32_t newInterval = atoi(payload);
        if (newInterval >= 1000 && newInterval <= 300000) {  // 1s to 5min
            config.sensorInterval = newInterval;
            Serial.print("Publish interval set to ");
            Serial.print(newInterval);
            Serial.println(" ms");

            char response[50];
            sprintf(response, "%lu", config.sensorInterval);
            espmega.iot.publishRelative("controls/interval/state", response);
        }
    }
}

/**
 * Handle config/* topics
 */
void handleConfigTopic(char* subtopic, char* payload) {
    Serial.print("[CONFIG] ");
    Serial.print(subtopic);
    Serial.print(" = ");
    Serial.println(payload);

    if (strcmp(subtopic, "get") == 0) {
        // Publish current configuration
        publishConfiguration();
    }
    else if (strcmp(subtopic, "set") == 0) {
        // Parse and apply JSON configuration
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            if (doc.containsKey("enableSensors")) {
                config.enableSensors = doc["enableSensors"];
            }
            if (doc.containsKey("enableControls")) {
                config.enableControls = doc["enableControls"];
            }
            if (doc.containsKey("sensorInterval")) {
                config.sensorInterval = doc["sensorInterval"];
            }
            if (doc.containsKey("location")) {
                strcpy(config.location, doc["location"]);
            }

            Serial.println("Configuration updated");
            publishConfiguration();
        } else {
            Serial.print("JSON parse error: ");
            Serial.println(error.c_str());
        }
    }
}

/**
 * Handle sensors/* topics
 */
void handleSensorsTopic(char* subtopic, char* payload) {
    if (strcmp(subtopic, "request") == 0) {
        Serial.println("[SENSORS] Data requested");
        publishSensorData();
    }
}

/**
 * Publish sensor data
 */
void publishSensorData() {
    char buffer[20];

    // Individual values
    sprintf(buffer, "%.1f", temperature);
    espmega.iot.publishRelative("sensors/temperature", buffer);

    sprintf(buffer, "%.1f", humidity);
    espmega.iot.publishRelative("sensors/humidity", buffer);

    sprintf(buffer, "%lu", pressure);
    espmega.iot.publishRelative("sensors/pressure", buffer);

    // Combined JSON
    StaticJsonDocument<256> doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["pressure"] = pressure;
    doc["location"] = config.location;
    doc["timestamp"] = millis() / 1000;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    espmega.iot.publishRelative("sensors/all", jsonBuffer);

    Serial.println("[SENSORS] Data published");
}

/**
 * Publish system status
 */
void publishSystemStatus() {
    StaticJsonDocument<512> doc;

    // System info
    doc["uptime"] = millis() / 1000;
    doc["ip"] = espmega.iot.getIp().toString();
    doc["mac"] = espmega.iot.getMac();
    doc["hostname"] = hostname;

    // Status
    doc["network_connected"] = espmega.iot.networkConnected();
    doc["mqtt_connected"] = espmega.iot.mqttConnected();

    // Configuration
    JsonObject cfg = doc.createNestedObject("config");
    cfg["enableSensors"] = config.enableSensors;
    cfg["enableControls"] = config.enableControls;
    cfg["sensorInterval"] = config.sensorInterval;
    cfg["location"] = config.location;

    // Current values
    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["temperature"] = temperature;
    sensors["humidity"] = humidity;
    sensors["pressure"] = pressure;

    JsonObject controls = doc.createNestedObject("controls");
    controls["led"] = ledState;

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    espmega.iot.publishRelative("status/system", jsonBuffer);

    Serial.println("[STATUS] System status published");
}

/**
 * Publish configuration
 */
void publishConfiguration() {
    StaticJsonDocument<256> doc;
    doc["enableSensors"] = config.enableSensors;
    doc["enableControls"] = config.enableControls;
    doc["sensorInterval"] = config.sensorInterval;
    doc["location"] = config.location;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    espmega.iot.publishRelative("config/current", jsonBuffer);

    Serial.println("[CONFIG] Configuration published");
}

/**
 * Simulate sensor value changes
 */
void updateSimulatedSensors() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) {  // Update every 2 seconds
        lastUpdate = millis();

        // Simulate temperature variation
        temperature += (random(-10, 11) / 10.0);
        temperature = constrain(temperature, 20.0, 30.0);

        // Simulate humidity variation
        humidity += (random(-5, 6) / 10.0);
        humidity = constrain(humidity, 30.0, 70.0);

        // Simulate pressure variation
        pressure += random(-2, 3);
        pressure = constrain(pressure, 1000, 1030);
    }
}

/**
 * Print topic map
 */
void printTopicMap() {
    Serial.println();
    Serial.println("Topic Map:");
    Serial.println("==========");
    Serial.println();
    Serial.println("SENSORS (Publish):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/sensors/temperature");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/sensors/humidity");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/sensors/pressure");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/sensors/all (JSON)");
    Serial.println();
    Serial.println("SENSORS (Subscribe):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/sensors/request");
    Serial.println();
    Serial.println("CONTROLS (Subscribe):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/controls/led [on|off|toggle]");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/controls/interval [milliseconds]");
    Serial.println();
    Serial.println("CONTROLS (Publish):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/controls/led/state");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/controls/interval/state");
    Serial.println();
    Serial.println("CONFIG (Subscribe):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/config/get");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/config/set (JSON)");
    Serial.println();
    Serial.println("CONFIG (Publish):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/config/current (JSON)");
    Serial.println();
    Serial.println("STATUS (Publish):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/status/system (JSON)");
    Serial.println();
}

/**
 * Example MQTT Commands:
 *
 * Request sensor data:
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/sensors/request" -m ""
 *
 * Control LED:
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/controls/led" -m "on"
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/controls/led" -m "off"
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/controls/led" -m "toggle"
 *
 * Set interval:
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/controls/interval" -m "5000"
 *
 * Get config:
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/config/get" -m ""
 *
 * Set config:
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/config/set" -m '{"enableSensors":true,"sensorInterval":15000,"location":"Bedroom"}'
 *
 * Subscribe to all:
 *   mosquitto_sub -h 192.168.1.5 -t "home/espmega/#"
 */
