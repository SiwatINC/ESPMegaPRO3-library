/**
 * @file sensor_interfacing.ino
 * @brief Sensor interfacing example for ESPMegaPRO Analog Card
 *
 * This example demonstrates how to interface various analog sensors:
 * - Temperature sensor (LM35 - 10mV/°C)
 * - Light sensor (LDR with voltage divider)
 * - Potentiometer (0-3.3V)
 * - Current sensor (ACS712 - Hall effect)
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Analog Expansion Card
 * - LM35 temperature sensor on ADC 0
 * - LDR (light dependent resistor) circuit on ADC 1
 * - Potentiometer on ADC 2
 * - ACS712 current sensor on ADC 3
 *
 * Features Demonstrated:
 * - Sensor calibration
 * - Signal conditioning
 * - Multi-sample averaging
 * - Unit conversion
 * - DAC output for control/feedback
 *
 * @author Siwat INC
 * @version 1.0
 * @date 2025-11-23
 */

#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

// Create AnalogCard instance
AnalogCard analogCard;

// ADC Configuration
const float ADC_REFERENCE = 6.144;
const float ADC_MAX = 32768.0;

// DAC Configuration
const float DAC_VDD = 3.3;
const uint16_t DAC_MAX_VALUE = 4095;

// Sensor Configuration
const uint8_t TEMP_SENSOR_PIN = 0;     // LM35 on ADC 0
const uint8_t LIGHT_SENSOR_PIN = 1;    // LDR on ADC 1
const uint8_t POT_SENSOR_PIN = 2;      // Potentiometer on ADC 2
const uint8_t CURRENT_SENSOR_PIN = 3;  // ACS712 on ADC 3

// Calibration values
struct SensorCalibration {
    float offset;
    float scale;
};

// LM35: 10mV/°C, outputs 0V at 0°C
SensorCalibration tempCal = {0.0, 1.0};

// LDR: Voltage divider with 10kΩ resistor
// Adjust based on your specific LDR characteristics
SensorCalibration lightCal = {0.0, 1.0};

// Potentiometer: Direct voltage reading
SensorCalibration potCal = {0.0, 1.0};

// ACS712-05B: 185mV/A, 2.5V at 0A
// For 20A version use 100mV/A, for 30A use 66mV/A
SensorCalibration currentCal = {2.5, 0.185}; // offset = zero point, scale = sensitivity

// Averaging
const uint8_t NUM_SAMPLES = 10;

// Update intervals
const unsigned long SENSOR_UPDATE_INTERVAL = 1000; // 1 second
unsigned long lastSensorUpdate = 0;

/**
 * @brief Convert raw ADC to voltage
 */
float adcToVoltage(uint16_t rawValue) {
    return (rawValue * ADC_REFERENCE) / ADC_MAX;
}

/**
 * @brief Convert voltage to DAC value
 */
uint16_t voltageToDac(float voltage) {
    if (voltage < 0.0) voltage = 0.0;
    if (voltage > DAC_VDD) voltage = DAC_VDD;
    return (uint16_t)((voltage / DAC_VDD) * DAC_MAX_VALUE);
}

/**
 * @brief Read ADC with averaging
 */
float readADCAveraged(uint8_t pin, uint8_t samples) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += analogCard.analogRead(pin);
        delay(2); // Small delay between samples
    }
    float avgRaw = sum / (float)samples;
    return adcToVoltage((uint16_t)avgRaw);
}

/**
 * @brief Read temperature from LM35 sensor
 * LM35 outputs 10mV per degree Celsius
 * @return Temperature in Celsius
 */
float readTemperature() {
    float voltage = readADCAveraged(TEMP_SENSOR_PIN, NUM_SAMPLES);

    // Apply calibration
    voltage = (voltage * tempCal.scale) + tempCal.offset;

    // LM35: 10mV/°C
    float tempC = voltage * 100.0; // Convert V to mV, then to °C

    return tempC;
}

/**
 * @brief Read light level from LDR
 * Returns percentage (0-100%)
 */
