/**
 * @file debouncing.ino
 * @brief Advanced debouncing demonstration with DigitalInputCard
 *
 * This example demonstrates:
 * - What debouncing is and why it's important
 * - How to configure debounce times
 * - Different debounce settings for different input types
 * - Visualizing debounce behavior
 * - Real-world debounce timing recommendations
 *
 * What is debouncing?
 * -------------------
 * Mechanical switches and buttons don't change state cleanly. When pressed,
 * the contacts "bounce" - making and breaking contact multiple times over
 * several milliseconds before settling. This causes multiple false triggers.
 *
 * Debouncing filters out these rapid changes and only triggers callbacks
 * when the input has been stable for a specified time.
 *
 * Hardware Requirements:
 * - ESPMegaPRO3 board
 * - DigitalInputCard expansion card
 * - Pin 0: Mechanical switch (bouncy)
 * - Pin 1: Push button (bouncy)
 * - Pin 2: Electronic sensor (minimal bounce)
 * - Pin 3: Clean digital signal (no bounce)
 *
 * @author Siwat Sirichai
 * @date 2025
 */

#include <DigitalInputCard.hpp>

// Create a DigitalInputCard instance
DigitalInputCard inputCard(false, false, false, false, false, false);

// Track callback invocations
volatile uint32_t pin0CallbackCount = 0;
volatile uint32_t pin1CallbackCount = 0;
volatile uint32_t pin2CallbackCount = 0;
volatile uint32_t pin3CallbackCount = 0;

// Track last change times for visualization
volatile unsigned long pin0LastChange = 0;
volatile unsigned long pin1LastChange = 0;
volatile unsigned long pin2LastChange = 0;
volatile unsigned long pin3LastChange = 0;

/**
 * @brief Callback function with debounce visualization
 *
 * This callback prints detailed timing information to help
 * understand the debouncing behavior.
 *
 * @param pin The pin that changed
 * @param state The new state
 */
