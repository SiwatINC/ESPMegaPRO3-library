# DigitalInputCard Documentation

## Table of Contents
1. [Overview](#overview)
2. [Hardware Specifications](#hardware-specifications)
3. [Class Hierarchy](#class-hierarchy)
4. [API Reference](#api-reference)
5. [Debouncing Mechanism](#debouncing-mechanism)
6. [Callback System](#callback-system)
7. [Pin Mapping Feature](#pin-mapping-feature)
8. [IoT Integration (MQTT)](#iot-integration-mqtt)
9. [RTU Integration](#rtu-integration)
10. [FRAM Usage](#fram-usage)
11. [Code Examples](#code-examples)

## Overview

The **DigitalInputCard** is an expansion card for the ESPMegaPRO3 platform that provides 16 isolated digital input channels. It is designed for reading the state of switches, buttons, sensors, and other digital signals in industrial and home automation applications.

### Key Features
- 16 digital input channels (organized in 2 banks of 8)
- Hardware-based debouncing support
- Configurable per-pin debounce timing
- Event-driven callback system
- Flexible pin mapping (virtual to physical)
- MQTT integration for IoT applications
- Remote Terminal Unit (RTU) support
- I2C communication using PCF8574 expanders

### Use Cases
- Reading mechanical switches and buttons
- Monitoring sensor states (motion, door/window, etc.)
- Industrial control applications
- Home automation systems
- Remote monitoring via MQTT

## Hardware Specifications

### Electrical Characteristics
- **Communication Protocol**: I2C
- **I2C Chips**: Dual PCF8574 I/O expanders
- **Number of Inputs**: 16 (2 banks × 8 pins)
- **Default I2C Base Address**: 0x20 (configurable via DIP switch)
- **Input Voltage Levels**: Standard PCF8574 logic levels
- **Pull-up**: Internal pull-up resistors in PCF8574

### Pin Organization
The 16 pins are organized into two banks:
- **Bank A**: Pins 0-7 (connected to first PCF8574)
- **Bank B**: Pins 8-15 (connected to second PCF8574)

### I2C Address Configuration
The card supports two methods of address configuration:

1. **Direct Address Method**: Specify the I2C addresses directly
2. **DIP Switch Method**: Use 6-bit DIP switch (3 bits per bank)
   - Bits 0-2: Bank A address offset
   - Bits 3-5: Bank B address offset
   - Base address: 0x20
   - Final address = 0x20 + offset

## Class Hierarchy

```
ExpansionCard (base class)
    └── DigitalInputCard
            ├── DigitalInputIoT (IoT integration layer)
            └── DigitalInputRTU (Remote Terminal Unit)
```

## API Reference

### DigitalInputCard Class

#### Constructors

##### Direct Address Constructor
```cpp
DigitalInputCard(uint8_t address_a, uint8_t address_b)
```
Creates a new DigitalInputCard with explicit I2C addresses.

**Parameters:**
- `address_a` - I2C address for Bank A (pins 0-7)
- `address_b` - I2C address for Bank B (pins 8-15)

**Example:**
```cpp
DigitalInputCard inputCard(0x20, 0x21);
```

##### DIP Switch Constructor
```cpp
DigitalInputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5)
```
Creates a new DigitalInputCard using DIP switch configuration.

**Parameters:**
- `bit0`, `bit1`, `bit2` - DIP switch bits for Bank A
- `bit3`, `bit4`, `bit5` - DIP switch bits for Bank B

**Note:** Bit 0 is the leftmost switch on the DIP switch.

**Example:**
```cpp
// DIP switch: ON OFF ON OFF OFF ON
DigitalInputCard inputCard(true, false, true, false, false, true);
```

#### Initialization

##### begin()
```cpp
bool begin()
```
Initializes the card and both PCF8574 I/O expanders.

**Returns:**
- `true` if initialization successful
- `false` if initialization failed

**Example:**
```cpp
if (!inputCard.begin()) {
    Serial.println("Failed to initialize input card!");
}
```

**Default Settings After Initialization:**
- Debounce time: 50ms for all pins
- Pin map: Identity mapping (virtual pin = physical pin)
- All callbacks: None registered

#### Reading Inputs

##### digitalRead(uint8_t pin)
```cpp
bool digitalRead(uint8_t pin)
```
Reads the state of the specified pin, always refreshing the input buffer.

**Parameters:**
- `pin` - Pin number (0-15)

**Returns:**
- `true` if pin is HIGH
- `false` if pin is LOW

**Example:**
```cpp
bool state = inputCard.digitalRead(5);
if (state) {
    Serial.println("Pin 5 is HIGH");
}
```

##### digitalRead(uint8_t pin, bool refresh)
```cpp
bool digitalRead(uint8_t pin, bool refresh)
```
Reads the state of the specified pin with optional buffer refresh.

**Parameters:**
- `pin` - Pin number (0-15)
- `refresh` - If `true`, refresh buffer before reading; if `false`, use cached value

**Returns:**
- `true` if pin is HIGH
- `false` if pin is LOW

**Example:**
```cpp
// Read without refreshing (faster, uses cached value)
bool state = inputCard.digitalRead(5, false);
```

#### Buffer Access

##### getInputBufferA()
```cpp
uint8_t getInputBufferA()
```
Gets the current state of Bank A (pins 0-7) as a byte.

**Returns:**
- 8-bit value where bit 0 = pin 0, bit 1 = pin 1, etc.

**Example:**
```cpp
uint8_t bankA = inputCard.getInputBufferA();
if (bankA & 0x01) {
    Serial.println("Pin 0 is HIGH");
}
```

##### getInputBufferB()
```cpp
uint8_t getInputBufferB()
```
Gets the current state of Bank B (pins 8-15) as a byte.

**Returns:**
- 8-bit value where bit 0 = pin 8, bit 1 = pin 9, etc.

#### Debouncing

##### setDebounceTime(uint8_t pin, uint32_t debounceTime)
```cpp
void setDebounceTime(uint8_t pin, uint32_t debounceTime)
```
Sets the debounce time for a specific pin.

**Parameters:**
- `pin` - Pin number (0-15)
- `debounceTime` - Debounce time in milliseconds

**Example:**
```cpp
// Set 100ms debounce for pin 0 (mechanical switch)
inputCard.setDebounceTime(0, 100);

// Set 10ms debounce for pin 1 (clean digital signal)
inputCard.setDebounceTime(1, 10);
```

#### Callbacks

##### registerCallback()
```cpp
uint8_t registerCallback(std::function<void(uint8_t, bool)> callback)
```
Registers a callback function to be called when any pin changes state.

**Parameters:**
- `callback` - Function to call with signature: `void callback(uint8_t pin, bool state)`
  - `pin` - Pin number that changed
  - `state` - New state of the pin (true = HIGH, false = LOW)

**Returns:**
- Handler ID for the callback (use to unregister later)

**Example:**
```cpp
void onInputChange(uint8_t pin, uint8_t value) {
    Serial.printf("Pin %d changed to %d\n", pin, value);
}

uint8_t handler = inputCard.registerCallback(onInputChange);
```

##### unregisterCallback()
```cpp
void unregisterCallback(uint8_t handler)
```
Unregisters a previously registered callback.

**Parameters:**
- `handler` - Handler ID returned by `registerCallback()`

**Example:**
```cpp
inputCard.unregisterCallback(handler);
```

#### Pin Mapping

##### loadPinMap()
```cpp
void loadPinMap(uint8_t pinMap[16])
```
Loads a custom pin mapping to translate virtual pins to physical pins.

**Parameters:**
- `pinMap` - Array of 16 elements mapping virtual pins to physical pins

**Example:**
```cpp
// Remap pins: virtual pin 0 -> physical pin 15, etc.
uint8_t customMap[16] = {15, 14, 13, 12, 11, 10, 9, 8,
                         7, 6, 5, 4, 3, 2, 1, 0};
inputCard.loadPinMap(customMap);

// Now reading pin 0 actually reads physical pin 15
bool state = inputCard.digitalRead(0);
```

#### Loop Processing

##### loop()
```cpp
void loop()
```
Main processing loop that refreshes buffers and triggers callbacks.

**Must be called regularly** (either in your main loop or automatically by ESPMega).

**Example:**
```cpp
void loop() {
    inputCard.loop();
    // Your other code...
}
```

#### Utility Functions

##### preloadInputBuffer()
```cpp
void preloadInputBuffer()
```
Preloads the input buffer and previous buffer with current values.

Useful for:
- Preventing false triggers on startup
- Synchronizing buffer state

**Example:**
```cpp
void setup() {
    inputCard.begin();
    inputCard.preloadInputBuffer(); // Prevent startup callbacks
}
```

##### getStatus()
```cpp
bool getStatus()
```
Checks if the card is properly initialized.

**Returns:**
- `true` if card is initialized
- `false` if initialization failed

##### getType()
```cpp
uint8_t getType()
```
Gets the card type identifier.

**Returns:**
- `CARD_TYPE_DIGITAL_INPUT` (0x01)

## Debouncing Mechanism

### What is Debouncing?

Mechanical switches and some sensors produce electrical noise when changing state, causing multiple rapid transitions instead of a clean HIGH/LOW change. Debouncing filters out this noise.

### How It Works

The DigitalInputCard uses time-based debouncing:

1. **Initial Change**: When a pin state changes, the timestamp is recorded
2. **Waiting Period**: The card waits for the configured debounce time
3. **Confirmation**: If the pin remains in the new state for the entire debounce period, the change is confirmed
4. **Callback Trigger**: Callbacks are fired only after confirmation
5. **Bounce Detection**: If the pin reverts to the old state during debouncing, the timer resets

### Debounce Flow

```
Pin State:     LOW ─┐  ┌─┐  ┌────────── (confirmed HIGH)
                    └──┘ └──┘
                    ↑       ↑
                    |       |
Time:               0ms    50ms (debounce time)
                    |<─────>|
                   Change   Callback
                  Detected  Triggered
```

### Default Settings
- **Default debounce time**: 50ms (set during `begin()`)
- **Per-pin configuration**: Each pin can have its own debounce time

### Recommended Debounce Times

| Input Type | Recommended Time | Reason |
|-----------|------------------|---------|
| Mechanical switches | 50-100ms | High contact bounce |
| Mechanical buttons | 50-100ms | High contact bounce |
| Relay contacts | 10-50ms | Moderate bounce |
| Electronic sensors | 5-20ms | Minimal bounce |
| Clean digital signals | 0-5ms | No bounce |

### Example
```cpp
// Configure debounce for different input types
inputCard.setDebounceTime(0, 100);  // Mechanical switch on pin 0
inputCard.setDebounceTime(1, 50);   // Button on pin 1
inputCard.setDebounceTime(2, 10);   // Electronic sensor on pin 2
inputCard.setDebounceTime(3, 0);    // Clean digital signal on pin 3
```

## Callback System

### Overview

The callback system allows you to register functions that are automatically called when pin states change, enabling event-driven programming.

### Features
- **Multiple callbacks**: Register multiple callback functions
- **Automatic debouncing**: Callbacks only fire after debounce confirmation
- **Handler-based management**: Each callback gets a unique handler ID
- **Pin and state information**: Callbacks receive pin number and new state

### Callback Function Signature
```cpp
void callbackFunction(uint8_t pin, bool state)
```
- `pin` - Virtual pin number (0-15)
- `state` - New state (true = HIGH, false = LOW)

### Registration Process

1. **Define callback function**
2. **Register with card**
3. **Receive handler ID**
4. **Callback fires on state changes**
5. **Optionally unregister**

### Multiple Callbacks Example

```cpp
void callback1(uint8_t pin, bool state) {
    Serial.printf("Callback 1: Pin %d = %d\n", pin, state);
}

void callback2(uint8_t pin, bool state) {
    Serial.printf("Callback 2: Pin %d = %d\n", pin, state);
}

// Both callbacks will be called on any pin change
uint8_t h1 = inputCard.registerCallback(callback1);
uint8_t h2 = inputCard.registerCallback(callback2);
```

### Lambda Functions

You can also use lambda functions for inline callbacks:

```cpp
uint8_t handler = inputCard.registerCallback([](uint8_t pin, bool state) {
    if (pin == 5 && state) {
        digitalWrite(LED_PIN, HIGH);
    }
});
```

### Best Practices
1. Keep callbacks short and fast
2. Avoid blocking operations in callbacks
3. Don't call `digitalRead()` with refresh=true in callbacks (already refreshed)
4. Use flags to communicate with main loop for complex operations

## Pin Mapping Feature

### Purpose

Pin mapping allows you to remap virtual pin numbers to physical pin numbers, making your code more intuitive and matching your hardware layout.

### Use Cases
- Match pin numbering to physical terminal labels
- Reverse pin order for easier PCB routing
- Group related pins logically
- Standardize pin numbers across different hardware revisions

### How It Works

- **Virtual Pins**: Pin numbers used in your code (0-15)
- **Physical Pins**: Actual hardware pins on the PCF8574 chips
- **Pin Map**: Array mapping virtual → physical
- **Virtual Pin Map**: Internal reverse mapping (physical → virtual)

### Default Mapping
By default, virtual pins match physical pins (identity mapping):
```
Virtual Pin:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
Physical Pin: 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
```

### Custom Mapping Example

Scenario: Your terminal block is labeled in reverse order.

```cpp
// Reverse the pin order
uint8_t reverseMap[16] = {
    15, 14, 13, 12, 11, 10, 9, 8,  // Bank A reversed
    7, 6, 5, 4, 3, 2, 1, 0          // Bank B reversed
};

inputCard.loadPinMap(reverseMap);

// Now virtual pin 0 reads physical pin 15
// Virtual pin 1 reads physical pin 14, etc.
```

### Grouped Mapping Example

Scenario: Group pins by function.

```cpp
// Map logical groups to physical pins
uint8_t functionalMap[16] = {
    // Pins 0-3: Door sensors (physical pins 0-3)
    0, 1, 2, 3,
    // Pins 4-7: Window sensors (physical pins 8-11)
    8, 9, 10, 11,
    // Pins 8-11: Motion sensors (physical pins 4-7)
    4, 5, 6, 7,
    // Pins 12-15: Button inputs (physical pins 12-15)
    12, 13, 14, 15
};

inputCard.loadPinMap(functionalMap);
```

## IoT Integration (MQTT)

### DigitalInputIoT Class

The `DigitalInputIoT` class provides MQTT integration for the DigitalInputCard, enabling remote monitoring and control.

**Important**: You should not instantiate this class directly. Use `ESPMegaIoT::registerCard()` instead.

### MQTT Topic Structure

All topics are relative to the base topic: `{base_topic}/{card_id}/`

#### Published Topics (Output)

| Topic | Payload | Description |
|-------|---------|-------------|
| `00` to `15` | `0` or `1` | Individual pin states (pin 0-15) |

#### Subscribed Topics (Input)

| Topic | Payload | Description |
|-------|---------|-------------|
| `publish_enable` | `0` or `1` | Enable/disable automatic publishing |
| `requeststate` | (any) | Request immediate publish of all pins |

### Topic Examples

Assuming base topic is `home/espmega` and card ID is `0`:

```
Published:
  home/espmega/0/00  -> "1"  (pin 0 is HIGH)
  home/espmega/0/01  -> "0"  (pin 1 is LOW)
  home/espmega/0/15  -> "1"  (pin 15 is HIGH)

Subscribed:
  home/espmega/0/publish_enable  <- "1"  (enable publishing)
  home/espmega/0/requeststate    <- ""   (request state update)
```

### API Reference - DigitalInputIoT

#### begin()
```cpp
bool begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic)
```
Initializes the IoT component.

**Parameters:**
- `card_id` - Card slot ID
- `card` - Pointer to DigitalInputCard
- `mqtt` - Pointer to MQTT client
- `base_topic` - Base MQTT topic

**Returns:**
- `true` if successful, `false` otherwise

#### setDigitalInputsPublishEnabled()
```cpp
void setDigitalInputsPublishEnabled(bool enabled)
```
Enables or disables automatic publishing of pin changes.

**Parameters:**
- `enabled` - `true` to enable, `false` to disable

#### publishDigitalInputs()
```cpp
void publishDigitalInputs()
```
Publishes the state of all 16 pins.

#### publishDigitalInput()
```cpp
void publishDigitalInput(uint8_t pin)
```
Publishes the state of a specific pin.

**Parameters:**
- `pin` - Pin number (0-15)

### Automatic Publishing

When enabled (default), the IoT component automatically publishes pin state changes:

1. Pin state changes
2. Debounce period passes
3. Callback triggered
4. State published to MQTT

### Message Flow Diagram

```
Input Change → Debounce → Callback → MQTT Publish
     ↓                                    ↓
Physical Pin              Topic: {base}/{card_id}/{pin}
                          Payload: "0" or "1"
```

### Integration Example

```cpp
#include <ESPMegaIoT.hpp>
#include <DigitalInputCard.hpp>

ESPMegaIoT iot;
DigitalInputCard inputCard(0x20, 0x21);

void setup() {
    // Initialize card
    inputCard.begin();

    // Initialize IoT (this automatically creates DigitalInputIoT)
    iot.begin();

    // Register the card (creates and manages DigitalInputIoT internally)
    iot.registerCard(0, &inputCard);
}

void loop() {
    iot.loop();
}
```

## RTU Integration

### DigitalInputRTU Class

The `DigitalInputRTU` (Remote Terminal Unit) class allows you to read digital inputs from a remote ESPMega device over MQTT. This enables distributed systems where one ESPMega can monitor inputs from another.

### Use Cases
- Multi-location monitoring
- Distributed control systems
- Remote sensor reading
- System redundancy

### Architecture

```
Remote Device                         Local Device
┌─────────────────┐                  ┌─────────────────┐
│ DigitalInputCard│ ───MQTT───────>  │DigitalInputRTU  │
│     + IoT       │                  │                 │
└─────────────────┘                  └─────────────────┘
   (Publisher)                          (Subscriber)
```

### API Reference - DigitalInputRTU

#### Constructor
```cpp
DigitalInputRTU()
```
Creates a new RTU instance.

#### begin()
```cpp
void begin(char* remoteBaseTopic, uint8_t remote_card_slot, ESPMegaIoT* iot)
```
Initializes the RTU connection.

**Parameters:**
- `remoteBaseTopic` - Base MQTT topic of the remote device
- `remote_card_slot` - Card slot number on remote device (0-99)
- `iot` - Pointer to local ESPMegaIoT instance

#### subscribe()
```cpp
void subscribe()
```
Subscribes to all pin topics from the remote card.

#### digitalRead()
```cpp
bool digitalRead(uint8_t pin)
```
Reads the cached state of a remote pin.

**Parameters:**
- `pin` - Pin number (0-15)

**Returns:**
- Cached state of the remote pin

**Note**: This reads from a local cache updated via MQTT. It does not directly communicate with the remote device.

#### registerCallback()
```cpp
uint8_t registerCallback(std::function<void(uint8_t, bool)> callback)
```
Registers a callback for remote pin changes.

**Parameters:**
- `callback` - Function to call when remote pin changes

**Returns:**
- Handler ID

#### unregisterCallback()
```cpp
void unregisterCallback(uint8_t handler)
```
Unregisters a callback.

**Parameters:**
- `handler` - Handler ID from `registerCallback()`

### RTU Topic Structure

The RTU subscribes to topics in this format:
```
{remoteBaseTopic}/{card_slot}/{pin}
```

Example:
```
Remote base topic: "building/floor1/room1"
Card slot: 0
Pin: 5

Topic: "building/floor1/room1/00/05"
```

### RTU Example

```cpp
#include <ESPMegaIoT.hpp>
#include <DigitalInputRTU.hpp>

ESPMegaIoT iot;
DigitalInputRTU remoteInputs;

void onRemoteChange(uint8_t pin, bool state) {
    Serial.printf("Remote pin %d changed to %d\n", pin, state);
}

void setup() {
    Serial.begin(115200);

    // Initialize IoT
    iot.begin();

    // Initialize RTU to monitor remote device
    remoteInputs.begin("remote/device", 0, &iot);
    remoteInputs.subscribe();

    // Register callback for remote changes
    remoteInputs.registerCallback(onRemoteChange);
}

void loop() {
    iot.loop();

    // Read remote pin states
    bool remoteDoorOpen = remoteInputs.digitalRead(0);
    if (remoteDoorOpen) {
        Serial.println("Remote door is open!");
    }

    delay(1000);
}
```

### RTU Limitations

1. **No direct refresh**: `digitalRead()` returns cached values, not real-time
2. **Network dependent**: Updates only occur when MQTT messages arrive
3. **Latency**: State changes are subject to network latency
4. **No debouncing**: Debouncing must be configured on the remote device

## FRAM Usage

The DigitalInputCard **does not directly use FRAM** (Ferroelectric RAM) for storing configuration or state.

However, the `DigitalInputIoT.hpp` file includes the `FRAM.h` header, which suggests that FRAM may be used by the parent `IoTComponent` class or the ESPMegaIoT system for:

- Storing MQTT configuration
- Preserving network settings
- Saving IoT component states

If you need to persist digital input card configuration (such as debounce times or pin mappings) across power cycles, you would need to implement this yourself using the ESPMega FRAM interface.

### Example: Saving Pin Configuration to FRAM

```cpp
// Pseudo-code example (actual FRAM API may differ)
#include <FRAM.h>

#define FRAM_INPUT_CONFIG_ADDR 0x100

void saveDebounceConfig() {
    for (int i = 0; i < 16; i++) {
        uint32_t debounce = inputCard.getDebounceTime(i); // Not a real method
        FRAM.write(FRAM_INPUT_CONFIG_ADDR + (i * 4), debounce);
    }
}

void loadDebounceConfig() {
    for (int i = 0; i < 16; i++) {
        uint32_t debounce;
        FRAM.read(FRAM_INPUT_CONFIG_ADDR + (i * 4), debounce);
        inputCard.setDebounceTime(i, debounce);
    }
}
```

**Note**: The above is illustrative. Consult the ESPMegaPRO3 FRAM documentation for the actual API.

## Code Examples

### Example 1: Basic Reading

See: `/home/user/ESPMegaPRO3-library/examples/DigitalInput/basic_reading/basic_reading.ino`

This example demonstrates:
- Initializing the card
- Reading individual pins
- Reading bank buffers
- Polling in the main loop

### Example 2: Using Callbacks

See: `/home/user/ESPMegaPRO3-library/examples/DigitalInput/with_callbacks/with_callbacks.ino`

This example demonstrates:
- Registering callback functions
- Event-driven programming
- Handling multiple callbacks
- Using lambda functions

### Example 3: Debouncing

See: `/home/user/ESPMegaPRO3-library/examples/DigitalInput/debouncing/debouncing.ino`

This example demonstrates:
- Configuring debounce times
- Different debounce settings for different pins
- Visualizing debounce behavior
- Best practices for mechanical switches

### Example 4: MQTT Integration

See: `/home/user/ESPMegaPRO3-library/examples/DigitalInput/mqtt_integration/mqtt_integration.ino`

This example demonstrates:
- Full IoT integration
- MQTT publishing and subscribing
- Remote monitoring
- Integration with ESPMegaIoT

## Troubleshooting

### Card Not Initializing

**Symptom**: `begin()` returns `false`

**Possible Causes**:
1. I2C address conflict
2. Wiring issues
3. Power supply problems
4. Incorrect DIP switch settings

**Solutions**:
- Use I2C scanner to verify addresses
- Check I2C wiring (SDA, SCL, GND, VCC)
- Verify DIP switch configuration
- Check power supply voltage

### False Triggers

**Symptom**: Callbacks fire multiple times for single event

**Possible Causes**:
1. Insufficient debounce time
2. Noisy input signal
3. Poor grounding

**Solutions**:
- Increase debounce time: `setDebounceTime(pin, 100)`
- Add hardware filtering (capacitor)
- Improve grounding

### No Callbacks Firing

**Symptom**: Pin states change but callbacks don't execute

**Possible Causes**:
1. `loop()` not being called
2. Callback not registered
3. Card not initialized

**Solutions**:
- Ensure `loop()` is called regularly
- Verify callback registration returns valid handler
- Check `getStatus()` returns `true`

### MQTT Not Publishing

**Symptom**: Pin changes don't appear on MQTT

**Possible Causes**:
1. Publishing disabled
2. MQTT not connected
3. Card not registered with IoT

**Solutions**:
- Enable publishing: `setDigitalInputsPublishEnabled(true)`
- Verify MQTT connection
- Check card registration with `ESPMegaIoT::registerCard()`

## Advanced Topics

### Interrupt-Based Reading

The current implementation uses polling (`loop()`). For ultra-low latency, consider:

1. Use PCF8574 interrupt pin
2. Attach ISR to interrupt pin
3. Read inputs in ISR or set flag

**Note**: Be cautious with ISRs and I2C communication.

### Multi-Card Systems

You can use multiple DigitalInputCards:

```cpp
DigitalInputCard card1(0x20, 0x21);  // First card
DigitalInputCard card2(0x22, 0x23);  // Second card

void setup() {
    card1.begin();
    card2.begin();
}

void loop() {
    card1.loop();
    card2.loop();
}
```

Ensure unique I2C addresses for each card.

### Performance Considerations

- **I2C Speed**: Each read operation requires I2C transaction (~100-400 kHz)
- **Loop Frequency**: Calling `loop()` faster than debounce time wastes CPU
- **Callback Complexity**: Keep callbacks short to avoid blocking
- **Buffer Reading**: Use `getInputBufferA/B()` for reading multiple pins efficiently

### Pin Inversion

If your hardware has inverted logic (active-low), you can:

1. Invert in software:
```cpp
bool state = !inputCard.digitalRead(pin);
```

2. Or wrap in a custom class:
```cpp
class InvertedInput {
    DigitalInputCard* card;
public:
    bool read(uint8_t pin) {
        return !card->digitalRead(pin);
    }
};
```

## Conclusion

The DigitalInputCard provides a robust, feature-rich solution for reading digital inputs in ESPMegaPRO3 systems. With built-in debouncing, flexible callbacks, pin mapping, and IoT integration, it's suitable for a wide range of applications from simple button reading to complex distributed monitoring systems.

For more examples and updates, visit the [ESPMegaPRO3 GitHub repository](https://github.com/SiwatINC/ESPMegaPRO3-library).
