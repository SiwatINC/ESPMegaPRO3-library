/**
 * Energy Logging with FRAM Persistence Example
 *
 * This example demonstrates energy monitoring with persistent storage using FRAM.
 * Energy data is saved to FRAM periodically to prevent data loss during power outages.
 * On restart, the previous energy value is restored from FRAM.
 *
 * Features:
 * - Energy accumulation and storage
 * - FRAM persistence for data retention
 * - Smart saving strategy to minimize FRAM wear
 * - Energy reset command via serial
 * - Detailed logging and statistics
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board with FRAM
 * - Analog Card
 * - Current Transformer (e.g., SCT-013-000)
 * - Burden resistor circuit
 *
 * Author: SIWAT INC
 * Date: 2024
 */

#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>
#include <FRAM.h>

// FRAM is a global instance in ESPMegaPRO
extern FRAM ESPMega_FRAM;

// FRAM address for storing energy data (uses 8 bytes)
#define CT_FRAM_ADDRESS 1000

// Create analog card instance
AnalogCard analogCard;

// Line voltage (adjust for your region)
float lineVoltage = 230.0;  // 230V for EU/Asia, 120V for US

// ADC to Current conversion for SCT-013-000 with 33Ω burden
auto adcToCurrent = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;
    const float SCALE_FACTOR = 0.0488;  // 100A / 2048 steps
    return (adc - ADC_ZERO) * SCALE_FACTOR;
};

// Create Current Transformer Card
CurrentTransformerCard ct(
    &analogCard,
    0,              // Analog input A0
    &lineVoltage,
    adcToCurrent,
    1000            // Sample every 1 second
);

// Energy saving configuration
const double ENERGY_SAVE_THRESHOLD = 5.0;   // Save every 5 Wh
const unsigned long TIME_SAVE_INTERVAL = 300000;  // Save every 5 minutes
double lastSavedEnergy = 0;
unsigned long lastSaveTime = 0;

// Statistics
unsigned long startTime = 0;
double maxPower = 0;
double minPower = 99999;
double totalPowerReadings = 0;
unsigned long powerReadingCount = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("========================================");
    Serial.println("  Energy Logger with FRAM Persistence");
    Serial.println("========================================");
    Serial.println();

    // Initialize FRAM
    Serial.print("Initializing FRAM...");
    if (!ESPMega_FRAM.begin()) {
        Serial.println(" FAILED!");
        Serial.println("ERROR: FRAM initialization failed!");
        Serial.println("Energy data will not be persisted.");
        Serial.println("System will continue without persistence.");
        delay(3000);
    } else {
        Serial.println(" OK");
    }

    // Initialize Analog Card
    Serial.print("Initializing Analog Card...");
    if (!analogCard.begin()) {
        Serial.println(" FAILED!");
        Serial.println("FATAL ERROR: Cannot continue without Analog Card");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" OK");

    // Initialize CT Card
    Serial.print("Initializing Current Transformer Card...");
    if (!ct.begin()) {
        Serial.println(" FAILED!");
        Serial.println("FATAL ERROR: CT initialization failed");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" OK");

    // Bind FRAM to CT card
    Serial.print("Binding FRAM to CT card...");
    ct.bindFRAM(&ESPMega_FRAM, CT_FRAM_ADDRESS);
    Serial.println(" OK");

    // Load previously stored energy from FRAM
    Serial.print("Loading energy from FRAM...");
    ct.loadEnergy();
    double restoredEnergy = ct.getEnergy();
    Serial.printf(" %.3f Wh\n", restoredEnergy);

    if (restoredEnergy > 0) {
        Serial.printf("Successfully restored %.3f Wh (%.3f kWh) from FRAM\n",
            restoredEnergy, restoredEnergy / 1000.0);
    } else {
        Serial.println("No previous energy data found or reset to zero");
    }

    lastSavedEnergy = restoredEnergy;
    startTime = millis();

    Serial.println();
    Serial.println("Configuration:");
    Serial.printf("  - FRAM Address: %d (8 bytes)\n", CT_FRAM_ADDRESS);
    Serial.printf("  - Save Threshold: %.1f Wh\n", ENERGY_SAVE_THRESHOLD);
    Serial.printf("  - Time Save Interval: %lu ms (%.1f min)\n",
        TIME_SAVE_INTERVAL, TIME_SAVE_INTERVAL / 60000.0);
    Serial.printf("  - Line Voltage: %.1f V\n", lineVoltage);
    Serial.println();

    Serial.println("Commands:");
    Serial.println("  - Type 'r' to reset energy counter");
    Serial.println("  - Type 's' to manually save energy to FRAM");
    Serial.println("  - Type 'i' to display system info");
    Serial.println();

    delay(2000);

    Serial.println("Logging started...");
    Serial.println();
}