float readLightLevel() {
    float voltage = readADCAveraged(LIGHT_SENSOR_PIN, NUM_SAMPLES);

    // Apply calibration
    voltage = (voltage * lightCal.scale) + lightCal.offset;

    // Convert to percentage (assuming 0-3.3V range)
    float percentage = (voltage / 3.3) * 100.0;

    return percentage;
}

/**
 * @brief Read potentiometer position
 * Returns percentage (0-100%)
 */
float readPotentiometer() {
    float voltage = readADCAveraged(POT_SENSOR_PIN, NUM_SAMPLES);

    // Apply calibration
    voltage = (voltage * potCal.scale) + potCal.offset;

    // Convert to percentage
    float percentage = (voltage / 3.3) * 100.0;

    return percentage;
}

/**
 * @brief Read current from ACS712 sensor
 * ACS712-05B: 185mV/A, centered at 2.5V (0A)
 * @return Current in Amperes
 */
float readCurrent() {
    float voltage = readADCAveraged(CURRENT_SENSOR_PIN, NUM_SAMPLES);

    // Calculate current: I = (V - V_zero) / Sensitivity
    float current = (voltage - currentCal.offset) / currentCal.scale;

    return current;
}

/**
 * @brief Control function based on sensor readings
 * Example: Control heater based on temperature
 */
void controlOutputs(float temperature, float lightLevel, float potValue, float current) {
    // DAC 0: Temperature control output
    // Example: Simple proportional control for heater
    // Target: 25°C, output 0-3.3V proportional to error
    float tempError = 25.0 - temperature;
    float tempControl = (tempError / 10.0) + 0.5; // Normalize around 0.5
    if (tempControl < 0.0) tempControl = 0.0;
    if (tempControl > 1.0) tempControl = 1.0;
    analogCard.dacWrite(0, voltageToDac(tempControl * 3.3));

    // DAC 1: Light-controlled output
    // Example: Inverse control (brighter = lower output)
    float lightControl = (100.0 - lightLevel) / 100.0;
    analogCard.dacWrite(1, voltageToDac(lightControl * 3.3));

    // DAC 2: Direct potentiometer mapping
    float potControl = potValue / 100.0;
    analogCard.dacWrite(2, voltageToDac(potControl * 3.3));

    // DAC 3: Current indicator
    // Map current (e.g., 0-5A) to voltage output
    float currentControl = abs(current) / 5.0;
    if (currentControl > 1.0) currentControl = 1.0;
    analogCard.dacWrite(3, voltageToDac(currentControl * 3.3));
}

/**
 * @brief Setup function
 */
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.println("  Sensor Interfacing - ESPMegaPRO");
    Serial.println("========================================");
    Serial.println();

    // Initialize Analog Card
    Serial.print("Initializing Analog Card... ");
    if (!analogCard.begin()) {
        Serial.println("FAILED!");
        while (1) delay(1000);
    }
    Serial.println("SUCCESS!");
    Serial.println();

    // Print sensor configuration
    Serial.println("Sensor Configuration:");
    Serial.println("----------------------------------------");
    Serial.printf("ADC %d: LM35 Temperature Sensor\n", TEMP_SENSOR_PIN);
    Serial.printf("  - Sensitivity: 10mV/°C\n");
    Serial.printf("  - Range: -55°C to +150°C\n");
    Serial.println();

    Serial.printf("ADC %d: LDR Light Sensor\n", LIGHT_SENSOR_PIN);
    Serial.printf("  - Type: Voltage divider\n");
    Serial.printf("  - Output: 0-100%%\n");
    Serial.println();

    Serial.printf("ADC %d: Potentiometer\n", POT_SENSOR_PIN);
    Serial.printf("  - Range: 0-3.3V\n");
    Serial.printf("  - Output: 0-100%%\n");
    Serial.println();

    Serial.printf("ADC %d: ACS712 Current Sensor\n", CURRENT_SENSOR_PIN);
    Serial.printf("  - Type: Hall effect\n");
    Serial.printf("  - Sensitivity: %.0f mV/A\n", currentCal.scale * 1000);
    Serial.printf("  - Zero point: %.2f V\n", currentCal.offset);
    Serial.printf("  - Range: ±5A (for -05B model)\n");
    Serial.println();

    Serial.println("DAC Outputs:");
    Serial.println("----------------------------------------");
    Serial.println("DAC 0: Temperature control (heater)");
    Serial.println("DAC 1: Light-controlled output");
    Serial.println("DAC 2: Potentiometer-controlled output");
    Serial.println("DAC 3: Current indicator");
    Serial.println();

    Serial.println("Starting sensor readings...");
    Serial.println("========================================\n");

    lastSensorUpdate = millis();
}

