/**
 * ESPMegaPRO Home Assistant Integration Example
 *
 * This example demonstrates how to integrate ESPMegaPRO with Home Assistant
 * using MQTT discovery and standard Home Assistant conventions.
 *
 * Features demonstrated:
 * - Home Assistant MQTT Discovery
 * - Binary sensors (digital inputs)
 * - Switches (digital outputs)
 * - Sensors (temperature, humidity, etc.)
 * - Availability tracking
 * - State persistence
 * - Proper Home Assistant topic structure
 *
 * Topic Structure (Home Assistant Convention):
 *   homeassistant/{component}/{node_id}/{object_id}/config
 *   homeassistant/{component}/{node_id}/{object_id}/state
 *   homeassistant/{component}/{node_id}/{object_id}/command
 *
 * Hardware Requirements:
 * - ESPMegaPRO R3 board
 * - Network connection
 * - Home Assistant with MQTT integration
 * - MQTT broker (e.g., Mosquitto)
 *
 * Setup:
 * 1. Install Mosquitto MQTT broker in Home Assistant
 * 2. Configure MQTT integration in Home Assistant
 * 3. Update MQTT_SERVER to your Home Assistant IP
 * 4. Upload this sketch
 * 5. Devices will auto-discover in Home Assistant
 *
 * Created: 2024
 * Author: SIWAT SYSTEM
 */

#include <ESPMegaPRO.h>
#include <ETH.h>
#include <ArduinoJson.h>

// Create ESPMegaPRO instance
ESPMegaPRO espmega;

// ===== HOME ASSISTANT CONFIGURATION =====
const char* HA_MQTT_SERVER = "192.168.1.100";      // Home Assistant IP
const uint16_t HA_MQTT_PORT = 1883;
const char* HA_MQTT_USER = "homeassistant";        // MQTT username
const char* HA_MQTT_PASSWORD = "your_password";    // MQTT password
const bool HA_USE_AUTH = true;

// Device identification (appears in Home Assistant)
const char* DEVICE_NAME = "ESPMega Living Room";
const char* DEVICE_ID = "espmega_living_room";     // Unique ID (no spaces)
const char* DEVICE_MANUFACTURER = "SIWAT SYSTEM";
const char* DEVICE_MODEL = "ESPMegaPRO R3";

// Home Assistant Discovery Prefix
const char* HA_DISCOVERY_PREFIX = "homeassistant";

// Base topic for state/command messages
char base_topic[64];

// ===== NETWORK CONFIGURATION =====
const char* hostname = "espmega-ha-01";
const IPAddress static_ip(192, 168, 1, 110);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns1(1, 1, 1, 1);

// ===== SIMULATED SENSORS =====
struct SensorData {
    float temperature;
    float humidity;
    uint32_t pressure;
    bool motion;
    bool door;
    float light;
} sensors = {
    23.5,   // temperature
    45.2,   // humidity
    1013,   // pressure
    false,  // motion
    false,  // door
    350.0   // light
};

// ===== ACTUATORS =====
bool relay1_state = false;
bool relay2_state = false;
bool led_state = false;

// Timing
unsigned long lastSensorPublish = 0;
const unsigned long SENSOR_PUBLISH_INTERVAL = 30000;  // 30 seconds
bool discoveryPublished = false;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("========================================");
    Serial.println("ESPMegaPRO Home Assistant Integration");
    Serial.println("========================================");
    Serial.println();

    // Initialize ESPMegaPRO
    espmega.begin();

    // Setup base topic
    sprintf(base_topic, "espmega/%s", DEVICE_ID);

    // Setup network
    setupNetwork();

    // Setup MQTT
    setupMQTT();

    Serial.println();
    Serial.println("========================================");
    Serial.println("Setup Complete");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Check Home Assistant for new devices!");
    Serial.println("They should appear automatically under:");
    Serial.println("Settings -> Devices & Services -> MQTT");
    Serial.println();
}

void loop() {
    espmega.loop();

    // Publish Home Assistant discovery on first MQTT connection
    if (espmega.iot.mqttConnected() && !discoveryPublished) {
        publishHomeAssistantDiscovery();
        discoveryPublished = true;

        // Publish initial states
        publishAllStates();
    }

    // Publish sensor data periodically
    if (millis() - lastSensorPublish > SENSOR_PUBLISH_INTERVAL) {
        lastSensorPublish = millis();

        if (espmega.iot.mqttConnected()) {
            publishSensorStates();
        }
    }

    // Simulate sensor changes
    updateSimulatedSensors();
}