void loop() {
    // Update CT card
    ct.loop();

    // Check for serial commands
    handleSerialCommands();

    // Smart energy saving logic
    handleEnergySaving();

    // Update statistics
    updateStatistics();

    // Print status every 5 seconds
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint >= 5000) {
        printStatus();
        lastStatusPrint = millis();
    }

    delay(100);
}

/**
 * Handle serial commands for user interaction
 */
void handleSerialCommands() {
    if (Serial.available() > 0) {
        char command = Serial.read();

        switch (command) {
            case 'r':
            case 'R':
                // Reset energy counter
                Serial.println();
                Serial.println("=== RESET ENERGY COUNTER ===");
                Serial.printf("Previous energy: %.3f Wh\n", ct.getEnergy());
                ct.resetEnergy();
                ct.saveEnergy();
                lastSavedEnergy = 0;
                Serial.println("Energy counter reset to 0");
                Serial.println("Reset saved to FRAM");
                Serial.println();
                break;

            case 's':
            case 'S':
                // Manual save
                Serial.println();
                Serial.println("=== MANUAL SAVE ===");
                ct.saveEnergy();
                lastSavedEnergy = ct.getEnergy();
                lastSaveTime = millis();
                Serial.printf("Energy saved: %.3f Wh\n", lastSavedEnergy);
                Serial.println();
                break;

            case 'i':
            case 'I':
                // Display info
                printDetailedInfo();
                break;

            default:
                // Ignore unknown commands
                break;
        }

        // Clear remaining serial buffer
        while (Serial.available() > 0) {
            Serial.read();
        }
    }
}

/**
 * Smart energy saving to minimize FRAM wear while preventing data loss
 */
void handleEnergySaving() {
    double currentEnergy = ct.getEnergy();
    unsigned long now = millis();

    // Calculate energy difference since last save
    double energyDelta = currentEnergy - lastSavedEnergy;

    // Trigger conditions
    bool thresholdTrigger = (energyDelta >= ENERGY_SAVE_THRESHOLD);
    bool timeTrigger = (now - lastSaveTime >= TIME_SAVE_INTERVAL);

    if (thresholdTrigger || timeTrigger) {
        // Save to FRAM
        ct.saveEnergy();
        lastSavedEnergy = currentEnergy;
        lastSaveTime = now;

        // Log the save event
        Serial.println();
        if (thresholdTrigger) {
            Serial.printf(">>> Threshold save: %.3f Wh (delta: %.3f Wh)\n",
                currentEnergy, energyDelta);
        } else {
            Serial.printf(">>> Periodic save: %.3f Wh\n", currentEnergy);
        }
        Serial.println();
    }
}

/**
 * Update power statistics
 */
void updateStatistics() {
    float power = ct.getPower();

    if (power > maxPower) {
        maxPower = power;
    }

    if (power < minPower) {
        minPower = power;
    }

    totalPowerReadings += power;
    powerReadingCount++;
}

/**
 * Print current status to serial
 */
void printStatus() {
    float current = ct.getCurrent();
    float power = ct.getPower();
    double energy = ct.getEnergy();

    // Calculate running time
    unsigned long runTime = (millis() - startTime) / 1000;
    unsigned long hours = runTime / 3600;
    unsigned long minutes = (runTime % 3600) / 60;
    unsigned long seconds = runTime % 60;

    // Calculate unsaved energy
    double unsavedEnergy = energy - lastSavedEnergy;

    Serial.printf("[%02lu:%02lu:%02lu] Current: %6.2f A | Power: %8.2f W | Energy: %10.3f Wh | Unsaved: %6.3f Wh\n",
        hours, minutes, seconds,
        current,
        power,
        energy,
        unsavedEnergy
    );
}