void onInputChangeWithTiming(uint8_t pin, bool state) {
    unsigned long now = millis();
    unsigned long timeSinceLastChange = 0;

    // Calculate time since last change for this pin
    switch(pin) {
        case 0:
            timeSinceLastChange = now - pin0LastChange;
            pin0LastChange = now;
            pin0CallbackCount++;
            break;
        case 1:
            timeSinceLastChange = now - pin1LastChange;
            pin1LastChange = now;
            pin1CallbackCount++;
            break;
        case 2:
            timeSinceLastChange = now - pin2LastChange;
            pin2LastChange = now;
            pin2CallbackCount++;
            break;
        case 3:
            timeSinceLastChange = now - pin3LastChange;
            pin3LastChange = now;
            pin3CallbackCount++;
            break;
    }

    // Print detailed information
    Serial.println();
    Serial.println("======================================");
    Serial.print("Pin ");
    Serial.print(pin);
    Serial.print(" changed to ");
    Serial.println(state ? "HIGH" : "LOW");
    Serial.print("  Time: ");
    Serial.print(now);
    Serial.println(" ms");
    Serial.print("  Time since last change: ");
    Serial.print(timeSinceLastChange);
    Serial.println(" ms");
    Serial.print("  Total callbacks for this pin: ");
    switch(pin) {
        case 0: Serial.println(pin0CallbackCount); break;
        case 1: Serial.println(pin1CallbackCount); break;
        case 2: Serial.println(pin2CallbackCount); break;
        case 3: Serial.println(pin3CallbackCount); break;
    }
    Serial.println("======================================");
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== DigitalInputCard Debouncing Example ===");
    Serial.println();

    // Initialize the input card
    Serial.print("Initializing Digital Input Card... ");
    if (!inputCard.begin()) {
        Serial.println("FAILED");
        while (1) delay(1000);
    }
    Serial.println("SUCCESS");

    // ========================================
    // Configure debounce times for different input types
    // ========================================
    Serial.println();
    Serial.println("Configuring debounce times...");
    Serial.println();

    // Pin 0: Mechanical switch (high bounce)
    // Mechanical switches can bounce for 50-100ms
    // Use longer debounce time for reliable operation
    inputCard.setDebounceTime(0, 100);
    Serial.println("Pin 0 (Mechanical Switch):");
    Serial.println("  Debounce time: 100ms");
    Serial.println("  Reason: Mechanical contacts bounce significantly");
    Serial.println();

    // Pin 1: Push button (moderate bounce)
    // Buttons typically bounce for 20-50ms
    // 50ms is a good middle ground
    inputCard.setDebounceTime(1, 50);
    Serial.println("Pin 1 (Push Button):");
    Serial.println("  Debounce time: 50ms");
    Serial.println("  Reason: Standard button debounce");
    Serial.println();

    // Pin 2: Electronic sensor (minimal bounce)
    // Electronic sensors have very little bounce
    // 10ms is usually sufficient
    inputCard.setDebounceTime(2, 10);
    Serial.println("Pin 2 (Electronic Sensor):");
    Serial.println("  Debounce time: 10ms");
    Serial.println("  Reason: Minimal electrical noise");
    Serial.println();

    // Pin 3: Clean digital signal (no bounce)
    // For clean digital signals from other electronics
    // Use minimal debounce just for electrical noise
    inputCard.setDebounceTime(3, 5);
    Serial.println("Pin 3 (Clean Digital Signal):");
    Serial.println("  Debounce time: 5ms");
    Serial.println("  Reason: Only filtering electrical noise");
    Serial.println();

    // Register callback
    inputCard.registerCallback(onInputChangeWithTiming);

    // Preload input buffer
    inputCard.preloadInputBuffer();

    Serial.println("========================================");
    Serial.println("Configuration complete!");
    Serial.println();
    Serial.println("EXPERIMENT INSTRUCTIONS:");
    Serial.println("1. Try toggling each input type");
    Serial.println("2. Observe the timing between callbacks");
    Serial.println("3. Notice how longer debounce times affect responsiveness");
    Serial.println("4. Try rapid toggling - debouncing prevents false triggers");
    Serial.println();
    Serial.println("WITHOUT debouncing, a single button press might trigger");
    Serial.println("multiple callbacks due to contact bounce!");
    Serial.println();
    Serial.println("WITH debouncing, only stable state changes trigger callbacks.");
    Serial.println("========================================");
    Serial.println();
}

void loop() {
    // Process inputs and trigger callbacks
    inputCard.loop();

    // Print statistics every 30 seconds
    static unsigned long lastStatsTime = 0;
    if (millis() - lastStatsTime >= 30000) {
        lastStatsTime = millis();
        printDebounceStatistics();
    }

    // Demonstrate real-time state reading (without callbacks)
    static unsigned long lastReadTime = 0;
    if (millis() - lastReadTime >= 5000) {
        lastReadTime = millis();
        printCurrentStates();
    }

    delay(10);
}

/**
 * @brief Print current states of all configured pins
 */
void printCurrentStates() {
    Serial.println();
    Serial.println("-------- Current States --------");
    Serial.print("Pin 0 (Switch):  ");
    Serial.println(inputCard.digitalRead(0, false) ? "HIGH" : "LOW");
    Serial.print("Pin 1 (Button):  ");
    Serial.println(inputCard.digitalRead(1, false) ? "HIGH" : "LOW");
    Serial.print("Pin 2 (Sensor):  ");
    Serial.println(inputCard.digitalRead(2, false) ? "HIGH" : "LOW");
    Serial.print("Pin 3 (Digital): ");
    Serial.println(inputCard.digitalRead(3, false) ? "HIGH" : "LOW");
    Serial.println("--------------------------------");
}

/**
 * @brief Print debouncing statistics
 */