/**
 * Setup network connection
 */
void setupNetwork() {
    Serial.println("[NETWORK] Configuring...");

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
 * Setup MQTT connection
 */
void setupMQTT() {
    Serial.println("[MQTT] Configuring...");

    MqttConfig mqttConfig;
    strcpy(mqttConfig.mqtt_server, HA_MQTT_SERVER);
    mqttConfig.mqtt_port = HA_MQTT_PORT;
    mqttConfig.mqtt_useauth = HA_USE_AUTH;

    if (HA_USE_AUTH) {
        strcpy(mqttConfig.mqtt_user, HA_MQTT_USER);
        strcpy(mqttConfig.mqtt_password, HA_MQTT_PASSWORD);
    }

    strcpy(mqttConfig.base_topic, base_topic);
    espmega.iot.setMqttConfig(mqttConfig);

    // Setup callbacks
    setupCallbacks();

    // Connect
    espmega.iot.connectToMqtt();

    unsigned long startTime = millis();
    while (!espmega.iot.mqttConnected() && (millis() - startTime < 10000)) {
        delay(500);
        Serial.print(".");
        espmega.loop();
    }
    Serial.println();

    if (espmega.iot.mqttConnected()) {
        Serial.println("[MQTT] Connected to Home Assistant!");
    }
}

/**
 * Setup MQTT callbacks
 */
void setupCallbacks() {
    // Register absolute callback for Home Assistant commands
    espmega.iot.registerMqttCallback([](char* topic, char* payload) {
        // Handle switch commands
        if (strstr(topic, "/switch/relay1/command")) {
            relay1_state = (strcmp(payload, "ON") == 0);
            Serial.print("[HA] Relay 1: ");
            Serial.println(relay1_state ? "ON" : "OFF");
            publishSwitchState("relay1", relay1_state);
        }
        else if (strstr(topic, "/switch/relay2/command")) {
            relay2_state = (strcmp(payload, "ON") == 0);
            Serial.print("[HA] Relay 2: ");
            Serial.println(relay2_state ? "ON" : "OFF");
            publishSwitchState("relay2", relay2_state);
        }
        else if (strstr(topic, "/light/led/command")) {
            led_state = (strcmp(payload, "ON") == 0);
            Serial.print("[HA] LED: ");
            Serial.println(led_state ? "ON" : "OFF");
            publishLightState("led", led_state);
        }
    });

    // Subscribe to command topics
    espmega.iot.registerSubscribeCallback([]() {
        Serial.println("[MQTT] Subscribing to Home Assistant topics...");
    });
}

/**
 * Publish Home Assistant MQTT Discovery messages
 */
void publishHomeAssistantDiscovery() {
    Serial.println("[HA] Publishing discovery messages...");

    // Device information (shared across all entities)
    StaticJsonDocument<512> deviceDoc;
    JsonObject device = deviceDoc.createNestedObject("device");
    device["name"] = DEVICE_NAME;
    device["identifiers"][0] = DEVICE_ID;
    device["manufacturer"] = DEVICE_MANUFACTURER;
    device["model"] = DEVICE_MODEL;
    device["sw_version"] = "1.0.0";

    // Availability
    char avail_topic[128];
    sprintf(avail_topic, "%s/availability", base_topic);

    // Publish discovery for each entity
    publishSensorDiscovery("temperature", "Temperature", "°C", "temperature", deviceDoc);
    publishSensorDiscovery("humidity", "Humidity", "%", "humidity", deviceDoc);
    publishSensorDiscovery("pressure", "Pressure", "hPa", "pressure", deviceDoc);
    publishSensorDiscovery("light", "Light Level", "lx", "illuminance", deviceDoc);

    publishBinarySensorDiscovery("motion", "Motion", "motion", deviceDoc);
    publishBinarySensorDiscovery("door", "Door", "door", deviceDoc);

    publishSwitchDiscovery("relay1", "Relay 1", deviceDoc);
    publishSwitchDiscovery("relay2", "Relay 2", deviceDoc);

    publishLightDiscovery("led", "LED", deviceDoc);

    // Publish availability as online
    espmega.iot.publish(avail_topic, "online");

    Serial.println("[HA] Discovery complete!");
}

/**
 * Publish sensor discovery
 */
void publishSensorDiscovery(const char* id, const char* name, const char* unit,
                            const char* device_class, StaticJsonDocument<512>& deviceDoc) {
    StaticJsonDocument<768> doc;

    // Copy device info
    doc["device"] = deviceDoc["device"];

    // Entity config
    doc["name"] = name;
    doc["unique_id"] = String(DEVICE_ID) + "_" + id;
    doc["device_class"] = device_class;
    doc["unit_of_measurement"] = unit;

    char state_topic[128];
    sprintf(state_topic, "%s/sensor/%s/state", base_topic, id);
    doc["state_topic"] = state_topic;

    char avail_topic[128];
    sprintf(avail_topic, "%s/availability", base_topic);
    doc["availability_topic"] = avail_topic;

    // Publish discovery
    char config_topic[128];
    sprintf(config_topic, "%s/sensor/%s/%s/config", HA_DISCOVERY_PREFIX, DEVICE_ID, id);

    char buffer[768];
    serializeJson(doc, buffer);
    espmega.iot.publish(config_topic, buffer);

    Serial.print("  - Sensor: ");
    Serial.println(name);
}

/**
 * Publish binary sensor discovery
 */
void publishBinarySensorDiscovery(const char* id, const char* name,
                                  const char* device_class, StaticJsonDocument<512>& deviceDoc) {
    StaticJsonDocument<768> doc;

    doc["device"] = deviceDoc["device"];
    doc["name"] = name;
    doc["unique_id"] = String(DEVICE_ID) + "_" + id;
    doc["device_class"] = device_class;

    char state_topic[128];
    sprintf(state_topic, "%s/binary_sensor/%s/state", base_topic, id);
    doc["state_topic"] = state_topic;

    char avail_topic[128];
    sprintf(avail_topic, "%s/availability", base_topic);
    doc["availability_topic"] = avail_topic;

    char config_topic[128];
    sprintf(config_topic, "%s/binary_sensor/%s/%s/config", HA_DISCOVERY_PREFIX, DEVICE_ID, id);

    char buffer[768];
    serializeJson(doc, buffer);
    espmega.iot.publish(config_topic, buffer);

    Serial.print("  - Binary Sensor: ");
    Serial.println(name);
}

/**
 * Publish switch discovery
 */
void publishSwitchDiscovery(const char* id, const char* name, StaticJsonDocument<512>& deviceDoc) {
    StaticJsonDocument<768> doc;

    doc["device"] = deviceDoc["device"];
    doc["name"] = name;
    doc["unique_id"] = String(DEVICE_ID) + "_" + id;

    char state_topic[128];
    sprintf(state_topic, "%s/switch/%s/state", base_topic, id);
    doc["state_topic"] = state_topic;

    char command_topic[128];
    sprintf(command_topic, "%s/switch/%s/command", base_topic, id);
    doc["command_topic"] = command_topic;

    char avail_topic[128];
    sprintf(avail_topic, "%s/availability", base_topic);
    doc["availability_topic"] = avail_topic;

    char config_topic[128];
    sprintf(config_topic, "%s/switch/%s/%s/config", HA_DISCOVERY_PREFIX, DEVICE_ID, id);

    char buffer[768];
    serializeJson(doc, buffer);
    espmega.iot.publish(config_topic, buffer);

    // Subscribe to command topic
    espmega.iot.subscribe(command_topic);

    Serial.print("  - Switch: ");
    Serial.println(name);
}

/**
 * Publish light discovery
 */
void publishLightDiscovery(const char* id, const char* name, StaticJsonDocument<512>& deviceDoc) {
    StaticJsonDocument<768> doc;

    doc["device"] = deviceDoc["device"];
    doc["name"] = name;
    doc["unique_id"] = String(DEVICE_ID) + "_" + id;
    doc["schema"] = "basic";

    char state_topic[128];
    sprintf(state_topic, "%s/light/%s/state", base_topic, id);
    doc["state_topic"] = state_topic;

    char command_topic[128];
    sprintf(command_topic, "%s/light/%s/command", base_topic, id);
    doc["command_topic"] = command_topic;

    char avail_topic[128];
    sprintf(avail_topic, "%s/availability", base_topic);
    doc["availability_topic"] = avail_topic;

    char config_topic[128];
    sprintf(config_topic, "%s/light/%s/%s/config", HA_DISCOVERY_PREFIX, DEVICE_ID, id);

    char buffer[768];
    serializeJson(doc, buffer);
    espmega.iot.publish(config_topic, buffer);

    espmega.iot.subscribe(command_topic);

    Serial.print("  - Light: ");
    Serial.println(name);
}

/**
 * Publish all states
 */
void publishAllStates() {
    publishSensorStates();
    publishBinarySensorStates();
    publishSwitchState("relay1", relay1_state);
    publishSwitchState("relay2", relay2_state);
    publishLightState("led", led_state);
}

/**
 * Publish sensor states
 */
void publishSensorStates() {
    char topic[128];
    char value[32];

    // Temperature
    sprintf(topic, "%s/sensor/temperature/state", base_topic);
    sprintf(value, "%.1f", sensors.temperature);
    espmega.iot.publish(topic, value);

    // Humidity
    sprintf(topic, "%s/sensor/humidity/state", base_topic);
    sprintf(value, "%.1f", sensors.humidity);
    espmega.iot.publish(topic, value);

    // Pressure
    sprintf(topic, "%s/sensor/pressure/state", base_topic);
    sprintf(value, "%lu", sensors.pressure);
    espmega.iot.publish(topic, value);

    // Light
    sprintf(topic, "%s/sensor/light/state", base_topic);
    sprintf(value, "%.1f", sensors.light);
    espmega.iot.publish(topic, value);

    Serial.println("[HA] Sensor states published");
}

/**
 * Publish binary sensor states
 */
void publishBinarySensorStates() {
    char topic[128];

    // Motion
    sprintf(topic, "%s/binary_sensor/motion/state", base_topic);
    espmega.iot.publish(topic, sensors.motion ? "ON" : "OFF");

    // Door
    sprintf(topic, "%s/binary_sensor/door/state", base_topic);
    espmega.iot.publish(topic, sensors.door ? "ON" : "OFF");
}

/**
 * Publish switch state
 */
void publishSwitchState(const char* id, bool state) {
    char topic[128];
    sprintf(topic, "%s/switch/%s/state", base_topic, id);
    espmega.iot.publish(topic, state ? "ON" : "OFF");
}

/**
 * Publish light state
 */
void publishLightState(const char* id, bool state) {
    char topic[128];
    sprintf(topic, "%s/light/%s/state", base_topic, id);
    espmega.iot.publish(topic, state ? "ON" : "OFF");
}

/**
 * Update simulated sensors
 */
void updateSimulatedSensors() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();

        // Temperature variation
        sensors.temperature += (random(-10, 11) / 10.0);
        sensors.temperature = constrain(sensors.temperature, 18.0, 28.0);

        // Humidity variation
        sensors.humidity += (random(-5, 6) / 10.0);
        sensors.humidity = constrain(sensors.humidity, 30.0, 70.0);

        // Pressure variation
        sensors.pressure += random(-2, 3);
        sensors.pressure = constrain(sensors.pressure, 1000, 1030);

        // Light variation
        sensors.light += (random(-50, 51) / 10.0);
        sensors.light = constrain(sensors.light, 0.0, 1000.0);

        // Random motion events
        if (random(0, 10) > 7) {
            bool oldMotion = sensors.motion;
            sensors.motion = random(0, 2);
            if (oldMotion != sensors.motion && espmega.iot.mqttConnected()) {
                publishBinarySensorStates();
            }
        }

        // Random door events
        if (random(0, 20) > 18) {
            bool oldDoor = sensors.door;
            sensors.door = random(0, 2);
            if (oldDoor != sensors.door && espmega.iot.mqttConnected()) {
                publishBinarySensorStates();
            }
        }
    }
}

/**
 * Home Assistant Integration Notes:
 *
 * After uploading this sketch:
 * 1. Devices will auto-discover in Home Assistant
 * 2. Go to: Settings -> Devices & Services -> MQTT
 * 3. Click on "MQTT" and you should see "ESPMega Living Room"
 * 4. All sensors, switches, and lights will be available
 *
 * Entities created:
 * - sensor.espmega_living_room_temperature
 * - sensor.espmega_living_room_humidity
 * - sensor.espmega_living_room_pressure
 * - sensor.espmega_living_room_light_level
 * - binary_sensor.espmega_living_room_motion
 * - binary_sensor.espmega_living_room_door
 * - switch.espmega_living_room_relay_1
 * - switch.espmega_living_room_relay_2
 * - light.espmega_living_room_led
 *
 * You can now use these in automations, dashboards, etc.!
 */
