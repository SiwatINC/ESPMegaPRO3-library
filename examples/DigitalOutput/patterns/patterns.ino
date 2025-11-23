/**
 * @file patterns.ino
 * @brief Output Pattern Examples
 *
 * This example demonstrates various output patterns and effects using the DigitalOutputCard.
 * It shows how to create:
 * - Sequential patterns (chaser, scanner)
 * - Binary counting pattern
 * - Random patterns
 * - Breathing effects
 * - Theater marquee effect
 * - Rainbow (using PWM)
 * - Multiple synchronized channels
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Digital Output Card (address 0x40)
 * - 8-16 LEDs or other 12V loads for best visual effect
 *
 * Pattern Descriptions:
 * - Knight Rider: Single LED scanning back and forth
 * - Cylon: Knight Rider with trailing fade
 * - Theater: Lights chasing in groups
 * - Binary Counter: Displays binary numbers
 * - Random: Random on/off patterns
 * - Wave: Smooth brightness wave across outputs
 * - Breathing: All outputs breathe in sync
 * - Fire: Flickering fire effect
 *
 * Created: 2025
 *
 * This example code is in the public domain.
 */

#include <DigitalOutputCard.hpp>

// Create card instance
DigitalOutputCard card(0x40);

// Pattern configuration
const uint8_t NUM_LEDS = 16;      // Number of outputs to use
const uint8_t PATTERN_DELAY = 50; // Base delay for patterns (ms)

// Current pattern selection
uint8_t currentPattern = 0;
const uint8_t NUM_PATTERNS = 10;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println("Digital Output Card - Pattern Examples");
    Serial.println("=====================================");

    // Initialize the card
    if (!card.begin()) {
        Serial.println("Card initialization failed!");
        while(1);
    }
    Serial.println("Card initialized successfully");

    // Turn all outputs OFF initially
    for (uint8_t pin = 0; pin < NUM_LEDS; pin++) {
        card.digitalWrite(pin, LOW);
    }

    Serial.println("\nStarting pattern demonstrations...");
    Serial.println("Patterns will cycle automatically");
    Serial.println();
}

void loop() {
    // Display pattern name
    printPatternName(currentPattern);

    // Run pattern
    switch(currentPattern) {
        case 0:
            patternKnightRider();
            break;
        case 1:
            patternCylon();
            break;
        case 2:
            patternTheaterChase();
            break;
        case 3:
            patternBinaryCounter();
            break;
        case 4:
            patternRandom();
            break;
        case 5:
            patternWave();
            break;
        case 6:
            patternBreathing();
            break;
        case 7:
            patternFire();
            break;
        case 8:
            patternAlternating();
            break;
        case 9:
            patternLarsonScanner();
            break;
    }

    // Move to next pattern
    currentPattern = (currentPattern + 1) % NUM_PATTERNS;

    // Clear all outputs between patterns
    for (uint8_t pin = 0; pin < NUM_LEDS; pin++) {
        card.digitalWrite(pin, LOW);
    }
    delay(1000);
}

/**
 * @brief Print current pattern name
 */
void printPatternName(uint8_t pattern) {
    Serial.print("\n>>> Running Pattern: ");
    switch(pattern) {
        case 0: Serial.println("Knight Rider"); break;
        case 1: Serial.println("Cylon"); break;
        case 2: Serial.println("Theater Chase"); break;
        case 3: Serial.println("Binary Counter"); break;
        case 4: Serial.println("Random"); break;
        case 5: Serial.println("Wave"); break;
        case 6: Serial.println("Breathing"); break;
        case 7: Serial.println("Fire"); break;
        case 8: Serial.println("Alternating"); break;
        case 9: Serial.println("Larson Scanner"); break;
    }
}

/**
 * @brief Knight Rider pattern - single LED scanning back and forth
 */
