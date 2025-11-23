/**
 * @file basic_display.ino
 * @brief Basic example of using the InternalDisplay with ESPMegaPRO
 *
 * This example demonstrates:
 * - Initializing the internal display
 * - Binding input and output cards
 * - Basic display functionality
 * - Automatic status updates
 *
 * Hardware Requirements:
 * - ESPMegaPRO board
 * - Internal display connected to Serial port
 * - Ethernet connection
 *
 * The display will show:
 * - Dashboard with network status
 * - Input page with digital input states
 * - Output page with PWM control
 * - Network and MQTT configuration pages
 *
 * @author ESPMegaPRO Team
 * @date 2025-11-23
 */

#include <ESPMegaProOS.hpp>
#include <InternalDisplay.hpp>
#include <ETH.h>

// Create ESPMegaPRO object
ESPMegaPRO espmega = ESPMegaPRO();

/**
 * @brief Callback function for input changes
 *
 * This function is called whenever a digital input changes state.
 * The display will automatically update, but you can add custom
 * actions here as well.
 *
 * @param pin The input pin number (0-15)
 * @param value The new state (0 or 1)
 */
void onInputChange(uint8_t pin, uint8_t value) {
    Serial.print("Input ");
    Serial.print(pin);
    Serial.print(" changed to: ");
    Serial.println(value ? "HIGH" : "LOW");

    // Example: Mirror input to output
    // espmega.outputs.setState(pin, value);
}

/**
 * @brief Callback function for MQTT messages
 *
 * This function is called when an MQTT message is received.
 * You can parse the topic and payload to control your device.
 *
 * @param topic MQTT topic
 * @param payload Message payload
 */
void onMqttMessage(char *topic, char *payload) {
    Serial.print("MQTT Message - Topic: ");
    Serial.print(topic);
    Serial.print(", Payload: ");
    Serial.println(payload);
}

/**
 * @brief Arduino setup function
 *
 * Initializes all components:
 * 1. ESPMegaPRO core
 * 2. IoT module (network and MQTT)
 * 3. Internal display
 * 4. Card bindings
 */
void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    Serial.println("=================================");
    Serial.println("ESPMegaPRO Display Basic Example");
    Serial.println("=================================");

    // ========================================
    // Step 1: Initialize ESPMegaPRO
    // ========================================
    Serial.println("Initializing ESPMegaPRO...");
    espmega.begin();

    // ========================================
    // Step 2: Enable and configure IoT module
    // ========================================
    Serial.println("Enabling IoT module...");
    espmega.enableIotModule();

    // Initialize Ethernet
    Serial.println("Starting Ethernet...");
    ETH.begin();

    // Bind Ethernet to IoT module
    Serial.println("Binding Ethernet interface...");
    espmega.iot->bindEthernetInterface(&ETH);

    // Load network configuration from FRAM
    Serial.println("Loading network configuration...");
    espmega.iot->loadNetworkConfig();

    // Connect to network
    Serial.println("Connecting to network...");
    espmega.iot->connectNetwork();

    // Load MQTT configuration from FRAM
    Serial.println("Loading MQTT configuration...");
    espmega.iot->loadMqttConfig();

    // Connect to MQTT broker
    Serial.println("Connecting to MQTT...");
    espmega.iot->connectToMqtt();

    // Register MQTT callback
    espmega.iot->registerMqttCallback(onMqttMessage);

    // Register input and output cards with MQTT
    // This allows remote control via MQTT
    Serial.println("Registering cards with MQTT...");
    espmega.iot->registerCard(0);  // Input card
    espmega.iot->registerCard(1);  // Output card

    // ========================================
    // Step 3: Enable and configure display
    // ========================================
    Serial.println("Enabling internal display...");
    espmega.enableInternalDisplay(&Serial);

    // Bind input card to display
    // This allows the display to show input states on page 2
    Serial.println("Binding input card to display...");
    espmega.display->bindInputCard(&espmega.inputs);

    // Bind output card to display
    // This allows the display to show and control PWM outputs on pages 3 and 5
    Serial.println("Binding output card to display...");
    espmega.display->bindOutputCard(&espmega.outputs);

    // ========================================
    // Step 4: Register input callback
    // ========================================
    Serial.println("Registering input callback...");
    espmega.inputs.registerCallback(onInputChange);

    // ========================================
    // Initialization complete
    // ========================================
    Serial.println("=================================");
    Serial.println("Initialization complete!");
    Serial.println("=================================");
    Serial.println();
    Serial.println("Display pages:");
    Serial.println("  Page 1: Dashboard - System status overview");
    Serial.println("  Page 2: Inputs - Digital input visualization");
    Serial.println("  Page 3: Outputs - PWM output control");
    Serial.println("  Page 5: PWM Adjust - Detailed PWM control");
    Serial.println("  Page 6: Network Config - Network settings");
    Serial.println("  Page 11: MQTT Config - MQTT settings");
    Serial.println();
    Serial.println("You can now interact with the display!");
    Serial.println("=================================");
}

