/**
 * @file voltage_monitor.ino
 * @brief Voltage monitoring with threshold alerts for ESPMegaPRO Analog Card
 *
 * This example demonstrates a voltage monitoring system with:
 * - Continuous monitoring of multiple ADC channels
 * - Configurable voltage thresholds (min/max)
 * - Alert system for out-of-range conditions
 * - Data logging with timestamps
 * - Visual indicators using DAC outputs
 *
 * Hardware Required:
 * - ESPMegaPRO board
 * - Analog Expansion Card
 * - Voltage sources to monitor (connected to ADC 0-3)
 * - Optional: LEDs on DAC outputs for visual alerts
 *
 * Features Demonstrated:
 * - Multi-channel ADC monitoring
 * - Threshold detection
 * - State management
 * - DAC output for alerts
 * - Statistical analysis
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

// Monitoring Configuration
const uint8_t NUM_MONITORED_CHANNELS = 4; // Monitor ADC 0-3
const unsigned long MONITOR_INTERVAL = 500; // Check every 500ms
const unsigned long STATS_INTERVAL = 5000;  // Print stats every 5 seconds

// Voltage thresholds for each channel
struct ChannelConfig {
    uint8_t adcChannel;
    const char* name;
    float minVoltage;
    float maxVoltage;
    float warningMargin; // Voltage margin before critical threshold
};

ChannelConfig channels[NUM_MONITORED_CHANNELS] = {
    {0, "Power Supply 5V",  4.5,  5.5,  0.2},
    {1, "Battery Voltage", 11.0, 13.0,  0.5},
    {2, "Solar Panel",      0.0, 20.0,  1.0},
    {3, "Load Voltage",     3.0,  3.6,  0.1}
};

// Alert states
enum AlertLevel {
    NORMAL,
    WARNING,
    CRITICAL
};

struct ChannelStatus {
    float currentVoltage;
    float minRecorded;
    float maxRecorded;
    uint32_t readings;
    AlertLevel alertLevel;
    unsigned long lastAlertTime;
};

ChannelStatus channelStatus[NUM_MONITORED_CHANNELS];

// Timing variables
unsigned long lastMonitorTime = 0;
unsigned long lastStatsTime = 0;

/**
 * @brief Convert raw ADC value to voltage
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
 * @brief Check voltage against thresholds and update alert level
 */
AlertLevel checkThresholds(uint8_t channelIndex, float voltage) {
    ChannelConfig& config = channels[channelIndex];

    // Check critical thresholds
    if (voltage < config.minVoltage || voltage > config.maxVoltage) {
        return CRITICAL;
    }

    // Check warning thresholds
    if (voltage < (config.minVoltage + config.warningMargin) ||
        voltage > (config.maxVoltage - config.warningMargin)) {
        return WARNING;
    }

    return NORMAL;
}

/**
 * @brief Get alert level name
 */
