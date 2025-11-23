/**
 * @file pwm_dimming.ino
 * @brief PWM Dimming and Analog Control Example
 *
 * This example demonstrates PWM control using the DigitalOutputCard.
 * It shows how to:
 * - Use analogWrite for PWM control
 * - Create smooth fading effects
 * - Control brightness/speed with PWM
 * - Understand state vs value concept
 * - Create various dimming patterns
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Digital Output Card (address 0x40)
 * - LED or dimmable 12V load connected to pins 0-3
 *
 * Circuit:
 * - Connect dimmable loads (LEDs work best) between output pins and ground
 * - Add appropriate current-limiting resistors for LEDs
 *
 * PWM Resolution: 12-bit (0-4095)
 * - 0 = 0% (OFF)
 * - 1024 = 25%
 * - 2048 = 50%
 * - 3072 = 75%
 * - 4095 = 100% (fully ON)
 *
 * Created: 2025
 *
 * This example code is in the public domain.
 */

#include <DigitalOutputCard.hpp>

// Create card instance
DigitalOutputCard card(0x40);

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println("Digital Output Card - PWM Dimming Example");
    Serial.println("=========================================");

    // Initialize the card
    if (card.begin()) {
        Serial.println("Card initialized successfully");
    } else {
        Serial.println("Card initialization failed");
        while(1);
    }

    // Turn all outputs OFF at startup
    Serial.println("Initializing all outputs to OFF...");
    for (uint8_t pin = 0; pin < 16; pin++) {
        card.analogWrite(pin, 0);
    }
    delay(1000);

    // Run demonstration functions
    demonstratePWMLevels();
    demonstrateFadeInOut();
    demonstrateStateVsValue();
    demonstrateMultiChannelDimming();

    Serial.println("\nSetup complete. Starting main loop...");
}

void loop() {
    // Continuously fade pin 0 in and out
    smoothFade(0);

    delay(1000);
}

/**
 * @brief Demonstrates different PWM levels
 */
void demonstratePWMLevels() {
    Serial.println("\n=== PWM Levels Demo ===");

    // Show discrete brightness levels
    uint16_t levels[] = {0, 512, 1024, 2048, 3072, 4095};
    const char* labels[] = {"0% (OFF)", "12.5%", "25%", "50%", "75%", "100% (Full)"};

    for (int i = 0; i < 6; i++) {
        Serial.printf("Setting pin 0 to %s (%d)\n", labels[i], levels[i]);
        card.analogWrite(0, levels[i]);
        delay(1500);
    }

    // Turn OFF
    card.analogWrite(0, 0);
    delay(1000);
}

/**
 * @brief Demonstrates smooth fade in and fade out
 */
void demonstrateFadeInOut() {
    Serial.println("\n=== Fade In/Out Demo ===");

    const uint8_t PIN = 0;
    const uint16_t STEP = 50;    // Brightness step
    const uint16_t DELAY_MS = 10; // Delay between steps

    // Fade in
    Serial.println("Fading in...");
    for (uint16_t brightness = 0; brightness <= 4095; brightness += STEP) {
        card.analogWrite(PIN, brightness);
        delay(DELAY_MS);
    }

    delay(1000);

    // Fade out
    Serial.println("Fading out...");
    for (uint16_t brightness = 4095; brightness > 0; brightness -= STEP) {
        card.analogWrite(PIN, brightness);
        delay(DELAY_MS);
    }
    card.analogWrite(PIN, 0); // Ensure fully OFF

    delay(1000);
}

/**
 * @brief Demonstrates the difference between state and value
 */
void demonstrateStateVsValue() {
    Serial.println("\n=== State vs Value Demo ===");

    const uint8_t PIN = 0;

    // Set a specific brightness level
    Serial.println("Setting value to 50% (2048)");
    card.setValue(PIN, 2048);
    delay(500);

    // Turn ON (output = state × value = 1 × 2048 = 2048)
    Serial.println("Turning ON (state = true)");
    Serial.println("Output = 2048 (50%)");
    card.setState(PIN, true);
    delay(2000);

    // Turn OFF (output = state × value = 0 × 2048 = 0)
    Serial.println("Turning OFF (state = false)");
    Serial.println("Output = 0, but value still = 2048");
    card.setState(PIN, false);
    delay(2000);

    // Turn ON again (returns to 50% because value is still 2048)
    Serial.println("Turning ON again (state = true)");
    Serial.println("Output = 2048 (50%) - brightness remembered!");
    card.setState(PIN, true);
    delay(2000);

    // Change brightness while ON
    Serial.println("Changing value to 75% while ON");
    card.setValue(PIN, 3072);
    delay(2000);

    // Turn OFF
    Serial.println("Turning OFF");
    card.setState(PIN, false);
    delay(1000);

    // Turn ON - now at 75%
    Serial.println("Turning ON - now at 75%");
    card.setState(PIN, true);
    delay(2000);

    // Clean up
    card.analogWrite(PIN, 0);
    delay(1000);
}

