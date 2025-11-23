/**
 * ESPMegaPRO Node-RED Integration Example
 *
 * This example demonstrates integration with Node-RED using MQTT.
 * It provides a comprehensive IoT interface suitable for Node-RED dashboards.
 *
 * Features demonstrated:
 * - JSON-based communication
 * - RESTful-like MQTT command structure
 * - Real-time sensor data streaming
 * - Control outputs via MQTT
 * - Status reporting
 * - System information
 * - Alarm/notification system
 * - Data logging format
 *
 * Topic Structure:
 *   {base}/sensors/data (JSON - all sensor data)
 *   {base}/sensors/{type} (individual sensor values)
 *   {base}/outputs/state (JSON - all output states)
 *   {base}/outputs/{id}/set (control individual output)
 *   {base}/status/system (JSON - system status)
 *   {base}/control/command (execute commands)
 *   {base}/alerts/{type} (alert notifications)
 *
 * Hardware Requirements:
 * - ESPMegaPRO R3 board
 * - Network connection
 * - MQTT broker
 * - Node-RED with MQTT nodes
 *
 * Node-RED Setup:
 * 1. Install MQTT broker (Mosquitto)
 * 2. Add MQTT-in and MQTT-out nodes
 * 3. Configure broker connection
 * 4. Import example flow (see end of file)
 *
 * Created: 2024
 * Author: SIWAT SYSTEM
 */

#include <ESPMegaPRO.h>
#include <ETH.h>
#include <ArduinoJson.h>

// Create ESPMegaPRO instance
ESPMegaPRO espmega;

// ===== NODE-RED MQTT CONFIGURATION =====
const char* MQTT_SERVER = "192.168.1.5";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_BASE_TOPIC = "nodered/espmega";

// ===== NETWORK CONFIGURATION =====
const char* hostname = "espmega-nodered-01";
const IPAddress static_ip(192, 168, 1, 111);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns1(1, 1, 1, 1);

// ===== DATA STRUCTURES =====
struct Sensors {
    float temperature;
    float humidity;
    float pressure;
    float voltage;
    float current;
    float power;
    bool motion;
    uint16_t light;
} sensors = {
    23.5,   // temperature (°C)
    45.2,   // humidity (%)
    1013.2, // pressure (hPa)
    230.0,  // voltage (V)
    1.5,    // current (A)
    345.0,  // power (W)
    false,  // motion detected
    450     // light level (lux)
};

struct Outputs {
    bool relay1;
    bool relay2;
    bool relay3;
    bool relay4;
    uint8_t dimmer1;  // 0-100%
    uint8_t dimmer2;  // 0-100%
} outputs = {
    false, false, false, false,  // All relays off
    0, 0                         // All dimmers off
};

struct SystemStatus {
    bool networkConnected;
    bool mqttConnected;
    unsigned long uptime;
    uint32_t freeHeap;
    int8_t rssi;  // WiFi signal strength (if using WiFi)
    float cpuTemp;
} systemStatus;

struct Alerts {
    bool temperatureHigh;
    bool temperatureLow;
    bool motionDetected;
    bool powerOverload;
} alerts = {false, false, false, false};

// ===== CONFIGURATION =====
struct Config {
    bool enableSensorStreaming;
    uint32_t sensorStreamInterval;
    float tempHighThreshold;
    float tempLowThreshold;
    float powerThreshold;
} config = {
    true,    // enableSensorStreaming
    5000,    // sensorStreamInterval (5 seconds)
    28.0,    // tempHighThreshold
    18.0,    // tempLowThreshold
    3000.0   // powerThreshold (watts)
};

// Timing
unsigned long lastSensorStream = 0;
unsigned long lastStatusUpdate = 0;
unsigned long lastHeartbeat = 0;
const unsigned long STATUS_INTERVAL = 10000;    // 10 seconds
const unsigned long HEARTBEAT_INTERVAL = 60000; // 60 seconds

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("========================================");
    Serial.println("ESPMegaPRO Node-RED Integration");
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
    Serial.println("Setup Complete - Ready for Node-RED");
    Serial.println("========================================");
    Serial.println();
    printTopicGuide();
}

