/**
 * @file adc_reading.ino
 * @brief Basic ADC reading example for ESPMegaPRO Analog Card
 *
 * This example demonstrates how to read analog values from all 8 ADC channels
 * and convert them to voltages. It also shows basic statistics and formatting.
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Analog Expansion Card
 * - Optional: Voltage sources connected to ADC inputs (0-6V range)
 *
 * Features Demonstrated:
 * - Initializing the Analog Card
 * - Reading from all ADC channels
 * - Converting raw values to voltages
 * - Formatted serial output
 *
 * @author Siwat INC
 * @version 1.0
 * @date 2025-11-23
 */

#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

// Create AnalogCard instance
AnalogCard analogCard;

// Timing variables
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 1000; // Read every 1000ms (1 second)

// ADC Configuration
const float ADC_REFERENCE_VOLTAGE = 6.144; // ADS1115 default reference voltage
const float ADC_MAX_VALUE = 32768.0;       // 16-bit ADC max value (signed)

/**
 * @brief Converts raw ADC value to voltage
 * @param rawValue Raw ADC reading (0-65535)
 * @return Voltage in volts
 */
float adcToVoltage(uint16_t rawValue) {
    return (rawValue * ADC_REFERENCE_VOLTAGE) / ADC_MAX_VALUE;
}

/**
 * @brief Setup function - runs once at startup
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000); // Wait for serial to stabilize

    Serial.println("========================================");
    Serial.println("  ESPMegaPRO Analog Card - ADC Reading");
    Serial.println("========================================");
    Serial.println();

    // Initialize the Analog Card
    Serial.print("Initializing Analog Card... ");
    if (!analogCard.begin()) {
        Serial.println("FAILED!");
        Serial.println("ERROR: Could not initialize Analog Card");
        Serial.println("Please check:");
        Serial.println("  - Card is properly seated");
        Serial.println("  - I2C connections are good");
        Serial.println("  - No I2C address conflicts");
        while (1) {
            delay(1000); // Halt execution
        }
    }
    Serial.println("SUCCESS!");
    Serial.println();

    // Print ADC information
    Serial.println("ADC Configuration:");
    Serial.printf("  - Channels: 8 (0-7)\n");
    Serial.printf("  - Resolution: 16-bit (0-65535)\n");
    Serial.printf("  - Reference Voltage: %.3f V\n", ADC_REFERENCE_VOLTAGE);
    Serial.printf("  - Bank A (Channels 0-3): I2C Address 0x48\n");
    Serial.printf("  - Bank B (Channels 4-7): I2C Address 0x49\n");
    Serial.println();

    Serial.println("Starting ADC readings...");
    Serial.println();
}

/**
 * @brief Main loop function - runs repeatedly
 */
void loop() {
    // Check if it's time to read the ADCs
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime >= READ_INTERVAL) {
        lastReadTime = currentTime;

        // Print timestamp
        Serial.println("========================================");
        Serial.printf("Time: %lu ms\n", currentTime);
        Serial.println("========================================");

        // Variables for statistics
        uint16_t minRaw = 65535;
        uint16_t maxRaw = 0;
        uint32_t sumRaw = 0;
        float minVoltage = 999.9;
        float maxVoltage = 0.0;
        float sumVoltage = 0.0;

        // Read all 8 ADC channels
        for (uint8_t channel = 0; channel < 8; channel++) {
            // Read raw ADC value
            uint16_t rawValue = analogCard.analogRead(channel);

            // Convert to voltage
            float voltage = adcToVoltage(rawValue);

            // Update statistics
            if (rawValue < minRaw) minRaw = rawValue;
            if (rawValue > maxRaw) maxRaw = rawValue;
            sumRaw += rawValue;

            if (voltage < minVoltage) minVoltage = voltage;
            if (voltage > maxVoltage) maxVoltage = voltage;
            sumVoltage += voltage;

            // Determine which bank this channel belongs to
            char bank = (channel < 4) ? 'A' : 'B';
            uint8_t bankChannel = (channel < 4) ? channel : (channel - 4);

            // Print channel reading
            Serial.printf("ADC %d (Bank %c, Ch %d): ", channel, bank, bankChannel);
            Serial.printf("Raw=%-5d  ", rawValue);
            Serial.printf("Voltage=%6.3f V  ", voltage);

            // Add visual bar graph (0-6V scale)
            int barLength = (int)((voltage / ADC_REFERENCE_VOLTAGE) * 40);
            Serial.print("[");
            for (int i = 0; i < 40; i++) {
                if (i < barLength) {
                    Serial.print("=");
                } else {
                    Serial.print(" ");
                }
            }
            Serial.println("]");
        }

        // Calculate and print statistics
        float avgRaw = sumRaw / 8.0;
        float avgVoltage = sumVoltage / 8.0;

        Serial.println("----------------------------------------");
        Serial.println("Statistics:");
        Serial.printf("  Raw Values    - Min: %-5d  Max: %-5d  Avg: %.1f\n",
                      minRaw, maxRaw, avgRaw);
        Serial.printf("  Voltages      - Min: %6.3f V  Max: %6.3f V  Avg: %6.3f V\n",
                      minVoltage, maxVoltage, avgVoltage);
        Serial.println();
    }
}

/**
 * ADC Channel Mapping Reference:
 *
 * Channel | Bank | Physical IC | IC Channel | I2C Address
 * --------|------|-------------|------------|-------------
 *    0    |  A   |  ADS1115 A  |     A0     |    0x48
 *    1    |  A   |  ADS1115 A  |     A1     |    0x48
 *    2    |  A   |  ADS1115 A  |     A2     |    0x48
 *    3    |  A   |  ADS1115 A  |     A3     |    0x48
 *    4    |  B   |  ADS1115 B  |     A0     |    0x49
 *    5    |  B   |  ADS1115 B  |     A1     |    0x49
 *    6    |  B   |  ADS1115 B  |     A2     |    0x49
 *    7    |  B   |  ADS1115 B  |     A3     |    0x49
 *
 * Voltage Conversion Formula:
 *   voltage = (rawValue * 6.144) / 32768.0
 *
 * Notes:
 * - The ADS1115 is a 16-bit ADC with programmable gain
 * - Default gain setting provides ±6.144V full-scale range
 * - In single-ended mode, measure 0 to +6.144V
 * - LSB size: 187.5 µV (6.144V / 32768)
 * - Maximum safe input: 6V (do not exceed VDD + 0.3V)
 */
