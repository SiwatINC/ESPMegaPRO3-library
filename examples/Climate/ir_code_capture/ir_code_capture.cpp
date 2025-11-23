/**
 * @file ir_code_capture.ino
 * @brief IR code capture utility for ClimateCard setup
 *
 * This utility helps you capture IR codes from your existing air conditioner
 * remote control. Use this to build the IR code database needed for the
 * ClimateCard to control your specific AC model.
 *
 * Hardware Required:
 * - ESP32 board
 * - IR Receiver module (e.g., TSOP38238, VS1838B) connected to GPIO 15
 * - Your AC remote control
 *
 * IR Receiver Wiring:
 *   GPIO 15 <--> IR Receiver Data/Out Pin
 *   3.3V or 5V --> IR Receiver VCC
 *   GND --> IR Receiver GND
 *
 * Usage Instructions:
 * 1. Upload this sketch to your ESP32
 * 2. Open Serial Monitor (115200 baud)
 * 3. Follow the on-screen prompts
 * 4. Systematically capture all combinations:
 *    - Each mode (off, cool, heat, fan, etc.)
 *    - Each fan speed (auto, low, medium, high, etc.)
 *    - Each temperature setting (16°C - 30°C)
 * 5. Copy the generated code arrays to your ClimateCard sketch
 *
 * Process for Each Code:
 * 1. Type 'start' and press Enter
 * 2. Point remote at receiver (5-10cm away)
 * 3. Press the desired button on your remote
 * 4. Type 'stop' and press Enter
 * 5. Code will be displayed in C array format
 * 6. Copy and save the code with a label (e.g., "Cool_Auto_24C")
 *
 * Tips:
 * - Keep a spreadsheet to track which codes correspond to which settings
 * - Capture codes in a systematic order (all temps for one mode/fan combo)
 * - Some remotes send different codes for same setting - capture multiple times
 * - Test captured codes by using the ir_test command
 *
 * @note Make sure your AC remote uses 38kHz carrier frequency (most common)
 */

#include <Arduino.h>
#include <IRReceiver.hpp>
#include <IRBlaster.hpp>

// ============================================================================
// Configuration
// ============================================================================

#define IR_RX_PIN 15        // GPIO pin for IR receiver
#define IR_TX_PIN 14        // GPIO pin for IR LED (for testing)
#define RMT_CHANNEL RMT_CHANNEL_0

// ============================================================================
// Global Variables
// ============================================================================

IRBlaster irBlaster(IR_TX_PIN, RMT_CHANNEL);

// Storage for captured codes
#define MAX_STORED_CODES 100
struct StoredCode {
    String label;
    uint16_t *data;
    size_t size;
};

StoredCode storedCodes[MAX_STORED_CODES];
uint8_t storedCodeCount = 0;

bool capturing = false;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Print IR code in C array format
 */
void printCodeArray(const char *name, uint16_t *data, size_t size) {
    Serial.println();
    Serial.println("// ============================================");
    Serial.printf("// IR Code: %s\n", name);
    Serial.printf("// Elements: %d\n", size);
    Serial.printf("// Bytes: %d\n", size * sizeof(uint16_t));
    Serial.println("// ============================================");
    Serial.printf("const uint16_t %s[%d] = {\n    ", name, size);

    for (size_t i = 0; i < size; i++) {
        Serial.print(data[i]);

        if (i < size - 1) {
            Serial.print(", ");

            // New line every 10 values for readability
            if ((i + 1) % 10 == 0) {
                Serial.println();
                Serial.print("    ");
            }
        }
    }

    Serial.println();
    Serial.println("};");
    Serial.println();
}

/**
 * Store captured code for later retrieval
 */
bool storeCode(const char *label, uint16_t *data, size_t size) {
    if (storedCodeCount >= MAX_STORED_CODES) {
        Serial.println("ERROR: Storage full! Maximum codes reached.");
        return false;
    }

    // Allocate memory for code
    storedCodes[storedCodeCount].data = (uint16_t *)malloc(size * sizeof(uint16_t));
    if (storedCodes[storedCodeCount].data == nullptr) {
        Serial.println("ERROR: Memory allocation failed!");
        return false;
    }

    // Copy code data
    memcpy(storedCodes[storedCodeCount].data, data, size * sizeof(uint16_t));
    storedCodes[storedCodeCount].size = size;
    storedCodes[storedCodeCount].label = String(label);

    storedCodeCount++;
    return true;
}