void loop() {
    espmega.loop();

    // Update system status
    updateSystemStatus();

    // Stream sensor data
    if (millis() - lastSensorStream > config.sensorStreamInterval) {
        lastSensorStream = millis();

        if (espmega.iot.mqttConnected() && config.enableSensorStreaming) {
            streamSensorData();
        }
    }

    // Publish system status
    if (millis() - lastStatusUpdate > STATUS_INTERVAL) {
        lastStatusUpdate = millis();

        if (espmega.iot.mqttConnected()) {
            publishSystemStatus();
        }
    }

    // Heartbeat
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
        lastHeartbeat = millis();

        if (espmega.iot.mqttConnected()) {
            publishHeartbeat();
        }
    }

    // Check for alerts
    checkAlerts();

    // Simulate sensor changes
    updateSimulatedSensors();
}

/**
 * Setup network
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
 * Setup MQTT
 */
void setupMQTT() {
    Serial.println("[MQTT] Configuring...");

    MqttConfig mqttConfig;
    strcpy(mqttConfig.mqtt_server, MQTT_SERVER);
    mqttConfig.mqtt_port = MQTT_PORT;
    mqttConfig.mqtt_useauth = false;
    strcpy(mqttConfig.base_topic, MQTT_BASE_TOPIC);

    espmega.iot.setMqttConfig(mqttConfig);

    setupCallbacks();

    espmega.iot.connectToMqtt();

    unsigned long startTime = millis();
    while (!espmega.iot.mqttConnected() && (millis() - startTime < 10000)) {
        delay(500);
        Serial.print(".");
        espmega.loop();
    }
    Serial.println();

    if (espmega.iot.mqttConnected()) {
        Serial.println("[MQTT] Connected to broker!");

        // Publish initial data
        publishOutputStates();
        publishSystemStatus();
    }
}

/**
 * Setup MQTT callbacks
 */
void setupCallbacks() {
    espmega.iot.registerRelativeMqttCallback([](char* topic, char* payload) {
        Serial.print("[RX] ");
        Serial.print(topic);
        Serial.print(" = ");
        Serial.println(payload);

        // Route commands
        if (strncmp(topic, "outputs/", 8) == 0) {
            handleOutputCommand(topic + 8, payload);
        }
        else if (strcmp(topic, "control/command") == 0) {
            handleControlCommand(payload);
        }
        else if (strcmp(topic, "config/set") == 0) {
            handleConfigUpdate(payload);
        }
    });

    espmega.iot.registerSubscribeCallback([]() {
        Serial.println("[MQTT] Subscribing to Node-RED topics...");

        // Subscribe to all output controls
        espmega.iot.subscribeRelative("outputs/+/set");

        // Subscribe to control commands
        espmega.iot.subscribeRelative("control/command");

        // Subscribe to config updates
        espmega.iot.subscribeRelative("config/set");

        Serial.println("  - outputs/+/set");
        Serial.println("  - control/command");
        Serial.println("  - config/set");
    });
}

/**
 * Stream sensor data (JSON)
 */
void streamSensorData() {
    StaticJsonDocument<512> doc;

    doc["timestamp"] = millis() / 1000;

    JsonObject env = doc.createNestedObject("environment");
    env["temperature"] = sensors.temperature;
    env["humidity"] = sensors.humidity;
    env["pressure"] = sensors.pressure;
    env["light"] = sensors.light;

    JsonObject electrical = doc.createNestedObject("electrical");
    electrical["voltage"] = sensors.voltage;
    electrical["current"] = sensors.current;
    electrical["power"] = sensors.power;

    JsonObject binary = doc.createNestedObject("binary");
    binary["motion"] = sensors.motion;

    char buffer[512];
    serializeJson(doc, buffer);
    espmega.iot.publishRelative("sensors/data", buffer);

    // Also publish individual values for easier charting
    char value[32];
    sprintf(value, "%.1f", sensors.temperature);
    espmega.iot.publishRelative("sensors/temperature", value);

    sprintf(value, "%.1f", sensors.humidity);
    espmega.iot.publishRelative("sensors/humidity", value);

    sprintf(value, "%.1f", sensors.power);
    espmega.iot.publishRelative("sensors/power", value);
}

/**
 * Publish output states
 */
void publishOutputStates() {
    StaticJsonDocument<256> doc;

    JsonObject relays = doc.createNestedObject("relays");
    relays["relay1"] = outputs.relay1;
    relays["relay2"] = outputs.relay2;
    relays["relay3"] = outputs.relay3;
    relays["relay4"] = outputs.relay4;

    JsonObject dimmers = doc.createNestedObject("dimmers");
    dimmers["dimmer1"] = outputs.dimmer1;
    dimmers["dimmer2"] = outputs.dimmer2;

    char buffer[256];
    serializeJson(doc, buffer);
    espmega.iot.publishRelative("outputs/state", buffer);
}

