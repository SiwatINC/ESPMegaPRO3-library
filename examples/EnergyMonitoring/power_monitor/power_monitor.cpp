/**
 * Power Monitor with Alerts and Callbacks
 *
 * This example demonstrates advanced power monitoring with real-time alerts,
 * callback functions, and comprehensive status tracking. Features include:
 * - Overload detection and alerts
 * - Under-load/idle detection
 * - Peak power tracking
 * - Energy milestones
 * - Moving average filtering
 * - Callback-driven event handling
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board
 * - Analog Card
 * - Current Transformer (e.g., SCT-013-000)
 * - Optional: Buzzer or LED for alarms on digital outputs
 *
 * Author: SIWAT INC
 * Date: 2024
 */

#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>
#include <ESPMegaPRO.h>

// Configuration Constants
const float LINE_VOLTAGE = 230.0;           // AC line voltage
const float OVERLOAD_THRESHOLD = 20.0;      // Overload alert at 20A
const float IDLE_THRESHOLD = 0.5;           // Idle detection below 0.5A
const float WARNING_THRESHOLD = 15.0;       // Warning at 15A
const unsigned long OVERLOAD_DELAY = 3000;  // Alarm delay (debounce)

// Hardware pin for alarm output (optional)
const int ALARM_PIN = 0;  // PWM output P0
const bool USE_ALARM_OUTPUT = false;  // Set to true to enable

// Create instances
AnalogCard analogCard;
float lineVoltage = LINE_VOLTAGE;

// ADC to Current conversion for SCT-013-000
auto adcToCurrent = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;
    const float SCALE_FACTOR = 0.0488;
    return (adc - ADC_ZERO) * SCALE_FACTOR;
};

CurrentTransformerCard ct(
    &analogCard,
    0,              // Analog input A0
    &lineVoltage,
    adcToCurrent,
    500             // Sample every 500ms for faster response
);

// Alert State Tracking
bool overloadAlarmActive = false;
bool warningAlarmActive = false;
bool idleState = false;
unsigned long overloadStartTime = 0;
unsigned long lastAlarmBeep = 0;

// Statistics and Tracking
struct PowerStats {
    float peakCurrent = 0;
    float peakPower = 0;
    double peakEnergy = 0;
    unsigned long peakTime = 0;
    double lastMilestoneKWh = 0;
    unsigned long totalOverloadEvents = 0;
    unsigned long totalWarningEvents = 0;
    unsigned long overloadDuration = 0;
};
PowerStats stats;

// Moving Average Filter
class CurrentFilter {
private:
    static const int SAMPLES = 10;
    float readings[SAMPLES] = {0};
    int index = 0;
    bool initialized = false;

public:
    float update(float newValue) {
        readings[index] = newValue;
        index = (index + 1) % SAMPLES;
        initialized = true;

        float sum = 0;
        for (int i = 0; i < SAMPLES; i++) {
            sum += readings[i];
        }
        return sum / SAMPLES;
    }

    bool isInitialized() {
        return initialized;
    }
};
CurrentFilter filter;

// Callback handler ID
uint8_t callbackHandler;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("========================================");
    Serial.println("   Power Monitor with Alerts");
    Serial.println("========================================");
    Serial.println();

    // Initialize ESPMega if using alarm output
    if (USE_ALARM_OUTPUT) {
        ESPMega_begin();
        ESPMega_digitalWrite(ALARM_PIN, LOW);
        Serial.println("Alarm output initialized on P0");
    }

    // Initialize Analog Card
    Serial.print("Initializing Analog Card...");
    if (!analogCard.begin()) {
        Serial.println(" FAILED!");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" OK");

    // Initialize CT Card
    Serial.print("Initializing Current Transformer Card...");
    if (!ct.begin()) {
        Serial.println(" FAILED!");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" OK");

    // Register callback for event-driven monitoring
    callbackHandler = ct.registerCallback(onEnergyUpdate);
    Serial.println("Monitoring callback registered");

    Serial.println();
    Serial.println("Configuration:");
    Serial.printf("  - Line Voltage:       %.1f V\n", LINE_VOLTAGE);
    Serial.printf("  - Overload Threshold: %.1f A (%.0f W)\n",
        OVERLOAD_THRESHOLD, OVERLOAD_THRESHOLD * LINE_VOLTAGE);
    Serial.printf("  - Warning Threshold:  %.1f A (%.0f W)\n",
        WARNING_THRESHOLD, WARNING_THRESHOLD * LINE_VOLTAGE);
    Serial.printf("  - Idle Threshold:     %.1f A\n", IDLE_THRESHOLD);
    Serial.printf("  - Sample Rate:        500 ms\n");
    Serial.printf("  - Alarm Delay:        %.1f s\n", OVERLOAD_DELAY / 1000.0);
    Serial.println();

    Serial.println("Alert Conditions:");
    Serial.println("  - OVERLOAD:  Current > 20.0 A (sustained for 3 sec)");
    Serial.println("  - WARNING:   Current > 15.0 A");
    Serial.println("  - IDLE:      Current < 0.5 A");
    Serial.println("  - MILESTONE: Every 1 kWh of energy");
    Serial.println();

    delay(2000);
    Serial.println("Monitoring started...");
    Serial.println();
}

