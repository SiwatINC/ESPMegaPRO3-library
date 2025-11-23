/**
 * @file mqtt_integration.ino
 * @brief Complete MQTT/IoT integration example with DigitalInputCard
 *
 * This example demonstrates:
 * - Integrating DigitalInputCard with ESPMegaIoT
 * - Automatic MQTT publishing of input states
 * - Remote monitoring via MQTT
 * - Subscribing to control topics
 * - Publishing on state changes
 * - Request-response pattern for state queries
 *
 * MQTT Topics:
 * ------------
 * Published (card publishes to these):
 *   {base_topic}/{card_id}/00 ... /15  -> Pin states ("0" or "1")
 *
 * Subscribed (card listens to these):
 *   {base_topic}/{card_id}/publish_enable  -> Enable/disable publishing ("0" or "1")
 *   {base_topic}/{card_id}/requeststate    -> Request immediate state publish (any payload)
 *
 * Example with base_topic="home/espmega" and card_id=0:
 *   home/espmega/0/00  -> "1"  (pin 0 is HIGH)
 *   home/espmega/0/05  -> "0"  (pin 5 is LOW)
 *   home/espmega/0/publish_enable <- "1" (enable publishing)
 *   home/espmega/0/requeststate <- "" (request all states)
 *
 * Hardware Requirements:
 * - ESPMegaPRO3 board with network connectivity
 * - DigitalInputCard expansion card
 * - MQTT broker (Mosquitto, HiveMQ, etc.)
 * - Digital inputs (switches, buttons, sensors)
 *
 * Software Requirements:
 * - ESPMegaIoT library
 * - PubSubClient library
 * - WiFi or Ethernet configured
 *
 * @author Siwat Sirichai
 * @date 2025
 */

#include <ESPMegaIoT.hpp>
#include <DigitalInputCard.hpp>

// ========================================
// Configuration
// ========================================

// WiFi Configuration
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// MQTT Configuration
const char* MQTT_SERVER = "192.168.1.100";  // Your MQTT broker IP
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "espmega";          // Optional: MQTT username
const char* MQTT_PASSWORD = "password";      // Optional: MQTT password
const char* MQTT_BASE_TOPIC = "home/espmega";

// Card Configuration
const uint8_t CARD_SLOT = 0;  // Slot number for the input card

// ========================================
// Global Objects
// ========================================

// Create IoT object
ESPMegaIoT iot;

// Create DigitalInputCard
DigitalInputCard inputCard(false, false, false, false, false, false);

// Status tracking
bool mqttConnected = false;
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;  // Try reconnecting every 5 seconds

// ========================================
// Callback Functions
// ========================================

/**
 * @brief Local callback for input changes (in addition to MQTT publishing)
 *
 * This callback runs locally and can trigger immediate actions
 * in addition to the MQTT publishing done by DigitalInputIoT
 *
 * @param pin The pin that changed
 * @param state The new state
 */
void onLocalInputChange(uint8_t pin, bool state) {
    // Print to serial for debugging
    Serial.print("[LOCAL] Pin ");
    Serial.print(pin);
    Serial.print(" changed to ");
    Serial.println(state ? "HIGH" : "LOW");

    // You can add immediate local actions here
    // For example, turn on an LED, activate a relay, etc.
    switch(pin) {
        case 0:
            // Example: Pin 0 controls something locally
            if (state) {
                Serial.println("  -> Emergency button pressed! Taking local action...");
                // Take immediate action without waiting for MQTT
            }
            break;

        case 1:
            // Example: Pin 1 triggers a local alarm
            if (state) {
                Serial.println("  -> Motion detected! Local alarm activated.");
            }
            break;

        // Add more local actions as needed
    }
}

/**
 * @brief WiFi event callback
 */