/**
 * Publish system status
 */
void publishSystemStatus() {
    StaticJsonDocument<512> doc;

    doc["uptime"] = systemStatus.uptime;
    doc["freeHeap"] = systemStatus.freeHeap;
    doc["networkConnected"] = systemStatus.networkConnected;
    doc["mqttConnected"] = systemStatus.mqttConnected;

    doc["ip"] = espmega.iot.getIp().toString();
    doc["mac"] = espmega.iot.getMac();
    doc["hostname"] = hostname;

    JsonObject cfg = doc.createNestedObject("config");
    cfg["sensorStreamInterval"] = config.sensorStreamInterval;
    cfg["enableSensorStreaming"] = config.enableSensorStreaming;

    char buffer[512];
    serializeJson(doc, buffer);
    espmega.iot.publishRelative("status/system", buffer);
}

/**
 * Publish heartbeat
 */
void publishHeartbeat() {
    char buffer[32];
    sprintf(buffer, "%lu", millis() / 1000);
    espmega.iot.publishRelative("status/heartbeat", buffer);
}

/**
 * Handle output commands
 */
void handleOutputCommand(char* output, char* payload) {
    if (strcmp(output, "relay1/set") == 0) {
        outputs.relay1 = (strcmp(payload, "1") == 0 || strcmp(payload, "ON") == 0);
        Serial.print("Relay 1: ");
        Serial.println(outputs.relay1 ? "ON" : "OFF");
    }
    else if (strcmp(output, "relay2/set") == 0) {
        outputs.relay2 = (strcmp(payload, "1") == 0 || strcmp(payload, "ON") == 0);
        Serial.print("Relay 2: ");
        Serial.println(outputs.relay2 ? "ON" : "OFF");
    }
    else if (strcmp(output, "relay3/set") == 0) {
        outputs.relay3 = (strcmp(payload, "1") == 0 || strcmp(payload, "ON") == 0);
    }
    else if (strcmp(output, "relay4/set") == 0) {
        outputs.relay4 = (strcmp(payload, "1") == 0 || strcmp(payload, "ON") == 0);
    }
    else if (strcmp(output, "dimmer1/set") == 0) {
        outputs.dimmer1 = constrain(atoi(payload), 0, 100);
        Serial.print("Dimmer 1: ");
        Serial.print(outputs.dimmer1);
        Serial.println("%");
    }
    else if (strcmp(output, "dimmer2/set") == 0) {
        outputs.dimmer2 = constrain(atoi(payload), 0, 100);
    }

    // Publish updated states
    publishOutputStates();
}

/**
 * Handle control commands
 */
void handleControlCommand(char* payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        // Try simple string commands
        if (strcmp(payload, "reset") == 0) {
            ESP.restart();
        }
        else if (strcmp(payload, "status") == 0) {
            publishSystemStatus();
        }
        else if (strcmp(payload, "outputs") == 0) {
            publishOutputStates();
        }
        return;
    }

    // JSON command
    const char* cmd = doc["command"];

    if (strcmp(cmd, "reset_all") == 0) {
        outputs.relay1 = outputs.relay2 = outputs.relay3 = outputs.relay4 = false;
        outputs.dimmer1 = outputs.dimmer2 = 0;
        publishOutputStates();
    }
    else if (strcmp(cmd, "test_alerts") == 0) {
        publishAlert("test", "Test alert triggered", 2);
    }
}

/**
 * Handle config updates
 */
void handleConfigUpdate(char* payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
        if (doc.containsKey("enableSensorStreaming")) {
            config.enableSensorStreaming = doc["enableSensorStreaming"];
        }
        if (doc.containsKey("sensorStreamInterval")) {
            config.sensorStreamInterval = doc["sensorStreamInterval"];
        }
        if (doc.containsKey("tempHighThreshold")) {
            config.tempHighThreshold = doc["tempHighThreshold"];
        }
        if (doc.containsKey("tempLowThreshold")) {
            config.tempLowThreshold = doc["tempLowThreshold"];
        }

        Serial.println("Configuration updated");
        publishSystemStatus();
    }
}

/**
 * Check and publish alerts
 */