/**
 * @brief Arduino main loop
 *
 * This function is called repeatedly and handles all system operations.
 * The espmega.loop() call manages:
 * - Display updates and touch events
 * - Input/output card processing
 * - Network connectivity
 * - MQTT communication
 * - All registered callbacks
 */
void loop() {
    // This single call handles everything!
    // - Display serial communication
    // - Touch event processing
    // - Status icon updates
    // - Clock updates
    // - Input/output card updates
    // - MQTT processing
    espmega.loop();

    // ========================================
    // Optional: Add your custom code here
    // ========================================

    // Example 1: Periodic status messages
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 30000) {  // Every 30 seconds
        lastStatusPrint = millis();

        Serial.println("--- System Status ---");
        Serial.print("Network: ");
        Serial.println(espmega.iot->networkConnected() ? "Connected" : "Disconnected");
        Serial.print("MQTT: ");
        Serial.println(espmega.iot->mqttConnected() ? "Connected" : "Disconnected");
        Serial.print("Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
        Serial.println("--------------------");
    }

    // Example 2: Custom PWM control
    // Set PWM output 0 to slowly fade in and out
    static unsigned long lastPwmUpdate = 0;
    static uint16_t pwmValue = 0;
    static bool pwmIncreasing = true;

    if (millis() - lastPwmUpdate > 10) {  // Update every 10ms
        lastPwmUpdate = millis();

        if (pwmIncreasing) {
            pwmValue += 10;
            if (pwmValue >= 4095) {
                pwmValue = 4095;
                pwmIncreasing = false;
            }
        } else {
            if (pwmValue < 10) {
                pwmValue = 0;
                pwmIncreasing = true;
            } else {
                pwmValue -= 10;
            }
        }

        // Set PWM value (this will automatically update the display)
        // Uncomment to enable this feature:
        // espmega.outputs.setValue(0, pwmValue);
    }

    // Example 3: React to specific input patterns
    // If input 0 and input 1 are both HIGH, do something
    static bool lastPattern = false;
    bool currentPattern = espmega.inputs.digitalRead(0) && espmega.inputs.digitalRead(1);

    if (currentPattern && !lastPattern) {
        Serial.println("Pattern detected: Input 0 and 1 both HIGH!");
        // Do something, like toggle an output
        // espmega.outputs.setState(15, !espmega.outputs.getState(15));
    }
    lastPattern = currentPattern;
}

/**
 * Additional Notes:
 *
 * 1. Network Configuration:
 *    - Use the display (Page 6) to configure network settings
 *    - Or set programmatically before first boot (see basic_firmware.cpp)
 *
 * 2. MQTT Configuration:
 *    - Use the display (Page 11) to configure MQTT settings
 *    - Server, port, authentication, and base topic
 *
 * 3. Input Card:
 *    - 16 digital inputs (pins 0-15)
 *    - View states on display page 2
 *    - Automatic debouncing handled by library
 *
 * 4. Output Card:
 *    - 16 PWM outputs (pins 0-15)
 *    - View and control on display pages 3 and 5
 *    - 12-bit resolution (0-4095)
 *
 * 5. Display Features:
 *    - Automatic network and MQTT status icons
 *    - Real-time clock display
 *    - Touch interface for all controls
 *    - Configuration persistence in FRAM
 *
 * 6. Extending This Example:
 *    - Add more custom logic in loop()
 *    - Register additional callbacks
 *    - Add web server (see full_integration example)
 *    - Add climate control (see with_climate example)
 */