/**
 * @brief Demonstrates controlling multiple channels with different brightness
 */
void demonstrateMultiChannelDimming() {
    Serial.println("\n=== Multi-Channel Dimming Demo ===");

    // Set 4 channels to different brightness levels
    Serial.println("Setting 4 channels to different brightness levels");
    card.analogWrite(0, 1024);  // 25%
    card.analogWrite(1, 2048);  // 50%
    card.analogWrite(2, 3072);  // 75%
    card.analogWrite(3, 4095);  // 100%

    Serial.println("Pin 0: 25%, Pin 1: 50%, Pin 2: 75%, Pin 3: 100%");
    delay(3000);

    // Fade all channels in sync
    Serial.println("Fading all channels from current levels to OFF...");
    for (uint16_t value = 4095; value > 0; value -= 50) {
        // Scale each channel proportionally
        card.analogWrite(0, min(value, (uint16_t)1024));
        card.analogWrite(1, min(value, (uint16_t)2048));
        card.analogWrite(2, min(value, (uint16_t)3072));
        card.analogWrite(3, min(value, (uint16_t)4095));
        delay(10);
    }

    // Ensure all OFF
    for (uint8_t pin = 0; pin < 4; pin++) {
        card.analogWrite(pin, 0);
    }

    delay(1000);
}

/**
 * @brief Smooth fade in and out for a single pin
 *
 * @param pin Pin number to fade
 */
void smoothFade(uint8_t pin) {
    Serial.printf("Smooth fading pin %d\n", pin);

    // Fade in
    for (uint16_t brightness = 0; brightness <= 4095; brightness += 25) {
        card.analogWrite(pin, brightness);
        delay(5);
    }

    // Hold at full brightness
    delay(500);

    // Fade out
    for (uint16_t brightness = 4095; brightness > 0; brightness -= 25) {
        card.analogWrite(pin, brightness);
        delay(5);
    }
    card.analogWrite(pin, 0);

    // Hold at OFF
    delay(500);
}

/**
 * @brief Breathing effect (smooth sinusoidal fade)
 *
 * @param pin Pin number
 * @param duration_ms Duration of one breath cycle in milliseconds
 */
void breathingEffect(uint8_t pin, uint16_t duration_ms) {
    const uint16_t steps = 100;
    const uint16_t delay_ms = duration_ms / steps;

    for (uint16_t i = 0; i < steps; i++) {
        // Use sine wave for natural breathing effect
        float angle = (float)i / steps * 2.0 * PI;
        float sinVal = (sin(angle) + 1.0) / 2.0; // 0.0 to 1.0
        uint16_t brightness = (uint16_t)(sinVal * 4095);

        card.analogWrite(pin, brightness);
        delay(delay_ms);
    }
}

/**
 * @brief Pulse effect (quick fade in, slow fade out)
 *
 * @param pin Pin number
 */
void pulseEffect(uint8_t pin) {
    // Quick fade in
    for (uint16_t brightness = 0; brightness <= 4095; brightness += 100) {
        card.analogWrite(pin, brightness);
        delay(2);
    }

    // Slow fade out
    for (uint16_t brightness = 4095; brightness > 0; brightness -= 20) {
        card.analogWrite(pin, brightness);
        delay(10);
    }
    card.analogWrite(pin, 0);
}

/**
 * @brief Strobe effect
 *
 * @param pin Pin number
 * @param count Number of flashes
 * @param on_time_ms ON time in milliseconds
 * @param off_time_ms OFF time in milliseconds
 */
void strobeEffect(uint8_t pin, uint8_t count, uint16_t on_time_ms, uint16_t off_time_ms) {
    for (uint8_t i = 0; i < count; i++) {
        card.analogWrite(pin, 4095);
        delay(on_time_ms);
        card.analogWrite(pin, 0);
        delay(off_time_ms);
    }
}
