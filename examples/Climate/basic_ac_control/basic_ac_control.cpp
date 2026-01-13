/**
 * @file basic_ac_control.ino
 * @brief Basic ClimateCard example demonstrating standalone AC control
 *
 * This example shows how to use the ClimateCard to control an air conditioner
 * using infrared signals without any environmental sensors.
 *
 * Hardware Required:
 * - ESP32 board
 * - IR LED connected to GPIO 14
 * - Optional: IR LED driver circuit for better range
 *
 * Features Demonstrated:
 * - ClimateCard initialization without sensor
 * - Setting AC temperature, mode, and fan speed
 * - Using state change callbacks
 * - Reading current AC state
 *
 * IR LED Connection:
 *   GPIO 14 --> [330Ω Resistor] --> IR LED Anode --> GND
 *
 * Or with transistor for better range:
 *   GPIO 14 --> [1kΩ] --> NPN Base
 *   VCC --> [100Ω] --> IR LED Anode --> NPN Collector
 *   GND --> NPN Emitter
 *
 * @note Before using this example, you must capture and define IR codes for your AC model.
 * @note See the ir_code_capture example for how to capture codes from your AC remote.
 */

#include <ClimateCard.hpp>

// ============================================================================
// IR Code Definitions
// ============================================================================

/**
 * IR code array structure: [mode][fan_speed][temperature]
 *
 * For this example AC:
 * - Modes: 0=Off, 1=Cool, 2=Fan Only
 * - Fan Speeds: 0=Auto, 1=Low, 2=Medium, 3=High
 * - Temperatures: 16°C to 30°C (15 settings, indexed 0-14)
 *
 * Each IR code is an array of microsecond timing values (ON/OFF pulses)
 * You must capture these from your actual AC remote using the ir_code_capture example
 */

// Example IR code (replace with your actual codes)
// This is a placeholder - real codes will be different for each AC model
const uint16_t irCodes[3][4][15][67] = {
    // Mode 0: Off
    {
        // Fan Speed 0-3 (all use same "off" code typically)
        {
            // Temperature 0-14 (off mode usually ignores temperature)
            {9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560},
            // ... repeat for all temperatures (simplified for this example)
        },
        // ... repeat for all fan speeds
    },
    // Mode 1: Cool
    {
        // Fan Speed 0: Auto
        {
            // Temperature 0: 16°C
            {9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560},
            // Temperature 1: 17°C
            {9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560},
            // ... more temperatures
        },
        // ... more fan speeds
    },
    // Mode 2: Fan Only
    {
        // ... similar structure
    }
};

/**
 * Function to retrieve IR code for a specific AC state
 *
 * @param mode Mode index (0-2)
 * @param fan_speed Fan speed index (0-3)
 * @param temperature Temperature offset from minimum (0=16°C, 1=17°C, etc.)
 * @param codePtr Pointer to set to the IR code array
 * @return Number of elements in the IR code array
 */
size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr) {
    // Point to the appropriate IR code array
    *codePtr = irCodes[mode][fan_speed][temperature];

    // Return the number of timing values in the array
    return sizeof(irCodes[mode][fan_speed][temperature]) / sizeof(uint16_t);
}

// ============================================================================
// AC Definition
// ============================================================================

// Define mode names (must match the number of modes)
const char *mode_names[] = {
    "off",
    "cool",
    "fan_only"
};

// Define fan speed names (must match the number of fan speeds)
const char *fan_speed_names[] = {
    "auto",
    "low",
    "medium",
    "high"
};

// Define air conditioner characteristics
AirConditioner myAC = {
    .max_temperature = 30,              // Maximum temperature: 30°C
    .min_temperature = 16,              // Minimum temperature: 16°C
    .modes = 3,                         // Number of modes: 3 (off, cool, fan_only)
    .mode_names = mode_names,           // Pointer to mode names array
    .fan_speeds = 4,                    // Number of fan speeds: 4 (auto, low, medium, high)
    .fan_speed_names = fan_speed_names, // Pointer to fan speed names array
    .getInfraredCode = getInfraredCode  // Function to retrieve IR codes
};

// ============================================================================
// Hardware Configuration
// ============================================================================

#define IR_TX_PIN 14        // GPIO pin for IR LED
#define RMT_CHANNEL RMT_CHANNEL_0  // RMT channel (must be unique)

// Create ClimateCard instance (no sensor)
ClimateCard ac(IR_TX_PIN, myAC, RMT_CHANNEL);

// ============================================================================
// Callback Functions
// ============================================================================

/**
 * Callback function called whenever AC state changes
 *
 * @param mode New mode index
 * @param fan_speed New fan speed index
 * @param temperature New temperature setting
 */
void onACStateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.println("========================================");
    Serial.println("AC State Changed:");
    Serial.printf("  Mode:       %s (%d)\n", ac.getModeName(), mode);
    Serial.printf("  Fan Speed:  %s (%d)\n", ac.getFanSpeedName(), fan_speed);
    Serial.printf("  Temperature: %d°C\n", temperature);
    Serial.println("========================================");
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);  // Wait for serial monitor to open

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  ClimateCard Basic AC Control Demo");
    Serial.println("========================================");
    Serial.println();

    // Initialize the ClimateCard
    Serial.println("Initializing ClimateCard...");
    if (!ac.begin()) {
        Serial.println("ERROR: Failed to initialize ClimateCard!");
        Serial.println("Please check hardware connections and restart.");
        while (1) {
            delay(1000);  // Halt execution
        }
    }
    Serial.println("ClimateCard initialized successfully!");
    Serial.println();

    // Register callback for state changes
    Serial.println("Registering state change callback...");
    ac.registerChangeCallback(onACStateChange);
    Serial.println("Callback registered!");
    Serial.println();

    // Display AC capabilities
    Serial.println("Air Conditioner Configuration:");
    Serial.printf("  Temperature Range: %d°C - %d°C\n", myAC.min_temperature, myAC.max_temperature);
    Serial.printf("  Number of Modes: %d\n", myAC.modes);
    Serial.print("  Available Modes: ");
    for (int i = 0; i < myAC.modes; i++) {
        Serial.print(mode_names[i]);
        if (i < myAC.modes - 1) Serial.print(", ");
    }
    Serial.println();
    Serial.printf("  Number of Fan Speeds: %d\n", myAC.fan_speeds);
    Serial.print("  Available Fan Speeds: ");
    for (int i = 0; i < myAC.fan_speeds; i++) {
        Serial.print(fan_speed_names[i]);
        if (i < myAC.fan_speeds - 1) Serial.print(", ");
    }
    Serial.println();
    Serial.println();

    // Set initial AC state
    Serial.println("Setting initial AC state...");
    Serial.println("  Mode: Cool");
    Serial.println("  Fan Speed: Auto");
    Serial.println("  Temperature: 24°C");
    Serial.println();

    // Method 1: Set each parameter individually
    ac.setMode(1);           // Cool mode (index 1)
    ac.setFanSpeed(0);       // Auto fan speed (index 0)
    ac.setTemperature(24);   // 24°C

    // Alternative Method 2: Set all at once using setState()
    // ac.setState(1, 0, 24);  // mode=1 (cool), fan=0 (auto), temp=24

    Serial.println("Initialization complete!");
    Serial.println();
    Serial.println("The AC will cycle through different states every 15 seconds.");
    Serial.println("Watch the serial output to see state changes.");
    Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    // Update the ClimateCard
    // This is required even without sensors for proper operation
    ac.loop();

    // Demonstrate different AC operations
    // Change AC state every 15 seconds to show different capabilities
    static unsigned long lastChange = 0;
    static uint8_t demoStep = 0;

    if (millis() - lastChange >= 15000) {  // Every 15 seconds
        lastChange = millis();

        Serial.println();
        Serial.printf(">>> Demo Step %d <<<\n", demoStep + 1);
        Serial.println();

        switch (demoStep) {
            case 0:
                // Step 1: Change temperature
                Serial.println("Action: Increasing temperature to 26°C");
                ac.setTemperature(26);
                break;

            case 1:
                // Step 2: Change fan speed
                Serial.println("Action: Setting fan speed to High");
                ac.setFanSpeedByName("high");  // Using name instead of index
                break;

            case 2:
                // Step 3: Change to fan-only mode
                Serial.println("Action: Switching to Fan Only mode");
                ac.setModeByName("fan_only");  // Using name instead of index
                break;

            case 3:
                // Step 4: Set all parameters at once
                Serial.println("Action: Setting all parameters (Cool, Medium, 22°C)");
                ac.setState(1, 2, 22);  // Cool mode, medium fan, 22°C
                break;

            case 4:
                // Step 5: Turn off
                Serial.println("Action: Turning AC off");
                ac.setMode(0);  // Off mode
                break;

            case 5:
                // Step 6: Back to initial state
                Serial.println("Action: Returning to initial state (Cool, Auto, 24°C)");
                ac.setState(1, 0, 24);
                break;

            default:
                demoStep = -1;  // Will reset to 0 on next increment
                break;
        }

        demoStep++;
    }

    // Optional: Monitor and display current state
    // Uncomment the following to print current state every 5 seconds
    /*
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus >= 5000) {
        lastStatus = millis();
        Serial.println();
        Serial.println("Current AC Status:");
        Serial.printf("  Mode: %s\n", ac.getModeName());
        Serial.printf("  Fan Speed: %s\n", ac.getFanSpeedName());
        Serial.printf("  Temperature: %d°C\n", ac.getTemperature());
        Serial.println();
    }
    */
}

// ============================================================================
// Additional Notes
// ============================================================================

/*
 * IMPORTANT: IR Code Customization
 *
 * The IR codes in this example are placeholders. To control your specific AC:
 *
 * 1. Use the ir_code_capture example to capture codes from your AC remote
 * 2. Test each button combination (all modes × fan speeds × temperatures)
 * 3. Replace the irCodes array with your captured values
 * 4. Update mode_names and fan_speed_names to match your AC
 * 5. Adjust max_temperature and min_temperature as needed
 *
 * Testing Your Codes:
 *
 * - Use a phone camera to verify IR LED is transmitting (you'll see it light up)
 * - Point IR LED directly at AC unit's receiver
 * - Increase range by using a transistor driver circuit
 * - If AC doesn't respond, verify code timing is correct
 *
 * Serial Commands (Future Enhancement):
 *
 * You can extend this example to accept serial commands for manual control:
 * - "temp 24" - Set temperature to 24°C
 * - "mode cool" - Set mode to cool
 * - "fan high" - Set fan speed to high
 * - "status" - Print current state
 */
