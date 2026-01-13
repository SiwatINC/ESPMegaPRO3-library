/**
 * @file basic_control.ino
 * @brief Basic Digital Output Control Example
 *
 * This example demonstrates basic digital output control using the DigitalOutputCard.
 * It shows how to:
 * - Initialize the card
 * - Turn outputs ON and OFF
 * - Read output states
 * - Use toggle functionality
 * - Control multiple outputs
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Digital Output Card (address 0x40)
 * - LED or other 12V load connected to pins 0-3
 *
 * Circuit:
 * - Connect loads between output pins and ground
 * - Ensure total current per group does not exceed 1.2A
 *
 * Created: 2025
 *
 * This example code is in the public domain.
 */

#include <DigitalOutputCard.hpp>

// Create card instance with I2C address 0x40
// Alternative: Use DIP switch constructor
// DigitalOutputCard card(false, false, false, false, false);
DigitalOutputCard card(0x40);

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    Serial.println("Digital Output Card - Basic Control Example");
    Serial.println("==========================================");

    // Initialize the card
    if (card.begin()) {
        Serial.println("Card initialized successfully");
    } else {
        Serial.println("Card initialization failed");
        while(1); // Halt if initialization fails
    }

    // Turn all outputs OFF at startup
    Serial.println("\nTurning all outputs OFF...");
    for (uint8_t pin = 0; pin < 16; pin++) {
        card.digitalWrite(pin, LOW);
    }
    delay(1000);

    // Demonstrate basic ON/OFF control
    demonstrateBasicControl();

    // Demonstrate reading states
    demonstrateStateReading();

    // Demonstrate toggle functionality
    demonstrateToggle();

    // Demonstrate group control
    demonstrateGroupControl();

    Serial.println("\nSetup complete. Starting main loop...");
}

void loop() {
    // Example 1: Simple blink on pin 0
    Serial.println("\nBlinking pin 0...");
    card.digitalWrite(0, HIGH);
    delay(1000);
    card.digitalWrite(0, LOW);
    delay(1000);

    // Example 2: Sequential activation
    Serial.println("Sequential activation of pins 0-3...");
    for (uint8_t pin = 0; pin < 4; pin++) {
        card.digitalWrite(pin, HIGH);
        Serial.printf("Pin %d ON\n", pin);
        delay(500);
    }

    delay(1000);

    // Turn all OFF
    for (uint8_t pin = 0; pin < 4; pin++) {
        card.digitalWrite(pin, LOW);
        Serial.printf("Pin %d OFF\n", pin);
        delay(500);
    }

    delay(2000);
}

/**
 * @brief Demonstrates basic ON/OFF control
 */
void demonstrateBasicControl() {
    Serial.println("\n=== Basic ON/OFF Control ===");

    // Turn pin 0 ON
    Serial.println("Turning pin 0 ON");
    card.digitalWrite(0, HIGH);
    delay(1000);

    // Turn pin 0 OFF
    Serial.println("Turning pin 0 OFF");
    card.digitalWrite(0, LOW);
    delay(1000);

    // Turn multiple pins ON
    Serial.println("Turning pins 0-3 ON");
    card.digitalWrite(0, HIGH);
    card.digitalWrite(1, HIGH);
    card.digitalWrite(2, HIGH);
    card.digitalWrite(3, HIGH);
    delay(2000);

    // Turn all OFF
    Serial.println("Turning all pins OFF");
    for (uint8_t pin = 0; pin < 4; pin++) {
        card.digitalWrite(pin, LOW);
    }
    delay(1000);
}

/**
 * @brief Demonstrates reading output states
 */
void demonstrateStateReading() {
    Serial.println("\n=== Reading Output States ===");

    // Set some outputs to different states
    card.digitalWrite(0, HIGH);
    card.digitalWrite(1, LOW);
    card.digitalWrite(2, HIGH);
    card.digitalWrite(3, LOW);

    // Read and display states
    Serial.println("Current output states:");
    for (uint8_t pin = 0; pin < 4; pin++) {
        bool state = card.getState(pin);
        Serial.printf("Pin %d: %s\n", pin, state ? "ON" : "OFF");
    }

    delay(2000);

    // Turn all OFF
    for (uint8_t pin = 0; pin < 4; pin++) {
        card.digitalWrite(pin, LOW);
    }
}

/**
 * @brief Demonstrates toggle functionality
 */
void demonstrateToggle() {
    Serial.println("\n=== Toggle Functionality ===");

    // Start with pin 0 OFF
    card.digitalWrite(0, LOW);
    Serial.printf("Pin 0 initial state: %s\n", card.getState(0) ? "ON" : "OFF");
    delay(1000);

    // Toggle several times
    for (int i = 0; i < 5; i++) {
        card.toggleState(0);
        Serial.printf("After toggle %d: %s\n", i + 1, card.getState(0) ? "ON" : "OFF");
        delay(500);
    }

    // Ensure pin is OFF
    card.digitalWrite(0, LOW);
    delay(1000);
}

/**
 * @brief Demonstrates controlling groups of outputs
 */
void demonstrateGroupControl() {
    Serial.println("\n=== Group Control ===");

    // Define groups (remember: hardware groups are 0-3, 4-7, 8-11, 12-15)
    // We'll use software groups for demonstration

    // Group 1: Pins 0-3
    Serial.println("Activating Group 1 (pins 0-3)");
    for (uint8_t pin = 0; pin < 4; pin++) {
        card.digitalWrite(pin, HIGH);
    }
    delay(2000);

    // Turn off Group 1
    Serial.println("Deactivating Group 1");
    for (uint8_t pin = 0; pin < 4; pin++) {
        card.digitalWrite(pin, LOW);
    }
    delay(500);

    // Group 2: Pins 4-7
    Serial.println("Activating Group 2 (pins 4-7)");
    for (uint8_t pin = 4; pin < 8; pin++) {
        card.digitalWrite(pin, HIGH);
    }
    delay(2000);

    // Turn off Group 2
    Serial.println("Deactivating Group 2");
    for (uint8_t pin = 4; pin < 8; pin++) {
        card.digitalWrite(pin, LOW);
    }
    delay(500);

    // Alternate groups
    Serial.println("Alternating groups...");
    for (int i = 0; i < 3; i++) {
        // Group 1 ON, Group 2 OFF
        for (uint8_t pin = 0; pin < 4; pin++) {
            card.digitalWrite(pin, HIGH);
            card.digitalWrite(pin + 4, LOW);
        }
        delay(500);

        // Group 1 OFF, Group 2 ON
        for (uint8_t pin = 0; pin < 4; pin++) {
            card.digitalWrite(pin, LOW);
            card.digitalWrite(pin + 4, HIGH);
        }
        delay(500);
    }

    // Turn all OFF
    Serial.println("Turning all outputs OFF");
    for (uint8_t pin = 0; pin < 16; pin++) {
        card.digitalWrite(pin, LOW);
    }
    delay(1000);
}
