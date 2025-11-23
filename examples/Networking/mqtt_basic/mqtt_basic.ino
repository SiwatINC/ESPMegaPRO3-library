/**
 * ESPMegaPRO MQTT Basic Example
 *
 * This example demonstrates basic MQTT connectivity with the ESPMegaPRO.
 * It shows how to connect to an MQTT broker and publish/subscribe to topics.
 *
 * Features demonstrated:
 * - MQTT connection with and without authentication
 * - Publishing messages to topics
 * - Subscribing to topics
 * - Handling incoming MQTT messages
 * - Automatic reconnection
 * - Base topic organization
 * - Periodic publishing
 *
 * Hardware Requirements:
 * - ESPMegaPRO R3 board
 * - Network connection (Ethernet or WiFi)
 * - MQTT broker (e.g., Mosquitto, HiveMQ, etc.)
 *
 * Configuration:
 * - Update MQTT broker settings below
 * - Update network settings for your environment
 *
 * Created: 2024
 * Author: SIWAT SYSTEM
 */

#include <ESPMegaPRO.h>
#include <ETH.h>

// Create ESPMegaPRO instance
ESPMegaPRO espmega;

// ===== MQTT CONFIGURATION =====
const char* MQTT_SERVER = "192.168.1.5";          // MQTT broker IP or hostname
const uint16_t MQTT_PORT = 1883;                  // MQTT broker port (default: 1883)
const bool MQTT_USE_AUTH = false;                 // Set to true if broker requires authentication
const char* MQTT_USER = "username";               // MQTT username (if authentication enabled)
const char* MQTT_PASSWORD = "password";           // MQTT password (if authentication enabled)
const char* MQTT_BASE_TOPIC = "home/espmega";     // Base topic for all messages

// ===== NETWORK CONFIGURATION =====
const char* hostname = "espmega-mqtt-01";
const IPAddress static_ip(192, 168, 1, 100);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns1(1, 1, 1, 1);

// Timing variables
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 10000;     // Publish every 10 seconds
unsigned long lastStatusCheck = 0;
const unsigned long STATUS_CHECK_INTERVAL = 5000; // Check status every 5 seconds

// Message counter
uint32_t messageCount = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(500);

    Serial.println("========================================");
    Serial.println("ESPMegaPRO MQTT Basic Example");
    Serial.println("========================================");
    Serial.println();

    // Initialize ESPMegaPRO
    Serial.println("[INIT] Initializing ESPMegaPRO...");
    espmega.begin();
    Serial.println("[INIT] ESPMegaPRO initialized successfully");
    Serial.println();

    // Configure network
    Serial.println("[NETWORK] Configuring network...");
    NetworkConfig netConfig;
    netConfig.ip = static_ip;
    netConfig.gateway = gateway;
    netConfig.subnet = subnet;
    netConfig.dns1 = dns1;
    strcpy(netConfig.hostname, hostname);
    netConfig.useStaticIp = true;
    netConfig.useWifi = false;  // Using Ethernet

    espmega.iot.setNetworkConfig(netConfig);
    espmega.iot.bindEthernetInterface(&ETH);

    // Connect to network
    Serial.println("[NETWORK] Connecting to network...");
    espmega.iot.connectNetwork();

    // Wait for network connection
    unsigned long startTime = millis();
    while (!espmega.iot.networkConnected() && (millis() - startTime < 30000)) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (!espmega.iot.networkConnected()) {
        Serial.println("[ERROR] Network connection failed!");
        return;
    }

    Serial.println("[SUCCESS] Network connected!");
    Serial.print("IP Address: ");
    Serial.println(espmega.iot.getIp());
    Serial.println();

    // Configure MQTT
    Serial.println("[MQTT] Configuring MQTT...");
    MqttConfig mqttConfig;
    strcpy(mqttConfig.mqtt_server, MQTT_SERVER);
    mqttConfig.mqtt_port = MQTT_PORT;
    mqttConfig.mqtt_useauth = MQTT_USE_AUTH;

    if (MQTT_USE_AUTH) {
        strcpy(mqttConfig.mqtt_user, MQTT_USER);
        strcpy(mqttConfig.mqtt_password, MQTT_PASSWORD);
    }

    strcpy(mqttConfig.base_topic, MQTT_BASE_TOPIC);

    espmega.iot.setMqttConfig(mqttConfig);

    Serial.println("MQTT Configuration:");
    Serial.print("  Server: ");
    Serial.print(MQTT_SERVER);
    Serial.print(":");
    Serial.println(MQTT_PORT);
    Serial.print("  Base Topic: ");
    Serial.println(MQTT_BASE_TOPIC);
    Serial.print("  Auth: ");
    Serial.println(MQTT_USE_AUTH ? "Enabled" : "Disabled");
    Serial.println();

    // Register MQTT callbacks BEFORE connecting
    setupMqttCallbacks();

    // Connect to MQTT
    Serial.println("[MQTT] Connecting to MQTT broker...");
    espmega.iot.connectToMqtt();

    // Wait for MQTT connection
    startTime = millis();
    while (!espmega.iot.mqttConnected() && (millis() - startTime < 10000)) {
        delay(500);
        Serial.print(".");
        espmega.loop();  // Allow reconnection attempts
    }
    Serial.println();

    if (espmega.iot.mqttConnected()) {
        Serial.println("[SUCCESS] MQTT connected!");

        // Publish initial message
        publishStatus("online");
        publishMessage("System started successfully");
    } else {
        Serial.println("[WARNING] MQTT connection failed");
        Serial.println("Will retry automatically...");
    }

    Serial.println();
    Serial.println("========================================");
    Serial.println("Setup Complete");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Available MQTT Topics:");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/status");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/message");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/uptime");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/counter");
    Serial.print("  ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.println("/command (subscribe)");
    Serial.println();
}

