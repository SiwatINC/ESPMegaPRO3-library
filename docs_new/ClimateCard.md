# ClimateCard Documentation

## Table of Contents
1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [AirConditioner Structure](#airconditioner-structure)
4. [Defining IR Codes](#defining-ir-codes)
5. [Supported Sensor Types](#supported-sensor-types)
6. [Complete API Reference](#complete-api-reference)
7. [FRAM Persistence](#fram-persistence)
8. [Callback Systems](#callback-systems)
9. [IoT Integration (MQTT)](#iot-integration-mqtt)
10. [Complete Code Examples](#complete-code-examples)

---

## Overview

The **ClimateCard** is an expansion card for the ESPMegaPRO system that provides air conditioner control via infrared (IR) signals. It can also read environmental data from temperature and humidity sensors, making it a complete climate control solution.

### Key Features
- **IR-based AC Control**: Control air conditioner temperature, mode, and fan speed via IR LED
- **Environmental Sensing**: Optional DHT22 (temperature + humidity) or DS18B20 (temperature only) sensor support
- **State Persistence**: Save and restore AC state using FRAM memory
- **Callback System**: React to AC state changes and sensor readings
- **MQTT Integration**: Full IoT support with publish/subscribe topics
- **Flexible Configuration**: Define custom IR codes for any AC model

### Use Cases
- Smart home climate control
- Automated thermostat functionality
- Remote AC monitoring and control via MQTT
- Temperature-based automation
- Integration with home automation systems

---

## Hardware Requirements

### Required Components
1. **ESPMegaPRO Board** or **ESP32** with RMT peripheral support
2. **IR LED** (940nm recommended) connected to a GPIO pin
3. **Current-limiting resistor** for IR LED (typically 100-330Ω)
4. Optional: **IR LED driver transistor** (recommended for better range)

### Optional Components
- **DHT22 Sensor**: For temperature and humidity monitoring
- **DS18B20 Sensor**: For temperature-only monitoring
- **FRAM Module**: For state persistence (when used with ESPMegaPRO)

### Wiring
```
IR LED Connection:
    ESP32 GPIO --> [Resistor] --> IR LED Anode --> GND

    Or with transistor (recommended):
    ESP32 GPIO --> [1kΩ] --> NPN Base
    VCC --> [100Ω] --> IR LED Anode --> NPN Collector
    GND --> NPN Emitter

DHT22 Connection:
    ESP32 GPIO <--> DHT22 Data Pin
    VCC (3.3V or 5V) --> DHT22 VCC
    GND --> DHT22 GND

DS18B20 Connection:
    ESP32 GPIO <--> DS18B20 Data Pin (with 4.7kΩ pullup to VCC)
    VCC (3.3V) --> DS18B20 VCC
    GND --> DS18B20 GND
```

### RMT Channel Allocation
The ClimateCard uses the ESP32's RMT (Remote Control) peripheral for IR transmission. Each ClimateCard requires one RMT channel:
- ESP32 has 8 RMT channels (0-7)
- Each channel can support one IRBlaster/ClimateCard
- Channels must be unique across all IR devices
- Default channel is RMT_CHANNEL_0 if not specified

---

## AirConditioner Structure

The `AirConditioner` struct defines the characteristics and capabilities of your specific AC model.

### Structure Definition
```cpp
struct AirConditioner {
    uint8_t max_temperature;     // Maximum temperature (e.g., 30°C)
    uint8_t min_temperature;     // Minimum temperature (e.g., 16°C)
    uint8_t modes;               // Number of modes (e.g., 3)
    const char **mode_names;     // Array of mode names
    uint8_t fan_speeds;          // Number of fan speeds (e.g., 4)
    const char **fan_speed_names;// Array of fan speed names
    size_t (*getInfraredCode)(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr);
};
```

### Field Descriptions

#### `max_temperature` and `min_temperature`
Define the temperature range your AC supports. The ClimateCard will automatically clamp values to this range.

**Example:**
```cpp
.max_temperature = 30,  // AC supports up to 30°C
.min_temperature = 16,  // AC supports down to 16°C
```

#### `modes` and `mode_names`
Define the operating modes your AC supports. Common modes include:
- Off
- Cool
- Heat
- Fan Only
- Dry/Dehumidify
- Auto

**Example:**
```cpp
const char *mode_names[] = {"off", "cool", "heat", "fan_only", "dry", "auto"};

.modes = 6,  // Number of modes (indices 0-5)
.mode_names = mode_names
```

**Important:** Mode indices start at 0. If you have 4 modes, valid indices are 0, 1, 2, 3.

#### `fan_speeds` and `fan_speed_names`
Define the fan speed options your AC supports.

**Example:**
```cpp
const char *fan_speed_names[] = {"auto", "low", "medium", "high"};

.fan_speeds = 4,  // Number of fan speeds (indices 0-3)
.fan_speed_names = fan_speed_names
```

#### `getInfraredCode` Function Pointer
This function retrieves the IR timing data for a specific AC state. It must return the size of the IR code array and set the provided pointer to the IR code data.

**Function Signature:**
```cpp
size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr)
```

**Parameters:**
- `mode`: Mode index (0 to modes-1)
- `fan_speed`: Fan speed index (0 to fan_speeds-1)
- `temperature`: Temperature offset from min_temperature (0 = min_temperature)
- `codePtr`: Pointer to set to the IR code array

**Returns:** Number of elements in the IR code array

---

## Defining IR Codes

IR codes are stored as arrays of timing values in microseconds, alternating between ON and OFF states.

### Storage Structure

The recommended structure is a multi-dimensional array indexed by [mode][fan_speed][temperature]:

```cpp
// Example for AC with 3 modes, 4 fan speeds, and 15 temperature settings (16°C-30°C)
const uint16_t irCodes[3][4][15][200] = {
    // Mode 0 (Off)
    {
        // Fan speed 0 (Auto)
        {
            {/* IR code for Off, Auto, 16°C */},
            {/* IR code for Off, Auto, 17°C */},
            // ... more temperatures
        },
        // ... more fan speeds
    },
    // Mode 1 (Cool)
    {
        // Fan speed 0 (Auto)
        {
            {9000, 4500, 560, 560, 560, 1690, /* ... more timing values ... */},  // 16°C
            {9000, 4500, 560, 560, 560, 1690, /* ... more timing values ... */},  // 17°C
            // ... more temperatures
        },
        // More fan speeds...
    },
    // More modes...
};
```

### getInfraredCode Implementation

```cpp
size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr) {
    // Set the pointer to the appropriate IR code array
    *codePtr = irCodes[mode][fan_speed][temperature];

    // Return the number of elements in the array
    return sizeof(irCodes[mode][fan_speed][temperature]) / sizeof(uint16_t);
}
```

### Alternative: Dynamic Code Generation

For some AC models with predictable patterns, you can generate codes dynamically:

```cpp
// Static buffer to hold generated code
static uint16_t generatedCode[200];

size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr) {
    // Generate IR code based on parameters
    size_t index = 0;

    // Header
    generatedCode[index++] = 9000;
    generatedCode[index++] = 4500;

    // Encode mode, fan_speed, temperature into IR pulses
    // ... your encoding logic here ...

    *codePtr = generatedCode;
    return index;
}
```

### Capturing IR Codes

Use the `IRReceiver` class to capture IR codes from your existing AC remote:

```cpp
#include <IRReceiver.hpp>

// Start receiving
IRReceiver::begin(IR_RX_PIN);
IRReceiver::start_long_receive();

// Wait for button press
delay(5000);

// Stop and get data
ir_data_t received = IRReceiver::end_long_receive();

// Print the code
for (size_t i = 0; i < received.size; i++) {
    Serial.print(received.data[i]);
    Serial.print(", ");
}

// Free memory when done
free(received.data);
```

See the `ir_code_capture.ino` example for a complete implementation.

---

## Supported Sensor Types

The ClimateCard supports three sensor configurations:

### AC_SENSOR_TYPE_NONE (0x00)
No sensor connected. Use this when you only need AC control without environmental monitoring.

**Constructor:**
```cpp
ClimateCard card(IR_PIN, airConditioner, RMT_CHANNEL_0);
```

### AC_SENSOR_TYPE_DHT22 (0x01)
DHT22 sensor provides both temperature and humidity readings.

**Features:**
- Temperature range: -40°C to 80°C (±0.5°C accuracy)
- Humidity range: 0-100% RH (±2% accuracy)
- Readings updated every 5 seconds (AC_SENSOR_READ_INTERVAL)

**Constructor:**
```cpp
ClimateCard card(IR_PIN, airConditioner, AC_SENSOR_TYPE_DHT22, SENSOR_PIN, RMT_CHANNEL_0);
```

**Reading Values:**
```cpp
float temperature = card.getRoomTemperature();  // Returns °C
float humidity = card.getHumidity();            // Returns % RH
```

### AC_SENSOR_TYPE_DS18B20 (0x02)
DS18B20 sensor provides temperature-only readings.

**Features:**
- Temperature range: -55°C to 125°C (±0.5°C accuracy)
- Digital 1-Wire interface
- No humidity reading (getHumidity() returns 0)
- Readings updated every 5 seconds
- 250ms timeout for conversion

**Constructor:**
```cpp
ClimateCard card(IR_PIN, airConditioner, AC_SENSOR_TYPE_DS18B20, SENSOR_PIN, RMT_CHANNEL_0);
```

**Reading Values:**
```cpp
float temperature = card.getRoomTemperature();  // Returns °C
float humidity = card.getHumidity();            // Returns 0 (not supported)
```

### Sensor Reading Interval
- Default interval: 5000ms (5 seconds) - `AC_SENSOR_READ_INTERVAL`
- Conversion timeout (DS18B20): 250ms - `AC_SENSOR_READ_TIMEOUT`
- Automatic updates in `loop()` function

---

## Complete API Reference

### Constructor

#### ClimateCard(uint8_t ir_pin, AirConditioner ac, uint8_t sensor_type, uint8_t sensor_pin, rmt_channel_t channel)
Create a ClimateCard with environmental sensor.

**Parameters:**
- `ir_pin`: GPIO pin connected to IR LED
- `ac`: AirConditioner struct defining AC characteristics
- `sensor_type`: Sensor type (AC_SENSOR_TYPE_DHT22, AC_SENSOR_TYPE_DS18B20, or AC_SENSOR_TYPE_NONE)
- `sensor_pin`: GPIO pin connected to sensor
- `channel`: RMT channel for IR transmission (must be unique)

**Example:**
```cpp
ClimateCard card(14, myAC, AC_SENSOR_TYPE_DHT22, 27, RMT_CHANNEL_0);
```

#### ClimateCard(uint8_t ir_pin, AirConditioner ac, rmt_channel_t channel)
Create a ClimateCard without environmental sensor.

**Parameters:**
- `ir_pin`: GPIO pin connected to IR LED
- `ac`: AirConditioner struct defining AC characteristics
- `channel`: RMT channel for IR transmission

**Example:**
```cpp
ClimateCard card(14, myAC, RMT_CHANNEL_0);
```

---

### Initialization

#### bool begin()
Initialize the ClimateCard and sensor (if present).

**Returns:** `true` if initialization successful, `false` otherwise

**Example:**
```cpp
if (!card.begin()) {
    Serial.println("ClimateCard initialization failed!");
}
```

**Note:** This must be called in `setup()` before using the card.

#### void loop()
Update sensor readings at regular intervals.

**Example:**
```cpp
void loop() {
    card.loop();  // Call this regularly to update sensor data
}
```

**Note:** When installed in ESPMegaPRO, this is called automatically.

---

### AC State Control

#### void setTemperature(uint8_t temperature)
Set the air conditioner target temperature.

**Parameters:**
- `temperature`: Target temperature in °C

**Behavior:**
- Values above `max_temperature` are clamped to maximum
- Values below `min_temperature` are clamped to minimum
- Sends IR command immediately
- Triggers state change callbacks
- Saves to FRAM if auto-save enabled

**Example:**
```cpp
card.setTemperature(24);  // Set to 24°C
```

#### uint8_t getTemperature()
Get the current AC target temperature.

**Returns:** Current temperature setting in °C

**Example:**
```cpp
uint8_t currentTemp = card.getTemperature();
Serial.printf("AC set to %d°C\n", currentTemp);
```

#### void setMode(uint8_t mode)
Set the air conditioner operating mode.

**Parameters:**
- `mode`: Mode index (0 to ac.modes-1)

**Behavior:**
- Invalid values (>= ac.modes) are set to 0
- Sends IR command immediately
- Triggers state change callbacks
- Saves to FRAM if auto-save enabled

**Example:**
```cpp
card.setMode(1);  // Set to mode 1 (e.g., "cool")
```

#### void setModeByName(const char* mode_name)
Set mode by name string.

**Parameters:**
- `mode_name`: Name of the mode (must match a name in ac.mode_names)

**Behavior:**
- Case-sensitive string comparison
- No action if name not found
- Otherwise same as setMode()

**Example:**
```cpp
card.setModeByName("cool");  // Set to cooling mode
```

#### uint8_t getMode()
Get the current mode index.

**Returns:** Current mode index (0 to ac.modes-1)

**Example:**
```cpp
uint8_t mode = card.getMode();
```

#### char* getModeName()
Get the current mode name.

**Returns:** Pointer to mode name string

**Example:**
```cpp
Serial.printf("Current mode: %s\n", card.getModeName());
```

#### void setFanSpeed(uint8_t fan_speed)
Set the fan speed.

**Parameters:**
- `fan_speed`: Fan speed index (0 to ac.fan_speeds-1)

**Behavior:**
- Invalid values (>= ac.fan_speeds) are set to 0
- Sends IR command immediately
- Triggers state change callbacks
- Saves to FRAM if auto-save enabled

**Example:**
```cpp
card.setFanSpeed(2);  // Set to fan speed 2 (e.g., "medium")
```

#### void setFanSpeedByName(const char* fan_speed_name)
Set fan speed by name string.

**Parameters:**
- `fan_speed_name`: Name of fan speed (must match a name in ac.fan_speed_names)

**Behavior:**
- Case-sensitive string comparison
- No action if name not found
- Otherwise same as setFanSpeed()

**Example:**
```cpp
card.setFanSpeedByName("high");  // Set to high fan speed
```

#### uint8_t getFanSpeed()
Get the current fan speed index.

**Returns:** Current fan speed index (0 to ac.fan_speeds-1)

**Example:**
```cpp
uint8_t speed = card.getFanSpeed();
```

#### char* getFanSpeedName()
Get the current fan speed name.

**Returns:** Pointer to fan speed name string

**Example:**
```cpp
Serial.printf("Fan speed: %s\n", card.getFanSpeedName());
```

#### void setState(uint8_t mode, uint8_t fan_speed, uint8_t temperature)
Set all AC parameters at once.

**Parameters:**
- `mode`: Mode index
- `fan_speed`: Fan speed index
- `temperature`: Temperature in °C

**Behavior:**
- No validation (make sure values are in range!)
- Sends IR command once with all parameters
- Triggers state change callbacks once
- Saves to FRAM if auto-save enabled

**Example:**
```cpp
card.setState(1, 2, 24);  // Cool mode, medium fan, 24°C
```

**Warning:** Values are not validated! Use setTemperature(), setMode(), and setFanSpeed() individually if you need validation.

---

### Sensor Reading

#### float getRoomTemperature()
Get the current room temperature from sensor.

**Returns:** Temperature in °C, or 0 if no sensor or no reading yet

**Example:**
```cpp
float roomTemp = card.getRoomTemperature();
Serial.printf("Room: %.1f°C\n", roomTemp);
```

**Note:** Updated automatically every 5 seconds by `loop()`.

#### float getHumidity()
Get the current humidity from sensor.

**Returns:** Relative humidity in %, or 0 if DHT22 not used

**Example:**
```cpp
if (card.getSensorType() == AC_SENSOR_TYPE_DHT22) {
    float humidity = card.getHumidity();
    Serial.printf("Humidity: %.1f%%\n", humidity);
}
```

**Note:** Only DHT22 provides humidity. DS18B20 and NONE always return 0.

#### uint8_t getSensorType()
Get the type of sensor connected.

**Returns:**
- `AC_SENSOR_TYPE_NONE` (0x00)
- `AC_SENSOR_TYPE_DHT22` (0x01)
- `AC_SENSOR_TYPE_DS18B20` (0x02)

**Example:**
```cpp
switch (card.getSensorType()) {
    case AC_SENSOR_TYPE_DHT22:
        Serial.println("DHT22 sensor");
        break;
    case AC_SENSOR_TYPE_DS18B20:
        Serial.println("DS18B20 sensor");
        break;
    default:
        Serial.println("No sensor");
        break;
}
```

---

### FRAM Persistence

The ClimateCard can save its state to FRAM (Ferroelectric RAM) memory for persistence across power cycles.

**FRAM Usage:** 3 bytes
- Byte 0: Temperature
- Byte 1: Mode
- Byte 2: Fan Speed

#### void bindFRAM(FRAM* fram, uint16_t fram_address)
Bind FRAM memory to the ClimateCard.

**Parameters:**
- `fram`: Pointer to FRAM object
- `fram_address`: Starting address in FRAM (requires 3 consecutive bytes)

**Example:**
```cpp
card.bindFRAM(&espmega.fram, 1001);  // Use FRAM starting at address 1001
```

**Note:** Must be called before loadStateFromFRAM() or saveStateToFRAM().

#### void loadStateFromFRAM()
Load AC state from FRAM memory.

**Behavior:**
- Reads 3 bytes from FRAM at configured address
- Validates values against AC limits
- Out-of-range values are clamped or set to 0
- Sends IR command with loaded state
- Triggers state change callbacks

**Example:**
```cpp
card.bindFRAM(&fram, 1001);
card.loadStateFromFRAM();  // Restore previous state
```

**Note:** Safe to call with uninitialized FRAM - values will be validated.

#### void saveStateToFRAM()
Save current AC state to FRAM memory.

**Example:**
```cpp
card.saveStateToFRAM();  // Manually save state
```

**Note:** No effect if bindFRAM() not called.

#### void setFRAMAutoSave(bool autoSave)
Enable or disable automatic FRAM saving.

**Parameters:**
- `autoSave`: `true` to auto-save on every state change, `false` to disable

**Behavior:**
- When enabled, setTemperature(), setMode(), setFanSpeed(), and setState() automatically save to FRAM
- When disabled, you must call saveStateToFRAM() manually

**Example:**
```cpp
card.setFRAMAutoSave(true);   // Auto-save enabled
card.setTemperature(25);      // Automatically saved to FRAM

card.setFRAMAutoSave(false);  // Auto-save disabled
card.setTemperature(26);      // NOT saved to FRAM
card.saveStateToFRAM();       // Manual save required
```

**Recommended:** Enable auto-save for most applications to ensure state is always persisted.

---

### Callback Systems

The ClimateCard provides two callback systems for monitoring changes.

#### uint8_t registerChangeCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback)
Register a callback for AC state changes.

**Parameters:**
- `callback`: Function with signature `void callback(uint8_t mode, uint8_t fan_speed, uint8_t temperature)`

**Returns:** Handler ID for later unregistration

**Trigger Conditions:**
- setTemperature() called
- setMode() called
- setFanSpeed() called
- setState() called
- loadStateFromFRAM() called

**Example:**
```cpp
void onACStateChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.printf("AC changed: mode=%d, fan=%d, temp=%d\n", mode, fan_speed, temperature);
}

void setup() {
    uint8_t handler = card.registerChangeCallback(onACStateChange);
}
```

**Lambda Example:**
```cpp
card.registerChangeCallback([](uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.printf("New temp: %d°C\n", temperature);
});
```

#### void unregisterChangeCallback(uint8_t handler)
Remove a previously registered state change callback.

**Parameters:**
- `handler`: Handler ID returned by registerChangeCallback()

**Example:**
```cpp
uint8_t handler = card.registerChangeCallback(myCallback);
// ... later ...
card.unregisterChangeCallback(handler);
```

#### uint8_t registerSensorCallback(std::function<void(float, float)> callback)
Register a callback for sensor readings.

**Parameters:**
- `callback`: Function with signature `void callback(float temperature, float humidity)`

**Returns:** Handler ID for later unregistration

**Trigger Conditions:**
- Sensor data updated (every 5 seconds in loop())
- Temperature parameter is always valid
- Humidity parameter is 0 if sensor is not DHT22

**Example:**
```cpp
void onSensorUpdate(float temperature, float humidity) {
    Serial.printf("Sensor: %.1f°C, %.1f%% RH\n", temperature, humidity);
}

void setup() {
    uint8_t handler = card.registerSensorCallback(onSensorUpdate);
}
```

#### void unregisterSensorCallback(uint8_t handler)
Remove a previously registered sensor callback.

**Parameters:**
- `handler`: Handler ID returned by registerSensorCallback()

**Example:**
```cpp
uint8_t handler = card.registerSensorCallback(mySensorCallback);
// ... later ...
card.unregisterSensorCallback(handler);
```

---

### Utility

#### uint8_t getType()
Get the expansion card type identifier.

**Returns:** `CARD_TYPE_CLIMATE` (0x03)

**Example:**
```cpp
if (card.getType() == CARD_TYPE_CLIMATE) {
    Serial.println("This is a Climate Card");
}
```

**Note:** Used by ESPMegaPRO system for card identification.

---

## IoT Integration (MQTT)

The `ClimateIoT` class provides MQTT integration for the ClimateCard, enabling remote monitoring and control.

### MQTT Topics

All topics are relative to the card's base topic: `<base_topic>/card/<card_id>/`

#### Subscribe Topics (Control)

| Topic | Payload | Description |
|-------|---------|-------------|
| `set/temperature` | Integer (e.g., "24") | Set AC temperature |
| `set/mode` | String (e.g., "cool") | Set AC mode by name |
| `set/fan_speed` | String (e.g., "high") | Set fan speed by name |
| `requeststate` | Any | Request state report |

**Examples:**
```
<base_topic>/card/2/set/temperature → "24"
<base_topic>/card/2/set/mode → "cool"
<base_topic>/card/2/set/fan_speed → "auto"
<base_topic>/card/2/requeststate → ""
```

#### Publish Topics (Status)

| Topic | Payload | Description |
|-------|---------|-------------|
| `temperature` | Integer | Current AC temperature setting |
| `mode` | String | Current AC mode name |
| `fan_speed` | String | Current fan speed name |
| `room_temperature` | Integer | Room temperature from sensor |
| `humidity` | Integer | Room humidity (DHT22 only) |

**Examples:**
```
<base_topic>/card/2/temperature → "24"
<base_topic>/card/2/mode → "cool"
<base_topic>/card/2/fan_speed → "medium"
<base_topic>/card/2/room_temperature → "26"
<base_topic>/card/2/humidity → "65"
```

### Publishing Behavior

- **Automatic Publishing:**
  - AC state changes trigger immediate publish
  - Sensor readings trigger publish every 5 seconds

- **Manual Publishing:**
  - Send any payload to `requeststate` topic
  - Triggers immediate publish of all topics

### ESPMegaPRO Integration

When using ESPMegaPRO, the IoT integration is automatic:

```cpp
#include <ESPMegaProOS.hpp>
#include <ClimateCard.hpp>

ESPMegaPRO espmega;
ClimateCard climateCard(14, myAC, AC_SENSOR_TYPE_DHT22, 27, RMT_CHANNEL_0);

void setup() {
    espmega.begin();
    espmega.enableIotModule();

    // ... network and MQTT setup ...

    // Install card at slot 2
    espmega.installCard(2, &climateCard);

    // Register for MQTT (creates ClimateIoT internally)
    espmega.iot->registerCard(2);
}
```

### ClimateIoT API Reference

**Note:** You typically don't instantiate ClimateIoT directly. The ESPMegaIoT system creates it via `registerCard()`.

#### bool begin(uint8_t card_id, ExpansionCard* card, PubSubClient* mqtt, char* base_topic)
Initialize the IoT component.

**Parameters:**
- `card_id`: Card slot number
- `card`: Pointer to ClimateCard
- `mqtt`: Pointer to MQTT client
- `base_topic`: Base MQTT topic

**Returns:** `true` on success

#### void subscribe()
Subscribe to all control topics.

#### void publishReport()
Publish all status topics immediately.

#### void publishClimate()
Publish AC state (temperature, mode, fan_speed).

#### void publishSensor()
Publish sensor data (room_temperature, humidity).

---

## Complete Code Examples

### Example 1: Basic AC Control

```cpp
#include <ClimateCard.hpp>

// Define your AC's IR codes
// This is a simplified example - real codes will be much longer
const uint16_t irCodes[3][4][15][100] = {
    // You would fill this with your AC's actual IR timing data
    // Format: [mode][fan_speed][temperature][timing_values]
};

// Mode names: off, cool, fan_only
const char *mode_names[] = {"off", "cool", "fan_only"};

// Fan speed names: auto, low, medium, high
const char *fan_speed_names[] = {"auto", "low", "medium", "high"};

// Function to get IR code for a specific state
size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t** codePtr) {
    *codePtr = irCodes[mode][fan_speed][temperature];
    return sizeof(irCodes[mode][fan_speed][temperature]) / sizeof(uint16_t);
}

// Define AC characteristics
AirConditioner myAC = {
    .max_temperature = 30,
    .min_temperature = 16,
    .modes = 3,
    .mode_names = mode_names,
    .fan_speeds = 4,
    .fan_speed_names = fan_speed_names,
    .getInfraredCode = getInfraredCode
};

// Create ClimateCard (IR on GPIO 14, no sensor)
ClimateCard ac(14, myAC, RMT_CHANNEL_0);

void setup() {
    Serial.begin(115200);

    // Initialize the card
    if (!ac.begin()) {
        Serial.println("Failed to initialize ClimateCard!");
        return;
    }

    Serial.println("ClimateCard initialized");

    // Set initial state
    ac.setMode(1);           // Cool mode
    ac.setFanSpeed(0);       // Auto fan
    ac.setTemperature(24);   // 24°C

    Serial.println("AC set to: Cool, Auto fan, 24°C");
}

void loop() {
    // Update the card
    ac.loop();

    // Example: Change temperature every 30 seconds
    static unsigned long lastChange = 0;
    if (millis() - lastChange > 30000) {
        lastChange = millis();

        uint8_t temp = ac.getTemperature();
        temp = (temp >= 30) ? 20 : temp + 1;
        ac.setTemperature(temp);

        Serial.printf("Temperature changed to %d°C\n", temp);
    }
}
```

### Example 2: With DHT22 Sensor

```cpp
#include <ClimateCard.hpp>

// ... (AC definition same as Example 1) ...

// Create ClimateCard with DHT22 sensor on GPIO 27
ClimateCard ac(14, myAC, AC_SENSOR_TYPE_DHT22, 27, RMT_CHANNEL_0);

// Callback for sensor readings
void onSensorUpdate(float temperature, float humidity) {
    Serial.printf("Room: %.1f°C, %.1f%% RH\n", temperature, humidity);

    // Optional: Adjust AC based on room conditions
    if (temperature > 28.0) {
        Serial.println("Room too hot! Setting AC to 22°C");
        ac.setTemperature(22);
    }
}

// Callback for AC state changes
void onACChange(uint8_t mode, uint8_t fan_speed, uint8_t temperature) {
    Serial.printf("AC State: mode=%s, fan=%s, temp=%d°C\n",
                  ac.getModeName(), ac.getFanSpeedName(), temperature);
}

void setup() {
    Serial.begin(115200);

    if (!ac.begin()) {
        Serial.println("Failed to initialize ClimateCard!");
        return;
    }

    // Register callbacks
    ac.registerSensorCallback(onSensorUpdate);
    ac.registerChangeCallback(onACChange);

    // Set initial state
    ac.setState(1, 0, 24);  // Cool, Auto, 24°C

    Serial.println("ClimateCard with DHT22 initialized");
}

void loop() {
    ac.loop();  // Updates sensor every 5 seconds
}
```

### Example 3: Thermostat Mode

```cpp
#include <ClimateCard.hpp>

// ... (AC definition same as Example 1) ...

ClimateCard ac(14, myAC, AC_SENSOR_TYPE_DHT22, 27, RMT_CHANNEL_0);

// Thermostat settings
float targetTemperature = 24.0;
float hysteresis = 1.0;  // +/- 1°C deadband
bool acRunning = false;

void thermostatControl() {
    float roomTemp = ac.getRoomTemperature();

    // Skip if no valid reading yet
    if (roomTemp == 0) return;

    // Check if we need to turn AC on or off
    if (!acRunning && roomTemp > targetTemperature + hysteresis) {
        // Room too hot, turn on AC
        Serial.printf("Room %.1f°C > Target %.1f°C - Turning AC ON\n",
                      roomTemp, targetTemperature);
        ac.setMode(1);  // Cool mode
        ac.setFanSpeed(0);  // Auto fan
        ac.setTemperature((uint8_t)targetTemperature);
        acRunning = true;
    }
    else if (acRunning && roomTemp < targetTemperature - hysteresis) {
        // Room cool enough, turn off AC
        Serial.printf("Room %.1f°C < Target %.1f°C - Turning AC OFF\n",
                      roomTemp, targetTemperature);
        ac.setMode(0);  // Off mode
        acRunning = false;
    }
}

void setup() {
    Serial.begin(115200);

    if (!ac.begin()) {
        Serial.println("Failed to initialize!");
        return;
    }

    Serial.printf("Thermostat mode: Target %.1f°C +/- %.1f°C\n",
                  targetTemperature, hysteresis);

    // Start with AC off
    ac.setMode(0);
}

void loop() {
    ac.loop();

    // Run thermostat logic every 10 seconds
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {
        lastCheck = millis();
        thermostatControl();
    }
}
```

### Example 4: MQTT Control (ESPMegaPRO)

```cpp
#include <ESPMegaProOS.hpp>
#include <ClimateCard.hpp>
#include <ETH.h>

// ... (AC definition same as Example 1) ...

ESPMegaPRO espmega;
ClimateCard climateCard(14, myAC, AC_SENSOR_TYPE_DHT22, 27, RMT_CHANNEL_0);

void setup() {
    Serial.begin(115200);

    // Initialize ESPMega
    espmega.begin();

    // Enable and configure IoT module
    espmega.enableIotModule();

    // Start Ethernet
    ETH.begin();
    espmega.iot->bindEthernetInterface(&ETH);

    // Configure network (or load from FRAM)
    NetworkConfig netConfig = {
        .ip = {192, 168, 1, 100},
        .gateway = {192, 168, 1, 1},
        .subnet = {255, 255, 255, 0},
        .dns1 = {8, 8, 8, 8},
        .dns2 = {8, 8, 4, 4},
        .useStaticIp = true,
        .useWifi = false
    };
    strcpy(netConfig.hostname, "espmega-climate");
    espmega.iot->setNetworkConfig(netConfig);
    espmega.iot->connectNetwork();

    // Configure MQTT (or load from FRAM)
    MqttConfig mqttConfig = {
        .mqtt_port = 1883,
        .mqtt_useauth = false
    };
    strcpy(mqttConfig.mqtt_server, "192.168.1.10");
    strcpy(mqttConfig.base_topic, "home/espmega");
    espmega.iot->setMqttConfig(mqttConfig);
    espmega.iot->connectToMqtt();

    // Install ClimateCard at slot 2
    espmega.installCard(2, &climateCard);

    // Bind FRAM for state persistence
    climateCard.bindFRAM(&espmega.fram, 1001);
    climateCard.loadStateFromFRAM();
    climateCard.setFRAMAutoSave(true);

    // Register card for MQTT integration
    espmega.iot->registerCard(2);

    Serial.println("MQTT Climate Control Ready");
    Serial.println("Topics:");
    Serial.println("  home/espmega/card/2/set/temperature");
    Serial.println("  home/espmega/card/2/set/mode");
    Serial.println("  home/espmega/card/2/set/fan_speed");
}

void loop() {
    espmega.loop();
}

/*
MQTT Control Examples:

Set temperature:
  mosquitto_pub -t "home/espmega/card/2/set/temperature" -m "24"

Set mode:
  mosquitto_pub -t "home/espmega/card/2/set/mode" -m "cool"

Set fan speed:
  mosquitto_pub -t "home/espmega/card/2/set/fan_speed" -m "high"

Request state:
  mosquitto_pub -t "home/espmega/card/2/requeststate" -m ""

Subscribe to status:
  mosquitto_sub -t "home/espmega/card/2/#"
*/
```

### Example 5: IR Code Capture

```cpp
#include <Arduino.h>
#include <IRReceiver.hpp>

#define IR_RX_PIN 15  // GPIO pin for IR receiver

void setup() {
    Serial.begin(115200);
    Serial.println("IR Code Capture Utility");
    Serial.println("======================");
    Serial.println();

    // Initialize IR receiver
    IRReceiver::begin(IR_RX_PIN);

    Serial.println("IR Receiver ready on GPIO " + String(IR_RX_PIN));
    Serial.println();
    Serial.println("Instructions:");
    Serial.println("1. Type 'start' to begin capturing");
    Serial.println("2. Point remote at receiver and press button");
    Serial.println("3. Type 'stop' to finish capturing");
    Serial.println("4. Code will be printed in C array format");
    Serial.println();
}

void loop() {
    // Check for serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();

        if (command == "start") {
            Serial.println("\n>>> Starting IR capture...");
            Serial.println(">>> Press the remote button NOW!");
            IRReceiver::start_long_receive();
            Serial.println(">>> Capturing... (type 'stop' when done)");
        }
        else if (command == "stop") {
            Serial.println("\n>>> Stopping capture...");
            ir_data_t received = IRReceiver::end_long_receive();

            if (received.size == 0) {
                Serial.println(">>> ERROR: No data captured!");
            } else {
                Serial.println(">>> Capture complete!");
                Serial.println();
                printIRCode(received);

                // Free allocated memory
                free(received.data);
            }

            Serial.println("\n>>> Type 'start' to capture another code");
        }
        else {
            Serial.println("Unknown command. Use 'start' or 'stop'.");
        }
    }
}

void printIRCode(ir_data_t &data) {
    Serial.println("// Captured IR Code");
    Serial.printf("// Total elements: %d\n", data.size);
    Serial.println("const uint16_t irCode[] = {");
    Serial.print("    ");

    for (size_t i = 0; i < data.size; i++) {
        Serial.print(data.data[i]);
        if (i < data.size - 1) {
            Serial.print(", ");
        }

        // New line every 10 values for readability
        if ((i + 1) % 10 == 0 && i < data.size - 1) {
            Serial.println();
            Serial.print("    ");
        }
    }

    Serial.println();
    Serial.println("};");
    Serial.println();
    Serial.printf("Size: %d elements\n", data.size);
    Serial.printf("Bytes: %d\n", data.size * sizeof(uint16_t));
}

/*
Example Usage Session:

>>> Starting IR capture...
>>> Press the remote button NOW!
>>> Capturing... (type 'stop' when done)
stop
>>> Stopping capture...
>>> Capture complete!

// Captured IR Code
// Total elements: 67
const uint16_t irCode[] = {
    9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 560, 560, 1690, 560, 560, 560, 560, 560, 1690,
    // ... more values ...
};

Size: 67 elements
Bytes: 134

>>> Type 'start' to capture another code
*/
```

---

## Tips and Best Practices

### IR LED Range
- Use a transistor driver for better range (up to 10 meters)
- Multiple IR LEDs in parallel increase range
- Aim IR LED directly at AC receiver
- Test range before permanent installation

### IR Code Organization
- Capture and test codes systematically (all combinations)
- Document which remote button produces each code
- Store codes in separate header file for cleaner code
- Use consistent temperature stepping (usually 1°C)

### FRAM Usage
- Always enable auto-save for reliable state persistence
- Choose FRAM address carefully to avoid conflicts
- ClimateCard uses 3 consecutive bytes
- Document your FRAM memory map

### Sensor Reliability
- DHT22: Wait at least 2 seconds between reads (library handles this)
- DS18B20: Use 4.7kΩ pullup resistor on data line
- Check for invalid readings (0 or NaN) before using
- Average multiple readings for critical applications

### MQTT Integration
- Use QoS 1 for control commands
- Implement reconnection logic for reliability
- Log state changes for debugging
- Consider using retained messages for status topics

### Power Considerations
- IR LED can draw significant current (50-100mA)
- Use external power supply for multiple IR LEDs
- Add decoupling capacitors near IR LED
- Consider power consumption in battery applications

### Debugging
- Enable serial logging for state changes
- Monitor MQTT traffic with mosquitto_sub
- Use IR receiver to verify transmitted codes
- Check RMT channel conflicts if IR not working

---

## Troubleshooting

### AC Not Responding to Commands

**Check:**
1. IR LED polarity (anode to current source)
2. IR LED is working (view with phone camera - you should see it light up)
3. RMT channel is unique (no conflicts)
4. IR codes are correct for your AC model
5. AC receiver has clear line of sight to IR LED

### Sensor Not Reading

**DHT22:**
- Check wiring (VCC, GND, Data)
- Verify GPIO pin number
- DHT22 needs 2+ second intervals
- Try different GPIO pin

**DS18B20:**
- Check 4.7kΩ pullup resistor
- Verify OneWire GPIO pin
- Check sensor power supply
- Try reading sensor directly without ClimateCard

### FRAM Not Persisting

**Check:**
1. bindFRAM() called before load/save
2. Auto-save enabled or manual save called
3. FRAM address not conflicting with other data
4. FRAM module properly connected to I2C bus

### MQTT Not Working

**Check:**
1. Network connection established
2. MQTT broker IP and port correct
3. Card registered with correct ID
4. Subscribe to '#' wildcard to see all messages
5. Check MQTT broker logs

---

## Hardware Specifications

### ClimateCard
- **IR Frequency:** 38 kHz (configurable in IRBlaster)
- **IR Timing Resolution:** 1 microsecond
- **Max IR Code Length:** Limited by RAM (typically ~1000 elements)
- **Sensor Update Rate:** 5 seconds (configurable via AC_SENSOR_READ_INTERVAL)
- **FRAM Usage:** 3 bytes

### Supported Sensors
- **DHT22:**
  - Temperature: -40°C to 80°C (±0.5°C)
  - Humidity: 0-100% RH (±2%)
  - Update rate: Maximum 0.5 Hz (2 seconds)

- **DS18B20:**
  - Temperature: -55°C to 125°C (±0.5°C)
  - Resolution: 9-12 bit (configurable)
  - Conversion time: 750ms (12-bit)

### RMT Peripheral
- **Channels:** 8 total (ESP32)
- **Resolution:** 1 microsecond (with clk_div=80)
- **Max Items:** Limited by available RAM
- **Carrier Frequency:** 38 kHz (configurable)

---

## Additional Resources

- **ESP32 RMT Documentation:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/rmt.html
- **DHT22 Datasheet:** Search for "AM2302" or "DHT22" datasheet
- **DS18B20 Datasheet:** Maxim Integrated DS18B20 datasheet
- **MQTT Protocol:** https://mqtt.org/

---

## Version History

- **v2.10.0:** Current version with full ClimateCard support
- State persistence via FRAM
- IoT/MQTT integration
- Multiple sensor support

---

## License

This documentation is part of the ESPMegaPRO3-library project.

---

## Support

For issues, questions, or contributions, please refer to the main ESPMegaPRO repository.
