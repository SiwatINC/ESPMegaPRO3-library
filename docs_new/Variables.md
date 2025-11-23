# Variables Documentation

This document provides comprehensive documentation for the SmartVariable and RemoteVariable classes, which enable local and remote variable management with MQTT integration in the ESPMegaPRO3 ecosystem.

## Table of Contents

1. [SmartVariable Class](#smartvariable-class)
   - [Overview](#smartvariable-overview)
   - [Public Methods](#smartvariable-public-methods)
   - [Use Cases](#smartvariable-use-cases)
   - [How It Works Internally](#smartvariable-internals)
   - [Code Examples](#smartvariable-examples)

2. [RemoteVariable Class](#remotevariable-class)
   - [Overview](#remotevariable-overview)
   - [Public Methods](#remotevariable-public-methods)
   - [MQTT Integration](#remotevariable-mqtt-integration)
   - [Use Cases](#remotevariable-use-cases)
   - [Code Examples](#remotevariable-examples)

3. [Complete Code Examples](#complete-code-examples)
   - [Basic SmartVariable Usage](#basic-smartvariable-usage)
   - [RemoteVariable with MQTT](#remotevariable-with-mqtt)
   - [Integration with IoT System](#integration-with-iot-system)
   - [Practical Applications](#practical-applications)

---

## SmartVariable Class

### SmartVariable Overview

The `SmartVariable` class provides a powerful abstraction for managing **local variables** that can be:
- **Accessed remotely** via MQTT
- **Persisted** to FRAM (Ferroelectric RAM) for non-volatile storage
- **Synchronized** across IoT networks automatically
- **Monitored** through callback mechanisms

SmartVariable is ideal for configuration values, system settings, sensor readings, or any data that needs to be both locally accessible and remotely controllable while maintaining persistence across power cycles.

**Key Features:**
- Automatic MQTT publishing on value changes
- Non-volatile storage with FRAM support
- Type-safe getters/setters for common data types (int, double, string)
- Callback system for value change notifications
- Optional auto-save to FRAM
- Remote value requests via MQTT
- Remote value setting via MQTT

**Header File:** `/home/user/ESPMegaPRO3-library/SmartVariable.hpp`
**Source File:** `/home/user/ESPMegaPRO3-library/SmartVariable.cpp`

---

### SmartVariable Public Methods

#### Constructor and Destructor

##### `SmartVariable()`
```cpp
SmartVariable();
```
**Description:** Default constructor. Creates an uninitialized SmartVariable object.

**Parameters:** None

**Returns:** SmartVariable object

**Usage:**
```cpp
SmartVariable myVariable;
```

---

##### `~SmartVariable()`
```cpp
~SmartVariable();
```
**Description:** Destructor. Automatically frees allocated memory for the variable value.

**Parameters:** None

**Returns:** None

---

#### Initialization

##### `begin()`
```cpp
void begin(size_t size);
```
**Description:** Initializes the SmartVariable with a specified maximum size. Must be called before using the variable.

**Parameters:**
- `size` (size_t): Maximum size of the variable in bytes, including null terminator

**Returns:** void

**Usage:**
```cpp
SmartVariable temperature;
temperature.begin(16); // Allocates 16 bytes for the variable
```

**Note:** The size should account for the null terminator. For a 15-character string, allocate 16 bytes.

---

#### IoT Integration

##### `enableIoT()`
```cpp
void enableIoT(ESPMegaIoT* iot, const char* topic);
```
**Description:** Enables MQTT integration for the SmartVariable. Automatically publishes value changes and handles remote requests.

**Parameters:**
- `iot` (ESPMegaIoT*): Pointer to the ESPMegaIoT instance
- `topic` (const char*): MQTT topic where the variable value will be published

**Returns:** void

**Behavior:**
- Registers MQTT callback handlers
- Subscribes to relevant topics
- Publishes current value immediately

**Usage:**
```cpp
ESPMegaIoT iot;
SmartVariable sensorValue;
sensorValue.begin(32);
sensorValue.enableIoT(&iot, "home/sensor/temperature");
```

---

##### `enableValueRequest()`
```cpp
void enableValueRequest(const char* valueRequestTopic);
```
**Description:** Enables remote value requests. Other devices can request the current value by publishing to the specified topic.

**Parameters:**
- `valueRequestTopic` (const char*): MQTT topic to subscribe to for value requests

**Returns:** void

**Behavior:**
- When a message is received on `valueRequestTopic`, the current value is published to the main topic

**Usage:**
```cpp
sensorValue.enableValueRequest("home/sensor/temperature/get");
```

**MQTT Flow:**
```
Remote Device → "home/sensor/temperature/get" (any payload)
This Device → "home/sensor/temperature" (current value)
```

---

##### `enableSetValue()`
```cpp
void enableSetValue(const char* setValueTopic);
```
**Description:** Enables remote value setting. Other devices can change the variable's value via MQTT.

**Parameters:**
- `setValueTopic` (const char*): MQTT topic to subscribe to for value updates

**Returns:** void

**Behavior:**
- Subscribes to the specified topic
- When a message is received, updates the variable value
- Triggers callbacks and auto-save if enabled

**Usage:**
```cpp
sensorValue.enableSetValue("home/sensor/temperature/set");
```

**MQTT Flow:**
```
Remote Device → "home/sensor/temperature/set" ("25.5")
This Device → Updates value to "25.5"
This Device → "home/sensor/temperature" ("25.5") [auto-publish]
```

---

##### `publishValue()`
```cpp
void publishValue();
```
**Description:** Manually publishes the current value to the MQTT topic.

**Parameters:** None

**Returns:** void

**Usage:**
```cpp
sensorValue.publishValue(); // Publishes current value to MQTT
```

**Note:** This is called automatically when the value changes if IoT is enabled.

---

#### Value Management

##### `setValue()`
```cpp
void setValue(const char* value);
```
**Description:** Sets the variable's value from a string.

**Parameters:**
- `value` (const char*): Null-terminated string to set as the value

**Returns:** void

**Behavior:**
1. Copies value (truncates if exceeds size limit)
2. Saves to FRAM if auto-save is enabled
3. Publishes to MQTT if IoT is enabled
4. Triggers all registered callbacks

**Usage:**
```cpp
sensorValue.setValue("23.5");
```

---

##### `getValue()`
```cpp
char* getValue();
```
**Description:** Retrieves the current value as a string.

**Parameters:** None

**Returns:** char* - Pointer to the null-terminated string value

**Usage:**
```cpp
char* currentValue = sensorValue.getValue();
Serial.println(currentValue); // Prints the current value
```

---

##### `setIntValue()`
```cpp
void setIntValue(int32_t value);
```
**Description:** Sets the variable's value from an integer.

**Parameters:**
- `value` (int32_t): Integer value to set

**Returns:** void

**Behavior:**
- Converts integer to string using base-10
- Calls `setValue()` internally (triggers all associated actions)

**Usage:**
```cpp
sensorValue.setIntValue(42);
```

---

##### `getIntValue()`
```cpp
int32_t getIntValue();
```
**Description:** Retrieves the current value as an integer.

**Parameters:** None

**Returns:** int32_t - Integer representation of the value (0 if not a valid number)

**Usage:**
```cpp
int32_t count = sensorValue.getIntValue();
```

---

##### `setDoubleValue()`
```cpp
void setDoubleValue(double value);
```
**Description:** Sets the variable's value from a double.

**Parameters:**
- `value` (double): Double-precision floating-point value to set

**Returns:** void

**Behavior:**
- Converts double to string with 2 decimal places
- Calls `setValue()` internally

**Usage:**
```cpp
sensorValue.setDoubleValue(23.45);
```

---

##### `getDoubleValue()`
```cpp
double getDoubleValue();
```
**Description:** Retrieves the current value as a double.

**Parameters:** None

**Returns:** double - Double representation of the value (0.0 if not a valid number)

**Usage:**
```cpp
double temperature = sensorValue.getDoubleValue();
```

---

#### FRAM Persistence

##### `bindFRAM()` - Auto-load
```cpp
void bindFRAM(FRAM* fram, uint32_t framAddress);
```
**Description:** Binds the SmartVariable to a FRAM memory location and automatically loads the stored value.

**Parameters:**
- `fram` (FRAM*): Pointer to the FRAM instance
- `framAddress` (uint32_t): Memory address in FRAM where the value is stored

**Returns:** void

**Behavior:**
- Associates the variable with a FRAM location
- Immediately loads the value from FRAM
- Subsequent saves will write to this location

**Usage:**
```cpp
FRAM fram;
SmartVariable config;
config.begin(64);
config.bindFRAM(&fram, 0x1000); // Binds to address 0x1000 and loads value
```

---

##### `bindFRAM()` - Optional load
```cpp
void bindFRAM(FRAM* fram, uint32_t framAddress, bool loadValue);
```
**Description:** Binds the SmartVariable to a FRAM memory location with control over initial loading.

**Parameters:**
- `fram` (FRAM*): Pointer to the FRAM instance
- `framAddress` (uint32_t): Memory address in FRAM
- `loadValue` (bool): If true, loads value from FRAM; if false, skips loading

**Returns:** void

**Usage:**
```cpp
config.bindFRAM(&fram, 0x1000, false); // Binds without loading
```

---

##### `loadValue()`
```cpp
void loadValue();
```
**Description:** Loads the value from FRAM into the variable.

**Parameters:** None

**Returns:** void

**Prerequisites:** FRAM must be bound using `bindFRAM()` first

**Behavior:**
- Reads bytes from FRAM
- Updates variable value
- Triggers callbacks and publishing

**Usage:**
```cpp
config.loadValue(); // Reloads value from FRAM
```

---

##### `saveValue()`
```cpp
void saveValue();
```
**Description:** Saves the current value to FRAM.

**Parameters:** None

**Returns:** void

**Prerequisites:** FRAM must be bound using `bindFRAM()` first

**Usage:**
```cpp
config.setValue("new_value");
config.saveValue(); // Manually save to FRAM
```

---

##### `setValueAutoSave()`
```cpp
void setValueAutoSave(bool autoSave);
```
**Description:** Enables or disables automatic saving to FRAM on value changes.

**Parameters:**
- `autoSave` (bool): true to enable auto-save, false to disable

**Returns:** void

**Usage:**
```cpp
config.setValueAutoSave(true); // Every setValue() will also saveValue()
```

**Note:** Auto-save is useful for configuration values but may reduce FRAM lifespan if values change frequently.

---

#### Callbacks

##### `registerCallback()`
```cpp
uint16_t registerCallback(std::function<void(char*)> callback);
```
**Description:** Registers a callback function to be called whenever the value changes.

**Parameters:**
- `callback` (std::function<void(char*)>): Function to call when value changes; receives the new value as parameter

**Returns:** uint16_t - Handler ID for the callback (used for unregistering)

**Usage:**
```cpp
void onValueChange(char* newValue) {
    Serial.print("Value changed to: ");
    Serial.println(newValue);
}

uint16_t handlerId = sensorValue.registerCallback(onValueChange);
```

**Lambda Example:**
```cpp
uint16_t handlerId = sensorValue.registerCallback([](char* value) {
    Serial.println(value);
});
```

---

##### `unregisterCallback()`
```cpp
void unregisterCallback(uint16_t handlerId);
```
**Description:** Unregisters a previously registered callback.

**Parameters:**
- `handlerId` (uint16_t): Handler ID returned by `registerCallback()`

**Returns:** void

**Usage:**
```cpp
sensorValue.unregisterCallback(handlerId);
```

---

### SmartVariable Use Cases

1. **System Configuration Settings**
   - Device name, location, network settings
   - Persistent across reboots
   - Remotely configurable via MQTT

2. **Sensor Calibration Values**
   - Offset and scaling factors
   - Stored in FRAM for persistence
   - Adjustable without firmware updates

3. **Operational Parameters**
   - Temperature setpoints, thresholds, timers
   - Real-time monitoring and adjustment
   - Auto-save to prevent loss on power failure

4. **User Preferences**
   - UI settings, display modes, alarm configurations
   - Synchronized across multiple devices
   - Callback-based UI updates

5. **State Synchronization**
   - Current mode, status flags
   - Broadcast changes to all connected devices
   - Request current state from any device

---

### SmartVariable Internals

#### How It Works

**Memory Management:**
- `begin()` allocates a buffer using `calloc()` initialized to zero
- Destructor automatically frees the buffer
- Size includes the null terminator

**MQTT Integration:**
- Uses std::bind to create bound callbacks
- Registers with ESPMegaIoT for both message reception and subscription events
- Automatically resubscribes on MQTT reconnection

**Value Change Flow:**
```
setValue() called
    ↓
1. Copy value to buffer (with bounds checking)
    ↓
2. If auto-save enabled → saveValue() to FRAM
    ↓
3. If IoT enabled → publishValue() to MQTT
    ↓
4. Iterate through all registered callbacks and invoke them
```

**FRAM Operations:**
- Direct read/write to specified address
- No wear-leveling (consider access patterns)
- Size-based operations (reads/writes full buffer)

**Topic Structure:**
- **Main topic:** Where values are published (e.g., "home/sensor/temp")
- **Value request topic:** For requesting current value (e.g., "home/sensor/temp/get")
- **Set value topic:** For remote updates (e.g., "home/sensor/temp/set")

**Thread Safety:**
- Not thread-safe by default
- MQTT callbacks run in network thread
- Use appropriate synchronization if accessing from multiple threads

---

### SmartVariable Examples

#### Basic Usage
```cpp
#include <SmartVariable.hpp>

SmartVariable deviceName;

void setup() {
    // Initialize with 32 bytes
    deviceName.begin(32);

    // Set initial value
    deviceName.setValue("MyESPDevice");

    // Read the value
    Serial.println(deviceName.getValue());
}
```

#### With FRAM Persistence
```cpp
#include <SmartVariable.hpp>
#include <FRAM.h>

FRAM fram;
SmartVariable temperature;

void setup() {
    // Initialize FRAM
    fram.begin();

    // Initialize variable
    temperature.begin(16);

    // Bind to FRAM address 0x0000 and load saved value
    temperature.bindFRAM(&fram, 0x0000);

    // Enable auto-save
    temperature.setValueAutoSave(true);

    // Now any setValue() will automatically save to FRAM
    temperature.setDoubleValue(23.5);
}
```

#### With MQTT and Callbacks
```cpp
#include <SmartVariable.hpp>
#include <ESPMegaIoT.hpp>

ESPMegaIoT iot;
SmartVariable sensorValue;

void onSensorChange(char* newValue) {
    Serial.print("Sensor changed to: ");
    Serial.println(newValue);
}

void setup() {
    // Initialize IoT
    iot.begin();

    // Initialize variable
    sensorValue.begin(32);

    // Enable IoT with main topic
    sensorValue.enableIoT(&iot, "home/living-room/temperature");

    // Enable remote value requests
    sensorValue.enableValueRequest("home/living-room/temperature/get");

    // Enable remote value setting
    sensorValue.enableSetValue("home/living-room/temperature/set");

    // Register callback
    sensorValue.registerCallback(onSensorChange);

    // Set initial value (will publish to MQTT)
    sensorValue.setDoubleValue(22.5);
}

void loop() {
    iot.loop();
}
```

---

## RemoteVariable Class

### RemoteVariable Overview

The `RemoteVariable` class represents a **variable that exists on another device** and can be accessed remotely via MQTT. Unlike SmartVariable (which is a local variable with remote access), RemoteVariable is a local proxy for a remote variable.

**Key Features:**
- Subscribe to remote variable updates via MQTT
- Request current value from remote device
- Optionally set values on remote device
- Type-safe getters/setters for common data types
- Callback system for value change notifications
- Automatic subscription management

**Primary Use Case:**
Reading sensor data or configuration from another ESPMegaPRO3 device in your IoT network.

**Header File:** `/home/user/ESPMegaPRO3-library/RemoteVariable.hpp`
**Source File:** `/home/user/ESPMegaPRO3-library/RemoteVariable.cpp`

---

### RemoteVariable Public Methods

#### Constructor and Destructor

##### `RemoteVariable()`
```cpp
RemoteVariable();
```
**Description:** Default constructor. Creates an uninitialized RemoteVariable object.

**Parameters:** None

**Returns:** RemoteVariable object

**Usage:**
```cpp
RemoteVariable remoteTemp;
```

---

##### `~RemoteVariable()`
```cpp
~RemoteVariable();
```
**Description:** Destructor. Automatically frees allocated memory.

**Parameters:** None

**Returns:** None

---

#### Initialization

##### `begin()` - Basic
```cpp
void begin(size_t size, const char* topic, ESPMegaIoT* iot);
```
**Description:** Initializes the RemoteVariable without value request capability.

**Parameters:**
- `size` (size_t): Maximum size of the variable in bytes (including null terminator)
- `topic` (const char*): MQTT topic where the remote variable publishes its value
- `iot` (ESPMegaIoT*): Pointer to the ESPMegaIoT instance

**Returns:** void

**Behavior:**
- Allocates memory for the value
- Subscribes to the specified topic
- Registers MQTT callbacks

**Usage:**
```cpp
ESPMegaIoT iot;
RemoteVariable remoteSensor;
remoteSensor.begin(32, "bedroom/sensor/humidity", &iot);
```

---

##### `begin()` - With Value Request
```cpp
void begin(size_t size, const char* topic, ESPMegaIoT* iot,
           bool useValueRequest, const char* valueRequestTopic);
```
**Description:** Initializes the RemoteVariable with value request capability.

**Parameters:**
- `size` (size_t): Maximum size of the variable in bytes
- `topic` (const char*): MQTT topic to subscribe to for value updates
- `iot` (ESPMegaIoT*): Pointer to the ESPMegaIoT instance
- `useValueRequest` (bool): Enable value request feature
- `valueRequestTopic` (const char*): Topic to publish value requests to

**Returns:** void

**Behavior:**
- Performs basic initialization
- Enables value request functionality
- Automatically requests initial value

**Usage:**
```cpp
remoteSensor.begin(32, "bedroom/sensor/humidity", &iot,
                   true, "bedroom/sensor/humidity/get");
```

---

#### Value Retrieval

##### `getValue()`
```cpp
char* getValue();
```
**Description:** Retrieves the current value as a string.

**Parameters:** None

**Returns:** char* - Null-terminated string containing the value

**Usage:**
```cpp
char* humidity = remoteSensor.getValue();
Serial.println(humidity);
```

---

##### `getValueAsInt()`
```cpp
int getValueAsInt();
```
**Description:** Retrieves the current value as an integer.

**Parameters:** None

**Returns:** int - Integer representation of the value (0 if not a valid number)

**Usage:**
```cpp
int count = remoteSensor.getValueAsInt();
```

---

##### `getValueAsLong()`
```cpp
long getValueAsLong();
```
**Description:** Retrieves the current value as a long integer.

**Parameters:** None

**Returns:** long - Long integer representation of the value (0 if not a valid number)

**Usage:**
```cpp
long timestamp = remoteSensor.getValueAsLong();
```

---

##### `getValueAsDouble()`
```cpp
double getValueAsDouble();
```
**Description:** Retrieves the current value as a double.

**Parameters:** None

**Returns:** double - Double-precision floating-point representation (0.0 if not a valid number)

**Usage:**
```cpp
double temperature = remoteSensor.getValueAsDouble();
```

---

#### Value Request

##### `requestValue()`
```cpp
void requestValue();
```
**Description:** Requests a value update from the remote device.

**Parameters:** None

**Returns:** void

**Prerequisites:** Must be initialized with value request enabled

**Behavior:**
- Publishes "request" to the value request topic
- Remote device responds by publishing current value

**Usage:**
```cpp
remoteSensor.requestValue(); // Ask remote device to publish its value
```

**MQTT Flow:**
```
This Device → "bedroom/sensor/humidity/get" ("request")
Remote Device → "bedroom/sensor/humidity" ("65.5")
This Device → Receives and stores "65.5"
```

---

#### Value Setting

##### `enableSetValue()`
```cpp
void enableSetValue(const char* setValueTopic);
```
**Description:** Enables the ability to set values on the remote device.

**Parameters:**
- `setValueTopic` (const char*): MQTT topic to publish set commands to

**Returns:** void

**Usage:**
```cpp
remoteSensor.enableSetValue("bedroom/sensor/humidity/set");
```

---

##### `setValue()`
```cpp
void setValue(const char* value);
```
**Description:** Sets the value on the remote device.

**Parameters:**
- `value` (const char*): String value to set

**Returns:** void

**Prerequisites:** `enableSetValue()` must be called first

**Behavior:**
- Publishes the value to the set value topic
- Remote device updates its value

**Usage:**
```cpp
remoteSensor.setValue("60.0");
```

---

##### `setIntValue()`
```cpp
void setIntValue(int value);
```
**Description:** Sets the value on the remote device as an integer.

**Parameters:**
- `value` (int): Integer value to set

**Returns:** void

**Usage:**
```cpp
remoteSensor.setIntValue(75);
```

---

##### `setLongValue()`
```cpp
void setLongValue(long value);
```
**Description:** Sets the value on the remote device as a long integer.

**Parameters:**
- `value` (long): Long integer value to set

**Returns:** void

**Usage:**
```cpp
remoteSensor.setLongValue(1234567890L);
```

---

##### `setDoubleValue()`
```cpp
void setDoubleValue(double value);
```
**Description:** Sets the value on the remote device as a double.

**Parameters:**
- `value` (double): Double-precision floating-point value to set

**Returns:** void

**Usage:**
```cpp
remoteSensor.setDoubleValue(23.5);
```

---

#### Subscription Management

##### `subscribe()`
```cpp
void subscribe();
```
**Description:** Subscribes to MQTT topics. Called internally but can be called manually if needed.

**Parameters:** None

**Returns:** void

**Behavior:**
- Subscribes to the main topic
- If value request is enabled, sends an initial request

**Usage:**
```cpp
remoteSensor.subscribe(); // Usually called automatically
```

---

#### Callbacks

##### `registerCallback()`
```cpp
uint8_t registerCallback(std::function<void(char*)> callback);
```
**Description:** Registers a callback function to be called when the remote value changes.

**Parameters:**
- `callback` (std::function<void(char*)>): Function to call when value changes

**Returns:** uint8_t - Handler ID for the callback

**Usage:**
```cpp
void onRemoteChange(char* newValue) {
    Serial.print("Remote value changed to: ");
    Serial.println(newValue);
}

uint8_t handlerId = remoteSensor.registerCallback(onRemoteChange);
```

---

##### `unregisterCallback()`
```cpp
void unregisterCallback(uint8_t handler);
```
**Description:** Unregisters a previously registered callback.

**Parameters:**
- `handler` (uint8_t): Handler ID returned by `registerCallback()`

**Returns:** void

**Usage:**
```cpp
remoteSensor.unregisterCallback(handlerId);
```

---

### RemoteVariable MQTT Integration

#### Topic Structure

RemoteVariable uses a multi-topic structure for full bidirectional communication:

1. **Value Topic** (Subscribe)
   - Purpose: Receive value updates from remote device
   - Example: `"bedroom/sensor/temperature"`
   - Message: Current value as string (e.g., "23.5")

2. **Value Request Topic** (Publish - Optional)
   - Purpose: Request current value from remote device
   - Example: `"bedroom/sensor/temperature/get"`
   - Message: "request" (any payload triggers response)

3. **Set Value Topic** (Publish - Optional)
   - Purpose: Command remote device to change value
   - Example: `"bedroom/sensor/temperature/set"`
   - Message: New value as string

#### MQTT Message Flow

**Passive Monitoring:**
```
Remote Device → "sensor/temp" ("24.5")
RemoteVariable → Receives and stores "24.5"
RemoteVariable → Triggers callbacks
```

**Active Request:**
```
RemoteVariable → "sensor/temp/get" ("request")
Remote Device → "sensor/temp" ("24.5")
RemoteVariable → Receives and stores "24.5"
```

**Remote Control:**
```
RemoteVariable → "sensor/temp/set" ("25.0")
Remote Device → Updates value to "25.0"
Remote Device → "sensor/temp" ("25.0")
RemoteVariable → Receives confirmation
```

#### QoS and Reliability

- RemoteVariable subscribes to topics via ESPMegaIoT
- Automatically resubscribes on reconnection
- No local caching beyond current value
- Value requests can be used to resync after connection loss

---

### RemoteVariable Use Cases

1. **Multi-Room Monitoring**
   - Monitor sensors across different rooms/devices
   - Each RemoteVariable represents a sensor in another location
   - Centralized dashboard or control logic

2. **Device Coordination**
   - Read status from other ESPMegaPRO3 devices
   - Coordinate actions based on remote states
   - Example: Turn on fan when remote temperature exceeds threshold

3. **Distributed Control Systems**
   - Control actuators on remote devices
   - Read feedback from remote systems
   - Implement master-slave architectures

4. **Data Aggregation**
   - Collect data from multiple sources
   - One device subscribes to multiple RemoteVariables
   - Centralized logging or decision making

5. **Configuration Mirroring**
   - Monitor configuration on other devices
   - Synchronize settings across device fleet
   - Detect configuration drift

---

### RemoteVariable vs SmartVariable

| Feature | SmartVariable | RemoteVariable |
|---------|---------------|----------------|
| **Purpose** | Local variable with remote access | Proxy for remote variable |
| **Data Location** | Stored locally | Stored on remote device |
| **FRAM Support** | Yes (persistence) | No |
| **Auto-publish** | Yes (on value change) | N/A |
| **Subscribe to MQTT** | Optional (for remote control) | Always (to receive updates) |
| **Publish to MQTT** | Yes (when value changes locally) | Optional (to request/set) |
| **Typical Use** | Device settings, sensor readings | Monitoring other devices |
| **Memory** | Persistent (with FRAM) | Volatile (RAM only) |

**Together in a System:**
- Device A: Uses SmartVariable for its temperature sensor
- Device B: Uses RemoteVariable to read Device A's temperature
- Device A publishes to "deviceA/temperature"
- Device B subscribes to "deviceA/temperature"

---

### RemoteVariable Examples

#### Basic Remote Monitoring
```cpp
#include <RemoteVariable.hpp>
#include <ESPMegaIoT.hpp>

ESPMegaIoT iot;
RemoteVariable bedroomTemp;

void setup() {
    Serial.begin(115200);

    // Initialize IoT
    iot.begin();

    // Monitor temperature from bedroom device
    bedroomTemp.begin(16, "bedroom/sensor/temperature", &iot);
}

void loop() {
    iot.loop();

    // Read current value
    double temp = bedroomTemp.getValueAsDouble();
    Serial.print("Bedroom Temperature: ");
    Serial.println(temp);

    delay(5000);
}
```

#### With Value Request
```cpp
#include <RemoteVariable.hpp>
#include <ESPMegaIoT.hpp>

ESPMegaIoT iot;
RemoteVariable kitchenHumidity;

void setup() {
    // Initialize with value request
    kitchenHumidity.begin(16,
                          "kitchen/sensor/humidity",
                          &iot,
                          true,
                          "kitchen/sensor/humidity/get");

    // Value is automatically requested on initialization
}

void loop() {
    iot.loop();

    // Manually request update every 30 seconds
    static unsigned long lastRequest = 0;
    if (millis() - lastRequest > 30000) {
        kitchenHumidity.requestValue();
        lastRequest = millis();
    }

    delay(100);
}
```

#### Remote Control
```cpp
#include <RemoteVariable.hpp>
#include <ESPMegaIoT.hpp>

ESPMegaIoT iot;
RemoteVariable garageDoorState;

void setup() {
    // Initialize
    garageDoorState.begin(16, "garage/door/state", &iot);

    // Enable remote control
    garageDoorState.enableSetValue("garage/door/state/set");
}

void openGarage() {
    garageDoorState.setValue("open");
}

void closeGarage() {
    garageDoorState.setValue("closed");
}

void loop() {
    iot.loop();

    // Monitor state
    char* state = garageDoorState.getValue();
    Serial.print("Garage door is: ");
    Serial.println(state);

    delay(1000);
}
```

#### With Callbacks
```cpp
#include <RemoteVariable.hpp>
#include <ESPMegaIoT.hpp>

ESPMegaIoT iot;
RemoteVariable outdoorTemp;

void onTemperatureChange(char* newValue) {
    Serial.print("Outdoor temperature changed to: ");
    Serial.println(newValue);

    double temp = atof(newValue);

    // Take action based on temperature
    if (temp > 30.0) {
        Serial.println("High temperature alert!");
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize
    outdoorTemp.begin(16, "outdoor/sensor/temperature", &iot);

    // Register callback
    outdoorTemp.registerCallback(onTemperatureChange);
}

void loop() {
    iot.loop();
}
```

---

## Complete Code Examples

### Basic SmartVariable Usage

This example demonstrates basic SmartVariable functionality without MQTT.

```cpp
#include <Arduino.h>
#include <SmartVariable.hpp>

SmartVariable deviceName;
SmartVariable deviceLocation;
SmartVariable sensorOffset;

void setup() {
    Serial.begin(115200);
    Serial.println("SmartVariable Basic Example");

    // Initialize variables
    deviceName.begin(32);
    deviceLocation.begin(64);
    sensorOffset.begin(16);

    // Set string values
    deviceName.setValue("ESP32-Living-Room");
    deviceLocation.setValue("Living Room, First Floor, North Wall");

    // Set numeric value
    sensorOffset.setDoubleValue(2.5);

    // Read values
    Serial.print("Device Name: ");
    Serial.println(deviceName.getValue());

    Serial.print("Location: ");
    Serial.println(deviceLocation.getValue());

    Serial.print("Sensor Offset: ");
    Serial.println(sensorOffset.getDoubleValue());

    // Numeric operations
    double currentOffset = sensorOffset.getDoubleValue();
    sensorOffset.setDoubleValue(currentOffset + 0.5);

    Serial.print("New Offset: ");
    Serial.println(sensorOffset.getDoubleValue());
}

void loop() {
    // Nothing to do
}
```

---

### RemoteVariable with MQTT

Monitor multiple remote sensors from different devices.

```cpp
#include <Arduino.h>
#include <ESPMegaIoT.hpp>
#include <RemoteVariable.hpp>

ESPMegaIoT iot;

// Remote sensors from different rooms
RemoteVariable bedroomTemp;
RemoteVariable kitchenTemp;
RemoteVariable garageTemp;
RemoteVariable outdoorHumidity;

void onBedroomTempChange(char* newValue) {
    Serial.print("[Bedroom] Temperature: ");
    Serial.print(newValue);
    Serial.println(" °C");
}

void onKitchenTempChange(char* newValue) {
    Serial.print("[Kitchen] Temperature: ");
    Serial.print(newValue);
    Serial.println(" °C");
}

void onGarageTempChange(char* newValue) {
    Serial.print("[Garage] Temperature: ");
    Serial.print(newValue);
    Serial.println(" °C");

    // Alert if garage is too hot
    double temp = atof(newValue);
    if (temp > 35.0) {
        Serial.println("WARNING: Garage temperature critical!");
    }
}

void onHumidityChange(char* newValue) {
    Serial.print("[Outdoor] Humidity: ");
    Serial.print(newValue);
    Serial.println(" %");
}

void setup() {
    Serial.begin(115200);
    Serial.println("RemoteVariable MQTT Example");

    // Initialize IoT (configure WiFi/MQTT in your setup)
    iot.begin();

    // Initialize remote variables with value request
    bedroomTemp.begin(16, "bedroom/sensor/temperature", &iot,
                     true, "bedroom/sensor/temperature/get");
    kitchenTemp.begin(16, "kitchen/sensor/temperature", &iot,
                     true, "kitchen/sensor/temperature/get");
    garageTemp.begin(16, "garage/sensor/temperature", &iot,
                    true, "garage/sensor/temperature/get");
    outdoorHumidity.begin(16, "outdoor/sensor/humidity", &iot,
                         true, "outdoor/sensor/humidity/get");

    // Register callbacks
    bedroomTemp.registerCallback(onBedroomTempChange);
    kitchenTemp.registerCallback(onKitchenTempChange);
    garageTemp.registerCallback(onGarageTempChange);
    outdoorHumidity.registerCallback(onHumidityChange);

    Serial.println("Monitoring remote sensors...");
}

void loop() {
    iot.loop();

    // Display summary every 60 seconds
    static unsigned long lastSummary = 0;
    if (millis() - lastSummary > 60000) {
        Serial.println("\n=== Temperature Summary ===");
        Serial.print("Bedroom:  ");
        Serial.print(bedroomTemp.getValueAsDouble());
        Serial.println(" °C");

        Serial.print("Kitchen:  ");
        Serial.print(kitchenTemp.getValueAsDouble());
        Serial.println(" °C");

        Serial.print("Garage:   ");
        Serial.print(garageTemp.getValueAsDouble());
        Serial.println(" °C");

        Serial.print("Humidity: ");
        Serial.print(outdoorHumidity.getValueAsDouble());
        Serial.println(" %\n");

        lastSummary = millis();
    }

    delay(100);
}
```

---

### Integration with IoT System

Complete example showing SmartVariable with FRAM persistence and full MQTT integration.

```cpp
#include <Arduino.h>
#include <ESPMegaIoT.hpp>
#include <SmartVariable.hpp>
#include <FRAM.h>

// Hardware components
ESPMegaIoT iot;
FRAM fram;

// Configuration variables with FRAM persistence
SmartVariable deviceName;
SmartVariable deviceLocation;
SmartVariable temperatureOffset;
SmartVariable humidityOffset;

// Runtime variables (sensor readings)
SmartVariable currentTemperature;
SmartVariable currentHumidity;

// FRAM addresses
#define FRAM_DEVICE_NAME      0x0000
#define FRAM_DEVICE_LOCATION  0x0100
#define FRAM_TEMP_OFFSET      0x0200
#define FRAM_HUMIDITY_OFFSET  0x0300

void onDeviceNameChange(char* newName) {
    Serial.print("Device name changed to: ");
    Serial.println(newName);
}

void onTemperatureOffsetChange(char* newOffset) {
    Serial.print("Temperature offset changed to: ");
    Serial.println(newOffset);
    // Recalculate current temperature with new offset
    updateTemperature();
}

void updateTemperature() {
    // Simulate reading from sensor
    float rawTemp = 23.5; // Replace with actual sensor reading
    float offset = temperatureOffset.getDoubleValue();
    float correctedTemp = rawTemp + offset;

    currentTemperature.setDoubleValue(correctedTemp);
}

void updateHumidity() {
    // Simulate reading from sensor
    float rawHumidity = 65.0; // Replace with actual sensor reading
    float offset = humidityOffset.getDoubleValue();
    float correctedHumidity = rawHumidity + offset;

    currentHumidity.setDoubleValue(correctedHumidity);
}

void setup() {
    Serial.begin(115200);
    Serial.println("SmartVariable IoT Integration Example");

    // Initialize FRAM
    fram.begin();
    Serial.println("FRAM initialized");

    // Initialize IoT
    iot.begin();
    Serial.println("IoT initialized");

    // Initialize configuration variables
    deviceName.begin(32);
    deviceLocation.begin(64);
    temperatureOffset.begin(16);
    humidityOffset.begin(16);

    // Bind to FRAM with auto-load
    deviceName.bindFRAM(&fram, FRAM_DEVICE_NAME);
    deviceLocation.bindFRAM(&fram, FRAM_DEVICE_LOCATION);
    temperatureOffset.bindFRAM(&fram, FRAM_TEMP_OFFSET);
    humidityOffset.bindFRAM(&fram, FRAM_HUMIDITY_OFFSET);

    // Enable auto-save for configuration
    deviceName.setValueAutoSave(true);
    deviceLocation.setValueAutoSave(true);
    temperatureOffset.setValueAutoSave(true);
    humidityOffset.setValueAutoSave(true);

    // Check if this is first boot (empty FRAM)
    if (strlen(deviceName.getValue()) == 0) {
        Serial.println("First boot detected, setting defaults");
        deviceName.setValue("ESPMegaPRO3-001");
        deviceLocation.setValue("Unknown");
        temperatureOffset.setDoubleValue(0.0);
        humidityOffset.setDoubleValue(0.0);
    }

    // Enable IoT for configuration variables
    deviceName.enableIoT(&iot, "device/config/name");
    deviceName.enableValueRequest("device/config/name/get");
    deviceName.enableSetValue("device/config/name/set");

    deviceLocation.enableIoT(&iot, "device/config/location");
    deviceLocation.enableValueRequest("device/config/location/get");
    deviceLocation.enableSetValue("device/config/location/set");

    temperatureOffset.enableIoT(&iot, "device/config/temp-offset");
    temperatureOffset.enableValueRequest("device/config/temp-offset/get");
    temperatureOffset.enableSetValue("device/config/temp-offset/set");

    humidityOffset.enableIoT(&iot, "device/config/humidity-offset");
    humidityOffset.enableValueRequest("device/config/humidity-offset/get");
    humidityOffset.enableSetValue("device/config/humidity-offset/set");

    // Initialize runtime variables
    currentTemperature.begin(16);
    currentHumidity.begin(16);

    // Enable IoT for sensor readings (publish only)
    currentTemperature.enableIoT(&iot, "device/sensor/temperature");
    currentTemperature.enableValueRequest("device/sensor/temperature/get");

    currentHumidity.enableIoT(&iot, "device/sensor/humidity");
    currentHumidity.enableValueRequest("device/sensor/humidity/get");

    // Register callbacks
    deviceName.registerCallback(onDeviceNameChange);
    temperatureOffset.registerCallback(onTemperatureOffsetChange);

    Serial.println("Setup complete");
    Serial.print("Device Name: ");
    Serial.println(deviceName.getValue());
    Serial.print("Location: ");
    Serial.println(deviceLocation.getValue());
}

void loop() {
    iot.loop();

    // Update sensor readings every 10 seconds
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 10000) {
        updateTemperature();
        updateHumidity();

        Serial.print("Temperature: ");
        Serial.print(currentTemperature.getValue());
        Serial.print(" °C, Humidity: ");
        Serial.print(currentHumidity.getValue());
        Serial.println(" %");

        lastUpdate = millis();
    }

    delay(100);
}
```

---

### Practical Applications

#### Home Automation System

A complete home automation controller using both SmartVariable and RemoteVariable.

```cpp
#include <Arduino.h>
#include <ESPMegaIoT.hpp>
#include <SmartVariable.hpp>
#include <RemoteVariable.hpp>
#include <FRAM.h>

// Hardware
ESPMegaIoT iot;
FRAM fram;

// Local control settings (SmartVariables)
SmartVariable hvacMode;           // "off", "heat", "cool", "auto"
SmartVariable targetTemperature;  // Desired temperature
SmartVariable fanSpeed;           // "low", "medium", "high", "auto"
SmartVariable scheduleEnabled;    // "true" or "false"

// Remote sensors (RemoteVariables)
RemoteVariable livingRoomTemp;
RemoteVariable bedroomTemp;
RemoteVariable kitchenTemp;
RemoteVariable outdoorTemp;

// Remote actuators (RemoteVariables for control)
RemoteVariable livingRoomHeater;
RemoteVariable bedroomHeater;
RemoteVariable kitchenHeater;

// FRAM addresses
#define FRAM_HVAC_MODE        0x0000
#define FRAM_TARGET_TEMP      0x0100
#define FRAM_FAN_SPEED        0x0200
#define FRAM_SCHEDULE_ENABLED 0x0300

void onHvacModeChange(char* newMode) {
    Serial.print("HVAC mode changed to: ");
    Serial.println(newMode);
    updateHvacSystem();
}

void onTargetTempChange(char* newTarget) {
    Serial.print("Target temperature changed to: ");
    Serial.println(newTarget);
    updateHvacSystem();
}

void onRoomTempChange(char* newTemp) {
    // Called when any room temperature changes
    updateHvacSystem();
}

void updateHvacSystem() {
    char* mode = hvacMode.getValue();

    if (strcmp(mode, "off") == 0) {
        // Turn off all heaters
        livingRoomHeater.setValue("off");
        bedroomHeater.setValue("off");
        kitchenHeater.setValue("off");
        return;
    }

    double targetTemp = targetTemperature.getDoubleValue();

    // Control each room independently
    controlRoomHeater(livingRoomTemp.getValueAsDouble(),
                     targetTemp, &livingRoomHeater);
    controlRoomHeater(bedroomTemp.getValueAsDouble(),
                     targetTemp, &bedroomHeater);
    controlRoomHeater(kitchenTemp.getValueAsDouble(),
                     targetTemp, &kitchenHeater);
}

void controlRoomHeater(double currentTemp, double targetTemp,
                      RemoteVariable* heater) {
    char* mode = hvacMode.getValue();

    if (strcmp(mode, "heat") == 0) {
        if (currentTemp < targetTemp - 0.5) {
            heater->setValue("on");
        } else if (currentTemp > targetTemp + 0.5) {
            heater->setValue("off");
        }
    } else if (strcmp(mode, "cool") == 0) {
        // Cooling logic (would control AC instead)
        if (currentTemp > targetTemp + 0.5) {
            heater->setValue("on"); // Actually AC
        } else if (currentTemp < targetTemp - 0.5) {
            heater->setValue("off");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Home Automation System Starting...");

    // Initialize hardware
    fram.begin();
    iot.begin();

    // Initialize local settings
    hvacMode.begin(16);
    targetTemperature.begin(16);
    fanSpeed.begin(16);
    scheduleEnabled.begin(8);

    // Bind to FRAM
    hvacMode.bindFRAM(&fram, FRAM_HVAC_MODE);
    targetTemperature.bindFRAM(&fram, FRAM_TARGET_TEMP);
    fanSpeed.bindFRAM(&fram, FRAM_FAN_SPEED);
    scheduleEnabled.bindFRAM(&fram, FRAM_SCHEDULE_ENABLED);

    // Enable auto-save
    hvacMode.setValueAutoSave(true);
    targetTemperature.setValueAutoSave(true);
    fanSpeed.setValueAutoSave(true);
    scheduleEnabled.setValueAutoSave(true);

    // Set defaults if first boot
    if (strlen(hvacMode.getValue()) == 0) {
        hvacMode.setValue("off");
        targetTemperature.setDoubleValue(22.0);
        fanSpeed.setValue("auto");
        scheduleEnabled.setValue("false");
    }

    // Enable IoT for local settings
    hvacMode.enableIoT(&iot, "home/hvac/mode");
    hvacMode.enableValueRequest("home/hvac/mode/get");
    hvacMode.enableSetValue("home/hvac/mode/set");

    targetTemperature.enableIoT(&iot, "home/hvac/target-temp");
    targetTemperature.enableValueRequest("home/hvac/target-temp/get");
    targetTemperature.enableSetValue("home/hvac/target-temp/set");

    fanSpeed.enableIoT(&iot, "home/hvac/fan-speed");
    fanSpeed.enableValueRequest("home/hvac/fan-speed/get");
    fanSpeed.enableSetValue("home/hvac/fan-speed/set");

    // Initialize remote sensors
    livingRoomTemp.begin(16, "home/living-room/temperature", &iot,
                        true, "home/living-room/temperature/get");
    bedroomTemp.begin(16, "home/bedroom/temperature", &iot,
                     true, "home/bedroom/temperature/get");
    kitchenTemp.begin(16, "home/kitchen/temperature", &iot,
                     true, "home/kitchen/temperature/get");
    outdoorTemp.begin(16, "home/outdoor/temperature", &iot,
                     true, "home/outdoor/temperature/get");

    // Initialize remote actuators
    livingRoomHeater.begin(16, "home/living-room/heater/state", &iot);
    livingRoomHeater.enableSetValue("home/living-room/heater/set");

    bedroomHeater.begin(16, "home/bedroom/heater/state", &iot);
    bedroomHeater.enableSetValue("home/bedroom/heater/set");

    kitchenHeater.begin(16, "home/kitchen/heater/state", &iot);
    kitchenHeater.enableSetValue("home/kitchen/heater/set");

    // Register callbacks
    hvacMode.registerCallback(onHvacModeChange);
    targetTemperature.registerCallback(onTargetTempChange);
    livingRoomTemp.registerCallback(onRoomTempChange);
    bedroomTemp.registerCallback(onRoomTempChange);
    kitchenTemp.registerCallback(onRoomTempChange);

    Serial.println("System ready");
    Serial.print("HVAC Mode: ");
    Serial.println(hvacMode.getValue());
    Serial.print("Target Temperature: ");
    Serial.println(targetTemperature.getValue());
}

void loop() {
    iot.loop();

    // Periodic status display
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 30000) {
        Serial.println("\n=== Home Automation Status ===");
        Serial.print("Mode: ");
        Serial.print(hvacMode.getValue());
        Serial.print(" | Target: ");
        Serial.print(targetTemperature.getValue());
        Serial.println(" °C");

        Serial.print("Living Room: ");
        Serial.print(livingRoomTemp.getValue());
        Serial.print(" °C [Heater: ");
        Serial.print(livingRoomHeater.getValue());
        Serial.println("]");

        Serial.print("Bedroom: ");
        Serial.print(bedroomTemp.getValue());
        Serial.print(" °C [Heater: ");
        Serial.print(bedroomHeater.getValue());
        Serial.println("]");

        Serial.print("Kitchen: ");
        Serial.print(kitchenTemp.getValue());
        Serial.print(" °C [Heater: ");
        Serial.print(kitchenHeater.getValue());
        Serial.println("]");

        Serial.print("Outdoor: ");
        Serial.print(outdoorTemp.getValue());
        Serial.println(" °C\n");

        lastStatus = millis();
    }

    delay(100);
}
```

---

## Summary

### When to Use SmartVariable

- **Local variables** that need remote access
- Configuration settings requiring **persistence**
- Sensor readings that should be **automatically published**
- Values that should **survive power loss**
- Data that needs **bidirectional sync** (local and remote changes)

### When to Use RemoteVariable

- Monitoring **variables on other devices**
- Reading **remote sensor data**
- Controlling **remote actuators**
- Implementing **distributed systems**
- Creating **centralized monitoring** dashboards

### Best Practices

1. **Choose appropriate sizes**: Allocate enough space for values plus null terminator
2. **Use callbacks wisely**: Register callbacks for actions that should happen on value changes
3. **FRAM considerations**: Enable auto-save only for infrequently changing values
4. **Topic naming**: Use hierarchical, descriptive MQTT topic names
5. **Error handling**: Check getValue() for null before using, especially on startup
6. **Type conversion**: Use type-specific getters (getIntValue, getDoubleValue) for numeric operations
7. **Memory management**: Destructors handle cleanup automatically, but be mindful of dynamic allocation
8. **Network reliability**: Use value requests after reconnection to resync RemoteVariables

---

## Related Documentation

- ESPMegaIoT Documentation - MQTT configuration and usage
- FRAM Documentation - Persistent storage details
- ESPMegaPRO3 System Architecture - How variables fit in the ecosystem

---

**Document Version:** 1.0
**Last Updated:** 2025-11-23
**Library Files:**
- `/home/user/ESPMegaPRO3-library/SmartVariable.hpp`
- `/home/user/ESPMegaPRO3-library/SmartVariable.cpp`
- `/home/user/ESPMegaPRO3-library/RemoteVariable.hpp`
- `/home/user/ESPMegaPRO3-library/RemoteVariable.cpp`
