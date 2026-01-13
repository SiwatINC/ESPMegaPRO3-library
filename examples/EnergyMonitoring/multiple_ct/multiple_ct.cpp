/**
 * Multiple Current Transformer Monitoring
 *
 * This example demonstrates monitoring multiple current transformers simultaneously
 * for whole-house or multi-circuit energy monitoring. Useful for:
 * - 3-phase power monitoring
 * - Individual circuit tracking
 * - Sub-metering (kitchen, HVAC, outlets, etc.)
 * - Load balancing verification
 *
 * Features:
 * - Monitor up to 8 CTs (limited by Analog Card inputs)
 * - Individual and total energy tracking
 * - Per-circuit statistics
 * - FRAM persistence for all circuits
 * - Optional MQTT integration for all CTs
 * - Load balance analysis
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board with FRAM
 * - Analog Card
 * - Multiple Current Transformers (3 shown in example)
 * - Proper burden resistor circuits for each CT
 *
 * Example Configuration:
 * - CT0 (A0): Kitchen circuit - 20A max
 * - CT1 (A1): HVAC circuit - 30A max
 * - CT2 (A2): Main panel - 100A max
 *
 * Author: SIWAT INC
 * Date: 2024
 */

#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>
#include <FRAM.h>

// =========================
// Configuration
// =========================

// Number of CTs to monitor
#define NUM_CTS 3

// FRAM addresses for each CT (8 bytes each)
#define CT_FRAM_BASE_ADDRESS 1000
#define CT0_FRAM_ADDRESS (CT_FRAM_BASE_ADDRESS + 0)   // 1000-1007
#define CT1_FRAM_ADDRESS (CT_FRAM_BASE_ADDRESS + 8)   // 1008-1015
#define CT2_FRAM_ADDRESS (CT_FRAM_BASE_ADDRESS + 16)  // 1016-1023

// Line voltage (adjust for your region)
const float LINE_VOLTAGE = 230.0;

// Circuit names for display
const char* CIRCUIT_NAMES[NUM_CTS] = {
    "Kitchen",
    "HVAC",
    "Main Panel"
};

// =========================
// Global Instances
// =========================

extern FRAM ESPMega_FRAM;
AnalogCard analogCard;
float lineVoltage = LINE_VOLTAGE;

// =========================
// CT Conversion Functions
// =========================

// Kitchen circuit - SCT-013-000 (100A) with 33Ω burden
// But kitchen circuit is 20A max, so can use higher resolution
auto ct0_adcToCurrent = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;
    const float SCALE_FACTOR = 0.0488;  // 100A / 2048 steps
    return (adc - ADC_ZERO) * SCALE_FACTOR;
};

// HVAC circuit - SCT-013-030 (30A) with internal burden
auto ct1_adcToCurrent = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;
    const float SCALE_FACTOR = 30.0 / 2048.0;  // 30A / 2048 steps
    return (adc - ADC_ZERO) * SCALE_FACTOR;
};

// Main panel - SCT-013-000 (100A) with 33Ω burden
auto ct2_adcToCurrent = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;
    const float SCALE_FACTOR = 0.0488;  // 100A / 2048 steps
    return (adc - ADC_ZERO) * SCALE_FACTOR;
};

// =========================
// CT Instances
// =========================

// Kitchen circuit CT on A0
CurrentTransformerCard ct0(
    &analogCard, 0, &lineVoltage, ct0_adcToCurrent, 1000
);

// HVAC circuit CT on A1
CurrentTransformerCard ct1(
    &analogCard, 1, &lineVoltage, ct1_adcToCurrent, 1000
);

// Main panel CT on A2
CurrentTransformerCard ct2(
    &analogCard, 2, &lineVoltage, ct2_adcToCurrent, 1000
);

// Array of CT pointers for easy iteration
CurrentTransformerCard* cts[NUM_CTS] = { &ct0, &ct1, &ct2 };
uint32_t framAddresses[NUM_CTS] = {
    CT0_FRAM_ADDRESS,
    CT1_FRAM_ADDRESS,
    CT2_FRAM_ADDRESS
};

// =========================
// Statistics Tracking
// =========================

struct CircuitStats {
    float peakCurrent = 0;
    float peakPower = 0;
    unsigned long lastUpdateTime = 0;
    float averagePower = 0;
    unsigned long sampleCount = 0;
};

