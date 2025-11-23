/**
 * @file with_callbacks.ino
 * @brief Event-driven example using callbacks with DigitalInputCard
 *
 * This example demonstrates the callback system for event-driven programming:
 * - Registering callback functions
 * - Handling pin state changes automatically
 * - Using multiple callbacks
 * - Using lambda functions
 * - Managing callback handlers
 *
 * Benefits of callbacks vs. polling:
 * - More efficient (react only to changes)
 * - Cleaner code structure
 * - No need to track previous states manually
 * - Automatic debouncing built-in
 *
 * Hardware Requirements:
 * - ESPMegaPRO3 board
 * - DigitalInputCard expansion card
 * - Buttons or switches on pins 0-3
 *
 * @author Siwat Sirichai
 * @date 2025
 */

#include <DigitalInputCard.hpp>

// Create a DigitalInputCard instance
DigitalInputCard inputCard(false, false, false, false, false, false);

// Variables for callback demonstration
volatile uint32_t pin0PressCount = 0;
volatile uint32_t pin1PressCount = 0;

// Callback handler IDs (for later unregistering if needed)
uint8_t mainCallbackHandler = 0;
uint8_t pin0CallbackHandler = 0;
uint8_t statisticsCallbackHandler = 0;

// ============================================================
// CALLBACK FUNCTION 1: Main callback for all pins
// ============================================================
/**
 * @brief Main callback function that handles all pin changes
 *
 * This callback is called whenever ANY pin changes state.
 * It demonstrates how to handle different pins differently.
 *
 * @param pin The pin number that changed (0-15)
 * @param state The new state of the pin (true=HIGH, false=LOW)
 */
void onInputChange(uint8_t pin, bool state) {
    // Print timestamp and pin info
    Serial.print("[");
    Serial.print(millis());
    Serial.print("ms] Main Callback: Pin ");
    Serial.print(pin);
    Serial.print(" changed to ");
    Serial.println(state ? "HIGH" : "LOW");

    // Handle specific pins differently
    switch(pin) {
        case 0:
            if (state) {
                Serial.println("  -> Button 0 PRESSED");
            } else {
                Serial.println("  -> Button 0 RELEASED");
            }
            break;

        case 1:
            if (state) {
                Serial.println("  -> Button 1 PRESSED");
            } else {
                Serial.println("  -> Button 1 RELEASED");
            }
            break;

        case 2:
            if (state) {
                Serial.println("  -> Switch 2 turned ON");
            } else {
                Serial.println("  -> Switch 2 turned OFF");
            }
            break;

        case 3:
            if (state) {
                Serial.println("  -> Sensor 3 TRIGGERED");
            } else {
                Serial.println("  -> Sensor 3 CLEARED");
            }
            break;

        default:
            // Other pins - generic handling
            Serial.print("  -> Generic input on pin ");
            Serial.println(pin);
            break;
    }
}

// ============================================================
// CALLBACK FUNCTION 2: Pin-specific callback
// ============================================================
/**
 * @brief Callback specifically for pin 0 press counting
 *
 * This demonstrates using multiple callbacks.
 * Both this and the main callback will be called when pin 0 changes.
 *
 * @param pin The pin number that changed
 * @param state The new state of the pin
 */
void onPin0Change(uint8_t pin, bool state) {
    // Only handle pin 0
    if (pin == 0 && state) {
        pin0PressCount++;
        Serial.print("  >> Pin 0 press count: ");
        Serial.println(pin0PressCount);
    }
}

// ============================================================
// CALLBACK FUNCTION 3: Statistics callback
// ============================================================
/**
 * @brief Callback that maintains statistics
 *
 * This callback counts presses on pins 0 and 1.
 * Demonstrates how multiple callbacks can process the same event.
 *
 * @param pin The pin number that changed
 * @param state The new state of the pin
 */
