/**
 * @file dac_output.ino
 * @brief DAC output control example for ESPMegaPRO Analog Card
 *
 * This example demonstrates various DAC output patterns including:
 * - Sawtooth wave
 * - Triangle wave
 * - Square wave
 * - Sine wave
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Analog Expansion Card
 * - Optional: Oscilloscope or multimeter to measure outputs
 *
 * Features Demonstrated:
 * - DAC initialization and control
 * - Generating waveforms
 * - State and value control
 * - DAC change callbacks
 *
 * DAC Connections:
 * - DAC 0: Sawtooth wave
 * - DAC 1: Triangle wave
 * - DAC 2: Square wave
 * - DAC 3: Sine wave
 *
 * @author Siwat INC
 * @version 1.0
 * @date 2025-11-23
 */

#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

// Create AnalogCard instance
AnalogCard analogCard;

// DAC Configuration
const float DAC_VDD = 3.3;           // DAC supply voltage (3.3V or 5.0V)
const uint16_t DAC_MAX_VALUE = 4095; // 12-bit DAC (0-4095)
const uint16_t DAC_MIN_VALUE = 0;

// Waveform parameters
const unsigned long WAVE_PERIOD = 5000; // Period in milliseconds (5 seconds)
const uint16_t WAVE_UPDATE_RATE = 10;   // Update every 10ms

// Timing variables
unsigned long lastUpdateTime = 0;

// Wave phase (0.0 to 1.0)
float wavePhase = 0.0;

/**
 * @brief Converts voltage to DAC value
 * @param voltage Desired output voltage (0 to VDD)
 * @return DAC value (0-4095)
 */
uint16_t voltageToDac(float voltage) {
    if (voltage < 0.0) voltage = 0.0;
    if (voltage > DAC_VDD) voltage = DAC_VDD;
    return (uint16_t)((voltage / DAC_VDD) * DAC_MAX_VALUE);
}

/**
 * @brief Converts DAC value to voltage
 * @param dacValue DAC value (0-4095)
 * @return Voltage in volts
 */
float dacToVoltage(uint16_t dacValue) {
    return (dacValue / (float)DAC_MAX_VALUE) * DAC_VDD;
}

/**
 * @brief Generates sawtooth wave value
 * @param phase Wave phase (0.0 to 1.0)
 * @return DAC value (0-4095)
 */
uint16_t sawtoothWave(float phase) {
    return (uint16_t)(phase * DAC_MAX_VALUE);
}

/**
 * @brief Generates triangle wave value
 * @param phase Wave phase (0.0 to 1.0)
 * @return DAC value (0-4095)
 */
uint16_t triangleWave(float phase) {
    if (phase < 0.5) {
        // Rising edge
        return (uint16_t)(phase * 2.0 * DAC_MAX_VALUE);
    } else {
        // Falling edge
        return (uint16_t)((2.0 - phase * 2.0) * DAC_MAX_VALUE);
    }
}

/**
 * @brief Generates square wave value
 * @param phase Wave phase (0.0 to 1.0)
 * @return DAC value (0 or 4095)
 */
uint16_t squareWave(float phase) {
    return (phase < 0.5) ? DAC_MAX_VALUE : DAC_MIN_VALUE;
}

/**
 * @brief Generates sine wave value
 * @param phase Wave phase (0.0 to 1.0)
 * @return DAC value (0-4095)
 */
uint16_t sineWave(float phase) {
    float angle = phase * 2.0 * PI;
    float sineValue = sin(angle);
    // Convert from -1..1 to 0..1
    float normalized = (sineValue + 1.0) / 2.0;
    return (uint16_t)(normalized * DAC_MAX_VALUE);
}

/**
 * @brief Callback function called when DAC value changes
 */
void onDACChange(uint8_t pin, bool state, uint16_t value) {
    float voltage = dacToVoltage(value);
    const char* waveType[] = {"Sawtooth", "Triangle", "Square", "Sine"};

    Serial.printf("DAC %d (%s): State=%s, Value=%4d, Voltage=%.3f V\n",
                  pin,
                  waveType[pin],
                  state ? "ON " : "OFF",
                  value,
                  voltage);
}