void patternKnightRider() {
    const uint8_t NUM_CYCLES = 3;

    for (uint8_t cycle = 0; cycle < NUM_CYCLES; cycle++) {
        // Scan forward
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            card.digitalWrite(i, HIGH);
            delay(PATTERN_DELAY);
            card.digitalWrite(i, LOW);
        }

        // Scan backward
        for (int i = NUM_LEDS - 2; i > 0; i--) {
            card.digitalWrite(i, HIGH);
            delay(PATTERN_DELAY);
            card.digitalWrite(i, LOW);
        }
    }
}

/**
 * @brief Cylon pattern - Knight Rider with trailing fade
 */
void patternCylon() {
    const uint8_t NUM_CYCLES = 3;
    const uint8_t TAIL_LENGTH = 4;

    for (uint8_t cycle = 0; cycle < NUM_CYCLES; cycle++) {
        // Scan forward
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            // Set current LED to full brightness
            card.analogWrite(i, 4095);

            // Create fading tail
            for (uint8_t j = 1; j <= TAIL_LENGTH && i >= j; j++) {
                uint16_t brightness = 4095 - (j * 1024);
                card.analogWrite(i - j, brightness);
            }

            delay(PATTERN_DELAY);

            // Fade out the tail
            for (uint8_t j = 0; j <= TAIL_LENGTH && i >= j; j++) {
                card.analogWrite(i - j, 0);
            }
        }

        // Scan backward
        for (int i = NUM_LEDS - 2; i > 0; i--) {
            card.analogWrite(i, 4095);

            for (uint8_t j = 1; j <= TAIL_LENGTH && i + j < NUM_LEDS; j++) {
                uint16_t brightness = 4095 - (j * 1024);
                card.analogWrite(i + j, brightness);
            }

            delay(PATTERN_DELAY);

            for (uint8_t j = 0; j <= TAIL_LENGTH && i + j < NUM_LEDS; j++) {
                card.analogWrite(i + j, 0);
            }
        }
    }
}

/**
 * @brief Theater chase pattern
 */
void patternTheaterChase() {
    const uint8_t NUM_CYCLES = 10;
    const uint8_t STEP = 3;

    for (uint8_t cycle = 0; cycle < NUM_CYCLES; cycle++) {
        for (uint8_t offset = 0; offset < STEP; offset++) {
            // Turn on every 3rd LED
            for (uint8_t i = 0; i < NUM_LEDS; i++) {
                if (i % STEP == offset) {
                    card.digitalWrite(i, HIGH);
                } else {
                    card.digitalWrite(i, LOW);
                }
            }
            delay(PATTERN_DELAY * 2);
        }
    }
}

/**
 * @brief Binary counter pattern
 */
void patternBinaryCounter() {
    const uint8_t BITS = min(NUM_LEDS, (uint8_t)8); // Use up to 8 bits
    const uint16_t MAX_COUNT = 1 << BITS; // 2^BITS

    for (uint16_t count = 0; count < MAX_COUNT; count++) {
        // Display count in binary
        for (uint8_t bit = 0; bit < BITS; bit++) {
            bool state = (count >> bit) & 1;
            card.digitalWrite(bit, state);
        }

        delay(PATTERN_DELAY * 4);
    }
}

/**
 * @brief Random pattern
 */
void patternRandom() {
    const uint16_t NUM_CHANGES = 50;

    for (uint16_t i = 0; i < NUM_CHANGES; i++) {
        uint8_t pin = random(NUM_LEDS);
        bool state = random(2);
        card.digitalWrite(pin, state);
        delay(PATTERN_DELAY);
    }
}

/**
 * @brief Wave pattern - brightness wave across outputs
 */
void patternWave() {
    const uint8_t NUM_CYCLES = 3;
    const uint16_t STEPS = 100;

    for (uint8_t cycle = 0; cycle < NUM_CYCLES; cycle++) {
        for (uint16_t step = 0; step < STEPS; step++) {
            for (uint8_t pin = 0; pin < NUM_LEDS; pin++) {
                // Calculate phase for this LED
                float phase = (float)step / STEPS * 2.0 * PI;
                phase += (float)pin / NUM_LEDS * 2.0 * PI;

                // Calculate brightness using sine wave
                float sinVal = (sin(phase) + 1.0) / 2.0; // 0.0 to 1.0
                uint16_t brightness = (uint16_t)(sinVal * 4095);

                card.analogWrite(pin, brightness);
            }
            delay(20);
        }
    }
}