/**
 * Print detailed system information
 */
void printDetailedInfo() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("           SYSTEM INFORMATION");
    Serial.println("========================================");
    Serial.println();

    // Current measurements
    Serial.println("Current Measurements:");
    Serial.printf("  Current:  %.3f A\n", ct.getCurrent());
    Serial.printf("  Voltage:  %.1f V\n", ct.getVoltage());
    Serial.printf("  Power:    %.2f W (%.3f kW)\n", ct.getPower(), ct.getPower() / 1000.0);
    Serial.println();

    // Energy data
    double energy = ct.getEnergy();
    Serial.println("Energy Data:");
    Serial.printf("  Total Energy:     %.3f Wh (%.3f kWh)\n", energy, energy / 1000.0);
    Serial.printf("  Last Saved:       %.3f Wh\n", lastSavedEnergy);
    Serial.printf("  Unsaved:          %.3f Wh\n", energy - lastSavedEnergy);
    Serial.printf("  FRAM Address:     %d\n", CT_FRAM_ADDRESS);
    Serial.println();

    // Statistics
    double avgPower = (powerReadingCount > 0) ? (totalPowerReadings / powerReadingCount) : 0;
    Serial.println("Statistics:");
    Serial.printf("  Peak Power:       %.2f W\n", maxPower);
    Serial.printf("  Min Power:        %.2f W\n", minPower);
    Serial.printf("  Average Power:    %.2f W\n", avgPower);
    Serial.printf("  Readings:         %lu\n", powerReadingCount);
    Serial.println();

    // Running time
    unsigned long runTime = (millis() - startTime) / 1000;
    unsigned long hours = runTime / 3600;
    unsigned long minutes = (runTime % 3600) / 60;
    unsigned long seconds = runTime % 60;
    Serial.println("Runtime:");
    Serial.printf("  Uptime:           %02lu:%02lu:%02lu\n", hours, minutes, seconds);
    Serial.printf("  Total Seconds:    %lu\n", runTime);
    Serial.println();

    // Cost estimation (example: $0.12 per kWh)
    float costPerKWh = 0.12;
    float totalCost = (energy / 1000.0) * costPerKWh;
    Serial.println("Cost Estimation:");
    Serial.printf("  Rate:             $%.3f per kWh\n", costPerKWh);
    Serial.printf("  Total Cost:       $%.4f\n", totalCost);
    Serial.println();

    // Save status
    unsigned long timeSinceLastSave = millis() - lastSaveTime;
    Serial.println("Save Status:");
    Serial.printf("  Last Save:        %lu ms ago (%.1f min)\n",
        timeSinceLastSave, timeSinceLastSave / 60000.0);
    Serial.printf("  Next Save:        In %.1f Wh or %.1f min\n",
        ENERGY_SAVE_THRESHOLD - (energy - lastSavedEnergy),
        (TIME_SAVE_INTERVAL - timeSinceLastSave) / 60000.0);
    Serial.println();

    Serial.println("========================================");
    Serial.println();
}

/*
 * Usage Notes:
 *
 * 1. Energy Persistence:
 *    - Energy is automatically saved based on threshold and time triggers
 *    - Maximum data loss: ENERGY_SAVE_THRESHOLD or TIME_SAVE_INTERVAL
 *    - Adjust these values based on your needs
 *
 * 2. FRAM Wear:
 *    - FRAM supports 10 trillion write cycles
 *    - With current settings (5 Wh threshold, 5 min interval):
 *      - At 1000W load: ~200 writes/day
 *      - At 100W load: ~20 writes/day
 *    - Expected FRAM lifetime: >100 years
 *
 * 3. Power Failure Recovery:
 *    - On power restoration, energy counter continues from last save
 *    - Use shorter TIME_SAVE_INTERVAL for critical applications
 *    - Use ct.setEnergyAutoSave(true) for maximum reliability (more wear)
 *
 * 4. Calibration:
 *    - Use 'i' command to monitor average power
 *    - Compare with known load to verify accuracy
 *    - Adjust SCALE_FACTOR in adcToCurrent if needed
 */