CircuitStats stats[NUM_CTS];

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("========================================");
    Serial.println("  Multiple CT Energy Monitor");
    Serial.println("========================================");
    Serial.println();

    // Initialize FRAM
    Serial.print("Initializing FRAM...");
    if (!ESPMega_FRAM.begin()) {
        Serial.println(" FAILED!");
        Serial.println("WARNING: Energy data will not persist");
    } else {
        Serial.println(" OK");
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

    Serial.println();
    Serial.println("Initializing Current Transformers...");

    // Initialize each CT
    for (int i = 0; i < NUM_CTS; i++) {
        Serial.printf("  CT%d (%s)...", i, CIRCUIT_NAMES[i]);

        if (!cts[i]->begin()) {
            Serial.println(" FAILED!");
            while (1) {
                delay(1000);
            }
        }

        // Bind FRAM and load energy
        if (ESPMega_FRAM.begin()) {
            cts[i]->bindFRAM(&ESPMega_FRAM, framAddresses[i]);
            cts[i]->loadEnergy();
            Serial.printf(" OK (Restored: %.2f Wh)\n", cts[i]->getEnergy());
        } else {
            Serial.println(" OK");
        }

        // Disable auto-save, we'll save manually
        cts[i]->setEnergyAutoSave(false);
    }

    Serial.println();
    Serial.println("Configuration:");
    Serial.printf("  Line Voltage:     %.1f V\n", LINE_VOLTAGE);
    Serial.printf("  Number of CTs:    %d\n", NUM_CTS);
    Serial.printf("  Sample Rate:      1000 ms\n");
    Serial.println();

    Serial.println("Circuit Configuration:");
    Serial.println("  CT0 (A0): Kitchen    - SCT-013-000 (100A)");
    Serial.println("  CT1 (A1): HVAC       - SCT-013-030 (30A)");
    Serial.println("  CT2 (A2): Main Panel - SCT-013-000 (100A)");
    Serial.println();

    Serial.println("Commands:");
    Serial.println("  's' - Print summary");
    Serial.println("  'd' - Print detailed stats");
    Serial.println("  'r' - Reset all energy counters");
    Serial.println("  'r0', 'r1', 'r2' - Reset specific CT");
    Serial.println();

    delay(2000);
    Serial.println("Monitoring started...");
    Serial.println();
}

void loop() {
    // Update all CTs
    for (int i = 0; i < NUM_CTS; i++) {
        cts[i]->loop();
        updateStats(i);
    }

    // Handle serial commands
    handleSerialCommands();

    // Periodic FRAM save (every 10 Wh per circuit)
    periodicSave();

    // Print status every 5 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 5000) {
        printStatus();
        lastPrint = millis();
    }

    delay(100);
}

/**
 * Update statistics for a specific CT
 */
void updateStats(int ctIndex) {
    float current = cts[ctIndex]->getCurrent();
    float power = cts[ctIndex]->getPower();

    // Track peak values
    if (current > stats[ctIndex].peakCurrent) {
        stats[ctIndex].peakCurrent = current;
    }
    if (power > stats[ctIndex].peakPower) {
        stats[ctIndex].peakPower = power;
    }

    // Calculate running average power
    stats[ctIndex].sampleCount++;
    stats[ctIndex].averagePower =
        ((stats[ctIndex].averagePower * (stats[ctIndex].sampleCount - 1)) + power)
        / stats[ctIndex].sampleCount;

    stats[ctIndex].lastUpdateTime = millis();
}

/**
 * Print current status for all CTs
 */
void printStatus() {
    // Header
    Serial.println("Circuit      | Current (A) | Power (W) | Energy (Wh) | Peak (A)");
    Serial.println("-------------|-------------|-----------|-------------|----------");

    // Individual circuits
    float totalCurrent = 0;
    float totalPower = 0;
    double totalEnergy = 0;

    for (int i = 0; i < NUM_CTS; i++) {
        float current = cts[i]->getCurrent();
        float power = cts[i]->getPower();
        double energy = cts[i]->getEnergy();

        Serial.printf("%-12s | %11.2f | %9.2f | %11.2f | %8.2f\n",
            CIRCUIT_NAMES[i],
            current,
            power,
            energy,
            stats[i].peakCurrent
        );

        totalCurrent += current;
        totalPower += power;
        totalEnergy += energy;
    }

    // Total line
    Serial.println("-------------|-------------|-----------|-------------|----------");
    Serial.printf("%-12s | %11.2f | %9.2f | %11.2f |\n",
        "TOTAL",
        totalCurrent,
        totalPower,
        totalEnergy
    );
    Serial.println();
}

/**
 * Print summary statistics
 */