/**
 * @brief Breathing pattern - all outputs breathe in sync
 */
void patternBreathing() {
    const uint8_t NUM_CYCLES = 3;
    const uint16_t STEPS = 100;

    for (uint8_t cycle = 0; cycle < NUM_CYCLES; cycle++) {
        // Breathe in and out
        for (uint16_t step = 0; step < STEPS; step++) {
            float angle = (float)step / STEPS * 2.0 * PI;
            float sinVal = (sin(angle) + 1.0) / 2.0;
            uint16_t brightness = (uint16_t)(sinVal * 4095);

            // Set all LEDs to same brightness
            for (uint8_t pin = 0; pin < NUM_LEDS; pin++) {
                card.analogWrite(pin, brightness);
            }

            delay(20);
        }
    }
}

/**
 * @brief Fire effect - flickering like fire
 */
void patternFire() {
    const uint16_t DURATION = 5000; // 5 seconds
    unsigned long startTime = millis();

    while (millis() - startTime < DURATION) {
        for (uint8_t pin = 0; pin < NUM_LEDS; pin++) {
            // Random brightness with bias toward mid-high values
            uint16_t brightness = random(2048, 4095);
            card.analogWrite(pin, brightness);
        }
        delay(PATTERN_DELAY);
    }
}

/**
 * @brief Alternating pattern
 */
void patternAlternating() {
    const uint8_t NUM_CYCLES = 10;

    for (uint8_t cycle = 0; cycle < NUM_CYCLES; cycle++) {
        // Turn on even pins
        for (uint8_t pin = 0; pin < NUM_LEDS; pin++) {
            card.digitalWrite(pin, pin % 2 == 0);
        }
        delay(PATTERN_DELAY * 5);

        // Turn on odd pins
        for (uint8_t pin = 0; pin < NUM_LEDS; pin++) {
            card.digitalWrite(pin, pin % 2 == 1);
        }
        delay(PATTERN_DELAY * 5);
    }
}

/**
 * @brief Larson Scanner (Battlestar Galactica Cylon)
 */
void patternLarsonScanner() {
    const uint8_t NUM_CYCLES = 3;
    const uint8_t EYE_SIZE = 3;

    for (uint8_t cycle = 0; cycle < NUM_CYCLES; cycle++) {
        // Scan forward
        for (int i = 0; i < NUM_LEDS; i++) {
            // Clear all
            for (uint8_t j = 0; j < NUM_LEDS; j++) {
                card.analogWrite(j, 0);
            }

            // Set eye
            for (int j = -EYE_SIZE; j <= EYE_SIZE; j++) {
                int pos = i + j;
                if (pos >= 0 && pos < NUM_LEDS) {
                    uint16_t brightness = 4095 - abs(j) * 1024;
                    if (brightness > 4095) brightness = 0;
                    card.analogWrite(pos, brightness);
                }
            }

            delay(PATTERN_DELAY);
        }

        // Scan backward
        for (int i = NUM_LEDS - 1; i >= 0; i--) {
            // Clear all
            for (uint8_t j = 0; j < NUM_LEDS; j++) {
                card.analogWrite(j, 0);
            }

            // Set eye
            for (int j = -EYE_SIZE; j <= EYE_SIZE; j++) {
                int pos = i + j;
                if (pos >= 0 && pos < NUM_LEDS) {
                    uint16_t brightness = 4095 - abs(j) * 1024;
                    if (brightness > 4095) brightness = 0;
                    card.analogWrite(pos, brightness);
                }
            }

            delay(PATTERN_DELAY);
        }
    }
}

/**
 * @brief Custom pattern function - create your own!
 */
void patternCustom() {
    // Add your own pattern here!
    // Use card.digitalWrite() or card.analogWrite()
    // Be creative!
}