/**
 * List all stored codes
 */
void listStoredCodes() {
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║              Stored IR Codes                      ║");
    Serial.println("╠════════════════════════════════════════════════════╣");

    if (storedCodeCount == 0) {
        Serial.println("║  No codes stored                                  ║");
    } else {
        Serial.printf("║  Total: %d codes                                    ║\n", storedCodeCount);
        Serial.println("╠════════════════════════════════════════════════════╣");

        for (uint8_t i = 0; i < storedCodeCount; i++) {
            Serial.printf("║  [%2d] %-35s %4d el ║\n",
                          i,
                          storedCodes[i].label.c_str(),
                          storedCodes[i].size);
        }
    }

    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

/**
 * Export all codes as C array structure
 */
void exportAllCodes() {
    Serial.println("\n// ============================================");
    Serial.println("// All Captured IR Codes");
    Serial.println("// ============================================");
    Serial.println();

    for (uint8_t i = 0; i < storedCodeCount; i++) {
        char codeName[64];
        // Replace spaces with underscores for valid C identifier
        String safeName = storedCodes[i].label;
        safeName.replace(" ", "_");
        safeName.replace("-", "_");
        safeName.replace("/", "_");

        snprintf(codeName, sizeof(codeName), "irCode_%s", safeName.c_str());

        printCodeArray(codeName, storedCodes[i].data, storedCodes[i].size);
    }

    Serial.println("// ============================================");
    Serial.println("// Usage in ClimateCard:");
    Serial.println("// ============================================");
    Serial.println("// Copy the arrays above, then organize them into");
    Serial.println("// a multi-dimensional array like this:");
    Serial.println("//");
    Serial.println("// const uint16_t irCodes[modes][fan_speeds][temperatures][max_size] = {");
    Serial.println("//     // Mode 0 (e.g., Off)");
    Serial.println("//     {");
    Serial.println("//         {{/* code for mode0, fan0, temp0 */}},");
    Serial.println("//         // ... more combinations");
    Serial.println("//     },");
    Serial.println("//     // More modes...");
    Serial.println("// };");
    Serial.println();
}

/**
 * Test a stored code by transmitting it
 */
void testCode(uint8_t index) {
    if (index >= storedCodeCount) {
        Serial.println("ERROR: Invalid code index!");
        return;
    }

    Serial.printf("Testing code [%d]: %s\n", index, storedCodes[index].label.c_str());
    Serial.println("IR LED should transmit now...");

    irBlaster.send(storedCodes[index].data, storedCodes[index].size);

    Serial.println("Transmission complete!");
    Serial.println("Did your AC respond?");
}

/**
 * Interactive code naming
 */
String promptForCodeName() {
    Serial.println("\nEnter a name for this code (e.g., 'Cool_Auto_24C'):");
    Serial.println("(or press Enter to use auto-generated name)");

    // Wait for input
    while (!Serial.available()) {
        delay(10);
    }

    String name = Serial.readStringUntil('\n');
    name.trim();

    if (name.length() == 0) {
        // Auto-generate name
        char autoName[32];
        snprintf(autoName, sizeof(autoName), "Code_%03d", storedCodeCount + 1);
        name = String(autoName);
        Serial.printf("Using auto-generated name: %s\n", name.c_str());
    }

    return name;
}

/**
 * Display help menu
 */
void displayHelp() {
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║         IR Code Capture - Commands                ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  start               - Start capturing IR code    ║");
    Serial.println("║  stop                - Stop capture and display   ║");
    Serial.println("║  list                - List all stored codes      ║");
    Serial.println("║  export              - Export all codes as C code ║");
    Serial.println("║  test <index>        - Test a stored code         ║");
    Serial.println("║  print <index>       - Print a specific code      ║");
    Serial.println("║  clear               - Clear all stored codes     ║");
    Serial.println("║  help                - Show this help menu        ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  Workflow:                                        ║");
    Serial.println("║    1. Type 'start'                                ║");
    Serial.println("║    2. Press button on AC remote                   ║");
    Serial.println("║    3. Type 'stop'                                 ║");
    Serial.println("║    4. Name the code                               ║");
    Serial.println("║    5. Repeat for all combinations                 ║");
    Serial.println("║    6. Type 'export' to get all codes              ║");
    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

/**
 * Template for systematic capture
 */
void displayCaptureTemplate() {
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║       Systematic Capture Template                 ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  Suggested naming convention:                     ║");
    Serial.println("║    <Mode>_<FanSpeed>_<Temp>                       ║");
    Serial.println("║                                                    ║");
    Serial.println("║  Examples:                                        ║");
    Serial.println("║    Off_Auto_16                                    ║");
    Serial.println("║    Cool_Auto_16                                   ║");
    Serial.println("║    Cool_Auto_17                                   ║");
    Serial.println("║    Cool_Auto_18                                   ║");
    Serial.println("║    ...                                            ║");
    Serial.println("║    Cool_Low_16                                    ║");
    Serial.println("║    Cool_Low_17                                    ║");
    Serial.println("║    ...                                            ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  Typical AC has:                                  ║");
    Serial.println("║    Modes: 3-6 (Off, Cool, Heat, Fan, Dry, Auto)   ║");
    Serial.println("║    Fan Speeds: 3-5 (Auto, Low, Medium, High)      ║");
    Serial.println("║    Temperatures: 15 (16°C - 30°C)                 ║");
    Serial.println("║                                                    ║");
    Serial.println("║  Total codes needed: modes × fans × temps         ║");
    Serial.println("║  Example: 3 × 4 × 15 = 180 codes                  ║");
    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

/**
 * Process serial commands
 */
void processCommand() {
    if (!Serial.available()) return;

    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();

    if (command == "start") {
        Serial.println("\n>>> Starting IR capture...");
        Serial.println(">>> Point remote at receiver and press button NOW!");
        Serial.println(">>> (Type 'stop' when done)");
        IRReceiver::start_long_receive();
        capturing = true;
    }
    else if (command == "stop") {
        if (!capturing) {
            Serial.println("ERROR: Not currently capturing. Type 'start' first.");
            return;
        }

        Serial.println("\n>>> Stopping capture...");
        ir_data_t received = IRReceiver::end_long_receive();
        capturing = false;

        if (received.size == 0) {
            Serial.println(">>> ERROR: No data captured!");
            Serial.println(">>> Make sure:");
            Serial.println(">>>   - IR receiver is connected correctly");
            Serial.println(">>>   - Remote is pointed at receiver");
            Serial.println(">>>   - Remote batteries are good");
        } else {
            Serial.println(">>> Capture complete!");
            Serial.printf(">>> Captured %d timing values\n", received.size);

            // Get name for this code
            String codeName = promptForCodeName();

            // Store the code
            if (storeCode(codeName.c_str(), received.data, received.size)) {
                Serial.printf(">>> Code stored as: %s\n", codeName.c_str());

                // Display the code
                printCodeArray(codeName.c_str(), received.data, received.size);
            }

            // Note: Memory is managed by stored codes, don't free here
        }

        Serial.println("\n>>> Type 'start' to capture another code");
    }
    else if (command == "list") {
        listStoredCodes();
    }
    else if (command == "export") {
        if (storedCodeCount == 0) {
            Serial.println("No codes to export. Capture some codes first!");
        } else {
            exportAllCodes();
        }
    }
    else if (command == "clear") {
        Serial.printf("Clearing %d stored codes...\n", storedCodeCount);
        for (uint8_t i = 0; i < storedCodeCount; i++) {
            free(storedCodes[i].data);
        }
        storedCodeCount = 0;
        Serial.println("All codes cleared!");
    }
    else if (command.startsWith("test ")) {
        uint8_t index = command.substring(5).toInt();
        testCode(index);
    }
    else if (command.startsWith("print ")) {
        uint8_t index = command.substring(6).toInt();
        if (index >= storedCodeCount) {
            Serial.println("ERROR: Invalid code index!");
        } else {
            printCodeArray(storedCodes[index].label.c_str(),
                           storedCodes[index].data,
                           storedCodes[index].size);
        }
    }
    else if (command == "template") {
        displayCaptureTemplate();
    }
    else if (command == "help") {
        displayHelp();
    }
    else {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════╗");
    Serial.println("║         IR Code Capture Utility v1.0              ║");
    Serial.println("║           for ESPMegaPRO ClimateCard               ║");
    Serial.println("╚════════════════════════════════════════════════════╝");
    Serial.println();

    // Initialize IR receiver
    Serial.println("Initializing IR receiver...");
    Serial.printf("  RX Pin: GPIO %d\n", IR_RX_PIN);
    IRReceiver::begin(IR_RX_PIN);
    Serial.println("  ✓ IR receiver ready");

    // Initialize IR blaster for testing
    Serial.println("Initializing IR transmitter for testing...");
    Serial.printf("  TX Pin: GPIO %d\n", IR_TX_PIN);
    Serial.println("  ✓ IR transmitter ready");

    Serial.println();
    Serial.println("════════════════════════════════════════════════════");
    Serial.println();

    // Display initial help
    Serial.println("Welcome to the IR Code Capture Utility!");
    Serial.println();
    Serial.println("This tool helps you capture IR codes from your AC remote");
    Serial.println("to use with the ClimateCard library.");
    Serial.println();

    displayHelp();
    displayCaptureTemplate();

    Serial.println("Ready to capture! Type 'start' to begin.");
    Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    processCommand();

    // Indicate capturing status with LED (if available)
    static unsigned long lastBlink = 0;
    if (capturing && millis() - lastBlink >= 500) {
        lastBlink = millis();
        // Optionally blink an LED to indicate capturing
        // digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
}

// ============================================================================
// Example Capture Session
// ============================================================================

/*
Typical capture session:

>>> start
>>> Starting IR capture...
>>> Point remote at receiver and press button NOW!
>>> (Type 'stop' when done)

[Point remote and press "Cool, Auto, 24°C" button]

>>> stop
>>> Stopping capture...
>>> Capture complete!
>>> Captured 67 timing values

Enter a name for this code (e.g., 'Cool_Auto_24C'):
Cool_Auto_24

>>> Code stored as: Cool_Auto_24

// ============================================
// IR Code: Cool_Auto_24
// Elements: 67
// Bytes: 134
// ============================================
const uint16_t Cool_Auto_24[67] = {
    9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    // ... rest of code ...
};

>>> Type 'start' to capture another code

>>> start
[Capture next code...]

[After capturing all codes:]

>>> export

// ============================================
// All Captured IR Codes
// ============================================

const uint16_t irCode_Cool_Auto_24[67] = {
    9000, 4500, 560, 560, ...
};

const uint16_t irCode_Cool_Auto_25[67] = {
    9000, 4500, 560, 560, ...
};

// ... all other codes ...

*/

// ============================================================================
// Tips for Success
// ============================================================================

/*
 * Hardware Tips:
 * - Use TSOP38238, VS1838B, or similar 38kHz IR receiver
 * - Keep receiver away from bright lights (IR interference)
 * - Point remote 5-10cm from receiver
 * - Make sure receiver is powered correctly
 *
 * Capture Tips:
 * - Capture each code 2-3 times to verify consistency
 * - Some remotes send different codes for same button - this is normal
 * - Keep detailed notes of what each code does
 * - Start with one mode and capture all temps for all fan speeds
 * - Use spreadsheet to track: Mode, Fan, Temp, Code Name
 *
 * Testing Tips:
 * - Test each captured code immediately using 'test' command
 * - Point IR LED at AC unit and verify it responds correctly
 * - If code doesn't work, recapture it
 * - Some ACs require exact timing - try capturing multiple times
 *
 * Organization Tips:
 * - Use consistent naming: Mode_Fan_Temp
 * - Keep backup of all codes (copy serial output to file)
 * - Document your AC model and remote model number
 * - Note any special features (turbo, sleep, swing, etc.)
 *
 * Common Issues:
 * - "No data captured": Check receiver wiring, try different pin
 * - Inconsistent codes: Try fresh batteries in remote
 * - Codes don't work: Verify carrier frequency is 38kHz
 * - Too many/few values: Normal variation, use most common length
 */
