/**
 * @file mqtt_control.ino
 * @brief MQTT Control Example
 *
 * This example demonstrates MQTT control of the DigitalOutputCard.
 * It shows how to:
 * - Set up DigitalOutputIoT component
 * - Control outputs via MQTT
 * - Monitor output changes via MQTT
 * - Handle MQTT topics and payloads
 * - Enable/disable MQTT publishing
 *
 * Hardware Required:
 * - ESPMegaPRO board with WiFi
 * - Digital Output Card (address 0x40)
 * - LEDs or other 12V loads for visual feedback
 * - Access to MQTT broker
 *
 * MQTT Topics:
 *
 * Subscribe (Control):
 *   - <base>/<pin>/set/state  (payload: 0 or 1)
 *   - <base>/<pin>/set/value  (payload: 0-4095)
 *   - <base>/requeststate     (payload: N/A)
 *   - <base>/publish_enable   (payload: 0 or 1)
 *
 * Publish (Status):
 *   - <base>/<pin>/state      (payload: 0 or 1)
 *   - <base>/<pin>/value      (payload: 0-4095)
 *
 * Example Topics (if base is "home/outputs"):
 *   - home/outputs/00/set/state  -> Set pin 0 state
 *   - home/outputs/05/set/value  -> Set pin 5 PWM value
 *   - home/outputs/requeststate  -> Request all states
 *
 * Created: 2025
 *
 * This example code is in the public domain.
 */

#include <DigitalOutputCard.hpp>
#include <DigitalOutputIoT.hpp>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// MQTT broker settings
const char* MQTT_BROKER = "mqtt.example.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "ESPMegaPRO_DigitalOutput";
const char* MQTT_USERNAME = "";  // Leave empty if no authentication
const char* MQTT_PASSWORD = "";  // Leave empty if no authentication

// MQTT base topic for digital outputs
const char* MQTT_BASE_TOPIC = "home/outputs";

// Create instances
DigitalOutputCard card(0x40);
DigitalOutputIoT iot;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Status tracking
unsigned long lastReconnectAttempt = 0;
unsigned long lastStatusPrint = 0;
bool systemReady = false;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);

    Serial.println("Digital Output Card - MQTT Control Example");
    Serial.println("==========================================");

    // Initialize the card
    Serial.println("\nInitializing Digital Output Card...");
    if (!card.begin()) {
        Serial.println("Card initialization failed!");
        while(1);
    }
    Serial.println("Card initialized successfully");

    // Set all outputs to OFF initially
    for (uint8_t pin = 0; pin < 16; pin++) {
        card.digitalWrite(pin, LOW);
    }

    // Connect to WiFi
    connectWiFi();

    // Configure MQTT
    Serial.println("\nConfiguring MQTT...");
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024); // Increase buffer if needed

    // Connect to MQTT broker
    connectMQTT();

    // Initialize IoT component
    Serial.println("\nInitializing IoT component...");
    char* baseTopic = new char[strlen(MQTT_BASE_TOPIC) + 1];
    strcpy(baseTopic, MQTT_BASE_TOPIC);

    if (!iot.begin(0, &card, &mqtt, baseTopic)) {
        Serial.println("IoT initialization failed!");
        while(1);
    }
    Serial.println("IoT initialized successfully");

    // Subscribe to MQTT topics
    Serial.println("\nSubscribing to MQTT topics...");
    iot.subscribe();
    Serial.println("Subscribed successfully");

    // Enable publishing
    iot.setDigitalOutputsPublishEnabled(true);

    // Publish initial state
    Serial.println("\nPublishing initial state...");
    iot.publishReport();

    systemReady = true;

    Serial.println("\n=== System Ready ===");
    Serial.println("Listening for MQTT commands...");
    printUsageInstructions();
}