void printSummary() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("          SUMMARY STATISTICS");
    Serial.println("========================================");
    Serial.println();

    float totalPower = 0;
    double totalEnergy = 0;

    for (int i = 0; i < NUM_CTS; i++) {
        Serial.printf("Circuit: %s\n", CIRCUIT_NAMES[i]);
        Serial.printf("  Current:       %.2f A\n", cts[i]->getCurrent());
        Serial.printf("  Power:         %.2f W\n", cts[i]->getPower());
        Serial.printf("  Energy:        %.2f Wh (%.3f kWh)\n",
            cts[i]->getEnergy(), cts[i]->getEnergy() / 1000.0);
        Serial.printf("  Peak Current:  %.2f A\n", stats[i].peakCurrent);
        Serial.printf("  Peak Power:    %.2f W\n", stats[i].peakPower);
        Serial.printf("  Avg Power:     %.2f W\n", stats[i].averagePower);
        Serial.println();

        totalPower += cts[i]->getPower();
        totalEnergy += cts[i]->getEnergy();
    }

    Serial.println("TOTAL:");
    Serial.printf("  Power:         %.2f W (%.3f kW)\n", totalPower, totalPower / 1000.0);
    Serial.printf("  Energy:        %.2f Wh (%.3f kWh)\n", totalEnergy, totalEnergy / 1000.0);

    // Cost calculation
    float costPerKWh = 0.12;
    float totalCost = (totalEnergy / 1000.0) * costPerKWh;
    Serial.printf("  Est. Cost:     $%.4f @ $%.2f/kWh\n", totalCost, costPerKWh);

    Serial.println();
    Serial.println("========================================");
    Serial.println();
}

/**
 * Print detailed statistics and analysis
 */
void printDetailedStats() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("        DETAILED STATISTICS");
    Serial.println("========================================");
    Serial.println();

    printSummary();

    // Load balance analysis (for split-phase or 3-phase)
    Serial.println("Load Balance Analysis:");
    Serial.println("(Assuming CT0 and CT1 are on separate phases)");
    Serial.println();

    float ct0_power = cts[0]->getPower();
    float ct1_power = cts[1]->getPower();
    float imbalance = abs(ct0_power - ct1_power);
    float totalLoad = ct0_power + ct1_power;
    float imbalancePercent = (totalLoad > 0) ? (imbalance / totalLoad * 100.0) : 0;

    Serial.printf("  Kitchen Power:  %.2f W\n", ct0_power);
    Serial.printf("  HVAC Power:     %.2f W\n", ct1_power);
    Serial.printf("  Imbalance:      %.2f W (%.1f%%)\n", imbalance, imbalancePercent);

    if (imbalancePercent < 10) {
        Serial.println("  Status:         Well balanced");
    } else if (imbalancePercent < 20) {
        Serial.println("  Status:         Acceptable balance");
    } else {
        Serial.println("  Status:         Imbalanced - consider redistributing loads");
    }

    Serial.println();

    // Verification check
    Serial.println("Verification:");
    float subCircuitsTotal = ct0_power + ct1_power;
    float mainPanelPower = cts[2]->getPower();
    float difference = abs(mainPanelPower - subCircuitsTotal);
    float differencePercent = (mainPanelPower > 0) ? (difference / mainPanelPower * 100.0) : 0;

    Serial.printf("  Sub-circuits:   %.2f W\n", subCircuitsTotal);
    Serial.printf("  Main Panel:     %.2f W\n", mainPanelPower);
    Serial.printf("  Difference:     %.2f W (%.1f%%)\n", difference, differencePercent);

    if (differencePercent < 5) {
        Serial.println("  Status:         Excellent agreement");
    } else if (differencePercent < 15) {
        Serial.println("  Status:         Good agreement (other loads present)");
    } else {
        Serial.println("  Status:         Check calibration or unmonitored loads");
    }

    Serial.println();
    Serial.println("========================================");
    Serial.println();
}

/**
 * Handle serial commands
 */