void checkAlerts() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 5000) return;
    lastCheck = millis();

    // Temperature high alert
    if (sensors.temperature > config.tempHighThreshold && !alerts.temperatureHigh) {
        alerts.temperatureHigh = true;
        publishAlert("temperature_high", "Temperature exceeded threshold", 3);
    } else if (sensors.temperature <= config.tempHighThreshold) {
        alerts.temperatureHigh = false;
    }

    // Temperature low alert
    if (sensors.temperature < config.tempLowThreshold && !alerts.temperatureLow) {
        alerts.temperatureLow = true;
        publishAlert("temperature_low", "Temperature below threshold", 2);
    } else if (sensors.temperature >= config.tempLowThreshold) {
        alerts.temperatureLow = false;
    }

    // Power overload
    if (sensors.power > config.powerThreshold && !alerts.powerOverload) {
        alerts.powerOverload = true;
        publishAlert("power_overload", "Power consumption too high", 4);
    } else if (sensors.power <= config.powerThreshold) {
        alerts.powerOverload = false;
    }

    // Motion detected
    if (sensors.motion && !alerts.motionDetected) {
        alerts.motionDetected = true;
        publishAlert("motion", "Motion detected", 1);
    } else if (!sensors.motion) {
        alerts.motionDetected = false;
    }
}

/**
 * Publish alert
 */
void publishAlert(const char* type, const char* message, uint8_t severity) {
    StaticJsonDocument<256> doc;
    doc["type"] = type;
    doc["message"] = message;
    doc["severity"] = severity;  // 1=info, 2=warning, 3=error, 4=critical
    doc["timestamp"] = millis() / 1000;

    char buffer[256];
    serializeJson(doc, buffer);

    char topic[64];
    sprintf(topic, "alerts/%s", type);
    espmega.iot.publishRelative(topic, buffer);

    Serial.print("[ALERT] ");
    Serial.print(type);
    Serial.print(": ");
    Serial.println(message);
}

/**
 * Update system status
 */
void updateSystemStatus() {
    systemStatus.networkConnected = espmega.iot.networkConnected();
    systemStatus.mqttConnected = espmega.iot.mqttConnected();
    systemStatus.uptime = millis() / 1000;
    systemStatus.freeHeap = ESP.getFreeHeap();
}

/**
 * Update simulated sensors
 */
void updateSimulatedSensors() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) {
        lastUpdate = millis();

        sensors.temperature += (random(-10, 11) / 10.0);
        sensors.temperature = constrain(sensors.temperature, 15.0, 35.0);

        sensors.humidity += (random(-5, 6) / 10.0);
        sensors.humidity = constrain(sensors.humidity, 20.0, 80.0);

        sensors.pressure += (random(-5, 6) / 10.0);
        sensors.pressure = constrain(sensors.pressure, 990.0, 1030.0);

        sensors.voltage += (random(-2, 3) / 10.0);
        sensors.voltage = constrain(sensors.voltage, 220.0, 240.0);

        sensors.current += (random(-10, 11) / 100.0);
        sensors.current = constrain(sensors.current, 0.0, 5.0);

        sensors.power = sensors.voltage * sensors.current;

        sensors.light = constrain(sensors.light + random(-50, 51), 0, 1000);

        if (random(0, 20) > 18) {
            sensors.motion = random(0, 2);
        }
    }
}

/**
 * Print topic guide
 */
void printTopicGuide() {
    Serial.println("Node-RED Topic Guide:");
    Serial.println("====================");
    Serial.println();
    Serial.println("SUBSCRIBE (Node-RED receives):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/sensors/data (JSON)");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/sensors/{type}");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/outputs/state (JSON)");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/status/system (JSON)");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/status/heartbeat");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/alerts/{type} (JSON)");
    Serial.println();
    Serial.println("PUBLISH (Node-RED sends):");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/outputs/{id}/set");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/control/command");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/config/set (JSON)");
    Serial.println();
}

/**
 * Example Node-RED Flow (JSON):
 *
 * Import this into Node-RED to get started:
 *
 * [{"id":"mqtt_in","type":"mqtt in","topic":"nodered/espmega/#","broker":"mqtt_broker"},
 *  {"id":"mqtt_out","type":"mqtt out","topic":"","broker":"mqtt_broker"},
 *  {"id":"json_parse","type":"json","action":"obj"},
 *  {"id":"dashboard_gauge","type":"ui_gauge","group":"sensors","min":0,"max":100},
 *  {"id":"dashboard_switch","type":"ui_switch","group":"controls","topic":"nodered/espmega/outputs/relay1/set"}]
 *
 * Broker config: mqtt://192.168.1.5:1883
 */
