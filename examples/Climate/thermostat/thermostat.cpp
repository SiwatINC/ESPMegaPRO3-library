/**
 * @file thermostat.ino
 * @brief Smart thermostat implementation using ClimateCard
 *
 * This example demonstrates an intelligent thermostat that automatically
 * controls the AC based on room temperature, using hysteresis to prevent
 * rapid on/off cycling.
 *
 * Hardware Required:
 * - ESP32 board
 * - IR LED connected to GPIO 14
 * - DHT22 or DS18B20 sensor connected to GPIO 27
 *
 * Features Demonstrated:
 * - Automatic temperature control with hysteresis
 * - Smart AC mode selection (cooling/heating/off)
 * - Configurable target temperature and deadband
 * - Runtime statistics and energy monitoring
 * - Manual override capability
 * - Serial command interface
 *
 * Thermostat Operation:
 * - Set target temperature (e.g., 24°C)
 * - Set hysteresis/deadband (e.g., ±1°C)
 * - AC turns ON when room temp > target + hysteresis
 * - AC turns OFF when room temp < target - hysteresis
 * - Prevents rapid cycling
 *
 * Example with 24°C target and 1°C hysteresis:
 * - AC turns ON at 25°C (24 + 1)
 * - AC turns OFF at 23°C (24 - 1)
 * - Between 23-25°C: AC maintains current state
 */

#include <ClimateCard.hpp>

// ============================================================================
// IR Code Definitions (same structure as other examples)
// ============================================================================

const uint16_t irCodes[3][4][15][67] = {
    // Mode 0: Off
    {{{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}}},
    // Mode 1: Cool
    {{{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}}},
    // Mode 2: Fan Only
    {{{9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560}}}
};

size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr) {
    *codePtr = irCodes[mode][fan_speed][temperature];
    return sizeof(irCodes[mode][fan_speed][temperature]) / sizeof(uint16_t);
}

const char *mode_names[] = {"off", "cool", "fan_only"};
const char *fan_speed_names[] = {"auto", "low", "medium", "high"};

AirConditioner myAC = {
    .max_temperature = 30,
    .min_temperature = 16,
    .modes = 3,
    .mode_names = mode_names,
    .fan_speeds = 4,
    .fan_speed_names = fan_speed_names,
    .getInfraredCode = getInfraredCode
};

// ============================================================================
// Hardware Configuration
// ============================================================================

#define IR_TX_PIN 14
#define SENSOR_PIN 27
#define SENSOR_TYPE AC_SENSOR_TYPE_DHT22  // Change to AC_SENSOR_TYPE_DS18B20 if using DS18B20
#define RMT_CHANNEL RMT_CHANNEL_0

ClimateCard ac(IR_TX_PIN, myAC, SENSOR_TYPE, SENSOR_PIN, RMT_CHANNEL);

// ============================================================================
// Thermostat Configuration
// ============================================================================

// Thermostat settings (adjustable via serial commands)
float targetTemperature = 24.0;     // Target temperature in °C
float hysteresis = 1.0;             // Deadband in °C (prevents rapid cycling)
float minRunTime = 300000;          // Minimum AC run time: 5 minutes (in milliseconds)
float minOffTime = 180000;          // Minimum AC off time: 3 minutes
bool thermostatEnabled = true;      // Thermostat on/off
bool manualOverride = false;        // Manual control override

// Thermostat state
enum ThermostatState {
    STATE_OFF,          // AC is off
    STATE_COOLING,      // AC is cooling
    STATE_FAN_ONLY,     // Fan only (between thresholds)
    STATE_WAITING       // Waiting for min run/off time
};

ThermostatState currentState = STATE_OFF;
unsigned long lastStateChange = 0;
unsigned long acOnTime = 0;
unsigned long acOffTime = 0;

// Statistics
unsigned long totalRunTime = 0;
unsigned long totalCycles = 0;
float maxRoomTemp = -100.0;
float minRoomTemp = 100.0;

// ============================================================================
// Callback Functions
// ============================================================================

void onSensorUpdate(float temperature, float humidity) {
    // Update temperature statistics
    if (temperature > maxRoomTemp) maxRoomTemp = temperature;
    if (temperature < minRoomTemp && temperature > 0) minRoomTemp = temperature;

    // Display reading
    Serial.printf("[SENSOR] Temp: %.1f°C", temperature);
    if (SENSOR_TYPE == AC_SENSOR_TYPE_DHT22) {
        Serial.printf(", Humidity: %.1f%%", humidity);
    }
    Serial.println();

    // Run thermostat logic if enabled and not in manual override
    if (thermostatEnabled && !manualOverride) {
        runThermostatLogic(temperature);
    }
}

void onACStateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.printf("[AC] State changed: %s, %s, %d°C\n",
                  ac.getModeName(), ac.getFanSpeedName(), temperature);
}

// ============================================================================
// Thermostat Logic
// ============================================================================

void runThermostatLogic(float roomTemp) {
    // Skip if no valid reading
    if (roomTemp == 0) return;

    unsigned long now = millis();
    unsigned long timeSinceStateChange = now - lastStateChange;

    // Calculate thresholds
    float upperThreshold = targetTemperature + hysteresis;
    float lowerThreshold = targetTemperature - hysteresis;

    ThermostatState newState = currentState;
    bool changeState = false;

    // Determine desired state based on temperature
    if (roomTemp > upperThreshold) {
        // Too hot - need cooling
        if (currentState == STATE_OFF) {
            // Check minimum off time
            if (timeSinceStateChange >= minOffTime) {
                newState = STATE_COOLING;
                changeState = true;
                Serial.printf("[THERMOSTAT] Room too hot (%.1f°C > %.1f°C) - Starting cooling\n",
                              roomTemp, upperThreshold);
            } else {
                Serial.printf("[THERMOSTAT] Waiting %.0f more seconds before starting AC\n",
                              (minOffTime - timeSinceStateChange) / 1000.0);
            }
        }
    }
    else if (roomTemp < lowerThreshold) {
        // Too cold - turn off AC
        if (currentState == STATE_COOLING) {
            // Check minimum run time
            if (timeSinceStateChange >= minRunTime) {
                newState = STATE_OFF;
                changeState = true;
                Serial.printf("[THERMOSTAT] Target reached (%.1f°C < %.1f°C) - Stopping AC\n",
                              roomTemp, lowerThreshold);
            } else {
                Serial.printf("[THERMOSTAT] Waiting %.0f more seconds before stopping AC\n",
                              (minRunTime - timeSinceStateChange) / 1000.0);
            }
        }
    }
    else {
        // In deadband (between thresholds) - maintain current state
        Serial.printf("[THERMOSTAT] In comfort zone (%.1f°C - %.1f°C)\n",
                      lowerThreshold, upperThreshold);

        // Optionally switch to fan-only in deadband if AC is running
        if (currentState == STATE_COOLING && timeSinceStateChange >= minRunTime) {
            // Uncomment to enable fan-only mode in deadband:
            // newState = STATE_FAN_ONLY;
            // changeState = true;
        }
    }

    // Execute state change if needed
    if (changeState && newState != currentState) {
        changeStateTo(newState);
    }
}

void changeStateTo(ThermostatState newState) {
    unsigned long now = millis();

    // Update statistics
    if (currentState == STATE_COOLING) {
        unsigned long runDuration = now - lastStateChange;
        totalRunTime += runDuration;
        acOnTime += runDuration;
        totalCycles++;
    } else {
        unsigned long offDuration = now - lastStateChange;
        acOffTime += offDuration;
    }

    // Change AC settings based on new state
    switch (newState) {
        case STATE_OFF:
            Serial.println("[THERMOSTAT] → Turning AC OFF");
            ac.setMode(0);  // Off
            break;

        case STATE_COOLING:
            Serial.printf("[THERMOSTAT] → Starting COOLING to %.0f°C\n", targetTemperature);
            // Set AC to cooling mode, auto fan, target temp
            ac.setState(1, 0, (uint8_t)targetTemperature);
            break;

        case STATE_FAN_ONLY:
            Serial.println("[THERMOSTAT] → Switching to FAN ONLY");
            ac.setMode(2);  // Fan only
            break;

        case STATE_WAITING:
            // No AC change, just waiting
            break;
    }

    // Update state
    currentState = newState;
    lastStateChange = now;

    // Display statistics
    displayStatistics();
}

// ============================================================================
// Display Functions
// ============================================================================