const char* getAlertLevelName(AlertLevel level) {
    switch (level) {
        case NORMAL: return "NORMAL";
        case WARNING: return "WARNING";
        case CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Update DAC outputs based on alert status
 */
void updateAlertOutputs() {
    // DAC 0-3 correspond to monitored channels 0-3
    for (uint8_t i = 0; i < NUM_MONITORED_CHANNELS; i++) {
        switch (channelStatus[i].alertLevel) {
            case NORMAL:
                // Green indication - low voltage
                analogCard.dacWrite(i, voltageToDac(0.5));
                break;

            case WARNING:
                // Yellow indication - medium voltage (flashing)
                {
                    uint16_t value = ((millis() / 500) % 2) ? voltageToDac(2.0) : voltageToDac(0.2);
                    analogCard.dacWrite(i, value);
                }
                break;

            case CRITICAL:
                // Red indication - high voltage (fast flashing)
                {
                    uint16_t value = ((millis() / 250) % 2) ? voltageToDac(3.0) : voltageToDac(0.0);
                    analogCard.dacWrite(i, value);
                }
                break;
        }
    }
}

/**
 * @brief Print alert message
 */
void printAlert(uint8_t channelIndex, AlertLevel level) {
    ChannelStatus& status = channelStatus[channelIndex];
    ChannelConfig& config = channels[channelIndex];

    Serial.println("\n!!! ALERT !!!");
    Serial.printf("Channel: %s (ADC %d)\n", config.name, config.adcChannel);
    Serial.printf("Current Voltage: %.3f V\n", status.currentVoltage);
    Serial.printf("Valid Range: %.3f - %.3f V\n", config.minVoltage, config.maxVoltage);
    Serial.printf("Alert Level: %s\n", getAlertLevelName(level));
    Serial.printf("Timestamp: %lu ms\n", millis());
    Serial.println();
}

/**
 * @brief Monitor all channels
 */
void monitorChannels() {
    bool anyAlerts = false;

    for (uint8_t i = 0; i < NUM_MONITORED_CHANNELS; i++) {
        // Read ADC
        uint16_t rawValue = analogCard.analogRead(channels[i].adcChannel);
        float voltage = adcToVoltage(rawValue);

        // Update status
        channelStatus[i].currentVoltage = voltage;
        channelStatus[i].readings++;

        // Update min/max
        if (voltage < channelStatus[i].minRecorded) {
            channelStatus[i].minRecorded = voltage;
        }
        if (voltage > channelStatus[i].maxRecorded) {
            channelStatus[i].maxRecorded = voltage;
        }

        // Check thresholds
        AlertLevel newLevel = checkThresholds(i, voltage);

        // Print alert if level changed or if critical
        if (newLevel != channelStatus[i].alertLevel ||
            (newLevel == CRITICAL && millis() - channelStatus[i].lastAlertTime > 5000)) {

            if (newLevel != NORMAL) {
                printAlert(i, newLevel);
                channelStatus[i].lastAlertTime = millis();
                anyAlerts = true;
            } else if (channelStatus[i].alertLevel != NORMAL) {
                // Alert cleared
                Serial.printf("✓ Alert cleared for %s (ADC %d)\n\n",
                              channels[i].name, channels[i].adcChannel);
            }

            channelStatus[i].alertLevel = newLevel;
        }
    }

    // Update DAC outputs for visual indication
    updateAlertOutputs();
}

/**
 * @brief Print statistics for all channels
 */
void printStatistics() {
    Serial.println("========================================");
    Serial.println("  VOLTAGE MONITORING STATISTICS");
    Serial.println("========================================");
    Serial.printf("Uptime: %lu ms\n\n", millis());

    for (uint8_t i = 0; i < NUM_MONITORED_CHANNELS; i++) {
        ChannelConfig& config = channels[i];
        ChannelStatus& status = channelStatus[i];

        Serial.printf("Channel %d: %s\n", i, config.name);
        Serial.println("----------------------------------------");
        Serial.printf("  Current:  %6.3f V  [%s]\n",
                      status.currentVoltage,
                      getAlertLevelName(status.alertLevel));
        Serial.printf("  Range:    %6.3f - %6.3f V (valid)\n",
                      config.minVoltage, config.maxVoltage);
        Serial.printf("  Recorded: %6.3f - %6.3f V (min/max)\n",
                      status.minRecorded, status.maxRecorded);
        Serial.printf("  Readings: %lu\n", status.readings);
        Serial.println();
    }
    Serial.println();
}

/**
 * @brief Setup function
 */
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.println("  Voltage Monitor - ESPMegaPRO");
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

    // Initialize channel status
    for (uint8_t i = 0; i < NUM_MONITORED_CHANNELS; i++) {
        channelStatus[i].currentVoltage = 0.0;
        channelStatus[i].minRecorded = 999.9;
        channelStatus[i].maxRecorded = 0.0;
        channelStatus[i].readings = 0;
        channelStatus[i].alertLevel = NORMAL;
        channelStatus[i].lastAlertTime = 0;
    }

    // Print configuration
    Serial.println("Monitoring Configuration:");
    Serial.println("----------------------------------------");
    for (uint8_t i = 0; i < NUM_MONITORED_CHANNELS; i++) {
        Serial.printf("Channel %d: %s\n", i, channels[i].name);
        Serial.printf("  ADC: %d\n", channels[i].adcChannel);
        Serial.printf("  Range: %.2f - %.2f V\n",
                      channels[i].minVoltage, channels[i].maxVoltage);
        Serial.printf("  Warning Margin: %.2f V\n", channels[i].warningMargin);
        Serial.println();
    }

    Serial.printf("Monitor Interval: %lu ms\n", MONITOR_INTERVAL);
    Serial.printf("Stats Interval: %lu ms\n", STATS_INTERVAL);
    Serial.println();

    Serial.println("Starting monitoring...");
    Serial.println("========================================\n");

    // Initial reading
    monitorChannels();
    lastMonitorTime = millis();
    lastStatsTime = millis();
}

/**
 * @brief Main loop
 */
void loop() {
    unsigned long currentTime = millis();

    // Monitor channels
    if (currentTime - lastMonitorTime >= MONITOR_INTERVAL) {
        lastMonitorTime = currentTime;
        monitorChannels();
    }

    // Print statistics
    if (currentTime - lastStatsTime >= STATS_INTERVAL) {
        lastStatsTime = currentTime;
        printStatistics();
    }

    // Call card loop
    analogCard.loop();
}

/**
 * Usage Notes:
 *
 * 1. Configure channel thresholds in the channels[] array
 * 2. Connect voltage sources to ADC inputs 0-3
 * 3. Connect LEDs to DAC outputs 0-3 for visual alerts:
 *    - Steady low: Normal (green)
 *    - Slow flash: Warning (yellow)
 *    - Fast flash: Critical (red)
 *
 * 4. Monitor serial output for alerts and statistics
 *
 * Alert Levels:
 * - NORMAL: Voltage within safe range
 * - WARNING: Approaching threshold (within warning margin)
 * - CRITICAL: Outside valid range
 *
 * Customization:
 * - Adjust MONITOR_INTERVAL for faster/slower checks
 * - Modify threshold values for your application
 * - Add more channels (up to 8 ADC inputs)
 * - Customize DAC output patterns for alerts
 */