/**
 * @brief Setup function - runs once at startup
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.println("  ESPMegaPRO Analog Card - DAC Output");
    Serial.println("========================================");
    Serial.println();

    // Initialize the Analog Card
    Serial.print("Initializing Analog Card... ");
    if (!analogCard.begin()) {
        Serial.println("FAILED!");
        Serial.println("ERROR: Could not initialize Analog Card");
        while (1) {
            delay(1000);
        }
    }
    Serial.println("SUCCESS!");
    Serial.println();

    // Register DAC change callback
    analogCard.registerDACChangeCallback(onDACChange);

    // Print DAC configuration
    Serial.println("DAC Configuration:");
    Serial.printf("  - Channels: 4 (0-3)\n");
    Serial.printf("  - Resolution: 12-bit (0-4095)\n");
    Serial.printf("  - VDD: %.1f V\n", DAC_VDD);
    Serial.printf("  - Output Range: 0.0 to %.1f V\n", DAC_VDD);
    Serial.println();

    Serial.println("Waveform Assignment:");
    Serial.println("  - DAC 0: Sawtooth wave");
    Serial.println("  - DAC 1: Triangle wave");
    Serial.println("  - DAC 2: Square wave");
    Serial.println("  - DAC 3: Sine wave");
    Serial.println();

    Serial.printf("Wave Period: %lu ms\n", WAVE_PERIOD);
    Serial.printf("Update Rate: %d ms\n", WAVE_UPDATE_RATE);
    Serial.println();

    // Initialize all DACs to mid-point and enable them
    Serial.println("Initializing DAC outputs...");
    for (uint8_t i = 0; i < 4; i++) {
        analogCard.setDACValue(i, DAC_MAX_VALUE / 2);
        analogCard.setDACState(i, true);
    }

    Serial.println("\nStarting waveform generation...");
    Serial.println("========================================\n");

    lastUpdateTime = millis();
}

/**
 * @brief Main loop function - runs repeatedly
 */
void loop() {
    unsigned long currentTime = millis();

    // Update waveforms at specified rate
    if (currentTime - lastUpdateTime >= WAVE_UPDATE_RATE) {
        lastUpdateTime = currentTime;

        // Calculate current phase (0.0 to 1.0)
        wavePhase = (float)((currentTime % WAVE_PERIOD)) / (float)WAVE_PERIOD;

        // Generate waveforms for each DAC
        uint16_t dac0Value = sawtoothWave(wavePhase);
        uint16_t dac1Value = triangleWave(wavePhase);
        uint16_t dac2Value = squareWave(wavePhase);
        uint16_t dac3Value = sineWave(wavePhase);

        // Update DAC outputs (using setDACValue to avoid triggering state changes)
        // Note: We're not calling the callback for every update to reduce serial spam
        // If you want to see every change, use dacWrite() instead
        analogCard.sendDataToDAC(0, dac0Value);
        analogCard.sendDataToDAC(1, dac1Value);
        analogCard.sendDataToDAC(2, dac2Value);
        analogCard.sendDataToDAC(3, dac3Value);

        // Print status every second
        static unsigned long lastPrintTime = 0;
        if (currentTime - lastPrintTime >= 1000) {
            lastPrintTime = currentTime;

            Serial.printf("Phase: %.2f  ", wavePhase);
            Serial.printf("DAC0=%4d (%.2fV)  ", dac0Value, dacToVoltage(dac0Value));
            Serial.printf("DAC1=%4d (%.2fV)  ", dac1Value, dacToVoltage(dac1Value));
            Serial.printf("DAC2=%4d (%.2fV)  ", dac2Value, dacToVoltage(dac2Value));
            Serial.printf("DAC3=%4d (%.2fV)\n", dac3Value, dacToVoltage(dac3Value));
        }
    }

    // Demonstrate state control - toggle DAC 2 every 10 seconds
    static unsigned long lastToggleTime = 0;
    if (currentTime - lastToggleTime >= 10000) {
        lastToggleTime = currentTime;

        bool currentState = analogCard.getDACState(2);
        analogCard.setDACState(2, !currentState);

        Serial.println("\n--- Toggled DAC 2 State ---");
        Serial.printf("New State: %s\n", !currentState ? "ON" : "OFF");
        Serial.println();
    }
}

/**
 * DAC Output Reference:
 *
 * Channel | IC        | I2C Address | Output Range
 * --------|-----------|-------------|---------------
 *    0    | MCP4725 0 |    0x60     | 0 to VDD
 *    1    | MCP4725 1 |    0x61     | 0 to VDD
 *    2    | MCP4725 2 |    0x62     | 0 to VDD
 *    3    | MCP4725 3 |    0x63     | 0 to VDD
 *
 * Voltage Conversion Formulas:
 *   DAC Value to Voltage: voltage = (value / 4095.0) * VDD
 *   Voltage to DAC Value: value = (voltage / VDD) * 4095
 *
 * Notes:
 * - MCP4725 is a 12-bit DAC (0-4095)
 * - VDD is typically 3.3V or 5.0V (check your hardware)
 * - Each DAC has independent state (ON/OFF) and value
 * - When state is OFF, output is 0V regardless of value
 * - DAC has EEPROM for storing values across power cycles
 * - Output impedance: typically 1Ω
 * - Maximum output current: ±25mA
 *
 * Waveform Characteristics:
 * - Sawtooth: Linear rise from 0 to max
 * - Triangle: Linear rise then linear fall
 * - Square: Alternates between 0 and max
 * - Sine: Smooth sinusoidal wave
 */