void onWiFiConnected() {
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

/**
 * @brief MQTT connection callback
 */
void onMqttConnected() {
    Serial.println("MQTT connected!");
    mqttConnected = true;

    // The DigitalInputIoT component automatically subscribes to its topics
    // and publishes initial state when MQTT connects
}

/**
 * @brief MQTT disconnection callback
 */
void onMqttDisconnected() {
    Serial.println("MQTT disconnected!");
    mqttConnected = false;
}

// ========================================
// Setup
// ========================================

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(2000);
    Serial.println();
    Serial.println("========================================");
    Serial.println("DigitalInputCard MQTT Integration Example");
    Serial.println("========================================");
    Serial.println();

    // ========================================
    // Initialize Input Card
    // ========================================
    Serial.println("Initializing Digital Input Card...");
    if (!inputCard.begin()) {
        Serial.println("ERROR: Failed to initialize input card!");
        Serial.println("Please check I2C connections and addresses.");
        while (1) delay(1000);
    }
    Serial.println("SUCCESS: Input card initialized");

    // Configure debounce times for better reliability
    for (int i = 0; i < 16; i++) {
        inputCard.setDebounceTime(i, 50);  // 50ms debounce for all pins
    }

    // Register local callback (optional - in addition to MQTT publishing)
    inputCard.registerCallback(onLocalInputChange);

    // Preload input buffer to avoid false triggers
    inputCard.preloadInputBuffer();

    Serial.println();

    // ========================================
    // Initialize IoT/Network
    // ========================================
    Serial.println("Initializing ESPMegaIoT...");

    // Configure WiFi
    iot.setWiFi(WIFI_SSID, WIFI_PASSWORD);

    // Configure MQTT
    iot.setMqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_BASE_TOPIC);

    // Set callbacks
    // Note: Use the appropriate callbacks for your version of ESPMegaIoT
    // These are example callback names - adjust to your library version

    // Initialize IoT
    if (!iot.begin()) {
        Serial.println("ERROR: Failed to initialize IoT!");
        while (1) delay(1000);
    }
    Serial.println("SUCCESS: IoT initialized");
    Serial.println();

    // ========================================
    // Register Input Card with IoT
    // ========================================
    Serial.println("Registering input card with IoT...");

    // This automatically creates a DigitalInputIoT instance
    // and handles all MQTT publishing/subscribing
    if (!iot.registerCard(CARD_SLOT, &inputCard)) {
        Serial.println("ERROR: Failed to register card!");
        while (1) delay(1000);
    }

    Serial.println("SUCCESS: Card registered");
    Serial.println();

    // ========================================
    // Print Configuration
    // ========================================
    Serial.println("========================================");
    Serial.println("Configuration:");
    Serial.println("========================================");
    Serial.print("WiFi SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("MQTT Broker: ");
    Serial.print(MQTT_SERVER);
    Serial.print(":");
    Serial.println(MQTT_PORT);
    Serial.print("MQTT Base Topic: ");
    Serial.println(MQTT_BASE_TOPIC);
    Serial.print("Card Slot: ");
    Serial.println(CARD_SLOT);
    Serial.println();

    Serial.println("MQTT Topics:");
    Serial.println("  Published (output):");
    for (int i = 0; i < 16; i++) {
        Serial.print("    ");
        Serial.print(MQTT_BASE_TOPIC);
        Serial.print("/");
        Serial.print(CARD_SLOT);
        Serial.print("/");
        if (i < 10) Serial.print("0");
        Serial.print(i);
        Serial.println("  -> Pin state (0 or 1)");
    }
    Serial.println();

    Serial.println("  Subscribed (input):");
    Serial.print("    ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.print("/");
    Serial.print(CARD_SLOT);
    Serial.println("/publish_enable  <- Enable/disable publishing");
    Serial.print("    ");
    Serial.print(MQTT_BASE_TOPIC);
    Serial.print("/");
    Serial.print(CARD_SLOT);
    Serial.println("/requeststate  <- Request state update");
    Serial.println("========================================");
    Serial.println();

    Serial.println("System ready! Monitoring inputs...");
    Serial.println();
}

// ========================================
// Main Loop
// ========================================

void loop() {
    // Process IoT - handles WiFi, MQTT, and card updates
    iot.loop();

    // The input card's loop is automatically called by iot.loop()
    // when the card is registered

    // Print status periodically
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint >= 30000) {  // Every 30 seconds
        lastStatusPrint = millis();
        printStatus();
    }

    delay(10);
}