void loop() {
    // Update CT card (triggers callback)
    ct.loop();

    // Handle alarm output
    if (USE_ALARM_OUTPUT) {
        updateAlarmOutput();
    }

    // Print filtered status every 2 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 2000) {
        printStatus();
        lastPrint = millis();
    }

    delay(100);
}

/**
 * Callback function called on every energy update
 * Handles all monitoring logic and alerts
 */
void onEnergyUpdate(float current, double energy) {
    // Apply moving average filter
    float filteredCurrent = filter.update(current);

    // Only use filtered values after initialization
    if (filter.isInitialized()) {
        current = filteredCurrent;
    }

    float power = current * LINE_VOLTAGE;

    // === OVERLOAD DETECTION ===
    if (current > OVERLOAD_THRESHOLD) {
        if (!overloadAlarmActive) {
            if (overloadStartTime == 0) {
                // Start overload timer
                overloadStartTime = millis();
            } else if (millis() - overloadStartTime >= OVERLOAD_DELAY) {
                // Sustained overload - activate alarm
                overloadAlarmActive = true;
                stats.totalOverloadEvents++;

                Serial.println();
                Serial.println("!!! OVERLOAD ALERT !!!");
                Serial.printf("Current: %.2f A (%.0f W) exceeds limit of %.1f A\n",
                    current, power, OVERLOAD_THRESHOLD);
                Serial.printf("Event #%lu - Immediate action required!\n",
                    stats.totalOverloadEvents);
                Serial.println();
            }
        } else {
            // Track overload duration
            stats.overloadDuration += 500;  // Sample interval
        }
    } else {
        if (overloadAlarmActive) {
            // Overload cleared
            Serial.println();
            Serial.println(">>> Overload condition cleared");
            Serial.printf("Duration: %.1f seconds\n", stats.overloadDuration / 1000.0);
            Serial.println();
        }
        overloadAlarmActive = false;
        overloadStartTime = 0;
    }

    // === WARNING DETECTION ===
    if (current > WARNING_THRESHOLD && !overloadAlarmActive) {
        if (!warningAlarmActive) {
            warningAlarmActive = true;
            stats.totalWarningEvents++;

            Serial.println();
            Serial.println("*** WARNING: High Load ***");
            Serial.printf("Current: %.2f A (%.0f W)\n", current, power);
            Serial.println();
        }
    } else if (current <= WARNING_THRESHOLD) {
        warningAlarmActive = false;
    }

    // === IDLE DETECTION ===
    if (current < IDLE_THRESHOLD) {
        if (!idleState) {
            idleState = true;
            Serial.println();
            Serial.println("--- Circuit Idle ---");
            Serial.printf("Current: %.3f A\n", current);
            Serial.println();
        }
    } else {
        if (idleState) {
            Serial.println();
            Serial.println("--- Circuit Active ---");
            Serial.printf("Current: %.2f A\n", current);
            Serial.println();
        }
        idleState = false;
    }

    // === PEAK TRACKING ===
    if (current > stats.peakCurrent) {
        stats.peakCurrent = current;
        stats.peakPower = power;
        stats.peakEnergy = energy;
        stats.peakTime = millis();

        Serial.println();
        Serial.println(">>> New Peak Recorded <<<");
        Serial.printf("Peak Current: %.2f A\n", stats.peakCurrent);
        Serial.printf("Peak Power:   %.2f W\n", stats.peakPower);
        Serial.printf("Energy:       %.3f Wh\n", stats.peakEnergy);
        Serial.println();
    }

    // === ENERGY MILESTONES ===
    double energyKWh = energy / 1000.0;
    int currentMilestone = (int)energyKWh;

    if (currentMilestone > stats.lastMilestoneKWh && currentMilestone > 0) {
        stats.lastMilestoneKWh = currentMilestone;

        Serial.println();
        Serial.println("========================================");
        Serial.printf("    ENERGY MILESTONE: %d kWh\n", currentMilestone);
        Serial.println("========================================");
        printDetailedStats();
        Serial.println("========================================");
        Serial.println();
    }
}

/**
 * Update alarm output (buzzer, LED, etc.)
 */
