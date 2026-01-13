/**
 * Basic Current Monitoring Example
 *
 * This example demonstrates basic current monitoring using a current transformer
 * connected to the ESPMegaPRO's Analog Card. It reads current and power continuously
 * and displays the values on the serial monitor.
 *
 * Hardware Required:
 * - ESPMegaPRO R3 board
 * - Analog Card
 * - Current Transformer (e.g., SCT-013-000)
 * - Burden resistor circuit (33Ω for SCT-013-000)
 *
 * Connections:
 * - CT connected to Analog Input A0 through burden resistor circuit
 * - See documentation for complete wiring diagram
 *
 * Author: SIWAT INC
 * Date: 2024
 */

#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>

// Create analog card instance
AnalogCard analogCard;

// Set your AC line voltage (230V for EU/Asia, 120V for US)
float lineVoltage = 230.0;

/**
 * ADC to Current Conversion Function
 *
 * This function converts the raw ADC value to current in Amperes.
 * The conversion depends on your CT model and burden resistor.
 *
 * For SCT-013-000 (100A:50mA) with 33Ω burden resistor:
 * - Max current: 100A
 * - Max secondary current: 50mA
 * - Max burden voltage: 50mA × 33Ω = 1.65V
 * - ADC biased at 1.65V (VCC/2 = 3.3V/2)
 * - Zero current: ADC = 2048 (middle of 12-bit range)
 * - Scale factor: 100A / 2048 steps = 0.0488 A/step
 */
auto adcToCurrent = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;      // Zero current ADC value (VCC/2)
    const float SCALE_FACTOR = 0.0488;  // 100A / 2048 steps
    return (adc - ADC_ZERO) * SCALE_FACTOR;
};

// Create Current Transformer Card instance
CurrentTransformerCard ct(
    &analogCard,        // Pointer to analog card
    0,                  // Use analog input A0
    &lineVoltage,       // Pointer to voltage reference
    adcToCurrent,       // Conversion function
    1000                // Sample every 1000ms (1 second)
);

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) {
        delay(10);  // Wait for serial port to connect
    }

    Serial.println("=======================================");
    Serial.println("  Current Transformer Monitor");
    Serial.println("  Basic Current Monitoring Example");
    Serial.println("=======================================");
    Serial.println();

    // Initialize the Analog Card
    Serial.print("Initializing Analog Card...");
    if (!analogCard.begin()) {
        Serial.println(" FAILED!");
        Serial.println("ERROR: Analog Card initialization failed!");
        Serial.println("Check connections and restart.");
        while (1) {
            delay(1000);  // Halt
        }
    }
    Serial.println(" OK");

    // Initialize the Current Transformer Card
    Serial.print("Initializing Current Transformer Card...");
    if (!ct.begin()) {
        Serial.println(" FAILED!");
        Serial.println("ERROR: CT initialization failed!");
        Serial.println("Check connections and restart.");
        while (1) {
            delay(1000);  // Halt
        }
    }
    Serial.println(" OK");

    // Display configuration
    Serial.println();
    Serial.println("Configuration:");
    Serial.printf("  - Line Voltage: %.1f V\n", lineVoltage);
    Serial.printf("  - Analog Input: A0\n");
    Serial.printf("  - Sample Rate: 1000 ms\n");
    Serial.printf("  - CT Model: SCT-013-000 (100A:50mA)\n");
    Serial.printf("  - Burden Resistor: 33Ω\n");
    Serial.println();

    // Wait a moment before starting measurements
    delay(2000);

    // Print table header
    Serial.println("Starting measurements...");
    Serial.println();
    Serial.println("Time (s) | Current (A) | Power (W) | Energy (Wh)");
    Serial.println("---------|-------------|-----------|-------------");
}

void loop() {
    // Update the CT card (performs measurement and energy accumulation)
    ct.loop();

    // Read current values
    float current = ct.getCurrent();
    float power = ct.getPower();
    double energy = ct.getEnergy();

    // Calculate time in seconds since startup
    unsigned long timeSeconds = millis() / 1000;

    // Print measurements in a formatted table
    Serial.printf("%8lu | %11.2f | %9.2f | %11.3f\n",
        timeSeconds,
        current,
        power,
        energy
    );

    // Optional: Additional information every 10 seconds
    static unsigned long lastDetailedPrint = 0;
    if (millis() - lastDetailedPrint >= 10000) {
        Serial.println();
        Serial.println("--- Detailed Status ---");
        Serial.printf("Current:  %.2f A\n", current);
        Serial.printf("Voltage:  %.1f V\n", ct.getVoltage());
        Serial.printf("Power:    %.2f W (%.2f kW)\n", power, power / 1000.0);
        Serial.printf("Energy:   %.3f Wh (%.3f kWh)\n", energy, energy / 1000.0);

        // Calculate estimated cost (example: $0.12 per kWh)
        float costPerKWh = 0.12;
        float totalCost = (energy / 1000.0) * costPerKWh;
        Serial.printf("Est Cost: $%.4f (@ $%.2f/kWh)\n", totalCost, costPerKWh);
        Serial.println("-----------------------");
        Serial.println();

        lastDetailedPrint = millis();
    }

    // Small delay before next iteration
    delay(100);
}

/*
 * Expected Output:
 *
 * =======================================
 *   Current Transformer Monitor
 *   Basic Current Monitoring Example
 * =======================================
 *
 * Initializing Analog Card... OK
 * Initializing Current Transformer Card... OK
 *
 * Configuration:
 *   - Line Voltage: 230.0 V
 *   - Analog Input: A0
 *   - Sample Rate: 1000 ms
 *   - CT Model: SCT-013-000 (100A:50mA)
 *   - Burden Resistor: 33Ω
 *
 * Starting measurements...
 *
 * Time (s) | Current (A) | Power (W) | Energy (Wh)
 * ---------|-------------|-----------|-------------
 *        2 |        5.43 |   1248.90 |       0.000
 *        3 |        5.45 |   1253.50 |       0.348
 *        4 |        5.42 |   1246.60 |       0.695
 *        5 |        5.44 |   1251.20 |       1.043
 *        ...
 */

/*
 * Troubleshooting:
 *
 * 1. Always reads 0A:
 *    - Check CT is installed on a conductor with current flowing
 *    - Verify CT is clipped around only ONE wire (not both hot and neutral)
 *    - Check burden resistor connections
 *    - Test with a known load (e.g., lamp, heater)
 *
 * 2. Negative readings:
 *    - CT may be installed backwards (reverse it or add abs() to conversion)
 *
 * 3. Inaccurate readings:
 *    - Verify your line voltage is correct
 *    - Calibrate using a known load
 *    - Check burden resistor value (should be 33Ω for SCT-013-000)
 *
 * 4. Noisy/erratic readings:
 *    - Add a larger filter capacitor (try 100µF instead of 10µF)
 *    - Improve grounding
 *    - Move CT away from noise sources
 *
 * Calibration Procedure:
 *
 * 1. Connect a known load (e.g., 1000W heater)
 * 2. Calculate expected current: I = P/V = 1000W/230V = 4.35A
 * 3. Read the displayed current
 * 4. Adjust SCALE_FACTOR: new_scale = SCALE_FACTOR × (expected / measured)
 * 5. Update the adcToCurrent function with new scale factor
 * 6. Test again and repeat if needed
 */
