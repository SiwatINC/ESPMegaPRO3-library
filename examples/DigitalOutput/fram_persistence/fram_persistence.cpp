/**
 * @file fram_persistence.ino
 * @brief FRAM Persistence Example
 *
 * This example demonstrates FRAM persistence for the DigitalOutputCard.
 * It shows how to:
 * - Bind FRAM to the card
 * - Save and load output states
 * - Use auto-save functionality
 * - Recover from power loss
 * - Selectively save states and values
 *
 * Hardware Required:
 * - ESPMegaPRO board with FRAM
 * - Digital Output Card (address 0x40)
 * - LEDs or other 12V loads for visual feedback
 *
 * FRAM Memory Usage:
 * - 34 bytes total per card
 * - Bytes 0-1: State bitmap (16 bits)
 * - Bytes 2-33: PWM values (16 × 2 bytes)
 *
 * How to Test:
 * 1. Upload and run the sketch
 * 2. Let it set some outputs and save to FRAM
 * 3. Reset or power cycle the board
 * 4. Observe that outputs restore to previous state
 *
 * Created: 2025
 *
 * This example code is in the public domain.
 */

#include <DigitalOutputCard.hpp>
#include <FRAM.h>

// Create card and FRAM instances
DigitalOutputCard card(0x40);
FRAM fram;

// FRAM configuration
const uint16_t FRAM_ADDRESS = 0x0000;  // Starting address in FRAM
const bool USE_AUTO_SAVE = false;      // Set to true for automatic saving

// Track whether this is first boot or recovery
bool isRecoveryBoot = false;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000); // Allow serial to stabilize

    Serial.println("Digital Output Card - FRAM Persistence Example");
    Serial.println("==============================================");

    // Initialize FRAM
    Serial.println("\nInitializing FRAM...");
    if (!fram.begin()) {
        Serial.println("FRAM initialization failed!");
        while(1);
    }
    Serial.println("FRAM initialized successfully");

    // Initialize the card
    Serial.println("Initializing Digital Output Card...");
    if (!card.begin()) {
        Serial.println("Card initialization failed!");
        while(1);
    }
    Serial.println("Card initialized successfully");

    // Bind FRAM to the card
    Serial.printf("Binding FRAM at address 0x%04X\n", FRAM_ADDRESS);
    card.bindFRAM(&fram, FRAM_ADDRESS);

    // Configure auto-save
    card.setAutoSaveToFRAM(USE_AUTO_SAVE);
    if (USE_AUTO_SAVE) {
        Serial.println("Auto-save to FRAM: ENABLED");
    } else {
        Serial.println("Auto-save to FRAM: DISABLED (manual save required)");
    }

    // Check if we're recovering from power loss
    // We'll check if FRAM has valid data by reading a known pattern
    // For this example, we'll just try to load and see if values are reasonable
    Serial.println("\nAttempting to load previous state from FRAM...");
    card.loadFromFRAM();

    // Check if any outputs are ON (indicates previous state exists)
    bool hasData = false;
    for (uint8_t pin = 0; pin < 16; pin++) {
        if (card.getState(pin) || card.getValue(pin) > 0) {
            hasData = true;
            break;
        }
    }

    if (hasData) {
        Serial.println("Previous state detected! Recovering...");
        isRecoveryBoot = true;
        displayCurrentState();
    } else {
        Serial.println("No previous state found. Starting fresh...");
        isRecoveryBoot = false;
        setupInitialState();
    }

    Serial.println("\nSetup complete.");
    Serial.println("Commands:");
    Serial.println("  's' - Save current state to FRAM");
    Serial.println("  'l' - Load state from FRAM");
    Serial.println("  'd' - Display current state");
    Serial.println("  'c' - Clear all outputs");
    Serial.println("  't' - Run test pattern");
    Serial.println();
}

void loop() {
    // Check for serial commands
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        handleCommand(cmd);
    }

    // If not in recovery mode, run automatic test
    if (!isRecoveryBoot) {
        runAutomaticTest();
    }

    delay(100);
}

/**
 * @brief Set up initial state for first boot
 */
void setupInitialState() {
    Serial.println("\nSetting up initial state...");

    // Set some outputs to different states and values
    Serial.println("Pin 0: ON at 100% (4095)");
    card.analogWrite(0, 4095);

    Serial.println("Pin 1: ON at 75% (3072)");
    card.analogWrite(1, 3072);

    Serial.println("Pin 2: ON at 50% (2048)");
    card.analogWrite(2, 2048);

    Serial.println("Pin 3: ON at 25% (1024)");
    card.analogWrite(3, 1024);

    // Save to FRAM
    Serial.println("\nSaving initial state to FRAM...");
    card.saveToFRAM();
    Serial.println("State saved!");

    Serial.println("\nTry resetting the board to see state recovery!");
}

/**
 * @brief Display current state of all outputs
 */
void displayCurrentState() {
    Serial.println("\n=== Current Output States ===");
    Serial.println("Pin | State | Value | Actual");
    Serial.println("----|-------|-------|-------");

    for (uint8_t pin = 0; pin < 16; pin++) {
        bool state = card.getState(pin);
        uint16_t value = card.getValue(pin);
        uint16_t actual = state ? value : 0;

        Serial.printf(" %2d |  %s  | %4d  | %4d\n",
                     pin,
                     state ? "ON " : "OFF",
                     value,
                     actual);
    }
    Serial.println();
}

