# AnalogCard Documentation

## Table of Contents
1. [Overview](#overview)
2. [Hardware Specifications](#hardware-specifications)
3. [API Reference](#api-reference)
4. [ADC Input Channels](#adc-input-channels)
5. [DAC Output Channels](#dac-output-channels)
6. [DAC State vs Value](#dac-state-vs-value)
7. [Voltage Conversion](#voltage-conversion)
8. [Callback System](#callback-system)
9. [IoT Integration](#iot-integration)
10. [Code Examples](#code-examples)

---

## Overview

The **AnalogCard** is an expansion card for the ESPMegaPRO system that provides high-precision analog input and output capabilities. It features 8 analog input channels and 4 analog output (DAC) channels, making it ideal for sensor interfacing, signal generation, and industrial control applications.

### Key Features
- 8 analog input channels with 16-bit resolution
- 4 analog output channels with 12-bit resolution
- I2C interface for communication
- Callback system for DAC change notifications
- Full MQTT integration for remote monitoring and control
- Independent enable/disable for each DAC output

### Card Type
- **Type ID**: `0x02` (`CARD_TYPE_ANALOG`)
- **Installation Limit**: Only one AnalogCard can be installed in an ESPMegaPRO board

---

## Hardware Specifications

### ADC Specifications
- **IC**: Adafruit ADS1115 (2 units)
- **Resolution**: 16-bit (0-65535)
- **Number of Channels**: 8 (0-7)
- **Configuration**:
  - Bank A: Channels 0-3 (I2C Address 0x48)
  - Bank B: Channels 4-7 (I2C Address 0x49)
- **Programmable Gain Amplifier (PGA)**: Yes (default ±6.144V)
- **Sample Rate**: Up to 860 samples per second

### DAC Specifications
- **IC**: MCP4725 (4 units)
- **Resolution**: 12-bit (0-4095)
- **Number of Channels**: 4 (0-3)
- **I2C Addresses**:
  - DAC 0: 0x60
  - DAC 1: 0x61
  - DAC 2: 0x62
  - DAC 3: 0x63
- **Output Voltage Range**: 0V to VDD (typically 3.3V or 5V)
- **EEPROM**: Yes (for storing DAC values on power-off)

### I2C Addresses Summary

| Component | I2C Address | Description |
|-----------|-------------|-------------|
| ADC Bank A | 0x48 | Analog inputs 0-3 |
| ADC Bank B | 0x49 | Analog inputs 4-7 |
| DAC 0 | 0x60 | Analog output 0 |
| DAC 1 | 0x61 | Analog output 1 |
| DAC 2 | 0x62 | Analog output 2 |
| DAC 3 | 0x63 | Analog output 3 |

---

## API Reference

### Constructor

#### `AnalogCard()`
Creates a new AnalogCard instance.

```cpp
AnalogCard card;
```

**Parameters**: None

**Returns**: AnalogCard object

---

### Initialization

#### `bool begin()`
Initializes the AnalogCard hardware (ADCs and DACs).

```cpp
bool success = card.begin();
```

**Parameters**: None

**Returns**:
- `true` if initialization is successful
- `false` if ADC banks fail to initialize (DAC failures are logged but don't affect return value)

**Note**: This function must be called in `setup()` before using any card functions.

---

### Loop Function

#### `void loop()`
Main loop function for the AnalogCard.

```cpp
card.loop();
```

**Parameters**: None

**Returns**: None

**Note**: Currently does nothing but should be called in the main loop for future compatibility.

---

### ADC Functions

#### `uint16_t analogRead(uint8_t pin)`
Reads the analog value from the specified ADC channel.

```cpp
uint16_t value = card.analogRead(3);
```

**Parameters**:
- `pin` - ADC channel number (0-7)

**Returns**:
- 16-bit ADC value (0-65535)
- Returns 65535 if pin is invalid

**Note**:
- Channels 0-3 read from Bank A (0x48)
- Channels 4-7 read from Bank B (0x49)

---

### DAC Functions

#### `void dacWrite(uint8_t pin, uint16_t value)`
Writes a value to the specified DAC channel. This is a convenience function that automatically:
- Sets the DAC state to ON if value > 0, OFF if value = 0
- Sets the DAC value
- Triggers callbacks

```cpp
card.dacWrite(0, 2048);  // Set DAC 0 to 2048 (mid-range) and enable
card.dacWrite(1, 0);     // Set DAC 1 to 0 and disable
```

**Parameters**:
- `pin` - DAC channel number (0-3)
- `value` - 12-bit value (0-4095)

**Returns**: None

---

#### `void setDACState(uint8_t pin, bool state)`
Sets the on/off state of the specified DAC channel without changing its value.

```cpp
card.setDACState(0, true);   // Enable DAC 0
card.setDACState(0, false);  // Disable DAC 0 (output goes to 0V)
```

**Parameters**:
- `pin` - DAC channel number (0-3)
- `state` - `true` for ON, `false` for OFF

**Returns**: None

**Note**: When state is OFF, the actual output voltage is 0V regardless of the stored value. Triggers DAC change callbacks.

---

#### `void setDACValue(uint8_t pin, uint16_t value)`
Sets the value of the specified DAC channel. The actual output depends on the DAC state.

```cpp
card.setDACValue(0, 4095);  // Set to maximum value
```

**Parameters**:
- `pin` - DAC channel number (0-3)
- `value` - 12-bit value (0-4095)

**Returns**: None

**Note**: If DAC state is OFF, the value is stored but not output until the state is set to ON. Triggers DAC change callbacks.

---

#### `bool getDACState(uint8_t pin)`
Gets the current state of the specified DAC channel.

```cpp
bool isEnabled = card.getDACState(0);
```

**Parameters**:
- `pin` - DAC channel number (0-3)

**Returns**:
- `true` if DAC is enabled
- `false` if DAC is disabled

---

#### `uint16_t getDACValue(uint8_t pin)`
Gets the stored value of the specified DAC channel.

```cpp
uint16_t value = card.getDACValue(0);
```

**Parameters**:
- `pin` - DAC channel number (0-3)

**Returns**: 12-bit value (0-4095)

**Note**: Returns the stored value regardless of the DAC state.

---

#### `void sendDataToDAC(uint8_t pin, uint16_t value)`
Low-level function to send data directly to the DAC hardware.

```cpp
card.sendDataToDAC(0, 2048);
```

**Parameters**:
- `pin` - DAC channel number (0-3)
- `value` - 12-bit value (0-4095)

**Returns**: None

**Note**: This function does NOT update internal state tracking or trigger callbacks. Use `setDACValue()` or `dacWrite()` instead for normal operation.

---

### Callback Functions

#### `uint8_t registerDACChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback)`
Registers a callback function that will be called whenever a DAC state or value changes.

```cpp
void onDACChange(uint8_t pin, bool state, uint16_t value) {
    Serial.printf("DAC %d changed: state=%d, value=%d\n", pin, state, value);
}

uint8_t handler_id = card.registerDACChangeCallback(onDACChange);
```

**Parameters**:
- `callback` - Function to call on DAC changes
  - Parameter 1: `uint8_t pin` - DAC channel that changed
  - Parameter 2: `bool state` - New state (ON/OFF)
  - Parameter 3: `uint16_t value` - New value

**Returns**: Handler ID (uint8_t) for later unregistering

**Note**: Multiple callbacks can be registered and all will be called on any DAC change.

---

#### `void unregisterDACChangeCallback(uint8_t handler)`
Removes a previously registered DAC change callback.

```cpp
card.unregisterDACChangeCallback(handler_id);
```

**Parameters**:
- `handler` - Handler ID returned by `registerDACChangeCallback()`

**Returns**: None

---

### Utility Functions

#### `uint8_t getType()`
Gets the card type identifier.

```cpp
uint8_t type = card.getType();  // Returns 0x02
```

**Parameters**: None

**Returns**: `CARD_TYPE_ANALOG` (0x02)

---

## ADC Input Channels

The AnalogCard provides 8 analog input channels (0-7) using two ADS1115 ADC chips.

### Channel Mapping

| Channel | Bank | Physical ADC | ADC Channel | I2C Address |
|---------|------|--------------|-------------|-------------|
| 0 | A | ADS1115 A | A0 | 0x48 |
| 1 | A | ADS1115 A | A1 | 0x48 |
| 2 | A | ADS1115 A | A2 | 0x48 |
| 3 | A | ADS1115 A | A3 | 0x48 |
| 4 | B | ADS1115 B | A0 | 0x49 |
| 5 | B | ADS1115 B | A1 | 0x49 |
| 6 | B | ADS1115 B | A2 | 0x49 |
| 7 | B | ADS1115 B | A3 | 0x49 |

### Reading ADC Channels

```cpp
// Read single channel
uint16_t value = card.analogRead(0);

// Read all channels
for (uint8_t i = 0; i < 8; i++) {
    uint16_t value = card.analogRead(i);
    Serial.printf("ADC %d: %d\n", i, value);
}
```

### ADC Characteristics
- **Resolution**: 16-bit (0-65535)
- **Default Gain**: ±6.144V full-scale (can be changed via Adafruit library)
- **Single-ended mode**: Measures voltage relative to ground
- **Conversion time**: Depends on sample rate setting (default ~8ms per conversion)

---

## DAC Output Channels

The AnalogCard provides 4 analog output channels (0-3) using four MCP4725 DAC chips.

### Channel Mapping

| Channel | Physical DAC | I2C Address | Output Voltage Range |
|---------|--------------|-------------|----------------------|
| 0 | MCP4725 #0 | 0x60 | 0V to VDD |
| 1 | MCP4725 #1 | 0x61 | 0V to VDD |
| 2 | MCP4725 #2 | 0x62 | 0V to VDD |
| 3 | MCP4725 #3 | 0x63 | 0V to VDD |

### Writing to DAC Channels

```cpp
// Simple write (value determines state)
card.dacWrite(0, 2048);  // 50% output, auto-enable
card.dacWrite(0, 0);     // 0V output, auto-disable

// Separate control of state and value
card.setDACValue(0, 3000);  // Set value
card.setDACState(0, true);  // Enable output

// Temporarily disable without losing value
card.setDACState(0, false); // Output goes to 0V
card.setDACState(0, true);  // Resumes at previous value (3000)
```

---

## DAC State vs Value

Each DAC channel has two independent properties: **state** and **value**.

### State (bool)
- **ON (true)**: DAC outputs the stored value
- **OFF (false)**: DAC outputs 0V regardless of stored value

### Value (uint16_t)
- **Range**: 0-4095 (12-bit)
- **Storage**: Value is always stored, even when state is OFF
- **Output**: Actual output = value × state

### Behavior Matrix

| State | Value | Actual Output |
|-------|-------|---------------|
| OFF | 0 | 0 (0V) |
| OFF | 2048 | 0 (0V) |
| OFF | 4095 | 0 (0V) |
| ON | 0 | 0 (0V) |
| ON | 2048 | 2048 (~50% VDD) |
| ON | 4095 | 4095 (~VDD) |

### Use Cases

**Example 1: Quick Enable/Disable**
```cpp
// Setup a voltage level
card.setDACValue(0, 3000);

// Enable/disable output without reconfiguring value
card.setDACState(0, true);   // Output 3000
delay(1000);
card.setDACState(0, false);  // Output 0
delay(1000);
card.setDACState(0, true);   // Output 3000 again
```

**Example 2: Pre-configure Before Enabling**
```cpp
// Set desired value while output is off
card.setDACState(0, false);
card.setDACValue(0, 4095);

// Enable output atomically
card.setDACState(0, true);  // Immediately outputs 4095
```

**Example 3: Using dacWrite()**
```cpp
// dacWrite() automatically manages state based on value
card.dacWrite(0, 2048);  // Sets value=2048, state=true
card.dacWrite(0, 0);     // Sets value=0, state=false
```

---

## Voltage Conversion

### ADC Voltage Conversion

The ADS1115 ADC provides 16-bit resolution. The voltage conversion depends on the programmable gain setting.

#### Default Configuration (±6.144V Full Scale)

```cpp
// For single-ended measurements (0 to +6.144V)
float voltage = (adcValue * 6.144) / 32768.0;

// Example:
uint16_t raw = card.analogRead(0);
float volts = (raw * 6.144) / 32768.0;
```

#### Common Gain Settings

| Gain | Full Scale Range | LSB Size | Formula |
|------|------------------|----------|---------|
| ±6.144V | ±6.144V | 187.5 µV | `(value * 6.144) / 32768` |
| ±4.096V | ±4.096V | 125 µV | `(value * 4.096) / 32768` |
| ±2.048V | ±2.048V | 62.5 µV | `(value * 2.048) / 32768` |
| ±1.024V | ±1.024V | 31.25 µV | `(value * 1.024) / 32768` |
| ±0.512V | ±0.512V | 15.625 µV | `(value * 0.512) / 32768` |
| ±0.256V | ±0.256V | 7.8125 µV | `(value * 0.256) / 32768` |

**Note**: The gain can be changed using the underlying Adafruit_ADS1115 library methods if needed.

### DAC Voltage Conversion

The MCP4725 provides 12-bit resolution with output from 0V to VDD.

#### Voltage to DAC Value

```cpp
// For VDD = 3.3V
uint16_t dacValue = (desiredVoltage / 3.3) * 4095;

// For VDD = 5.0V
uint16_t dacValue = (desiredVoltage / 5.0) * 4095;

// Example: Output 1.65V with VDD=3.3V
float targetVoltage = 1.65;
uint16_t value = (targetVoltage / 3.3) * 4095;  // = 2048
card.dacWrite(0, value);
```

#### DAC Value to Voltage

```cpp
// For VDD = 3.3V
float voltage = (dacValue / 4095.0) * 3.3;

// For VDD = 5.0V
float voltage = (dacValue / 4095.0) * 5.0;

// Example:
uint16_t value = card.getDACValue(0);
float volts = (value / 4095.0) * 3.3;
```

### Practical Conversion Examples

```cpp
// Read sensor voltage (0-3.3V range)
uint16_t raw = card.analogRead(0);
float sensorVoltage = (raw * 6.144) / 32768.0;

// Convert to percentage (0-100%)
float percentage = (sensorVoltage / 3.3) * 100.0;

// Output proportional control signal
uint16_t controlValue = (percentage / 100.0) * 4095;
card.dacWrite(0, controlValue);
```

---

## Callback System

The AnalogCard provides a callback system that notifies your code whenever a DAC channel's state or value changes.

### Callback Function Signature

```cpp
void callbackFunction(uint8_t pin, bool state, uint16_t value)
```

**Parameters**:
- `pin` - DAC channel that changed (0-3)
- `state` - New state (true=ON, false=OFF)
- `value` - New value (0-4095)

### Registering Callbacks

#### Method 1: Function Pointer
```cpp
void onDACChange(uint8_t pin, bool state, uint16_t value) {
    Serial.printf("DAC %d: state=%s, value=%d\n",
                  pin, state ? "ON" : "OFF", value);
}

void setup() {
    card.begin();
    uint8_t handler = card.registerDACChangeCallback(onDACChange);
}
```

#### Method 2: Lambda Function
```cpp
void setup() {
    card.begin();

    uint8_t handler = card.registerDACChangeCallback(
        [](uint8_t pin, bool state, uint16_t value) {
            Serial.printf("DAC changed: %d, %d, %d\n", pin, state, value);
        }
    );
}
```

#### Method 3: Class Member Function
```cpp
class MyController {
public:
    void onDACChange(uint8_t pin, bool state, uint16_t value) {
        Serial.printf("DAC %d changed\n", pin);
    }

    void setup(AnalogCard& card) {
        auto callback = std::bind(&MyController::onDACChange, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2,
                                  std::placeholders::_3);
        handler_id = card.registerDACChangeCallback(callback);
    }

private:
    uint8_t handler_id;
};
```

### Multiple Callbacks

You can register multiple callbacks, and all will be called when a DAC changes:

```cpp
// Register multiple handlers
uint8_t handler1 = card.registerDACChangeCallback(callback1);
uint8_t handler2 = card.registerDACChangeCallback(callback2);
uint8_t handler3 = card.registerDACChangeCallback(callback3);

// All three callbacks will be called on any DAC change
card.dacWrite(0, 2048);  // Triggers callback1, callback2, callback3
```

### Unregistering Callbacks

```cpp
uint8_t handler = card.registerDACChangeCallback(myCallback);

// Later, remove the callback
card.unregisterDACChangeCallback(handler);
```

### Callback Triggering

Callbacks are triggered by:
- `setDACState()` - Called with new state and current value
- `setDACValue()` - Called with current state and new value
- `dacWrite()` - Called with new state and new value

Callbacks are NOT triggered by:
- `sendDataToDAC()` - Low-level function

### Use Cases

**Data Logging**
```cpp
card.registerDACChangeCallback([](uint8_t pin, bool state, uint16_t value) {
    logToSD(pin, state, value, millis());
});
```

**MQTT Publishing** (handled automatically by AnalogIoT)
```cpp
card.registerDACChangeCallback([](uint8_t pin, bool state, uint16_t value) {
    mqtt.publish(getTopic(pin), getValue(value));
});
```

**Safety Interlocks**
```cpp
card.registerDACChangeCallback([](uint8_t pin, bool state, uint16_t value) {
    if (value > SAFETY_THRESHOLD) {
        emergencyShutdown();
    }
});
```

---

## IoT Integration

The **AnalogIoT** class provides full MQTT integration for remote monitoring and control of the AnalogCard.

### Setup

```cpp
#include <ESPMegaIoT.hpp>
#include <AnalogCard.hpp>

AnalogCard analogCard;
ESPMegaIoT iot;

void setup() {
    analogCard.begin();

    // Register card with IoT system
    iot.registerCard(0, &analogCard);

    // IoT system will automatically create AnalogIoT component
}
```

### MQTT Topics

All topics are prefixed with the base topic configured in ESPMegaIoT.

#### DAC Control Topics (Subscribe)

| Topic | Payload | Description |
|-------|---------|-------------|
| `dac/00/set/state` | `0` or `1` | Set DAC 0 state (OFF/ON) |
| `dac/01/set/state` | `0` or `1` | Set DAC 1 state (OFF/ON) |
| `dac/02/set/state` | `0` or `1` | Set DAC 2 state (OFF/ON) |
| `dac/03/set/state` | `0` or `1` | Set DAC 3 state (OFF/ON) |
| `dac/00/set/value` | `0-4095` | Set DAC 0 value |
| `dac/01/set/value` | `0-4095` | Set DAC 1 value |
| `dac/02/set/value` | `0-4095` | Set DAC 2 value |
| `dac/03/set/value` | `0-4095` | Set DAC 3 value |

#### DAC Status Topics (Publish)

| Topic | Payload | Description |
|-------|---------|-------------|
| `dac/00/state` | `0` or `1` | DAC 0 current state |
| `dac/01/state` | `0` or `1` | DAC 1 current state |
| `dac/02/state` | `0` or `1` | DAC 2 current state |
| `dac/03/state` | `0` or `1` | DAC 3 current state |
| `dac/00/value` | `0-4095` | DAC 0 current value |
| `dac/01/value` | `0-4095` | DAC 1 current value |
| `dac/02/value` | `0-4095` | DAC 2 current value |
| `dac/03/value` | `0-4095` | DAC 3 current value |

#### ADC Control Topics (Subscribe)

| Topic | Payload | Description |
|-------|---------|-------------|
| `adc/00/set/conversion_interval` | milliseconds | Set ADC 0 read interval |
| `adc/00/set/conversion_enabled` | `0` or `1` | Enable/disable ADC 0 publishing |
| `adc/01/set/conversion_interval` | milliseconds | Set ADC 1 read interval |
| `adc/01/set/conversion_enabled` | `0` or `1` | Enable/disable ADC 1 publishing |
| ... | ... | (Same for ADC 02-07) |

#### ADC Status Topics (Publish)

| Topic | Payload | Description |
|-------|---------|-------------|
| `adc/00/value` | `0-65535` | ADC 0 current reading |
| `adc/01/value` | `0-65535` | ADC 1 current reading |
| ... | ... | (Same for ADC 02-07) |

#### System Topics

| Topic | Payload | Description |
|-------|---------|-------------|
| `requeststate` | any | Request publish of all DAC and ADC states |

### MQTT Examples

#### Setting DAC Output via MQTT

```bash
# Set DAC 0 to 2.5V (assuming 5V VDD, value = 2048)
mosquitto_pub -h broker.local -t "espmega/card00/dac/00/set/value" -m "2048"

# Enable DAC 0
mosquitto_pub -h broker.local -t "espmega/card00/dac/00/set/state" -m "1"

# Disable DAC 1
mosquitto_pub -h broker.local -t "espmega/card00/dac/01/set/state" -m "0"
```

#### Monitoring ADC via MQTT

```bash
# Enable ADC 0 publishing every 1000ms
mosquitto_pub -h broker.local -t "espmega/card00/adc/00/set/conversion_enabled" -m "1"
mosquitto_pub -h broker.local -t "espmega/card00/adc/00/set/conversion_interval" -m "1000"

# Subscribe to ADC readings
mosquitto_sub -h broker.local -t "espmega/card00/adc/00/value"
```

#### Request Current State

```bash
# Request all current DAC and ADC states
mosquitto_pub -h broker.local -t "espmega/card00/requeststate" -m "1"
```

### AnalogIoT API Reference

#### ADC Publishing Control

```cpp
AnalogIoT* analogIoT = (AnalogIoT*)iot.getCard(0);

// Enable publishing for all ADCs
analogIoT->setADCsPublishEnabled(true);

// Set publish interval for all ADCs (milliseconds)
analogIoT->setADCsPublishInterval(1000);

// Control individual ADC
analogIoT->setADCConversionEnabled(0, true);   // Enable ADC 0
analogIoT->setADCConversionInterval(0, 500);   // Read every 500ms

// Manually publish
analogIoT->publishADC(0);      // Publish single ADC
analogIoT->publishADCs();      // Publish all ADCs
analogIoT->publishDAC(0);      // Publish single DAC
analogIoT->publishDACs();      // Publish all DACs
```

#### ADC Conversion Callbacks

```cpp
void onADCConversion(uint8_t pin, uint16_t value) {
    Serial.printf("ADC %d: %d\n", pin, value);
}

// Register callback
uint8_t handler = analogIoT->registerADCConversionCallback(onADCConversion);

// Unregister callback
analogIoT->unregisterADCConversionCallback(handler);
```

---

## Code Examples

### Example 1: Basic ADC Reading

```cpp
#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

AnalogCard analogCard;

void setup() {
    Serial.begin(115200);

    // Initialize card
    if (!analogCard.begin()) {
        Serial.println("Failed to initialize Analog Card!");
        while (1);
    }

    Serial.println("Analog Card initialized successfully");
}

void loop() {
    // Read all ADC channels
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t value = analogCard.analogRead(i);
        float voltage = (value * 6.144) / 32768.0;

        Serial.printf("ADC %d: Raw=%d, Voltage=%.3fV\n", i, value, voltage);
    }

    Serial.println("---");
    delay(1000);
}
```

### Example 2: DAC Output Control

```cpp
#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

AnalogCard analogCard;

void setup() {
    Serial.begin(115200);
    analogCard.begin();

    // Set DAC 0 to 1.65V (50% of 3.3V)
    uint16_t value = (1.65 / 3.3) * 4095;  // = 2048
    analogCard.dacWrite(0, value);

    Serial.printf("DAC 0 set to %d (1.65V)\n", value);
}

void loop() {
    // Generate sawtooth wave on DAC 1
    static uint16_t dacValue = 0;

    analogCard.dacWrite(1, dacValue);

    dacValue += 10;
    if (dacValue > 4095) {
        dacValue = 0;
    }

    delay(10);
}
```

### Example 3: DAC State Control

```cpp
#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

AnalogCard analogCard;

void setup() {
    Serial.begin(115200);
    analogCard.begin();

    // Pre-configure DAC value
    analogCard.setDACValue(0, 3000);
    analogCard.setDACState(0, false);  // Keep off initially

    Serial.println("DAC configured, press button to toggle");
}

void loop() {
    // Toggle DAC state every 2 seconds
    static unsigned long lastToggle = 0;
    static bool dacState = false;

    if (millis() - lastToggle > 2000) {
        dacState = !dacState;
        analogCard.setDACState(0, dacState);

        Serial.printf("DAC State: %s\n", dacState ? "ON" : "OFF");
        lastToggle = millis();
    }
}
```

### Example 4: Callback Usage

```cpp
#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

AnalogCard analogCard;

void onDACChange(uint8_t pin, bool state, uint16_t value) {
    float voltage = (value / 4095.0) * 3.3;
    Serial.printf("DAC %d changed: State=%s, Value=%d (%.2fV)\n",
                  pin, state ? "ON" : "OFF", value, voltage);
}

void setup() {
    Serial.begin(115200);
    analogCard.begin();

    // Register callback
    analogCard.registerDACChangeCallback(onDACChange);

    Serial.println("Callback registered");
}

void loop() {
    // Changes will trigger the callback
    analogCard.dacWrite(0, random(0, 4096));
    delay(2000);

    analogCard.setDACState(1, !analogCard.getDACState(1));
    delay(2000);
}
```

### Example 5: Sensor Reading with Calibration

```cpp
#include <ESPMegaPRO.h>
#include <AnalogCard.hpp>

AnalogCard analogCard;

// Calibration values
const float ADC_OFFSET = 0.0;
const float ADC_SCALE = 1.0;

float readCalibratedVoltage(uint8_t pin) {
    uint16_t raw = analogCard.analogRead(pin);
    float voltage = (raw * 6.144) / 32768.0;
    return (voltage * ADC_SCALE) + ADC_OFFSET;
}

void setup() {
    Serial.begin(115200);
    analogCard.begin();
}

void loop() {
    // Read temperature sensor (example: LM35, 10mV/°C)
    float voltage = readCalibratedVoltage(0);
    float temperature = voltage * 100.0;  // LM35: 10mV/°C

    Serial.printf("Temperature: %.1f°C (%.3fV)\n", temperature, voltage);

    delay(1000);
}
```

### Example 6: MQTT Integration

```cpp
#include <ESPMegaPRO.h>
#include <ESPMegaIoT.hpp>
#include <AnalogCard.hpp>

AnalogCard analogCard;
ESPMegaIoT iot;

void setup() {
    Serial.begin(115200);

    // Initialize card
    analogCard.begin();

    // Initialize IoT (configure WiFi and MQTT in ESPMegaIoT)
    iot.begin();

    // Register card with IoT system
    iot.registerCard(0, &analogCard);

    // Get AnalogIoT component
    AnalogIoT* analogIoT = (AnalogIoT*)iot.getCard(0);

    // Enable ADC publishing
    analogIoT->setADCConversionEnabled(0, true);
    analogIoT->setADCConversionInterval(0, 1000);

    // Register ADC callback
    analogIoT->registerADCConversionCallback(
        [](uint8_t pin, uint16_t value) {
            Serial.printf("ADC %d: %d\n", pin, value);
        }
    );

    Serial.println("MQTT integration ready");
}

void loop() {
    iot.loop();
    analogCard.loop();
}
```

---

## Troubleshooting

### Common Issues

**Problem**: `analogRead()` returns 65535
- **Cause**: Invalid pin number or ADC not initialized
- **Solution**: Check pin is 0-7 and `begin()` was called successfully

**Problem**: DAC output is always 0V
- **Cause**: DAC state is OFF
- **Solution**: Check `getDACState()` and use `setDACState(pin, true)`

**Problem**: DAC not responding
- **Cause**: I2C communication failure
- **Solution**: Check I2C wiring, verify I2C addresses aren't conflicting

**Problem**: MQTT commands not working
- **Cause**: Incorrect topic format or card not registered
- **Solution**: Verify topic format and ensure `iot.registerCard()` was called

**Problem**: ADC readings are noisy
- **Cause**: Electrical noise, insufficient grounding
- **Solution**: Add filtering capacitors, use shielded cables, average multiple readings

### Tips

1. Always call `begin()` before using any card functions
2. Keep DAC values within 0-4095 range
3. Remember that DAC output = value × state
4. Use callbacks for event-driven programming
5. Enable only needed ADC channels in IoT mode to reduce processing
6. Consider adding local filtering for noisy ADC readings

---

## Additional Resources

- [ADS1115 Datasheet](https://www.ti.com/product/ADS1115)
- [MCP4725 Datasheet](https://www.microchip.com/en-us/product/MCP4725)
- [Adafruit ADS1X15 Library](https://github.com/adafruit/Adafruit_ADS1X15)
- [MCP4725 Library](https://github.com/RobTillaart/MCP4725)

---

**Document Version**: 1.0
**Last Updated**: 2025-11-23
**Compatible with**: ESPMegaPRO Library v2.10.0+