void handleSerialCommands() {
    if (Serial.available() > 0) {
        char command = Serial.read();
        int ctNum = -1;

        // Check for CT-specific reset command
        if (command == 'r' && Serial.available() > 0) {
            char numChar = Serial.read();
            if (numChar >= '0' && numChar < '0' + NUM_CTS) {
                ctNum = numChar - '0';
            }
        }

        switch (command) {
            case 's':
            case 'S':
                printSummary();
                break;

            case 'd':
            case 'D':
                printDetailedStats();
                break;

            case 'r':
            case 'R':
                if (ctNum >= 0) {
                    // Reset specific CT
                    Serial.println();
                    Serial.printf("Resetting CT%d (%s)...\n", ctNum, CIRCUIT_NAMES[ctNum]);
                    cts[ctNum]->resetEnergy();
                    cts[ctNum]->saveEnergy();
                    stats[ctNum].peakCurrent = 0;
                    stats[ctNum].peakPower = 0;
                    stats[ctNum].averagePower = 0;
                    stats[ctNum].sampleCount = 0;
                    Serial.println("Reset complete");
                    Serial.println();
                } else {
                    // Reset all CTs
                    Serial.println();
                    Serial.println("Resetting ALL energy counters...");
                    for (int i = 0; i < NUM_CTS; i++) {
                        Serial.printf("  CT%d (%s): %.2f Wh → 0.00 Wh\n",
                            i, CIRCUIT_NAMES[i], cts[i]->getEnergy());
                        cts[i]->resetEnergy();
                        cts[i]->saveEnergy();
                        stats[i].peakCurrent = 0;
                        stats[i].peakPower = 0;
                        stats[i].averagePower = 0;
                        stats[i].sampleCount = 0;
                    }
                    Serial.println("All counters reset");
                    Serial.println();
                }
                break;

            default:
                // Ignore unknown commands
                break;
        }

        // Clear remaining buffer
        while (Serial.available() > 0) {
            Serial.read();
        }
    }
}

/**
 * Periodic FRAM save for all CTs
 */
void periodicSave() {
    static double lastSavedEnergy[NUM_CTS] = {0};
    const double SAVE_THRESHOLD = 10.0;  // Save every 10 Wh

    for (int i = 0; i < NUM_CTS; i++) {
        double currentEnergy = cts[i]->getEnergy();
        if (currentEnergy - lastSavedEnergy[i] >= SAVE_THRESHOLD) {
            cts[i]->saveEnergy();
            lastSavedEnergy[i] = currentEnergy;
            Serial.printf(">>> CT%d (%s) saved: %.2f Wh\n",
                i, CIRCUIT_NAMES[i], currentEnergy);
        }
    }
}

/*
 * ========================================
 * MQTT Integration for Multiple CTs
 * ========================================
 *
 * To integrate with MQTT, add this to your setup():
 *
 * #include <ESPMegaIoT.hpp>
 * #include <CurrentTransformerIoT.hpp>
 *
 * ESPMegaIoT iot;
 * CurrentTransformerIoT ctIot0, ctIot1, ctIot2;
 * ExpansionCard* cards[3];
 *
 * void setup() {
 *     // ... existing setup code ...
 *
 *     // Setup expansion cards array
 *     cards[0] = &ct0;
 *     cards[1] = &ct1;
 *     cards[2] = &ct2;
 *
 *     // Initialize IoT
 *     iot.intr_begin(cards);
 *     // ... configure network and MQTT ...
 *
 *     // Initialize CT IoT components
 *     ctIot0.begin(0, &ct0, mqtt, baseTopic);
 *     ctIot1.begin(1, &ct1, mqtt, baseTopic);
 *     ctIot2.begin(2, &ct2, mqtt, baseTopic);
 *
 *     ctIot0.subscribe();
 *     ctIot1.subscribe();
 *     ctIot2.subscribe();
 *
 *     iot.registerCard(0);
 *     iot.registerCard(1);
 *     iot.registerCard(2);
 * }
 *
 * void loop() {
 *     // ... existing loop code ...
 *     iot.loop();
 * }
 *
 * MQTT Topics:
 *   home/espmega/ct/0/current  - Kitchen current
 *   home/espmega/ct/0/power    - Kitchen power
 *   home/espmega/ct/0/energy   - Kitchen energy
 *   home/espmega/ct/1/current  - HVAC current
 *   home/espmega/ct/1/power    - HVAC power
 *   home/espmega/ct/1/energy   - HVAC energy
 *   home/espmega/ct/2/current  - Main panel current
 *   home/espmega/ct/2/power    - Main panel power
 *   home/espmega/ct/2/energy   - Main panel energy
 *
 * ========================================
 * Application Ideas
 * ========================================
 *
 * 1. Whole-House Monitoring:
 *    - Main panel CT tracks total consumption
 *    - Individual circuit CTs track major loads
 *    - Calculate "Other" loads by subtraction
 *
 * 2. 3-Phase Monitoring:
 *    - One CT per phase (L1, L2, L3)
 *    - Balance verification
 *    - Per-phase energy tracking
 *
 * 3. Solar + Grid Monitoring:
 *    - CT on grid connection
 *    - CT on solar inverter output
 *    - Calculate net consumption/production
 *
 * 4. Multi-Tenant Metering:
 *    - Separate CT per tenant/apartment
 *    - Individual energy billing
 *    - Central monitoring dashboard
 *
 * 5. Equipment Monitoring:
 *    - Dedicated CT per major equipment
 *    - Runtime tracking
 *    - Maintenance scheduling based on usage
 */
