/**
 * @file basic_reading.ino
 * @brief Basic example for reading digital inputs using DigitalInputCard
 *
 * This example demonstrates the fundamental operations of the DigitalInputCard:
 * - Initializing the card
 * - Reading individual pin states
 * - Reading bank buffers
 * - Polling in the main loop
 *
 * Hardware Requirements:
 * - ESPMegaPRO3 board
 * - DigitalInputCard expansion card
 * - Digital input sources (switches, buttons, sensors, etc.)
 *
 * Wiring:
 * - Connect digital input sources to pins 0-15 on the DigitalInputCard
 * - Configure the DIP switch on the card (or note the I2C addresses)
 *
 * @author Siwat Sirichai
 * @date 2025
 */

#include <DigitalInputCard.hpp>

// Create a DigitalInputCard instance
// Method 1: Using I2C addresses directly
// Uncomment the following line and comment out the DIP switch constructor
// DigitalInputCard inputCard(0x20, 0x21);  // Bank A at 0x20, Bank B at 0x21

// Method 2: Using DIP switch configuration (recommended)
// Configure based on your DIP switch settings
// In this example: all switches are OFF (false, false, false, false, false, false)
DigitalInputCard inputCard(false, false, false, false, false, false);

// Variables for timing
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 500; // Print every 500ms

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== DigitalInputCard Basic Reading Example ===");
    Serial.println();

    // Initialize the input card
    Serial.print("Initializing Digital Input Card... ");
    if (inputCard.begin()) {
        Serial.println("SUCCESS");
    } else {
        Serial.println("FAILED");
        Serial.println("Please check:");
        Serial.println("  1. I2C wiring (SDA, SCL)");
        Serial.println("  2. Card power supply");
        Serial.println("  3. I2C address configuration");
        while (1) {
            delay(1000);
        }
    }

    Serial.println();
    Serial.println("Card initialized successfully!");
    Serial.println("Reading inputs...");
    Serial.println();

    // Preload the input buffer to avoid false triggers on startup
    inputCard.preloadInputBuffer();
}

void loop() {
    // Call the card's loop function to update internal state
    // This is essential for proper operation
    inputCard.loop();

    // Print input states periodically
    if (millis() - lastPrintTime >= PRINT_INTERVAL) {
        lastPrintTime = millis();
        printInputStates();
    }

    // You can read individual pins anytime
    // Example: Check if pin 0 is HIGH
    if (inputCard.digitalRead(0)) {
        // Pin 0 is HIGH - do something
        // Note: This will be printed every 500ms if pin 0 is HIGH
    }
}

/**
 * @brief Print the state of all input pins
 *
 * This function demonstrates different methods of reading inputs:
 * - Individual pin reading
 * - Bank buffer reading
 */
void printInputStates() {
    Serial.println("====================================");
    Serial.print("Time: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println();

    // Method 1: Read individual pins (refreshes buffer each time)
    Serial.println("Individual Pin States:");
    Serial.print("  Pins 0-7:  ");
    for (int i = 0; i < 8; i++) {
        Serial.print("P");
        Serial.print(i);
        Serial.print("=");
        Serial.print(inputCard.digitalRead(i) ? "HIGH " : "LOW  ");
    }
    Serial.println();

    Serial.print("  Pins 8-15: ");
    for (int i = 8; i < 16; i++) {
        Serial.print("P");
        Serial.print(i);
        Serial.print("=");
        Serial.print(inputCard.digitalRead(i) ? "HIGH " : "LOW  ");
    }
    Serial.println();
    Serial.println();

    // Method 2: Read bank buffers (more efficient for multiple pins)
    Serial.println("Bank Buffers:");

    uint8_t bankA = inputCard.getInputBufferA();
    Serial.print("  Bank A (Pins 0-7):  0b");
    Serial.print(bankA, BIN);
    Serial.print(" (0x");
    Serial.print(bankA, HEX);
    Serial.println(")");

    uint8_t bankB = inputCard.getInputBufferB();
    Serial.print("  Bank B (Pins 8-15): 0b");
    Serial.print(bankB, BIN);
    Serial.print(" (0x");
    Serial.print(bankB, HEX);
    Serial.println(")");
    Serial.println();

    // Method 3: Using bank buffer to check individual pins (most efficient)
    Serial.println("Checking specific pins from buffers:");

    // Check if pin 0 is HIGH using bank buffer
    if (bankA & 0x80) {  // Bit 7 corresponds to pin 0
        Serial.println("  Pin 0 is HIGH (from buffer)");
    }

    // Check if pin 5 is HIGH using bank buffer
    if (bankA & 0x04) {  // Bit 2 corresponds to pin 5
        Serial.println("  Pin 5 is HIGH (from buffer)");
    }

    // Check if pin 10 is HIGH using bank buffer
    if (bankB & 0x20) {  // Bit 5 corresponds to pin 10
        Serial.println("  Pin 10 is HIGH (from buffer)");
    }

    Serial.println();
}

/**
 * UNDERSTANDING THE OUTPUT:
 *
 * Individual Pin States:
 * - Shows each pin (0-15) as either HIGH or LOW
 * - Uses digitalRead() which refreshes the buffer
 *
 * Bank Buffers:
 * - Bank A: Pins 0-7 as an 8-bit value
 * - Bank B: Pins 8-15 as an 8-bit value
 * - Binary representation: bit 7 = pin 0, bit 6 = pin 1, etc.
 * - Hexadecimal representation: compact form of the binary value
 *
 * PERFORMANCE TIPS:
 *
 * 1. For reading multiple pins, use getInputBufferA/B() instead of
 *    multiple digitalRead() calls to minimize I2C transactions
 *
 * 2. Use digitalRead(pin, false) to read without refreshing if you've
 *    already called loop() or another read operation
 *
 * 3. The loop() function refreshes both banks automatically, so reading
 *    with refresh=false after loop() gives you fresh data
 *
 * NEXT STEPS:
 *
 * 1. Try connecting switches/buttons to different pins
 * 2. Observe how the states change in real-time
 * 3. Experiment with reading specific pins vs. bank buffers
 * 4. Move on to the callbacks example for event-driven programming
 */