// ========================================
// Utility Functions
// ========================================

/**
 * @brief Print system status
 */
void printStatus() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("SYSTEM STATUS");
    Serial.println("========================================");

    // WiFi Status
    Serial.print("WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected (");
        Serial.print(WiFi.localIP());
        Serial.print(") RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("Disconnected");
    }

    // MQTT Status
    Serial.print("MQTT: ");
    Serial.println(mqttConnected ? "Connected" : "Disconnected");

    // Input Card Status
    Serial.print("Input Card: ");
    Serial.println(inputCard.getStatus() ? "OK" : "ERROR");

    // Current Input States
    Serial.println();
    Serial.println("Current Input States:");
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0) Serial.print("  ");
        Serial.print("P");
        if (i < 10) Serial.print("0");
        Serial.print(i);
        Serial.print("=");
        Serial.print(inputCard.digitalRead(i, false) ? "HIGH" : "LOW ");
        if ((i + 1) % 4 == 0) {
            Serial.println();
        } else {
            Serial.print("  ");
        }
    }

    Serial.println();
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println("========================================");
    Serial.println();
}

/**
 * TESTING WITH MQTT:
 *
 * Use an MQTT client (like MQTT Explorer, mosquitto_sub, or Home Assistant)
 * to monitor and control the inputs:
 *
 * MONITORING INPUT STATES:
 * ------------------------
 * Subscribe to all pin topics:
 *   mosquitto_sub -h <broker_ip> -t "home/espmega/0/#"
 *
 * Subscribe to a specific pin:
 *   mosquitto_sub -h <broker_ip> -t "home/espmega/0/05"
 *
 * REQUEST ALL STATES:
 * -------------------
 * Publish to requeststate topic:
 *   mosquitto_pub -h <broker_ip> -t "home/espmega/0/requeststate" -m ""
 *
 * This will cause all 16 pin states to be published immediately
 *
 * ENABLE/DISABLE PUBLISHING:
 * --------------------------
 * Disable automatic publishing:
 *   mosquitto_pub -h <broker_ip> -t "home/espmega/0/publish_enable" -m "0"
 *
 * Enable automatic publishing:
 *   mosquitto_pub -h <broker_ip> -t "home/espmega/0/publish_enable" -m "1"
 *
 * When disabled, pin changes won't be published automatically.
 * You can still request states manually.
 *
 * HOME ASSISTANT INTEGRATION:
 * ---------------------------
 * Add binary sensors to configuration.yaml:
 *
 * binary_sensor:
 *   - platform: mqtt
 *     name: "Front Door"
 *     state_topic: "home/espmega/0/00"
 *     payload_on: "1"
 *     payload_off: "0"
 *     device_class: door
 *
 *   - platform: mqtt
 *     name: "Motion Sensor"
 *     state_topic: "home/espmega/0/01"
 *     payload_on: "1"
 *     payload_off: "0"
 *     device_class: motion
 *
 * NODE-RED INTEGRATION:
 * ---------------------
 * Use MQTT In nodes to subscribe to pin topics
 * Process and route based on your automation logic
 * Can trigger notifications, control other devices, log to database, etc.
 *
 * ADVANCED USAGE:
 * ---------------
 * 1. Multiple cards: Register additional cards in different slots
 * 2. Remote monitoring: Access from anywhere with MQTT broker on internet
 * 3. Data logging: Subscribe to topics and log to database
 * 4. Alerts: Trigger notifications on specific pin changes
 * 5. Automation: Use input states to control other IoT devices
 *
 * TROUBLESHOOTING:
 * ----------------
 * 1. Check WiFi connection first
 * 2. Verify MQTT broker is accessible
 * 3. Check MQTT credentials
 * 4. Monitor serial output for connection status
 * 5. Use MQTT Explorer to verify topics and messages
 * 6. Ensure firewall allows MQTT port (1883)
 *
 * NEXT STEPS:
 * -----------
 * 1. Connect actual inputs and test MQTT publishing
 * 2. Integrate with home automation system
 * 3. Add additional cards for more inputs
 * 4. Implement remote control logic
 * 5. Create dashboards to visualize input states
 */