void updateAlarmOutput() {
    if (overloadAlarmActive) {
        // Beeping pattern: 200ms on, 200ms off
        if (millis() - lastAlarmBeep >= 200) {
            bool currentState = ESPMega_digitalRead(ALARM_PIN);
            ESPMega_digitalWrite(ALARM_PIN, !currentState);
            lastAlarmBeep = millis();
        }
    } else if (warningAlarmActive) {
        // Slow beep: 500ms on, 500ms off
        if (millis() - lastAlarmBeep >= 500) {
            bool currentState = ESPMega_digitalRead(ALARM_PIN);
            ESPMega_digitalWrite(ALARM_PIN, !currentState);
            lastAlarmBeep = millis();
        }
    } else {
        // No alarm - turn off
        ESPMega_digitalWrite(ALARM_PIN, LOW);
    }
}

/**
 * Print current status
 */
void printStatus() {
    float current = ct.getCurrent();
    float power = ct.getPower();
    double energy = ct.getEnergy();

    // Status indicators
    String status = "NORMAL";
    if (overloadAlarmActive) {
        status = "OVERLOAD!";
    } else if (warningAlarmActive) {
        status = "WARNING";
    } else if (idleState) {
        status = "IDLE";
    }

    Serial.printf("[%10s] Current: %6.2f A | Power: %8.2f W | Energy: %10.3f Wh | Peak: %6.2f A\n",
        status.c_str(),
        current,
        power,
        energy,
        stats.peakCurrent
    );
}

/**
 * Print detailed statistics
 */
void printDetailedStats() {
    Serial.println();
    Serial.println("Current Status:");
    Serial.printf("  Current:       %.2f A\n", ct.getCurrent());
    Serial.printf("  Power:         %.2f W\n", ct.getPower());
    Serial.printf("  Energy:        %.3f Wh (%.3f kWh)\n",
        ct.getEnergy(), ct.getEnergy() / 1000.0);
    Serial.println();

    Serial.println("Peak Values:");
    Serial.printf("  Peak Current:  %.2f A\n", stats.peakCurrent);
    Serial.printf("  Peak Power:    %.2f W\n", stats.peakPower);
    Serial.printf("  Peak Time:     %lu ms ago\n", millis() - stats.peakTime);
    Serial.println();

    Serial.println("Alert History:");
    Serial.printf("  Overload Events:   %lu\n", stats.totalOverloadEvents);
    Serial.printf("  Warning Events:    %lu\n", stats.totalWarningEvents);
    Serial.printf("  Total Overload:    %.1f seconds\n",
        stats.overloadDuration / 1000.0);
    Serial.println();

    // Calculate estimated cost
    float costPerKWh = 0.12;
    float cost = (ct.getEnergy() / 1000.0) * costPerKWh;
    Serial.println("Cost:");
    Serial.printf("  Total Cost:        $%.4f @ $%.2f/kWh\n", cost, costPerKWh);
}

/*
 * Application Notes:
 *
 * 1. Overload Protection:
 *    - The OVERLOAD_DELAY prevents false alarms from brief spikes
 *    - Adjust based on your load characteristics
 *    - For motor loads: increase delay (inrush current)
 *    - For resistive loads: decrease delay (faster response)
 *
 * 2. Moving Average Filter:
 *    - Smooths out noise and fluctuations
 *    - 10-sample average provides good balance
 *    - Increase samples for smoother, slower response
 *    - Decrease samples for faster, noisier response
 *
 * 3. Callback-Driven Design:
 *    - All monitoring logic in callback function
 *    - Event-driven, efficient processing
 *    - Main loop remains free for other tasks
 *    - Keep callback fast - no delays!
 *
 * 4. Hardware Alarm Output:
 *    - Set USE_ALARM_OUTPUT = true to enable
 *    - Connect buzzer or LED to P0 (PWM output)
 *    - Beep patterns:
 *      - Overload: Fast beep (200ms)
 *      - Warning: Slow beep (500ms)
 *      - Normal: Off
 *
 * 5. Threshold Customization:
 *    - Adjust thresholds based on circuit rating
 *    - Example for 15A circuit breaker:
 *      - OVERLOAD_THRESHOLD = 13.0  (87% of rating)
 *      - WARNING_THRESHOLD = 11.0   (73% of rating)
 *
 * 6. Safety Considerations:
 *    - This is a MONITORING system, not a protection device
 *    - Always use proper circuit breakers
 *    - Do not rely solely on software for safety
 *    - Regular calibration recommended
 *
 * 7. Integration Ideas:
 *    - Connect to ESPMegaIoT for remote alerts
 *    - Log events to SD card
 *    - Send notifications via MQTT/HTTP
 *    - Integrate with home automation
 *    - Create web dashboard for monitoring
 */