void loop() {
    // Update ESPMegaPRO (handles MQTT and network)
    espmega.loop();

    // Periodic status check
    if (millis() - lastStatusCheck > STATUS_CHECK_INTERVAL) {
        lastStatusCheck = millis();

        Serial.print("[STATUS] Network: ");
        Serial.print(espmega.iot.networkConnected() ? "CONNECTED" : "DISCONNECTED");
        Serial.print(" | MQTT: ");
        Serial.println(espmega.iot.mqttConnected() ? "CONNECTED" : "DISCONNECTED");
    }

    // Periodic publishing
    if (millis() - lastPublish > PUBLISH_INTERVAL) {
        lastPublish = millis();

        if (espmega.iot.mqttConnected()) {
            // Publish uptime
            unsigned long uptime = millis() / 1000;
            char buffer[50];
            sprintf(buffer, "%lu", uptime);
            espmega.iot.publishRelative("uptime", buffer);
            Serial.print("[PUBLISH] Uptime: ");
            Serial.print(uptime);
            Serial.println(" seconds");

            // Publish message counter
            messageCount++;
            sprintf(buffer, "%lu", messageCount);
            espmega.iot.publishRelative("counter", buffer);
            Serial.print("[PUBLISH] Counter: ");
            Serial.println(messageCount);

            Serial.println();
        }
    }

    // Your application code here
}

/**
 * Setup MQTT callback handlers
 */
void setupMqttCallbacks() {
    Serial.println("[MQTT] Setting up callbacks...");

    // Register relative MQTT callback
    // This handles messages under our base topic
    espmega.iot.registerRelativeMqttCallback([](char* topic, char* payload) {
        Serial.println();
        Serial.println("--- MQTT Message Received ---");
        Serial.print("Topic: ");
        Serial.print(MQTT_BASE_TOPIC);
        Serial.print("/");
        Serial.println(topic);
        Serial.print("Payload: ");
        Serial.println(payload);
        Serial.println("----------------------------");
        Serial.println();

        // Handle specific topics
        if (strcmp(topic, "command") == 0) {
            handleCommand(payload);
        }
    });

    // Register subscribe callback
    // This is called when MQTT connects/reconnects
    espmega.iot.registerSubscribeCallback([]() {
        Serial.println("[MQTT] Subscribing to topics...");

        // Subscribe to command topic
        espmega.iot.subscribeRelative("command");
        Serial.print("  Subscribed to: ");
        Serial.print(MQTT_BASE_TOPIC);
        Serial.println("/command");
    });

    Serial.println("[MQTT] Callbacks registered");
}

/**
 * Handle incoming commands
 */
void handleCommand(char* payload) {
    Serial.print("[COMMAND] Received: ");
    Serial.println(payload);

    if (strcmp(payload, "status") == 0) {
        publishStatus("online");
        publishMessage("Status: OK");
    }
    else if (strcmp(payload, "ping") == 0) {
        publishMessage("pong");
    }
    else if (strcmp(payload, "reset_counter") == 0) {
        messageCount = 0;
        publishMessage("Counter reset");
    }
    else if (strcmp(payload, "info") == 0) {
        publishSystemInfo();
    }
    else {
        Serial.println("[COMMAND] Unknown command");
        publishMessage("Unknown command");
    }
}

/**
 * Publish status message
 */
void publishStatus(const char* status) {
    espmega.iot.publishRelative("status", status);
    Serial.print("[PUBLISH] Status: ");
    Serial.println(status);
}

/**
 * Publish general message
 */
void publishMessage(const char* message) {
    espmega.iot.publishRelative("message", message);
    Serial.print("[PUBLISH] Message: ");
    Serial.println(message);
}

/**
 * Publish system information
 */
void publishSystemInfo() {
    char buffer[100];

    // Uptime
    unsigned long uptime = millis() / 1000;
    sprintf(buffer, "Uptime: %lu seconds", uptime);
    publishMessage(buffer);

    // IP Address
    sprintf(buffer, "IP: %s", espmega.iot.getIp().toString().c_str());
    publishMessage(buffer);

    // Message count
    sprintf(buffer, "Messages: %lu", messageCount);
    publishMessage(buffer);
}

/**
 * Test MQTT Topics:
 *
 * To test this example, use an MQTT client (like MQTT Explorer or mosquitto_pub/sub):
 *
 * Subscribe to see messages:
 *   mosquitto_sub -h 192.168.1.5 -t "home/espmega/#"
 *
 * Send commands:
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/command" -m "ping"
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/command" -m "status"
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/command" -m "reset_counter"
 *   mosquitto_pub -h 192.168.1.5 -t "home/espmega/command" -m "info"
 */