/**
 * @brief Main loop
 */
void loop() {
    unsigned long currentTime = millis();

    if (currentTime - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = currentTime;

        // Read all sensors
        float temperature = readTemperature();
        float lightLevel = readLightLevel();
        float potValue = readPotentiometer();
        float current = readCurrent();

        // Calculate derived values
        float tempF = (temperature * 9.0 / 5.0) + 32.0; // Convert to Fahrenheit
        float power = current * 5.0; // Assuming 5V supply (adjust as needed)

        // Print readings
        Serial.println("========================================");
        Serial.printf("Time: %lu ms\n", currentTime);
        Serial.println("========================================");

        // Temperature
        Serial.printf("Temperature:  %6.2f °C  (%6.2f °F)\n", temperature, tempF);

        // Light level with bar graph
        Serial.printf("Light Level:  %6.2f %%  ", lightLevel);
        printBarGraph(lightLevel, 100.0);

        // Potentiometer
        Serial.printf("Potentiometer: %6.2f %%  ", potValue);
        printBarGraph(potValue, 100.0);

        // Current and power
        Serial.printf("Current:      %6.3f A\n", current);
        Serial.printf("Power:        %6.3f W\n", power);

        Serial.println();

        // Update control outputs
        controlOutputs(temperature, lightLevel, potValue, current);

        // Print DAC outputs
        Serial.println("DAC Outputs:");
        for (uint8_t i = 0; i < 4; i++) {
            if (analogCard.getDACState(i)) {
                uint16_t value = analogCard.getDACValue(i);
                float voltage = (value / (float)DAC_MAX_VALUE) * DAC_VDD;
                Serial.printf("  DAC %d: %4d (%.3f V)\n", i, value, voltage);
            } else {
                Serial.printf("  DAC %d: OFF\n", i);
            }
        }
        Serial.println();
    }

    analogCard.loop();
}

/**
 * @brief Print bar graph for visual representation
 */
void printBarGraph(float value, float maxValue) {
    int barLength = (int)((value / maxValue) * 30);
    Serial.print("[");
    for (int i = 0; i < 30; i++) {
        if (i < barLength) {
            Serial.print("=");
        } else {
            Serial.print(" ");
        }
    }
    Serial.println("]");
}

/**
 * Sensor Wiring Guide:
 *
 * LM35 Temperature Sensor:
 * ┌─────────┐
 * │  LM35   │
 * │ (TO-92) │
 * └─┬───┬───┘
 *   │   │   │
 *   │   │   └─ GND
 *   │   └───── Vout → ADC 0
 *   └───────── VCC (+5V)
 *
 * LDR Circuit (Voltage Divider):
 *  VCC (+3.3V)
 *      │
 *     LDR
 *      │
 *      ├──→ ADC 1
 *      │
 *    10kΩ
 *      │
 *     GND
 *
 * Potentiometer:
 *  VCC (+3.3V) ─── 1 (Top)
 *                  2 (Wiper) ──→ ADC 2
 *  GND ─────────── 3 (Bottom)
 *
 * ACS712 Current Sensor:
 * ┌──────────┐
 * │ ACS712   │
 * │  Module  │
 * └──────────┘
 *  VCC ────── +5V
 *  GND ────── GND
 *  OUT ────── ADC 3
 *  IP+ ────── Load +
 *  IP- ────── Load -
 *
 * Notes:
 * - Ensure proper grounding between ESPMegaPRO and sensors
 * - Use shielded cables for long sensor runs
 * - Add 0.1µF capacitors near sensor power pins
 * - Calibrate sensors for your specific environment
 * - Check voltage levels match ADC input range (0-6V max)
 */