/**
 * @brief Handle serial commands
 *
 * @param cmd Command character
 */
void handleCommand(char cmd) {
    switch(cmd) {
        case 's':
        case 'S':
            Serial.println("\n>> Saving to FRAM...");
            card.saveToFRAM();
            Serial.println("State saved!");
            break;

        case 'l':
        case 'L':
            Serial.println("\n>> Loading from FRAM...");
            card.loadFromFRAM();
            Serial.println("State loaded!");
            displayCurrentState();
            break;

        case 'd':
        case 'D':
            displayCurrentState();
            break;

        case 'c':
        case 'C':
            Serial.println("\n>> Clearing all outputs...");
            for (uint8_t pin = 0; pin < 16; pin++) {
                card.analogWrite(pin, 0);
            }
            Serial.println("All outputs OFF");
            if (!USE_AUTO_SAVE) {
                Serial.println("Remember to save with 's' if you want to persist this!");
            }
            break;

        case 't':
        case 'T':
            Serial.println("\n>> Running test pattern...");
            runTestPattern();
            break;

        default:
            // Ignore unknown commands
            break;
    }
}

/**
 * @brief Run a test pattern
 */
void runTestPattern() {
    Serial.println("Setting test pattern...");

    // Create a walking pattern
    for (uint8_t pin = 0; pin < 8; pin++) {
        // Turn on current pin
        card.analogWrite(pin, 2048);

        // Turn off previous pin
        if (pin > 0) {
            card.analogWrite(pin - 1, 0);
        }

        delay(200);
    }

    // Turn off last pin
    card.analogWrite(7, 0);

    if (!USE_AUTO_SAVE) {
        Serial.println("Test complete. Use 's' to save if desired.");
    }
}

/**
 * @brief Run automatic test cycle
 */
void runAutomaticTest() {
    static unsigned long lastTest = 0;
    static uint8_t testPhase = 0;

    unsigned long now = millis();

    // Run test every 10 seconds
    if (now - lastTest >= 10000) {
        lastTest = now;

        Serial.printf("\n>> Running automatic test phase %d...\n", testPhase);

        switch(testPhase) {
            case 0:
                // Phase 0: Set gradient
                Serial.println("Setting brightness gradient");
                for (uint8_t pin = 0; pin < 8; pin++) {
                    uint16_t value = (pin + 1) * 512; // 512, 1024, 1536, ...
                    card.analogWrite(pin, value);
                }
                card.saveToFRAM();
                Serial.println("Saved to FRAM");
                break;

            case 1:
                // Phase 1: Toggle states
                Serial.println("Toggling states");
                for (uint8_t pin = 0; pin < 8; pin++) {
                    card.toggleState(pin);
                }
                card.saveStateToFRAM();
                Serial.println("Saved states to FRAM");
                break;

            case 2:
                // Phase 2: Change values
                Serial.println("Changing values");
                for (uint8_t pin = 0; pin < 8; pin++) {
                    card.setValue(pin, 4095 - pin * 512);
                }
                // Save each pin individually (demonstration)
                for (uint8_t pin = 0; pin < 8; pin++) {
                    card.savePinValueToFRAM(pin);
                }
                Serial.println("Saved individual values to FRAM");
                break;

            case 3:
                // Phase 3: All ON at full brightness
                Serial.println("All ON at full brightness");
                for (uint8_t pin = 0; pin < 8; pin++) {
                    card.analogWrite(pin, 4095);
                }
                card.saveToFRAM();
                Serial.println("Saved to FRAM");
                break;

            case 4:
                // Phase 4: All OFF
                Serial.println("All OFF");
                for (uint8_t pin = 0; pin < 8; pin++) {
                    card.analogWrite(pin, 0);
                }
                card.saveToFRAM();
                Serial.println("Saved to FRAM");
                break;
        }

        displayCurrentState();

        testPhase = (testPhase + 1) % 5;

        if (testPhase == 0) {
            Serial.println("\n>>> TEST CYCLE COMPLETE <<<");
            Serial.println(">>> Try resetting the board now to see recovery! <<<\n");
        }
    }
}

/**
 * @brief Demonstrate selective saving
 */
void demonstrateSelectiveSaving() {
    Serial.println("\n=== Selective Saving Demo ===");

    // Set different values
    card.analogWrite(0, 1024);
    card.analogWrite(1, 2048);
    card.analogWrite(2, 3072);

    // Save only states
    Serial.println("Saving only states...");
    card.saveStateToFRAM();

    // Change values but don't save
    card.setValue(0, 4095);
    card.setValue(1, 4095);

    // Save only pin 2's value
    Serial.println("Saving only pin 2's value...");
    card.savePinValueToFRAM(2);

    // Now if we load, pins 0 and 1 will have old values, pin 2 will have new value
    Serial.println("Loading from FRAM...");
    card.loadFromFRAM();

    displayCurrentState();
}