void displayStatus() {
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║            Thermostat Status                      ║");
    Serial.println("╠════════════════════════════════════════════════════╣");

    // Thermostat settings
    Serial.printf("║  Thermostat: %-35s ║\n", thermostatEnabled ? "ENABLED" : "DISABLED");
    Serial.printf("║  Mode: %-42s ║\n", manualOverride ? "MANUAL OVERRIDE" : "AUTOMATIC");
    Serial.printf("║  Target Temperature: %6.1f°C                     ║\n", targetTemperature);
    Serial.printf("║  Hysteresis: ±%6.1f°C                          ║\n", hysteresis);
    Serial.printf("║  Control Range: %.1f°C - %.1f°C                    ║\n",
                  targetTemperature - hysteresis, targetTemperature + hysteresis);

    Serial.println("╠════════════════════════════════════════════════════╣");

    // Current readings
    float roomTemp = ac.getRoomTemperature();
    Serial.printf("║  Room Temperature: %6.1f°C                       ║\n", roomTemp);
    if (SENSOR_TYPE == AC_SENSOR_TYPE_DHT22) {
        Serial.printf("║  Room Humidity: %6.1f%%                          ║\n", ac.getHumidity());
    }

    // AC status
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.printf("║  AC State: %-39s ║\n",
                  currentState == STATE_OFF ? "OFF" :
                  currentState == STATE_COOLING ? "COOLING" :
                  currentState == STATE_FAN_ONLY ? "FAN ONLY" : "WAITING");
    Serial.printf("║  AC Mode: %-40s ║\n", ac.getModeName());
    Serial.printf("║  AC Fan: %-41s ║\n", ac.getFanSpeedName());
    Serial.printf("║  AC Temperature: %6d°C                         ║\n", ac.getTemperature());

    // Time in current state
    unsigned long timeInState = (millis() - lastStateChange) / 1000;
    Serial.printf("║  Time in State: %3lu min %2lu sec                    ║\n",
                  timeInState / 60, timeInState % 60);

    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

void displayStatistics() {
    Serial.println("\n┌──────────────────────────────────────┐");
    Serial.println("│  Thermostat Statistics               │");
    Serial.println("├──────────────────────────────────────┤");
    Serial.printf("│  Total Cycles: %6lu                 │\n", totalCycles);
    Serial.printf("│  Total Run Time: %3lu min             │\n", totalRunTime / 60000);
    Serial.printf("│  Total Off Time: %3lu min             │\n", acOffTime / 60000);

    if (totalCycles > 0) {
        unsigned long avgCycleTime = totalRunTime / totalCycles;
        Serial.printf("│  Avg Cycle: %3lu min                  │\n", avgCycleTime / 60000);
    }

    if (maxRoomTemp > -100.0) {
        Serial.printf("│  Max Temp: %.1f°C                     │\n", maxRoomTemp);
        Serial.printf("│  Min Temp: %.1f°C                     │\n", minRoomTemp);
    }

    // Energy estimate (very rough)
    // Assuming 1kW AC power consumption
    float energyKWh = (totalRunTime / 1000.0 / 3600.0) * 1.0;
    Serial.printf("│  Est. Energy: %.2f kWh                │\n", energyKWh);

    Serial.println("└──────────────────────────────────────┘\n");
}

void displayHelp() {
    Serial.println("\n╔════════════════════════════════════════════════════╗");
    Serial.println("║            Serial Commands                        ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  status              - Display current status     ║");
    Serial.println("║  stats               - Display statistics         ║");
    Serial.println("║  target <temp>       - Set target temperature     ║");
    Serial.println("║  hysteresis <value>  - Set hysteresis (°C)        ║");
    Serial.println("║  enable              - Enable thermostat          ║");
    Serial.println("║  disable             - Disable thermostat         ║");
    Serial.println("║  manual <mode>       - Manual override (on/off)   ║");
    Serial.println("║  auto                - Return to automatic mode   ║");
    Serial.println("║  reset               - Reset statistics           ║");
    Serial.println("║  help                - Show this help             ║");
    Serial.println("╠════════════════════════════════════════════════════╣");
    Serial.println("║  Examples:                                        ║");
    Serial.println("║    target 25         - Set target to 25°C         ║");
    Serial.println("║    hysteresis 1.5    - Set ±1.5°C deadband        ║");
    Serial.println("║    manual on         - Turn AC on manually        ║");
    Serial.println("╚════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// Serial Command Processing
// ============================================================================

void processSerialCommand() {
    if (!Serial.available()) return;

    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();

    if (command == "status") {
        displayStatus();
    }
    else if (command == "stats") {
        displayStatistics();
    }
    else if (command == "help") {
        displayHelp();
    }
    else if (command == "enable") {
        thermostatEnabled = true;
        Serial.println("[CMD] Thermostat ENABLED");
    }
    else if (command == "disable") {
        thermostatEnabled = false;
        Serial.println("[CMD] Thermostat DISABLED");
    }
    else if (command == "auto") {
        manualOverride = false;
        Serial.println("[CMD] Switched to AUTOMATIC mode");
    }
    else if (command == "reset") {
        totalRunTime = 0;
        totalCycles = 0;
        acOnTime = 0;
        acOffTime = 0;
        maxRoomTemp = -100.0;
        minRoomTemp = 100.0;
        Serial.println("[CMD] Statistics reset");
    }
    else if (command.startsWith("target ")) {
        float newTarget = command.substring(7).toFloat();
        if (newTarget >= myAC.min_temperature && newTarget <= myAC.max_temperature) {
            targetTemperature = newTarget;
            Serial.printf("[CMD] Target temperature set to %.1f°C\n", targetTemperature);
        } else {
            Serial.printf("[CMD] ERROR: Temperature must be %d-%d°C\n",
                          myAC.min_temperature, myAC.max_temperature);
        }
    }
    else if (command.startsWith("hysteresis ")) {
        float newHyst = command.substring(11).toFloat();
        if (newHyst >= 0.5 && newHyst <= 5.0) {
            hysteresis = newHyst;
            Serial.printf("[CMD] Hysteresis set to ±%.1f°C\n", hysteresis);
        } else {
            Serial.println("[CMD] ERROR: Hysteresis must be 0.5-5.0°C");
        }
    }
    else if (command.startsWith("manual ")) {
        String mode = command.substring(7);
        manualOverride = true;
        if (mode == "on") {
            Serial.println("[CMD] Manual override: AC ON");
            ac.setState(1, 0, (uint8_t)targetTemperature);
        } else if (mode == "off") {
            Serial.println("[CMD] Manual override: AC OFF");
            ac.setMode(0);
        } else {
            Serial.println("[CMD] ERROR: Use 'manual on' or 'manual off'");
        }
    }
    else {
        Serial.println("[CMD] Unknown command. Type 'help' for available commands.");
    }
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════╗");
    Serial.println("║        Smart Thermostat with ClimateCard          ║");
    Serial.println("╚════════════════════════════════════════════════════╝");
    Serial.println();

    // Initialize ClimateCard
    Serial.println("Initializing ClimateCard...");
    if (!ac.begin()) {
        Serial.println("ERROR: ClimateCard initialization failed!");
        while (1) delay(1000);
    }
    Serial.println("✓ ClimateCard initialized");
    Serial.println();

    // Register callbacks
    ac.registerSensorCallback(onSensorUpdate);
    ac.registerChangeCallback(onACStateChange);

    // Display configuration
    Serial.println("Thermostat Configuration:");
    Serial.printf("  Target Temperature: %.1f°C\n", targetTemperature);
    Serial.printf("  Hysteresis: ±%.1f°C\n", hysteresis);
    Serial.printf("  Control Range: %.1f°C - %.1f°C\n",
                  targetTemperature - hysteresis,
                  targetTemperature + hysteresis);
    Serial.printf("  Min Run Time: %.0f minutes\n", minRunTime / 60000.0);
    Serial.printf("  Min Off Time: %.0f minutes\n", minOffTime / 60000.0);
    Serial.println();

    // Start with AC off
    ac.setMode(0);
    currentState = STATE_OFF;
    lastStateChange = millis();

    Serial.println("Thermostat is ACTIVE");
    Serial.println("Waiting for sensor readings...");
    Serial.println();
    Serial.println("Type 'help' for available commands");
    Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    // Update ClimateCard (sensor reads every 5 seconds)
    ac.loop();

    // Process serial commands
    processSerialCommand();

    // Periodic status display (every 60 seconds)
    static unsigned long lastStatusDisplay = 0;
    if (millis() - lastStatusDisplay >= 60000) {
        lastStatusDisplay = millis();
        displayStatus();
    }
}

// ============================================================================
// Notes
// ============================================================================

/*
 * Hysteresis Explained:
 *
 * Hysteresis creates a "deadband" around the target temperature to prevent
 * the AC from cycling on and off too frequently (called "short cycling").
 *
 * Example with target=24°C, hysteresis=1°C:
 *   - AC turns ON when room reaches 25°C (24 + 1)
 *   - AC runs until room cools to 23°C (24 - 1)
 *   - AC turns OFF at 23°C
 *   - Between 23-25°C: AC maintains current state (no change)
 *
 * Benefits of hysteresis:
 *   - Prevents mechanical wear from frequent starts
 *   - Reduces energy consumption
 *   - More comfortable (less temperature swings)
 *   - Quieter operation
 *
 * Tuning Tips:
 *
 * - Larger hysteresis (2-3°C): Fewer cycles, more temperature variation
 * - Smaller hysteresis (0.5-1°C): More cycles, tighter temperature control
 * - Adjust based on room size, insulation, and personal preference
 * - Consider AC capacity: Oversized AC needs larger hysteresis
 *
 * Advanced Features to Add:
 *
 * - PID control for precise temperature maintenance
 * - Schedule-based temperature settings (day/night)
 * - Learning algorithm to predict cooling times
 * - Multiple zones with individual setpoints
 * - Integration with occupancy sensors
 * - Weather-based adjustments
 * - Energy usage optimization
 */