void printDebounceStatistics() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("DEBOUNCING STATISTICS");
    Serial.println("========================================");
    Serial.println();

    Serial.println("Callback Counts:");
    Serial.print("  Pin 0 (100ms debounce): ");
    Serial.print(pin0CallbackCount);
    Serial.println(" callbacks");
    Serial.print("  Pin 1 (50ms debounce):  ");
    Serial.print(pin1CallbackCount);
    Serial.println(" callbacks");
    Serial.print("  Pin 2 (10ms debounce):  ");
    Serial.print(pin2CallbackCount);
    Serial.println(" callbacks");
    Serial.print("  Pin 3 (5ms debounce):   ");
    Serial.print(pin3CallbackCount);
    Serial.println(" callbacks");
    Serial.println();

    Serial.println("Last Change Times:");
    if (pin0LastChange > 0) {
        Serial.print("  Pin 0: ");
        Serial.print((millis() - pin0LastChange) / 1000);
        Serial.println(" seconds ago");
    }
    if (pin1LastChange > 0) {
        Serial.print("  Pin 1: ");
        Serial.print((millis() - pin1LastChange) / 1000);
        Serial.println(" seconds ago");
    }
    if (pin2LastChange > 0) {
        Serial.print("  Pin 2: ");
        Serial.print((millis() - pin2LastChange) / 1000);
        Serial.println(" seconds ago");
    }
    if (pin3LastChange > 0) {
        Serial.print("  Pin 3: ");
        Serial.print((millis() - pin3LastChange) / 1000);
        Serial.println(" seconds ago");
    }
    Serial.println();

    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.println("========================================");
    Serial.println();
}

/**
 * DEBOUNCING CONCEPTS:
 *
 * WHAT HAPPENS WITHOUT DEBOUNCING:
 * --------------------------------
 * Button Press Event:
 * Physical:  ─┐ ┌┐┌┐  ┌───────  (contact bounces)
 *             └─┘└┘└──┘
 * Detected:   ▲ ▲▲▲▲  ▲          (multiple false triggers!)
 *
 * Result: One button press triggers 5+ callbacks
 *
 * WHAT HAPPENS WITH DEBOUNCING:
 * ------------------------------
 * Button Press Event:
 * Physical:  ─┐ ┌┐┌┐  ┌───────  (contact bounces)
 *             └─┘└┘└──┘
 *              |<-50ms->|
 * Detected:              ▲        (single callback after stable)
 *
 * Result: One button press triggers 1 callback
 *
 * HOW THE DEBOUNCE ALGORITHM WORKS:
 * ----------------------------------
 * 1. Pin changes from LOW to HIGH
 * 2. Timer starts (0ms)
 * 3. Pin bounces to LOW (5ms) -> Timer resets
 * 4. Pin bounces to HIGH (8ms) -> Timer resets
 * 5. Pin bounces to LOW (12ms) -> Timer resets
 * 6. Pin settles HIGH (15ms) -> Timer continues
 * 7. 50ms passes with stable HIGH -> Callback triggered!
 *
 * CHOOSING DEBOUNCE TIME:
 * -----------------------
 * Too short:
 * - May not filter all bounces
 * - Can cause false multiple triggers
 *
 * Too long:
 * - Delayed response to user input
 * - May miss rapid legitimate changes
 *
 * Recommended values:
 * - Mechanical switches: 80-150ms
 * - Push buttons: 40-80ms
 * - Tactile buttons: 20-50ms
 * - Electronic sensors: 5-20ms
 * - Digital signals: 0-10ms
 *
 * REAL-WORLD TESTING:
 * -------------------
 * 1. Start with recommended values above
 * 2. Test with actual hardware
 * 3. If you see double-triggers, increase debounce time
 * 4. If response feels sluggish, decrease slightly
 * 5. Monitor callback counts over time
 *
 * ADVANCED TIPS:
 * --------------
 * 1. Different inputs can have different debounce times
 * 2. You can change debounce time at runtime
 * 3. For critical inputs, use longer debounce times
 * 4. For gaming/UI, optimize for responsiveness (shorter times)
 * 5. Test thoroughly with your specific hardware
 *
 * NEXT STEPS:
 * -----------
 * 1. Experiment with different debounce values
 * 2. Try rapid button pressing - observe callback counts
 * 3. Compare behavior with short (5ms) vs long (100ms) debounce
 * 4. Move on to MQTT example for remote monitoring
 */