void onStatisticsUpdate(uint8_t pin, bool state) {
    if (state) {  // Only count HIGH transitions (presses)
        if (pin == 0) {
            // Already counted in pin0 callback
        } else if (pin == 1) {
            pin1PressCount++;
        }
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== DigitalInputCard Callbacks Example ===");
    Serial.println();

    // Initialize the input card
    Serial.print("Initializing Digital Input Card... ");
    if (!inputCard.begin()) {
        Serial.println("FAILED");
        while (1) delay(1000);
    }
    Serial.println("SUCCESS");

    // Preload input buffer to avoid false triggers on startup
    inputCard.preloadInputBuffer();

    // ========================================
    // Register callbacks
    // ========================================
    Serial.println();
    Serial.println("Registering callbacks...");

    // Register the main callback for all pins
    mainCallbackHandler = inputCard.registerCallback(onInputChange);
    Serial.print("  Main callback registered with handler ID: ");
    Serial.println(mainCallbackHandler);

    // Register pin-specific callback
    pin0CallbackHandler = inputCard.registerCallback(onPin0Change);
    Serial.print("  Pin 0 callback registered with handler ID: ");
    Serial.println(pin0CallbackHandler);

    // Register statistics callback
    statisticsCallbackHandler = inputCard.registerCallback(onStatisticsUpdate);
    Serial.print("  Statistics callback registered with handler ID: ");
    Serial.println(statisticsCallbackHandler);

    // ========================================
    // Demonstrate lambda function callback
    // ========================================
    // Lambda functions are great for simple inline callbacks
    uint8_t lambdaHandler = inputCard.registerCallback([](uint8_t pin, bool state) {
        // This lambda only handles pin 2
        if (pin == 2) {
            Serial.println("  >> Lambda: Pin 2 event detected!");
        }
    });
    Serial.print("  Lambda callback registered with handler ID: ");
    Serial.println(lambdaHandler);

    Serial.println();
    Serial.println("All callbacks registered!");
    Serial.println("Waiting for input changes...");
    Serial.println();
    Serial.println("Try pressing buttons or toggling switches on pins 0-3");
    Serial.println("Notice how multiple callbacks can handle the same event");
    Serial.println();
}

void loop() {
    // Call the card's loop function to process inputs and trigger callbacks
    // This MUST be called regularly for callbacks to work
    inputCard.loop();

    // Print statistics every 10 seconds
    static unsigned long lastStatsTime = 0;
    if (millis() - lastStatsTime >= 10000) {
        lastStatsTime = millis();
        printStatistics();
    }

    // Main loop can do other things without constantly polling inputs
    // The callbacks handle input events automatically!

    // Small delay to prevent overwhelming the system
    delay(10);
}

/**
 * @brief Print statistics about button presses
 */
void printStatistics() {
    Serial.println();
    Serial.println("========== STATISTICS ==========");
    Serial.print("Pin 0 (Button) press count: ");
    Serial.println(pin0PressCount);
    Serial.print("Pin 1 (Button) press count: ");
    Serial.println(pin1PressCount);
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println("================================");
    Serial.println();
}

/**
 * CALLBACK BEST PRACTICES:
 *
 * 1. Keep callbacks SHORT and FAST
 *    - Avoid long processing or delays
 *    - Use flags to communicate with main loop for complex operations
 *
 * 2. Be careful with shared variables
 *    - Use 'volatile' for variables modified in callbacks
 *    - Consider using atomic operations for counters
 *
 * 3. Don't refresh buffers in callbacks
 *    - The buffer is already refreshed when callback is called
 *    - Use digitalRead(pin, false) if you must read in a callback
 *
 * 4. Multiple callbacks are allowed
 *    - All registered callbacks will be called for each event
 *    - They are called in registration order
 *
 * 5. Unregister callbacks when no longer needed
 *    - Use unregisterCallback(handler) to remove a callback
 *    - Example: inputCard.unregisterCallback(mainCallbackHandler);
 *
 * UNDERSTANDING THE FLOW:
 *
 * 1. Pin state changes (e.g., button pressed)
 * 2. loop() detects the change
 * 3. Debounce timer waits (default 50ms)
 * 4. If state is stable, all callbacks are triggered
 * 5. Each callback receives pin number and new state
 *
 * ADVANTAGES OVER POLLING:
 *
 * - No need to track previous states
 * - No need to check every pin every loop
 * - Automatic debouncing
 * - Cleaner, more maintainable code
 * - More efficient CPU usage
 *
 * NEXT STEPS:
 *
 * 1. Try adding more callbacks for different purposes
 * 2. Experiment with unregistering and re-registering callbacks
 * 3. Move on to the debouncing example to learn about timing
 */