void loop() {
    // Maintain MQTT connection
    if (!mqtt.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (connectMQTT()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        mqtt.loop();
    }

    // Print status periodically
    unsigned long now = millis();
    if (now - lastStatusPrint >= 30000) {
        lastStatusPrint = now;
        printStatus();
    }

    // Small delay to prevent watchdog issues
    delay(10);
}

/**
 * @brief Connect to WiFi network
 */
void connectWiFi() {
    Serial.println("\nConnecting to WiFi...");
    Serial.printf("SSID: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("\nWiFi connection failed!");
        while(1);
    }
}

/**
 * @brief Connect to MQTT broker
 *
 * @return true if connected successfully
 */
bool connectMQTT() {
    Serial.println("\nConnecting to MQTT broker...");
    Serial.printf("Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    Serial.printf("Client ID: %s\n", MQTT_CLIENT_ID);

    bool connected;

    if (strlen(MQTT_USERNAME) > 0) {
        connected = mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    } else {
        connected = mqtt.connect(MQTT_CLIENT_ID);
    }

    if (connected) {
        Serial.println("MQTT connected!");

        // Resubscribe if this is a reconnection
        if (systemReady) {
            Serial.println("Resubscribing to topics...");
            iot.subscribe();

            // Republish current state
            Serial.println("Republishing current state...");
            iot.publishReport();
        }

        return true;
    } else {
        Serial.printf("MQTT connection failed, rc=%d\n", mqtt.state());
        Serial.println("Will retry in 5 seconds...");
        return false;
    }
}

/**
 * @brief MQTT message callback
 *
 * @param topic Topic that received a message
 * @param payload Message payload
 * @param length Payload length
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Null-terminate the payload
    char* payloadStr = new char[length + 1];
    memcpy(payloadStr, payload, length);
    payloadStr[length] = '\0';

    // Extract relative topic (remove base topic)
    char* relativeTopic = topic + strlen(MQTT_BASE_TOPIC);
    if (relativeTopic[0] == '/') {
        relativeTopic++; // Skip leading slash
    }

    Serial.printf("\n[MQTT] Topic: %s\n", relativeTopic);
    Serial.printf("[MQTT] Payload: %s\n", payloadStr);

    // Pass to IoT component
    iot.handleMqttMessage(relativeTopic, payloadStr);

    delete[] payloadStr;
}

/**
 * @brief Print usage instructions
 */
void printUsageInstructions() {
    Serial.println("\n=== MQTT Usage Instructions ===");
    Serial.println("\nControl Topics:");
    Serial.printf("  %s/<pin>/set/state   - Set pin state (0 or 1)\n", MQTT_BASE_TOPIC);
    Serial.printf("  %s/<pin>/set/value   - Set pin value (0-4095)\n", MQTT_BASE_TOPIC);
    Serial.printf("  %s/requeststate      - Request all states\n", MQTT_BASE_TOPIC);
    Serial.printf("  %s/publish_enable    - Enable/disable publishing (0 or 1)\n", MQTT_BASE_TOPIC);

    Serial.println("\nStatus Topics:");
    Serial.printf("  %s/<pin>/state       - Pin state (0 or 1)\n", MQTT_BASE_TOPIC);
    Serial.printf("  %s/<pin>/value       - Pin value (0-4095)\n", MQTT_BASE_TOPIC);

    Serial.println("\nExample Commands:");
    Serial.println("Using mosquitto_pub:");
    Serial.printf("  mosquitto_pub -h %s -t '%s/00/set/state' -m '1'\n",
                 MQTT_BROKER, MQTT_BASE_TOPIC);
    Serial.printf("  mosquitto_pub -h %s -t '%s/00/set/value' -m '2048'\n",
                 MQTT_BROKER, MQTT_BASE_TOPIC);
    Serial.printf("  mosquitto_pub -h %s -t '%s/requeststate' -m ''\n",
                 MQTT_BROKER, MQTT_BASE_TOPIC);

    Serial.println("\nPin Numbers:");
    Serial.println("  Use 2-digit format: 00, 01, 02, ..., 15");
    Serial.println();
}

/**
 * @brief Print current status
 */
void printStatus() {
    Serial.println("\n=== System Status ===");
    Serial.printf("WiFi: %s (RSSI: %d dBm)\n",
                 WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                 WiFi.RSSI());
    Serial.printf("MQTT: %s\n", mqtt.connected() ? "Connected" : "Disconnected");
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());

    Serial.println("\nOutput States:");
    for (uint8_t pin = 0; pin < 16; pin++) {
        bool state = card.getState(pin);
        uint16_t value = card.getValue(pin);
        if (state || value > 0) {
            Serial.printf("  Pin %02d: %s, Value: %d\n",
                         pin,
                         state ? "ON " : "OFF",
                         value);
        }
    }
    Serial.println();
}

/**
 * @brief Demonstrate local control with MQTT publishing
 */
void demonstrateLocalControl() {
    Serial.println("\n=== Demonstrating Local Control ===");
    Serial.println("Changing outputs locally - watch MQTT publish messages!");

    // Turn on pin 0
    Serial.println("Turning on pin 0 at 100%");
    card.digitalWrite(0, HIGH);
    delay(1000);

    // Dim pin 0 to 50%
    Serial.println("Dimming pin 0 to 50%");
    card.analogWrite(0, 2048);
    delay(1000);

    // Turn off pin 0
    Serial.println("Turning off pin 0");
    card.digitalWrite(0, LOW);
    delay(1000);

    Serial.println("Check your MQTT client - you should see publish messages!");
}
