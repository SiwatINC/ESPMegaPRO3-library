# DigitalOutputCard Documentation

## Table of Contents
1. [Overview](#overview)
2. [Hardware Specifications](#hardware-specifications)
3. [PWM vs Digital Mode](#pwm-vs-digital-mode)
4. [State vs Value](#state-vs-value)
5. [API Reference](#api-reference)
6. [Callback System](#callback-system)
7. [FRAM Persistence](#fram-persistence)
8. [IoT Integration (MQTT)](#iot-integration-mqtt)
9. [RTU Integration](#rtu-integration)
10. [Code Examples](#code-examples)

---

## Overview

The **DigitalOutputCard** is an expansion card for the ESPMegaPRO platform that provides 16 PWM-capable digital outputs. Each output is a 12V push-pull output suitable for controlling LEDs, small motors, solenoids, and other 12V loads.

### Key Features
- 16 independent PWM-capable outputs
- 12-bit PWM resolution (0-4095)
- 12V push-pull output topology
- Individual state and value control
- FRAM persistence for power-loss recovery
- MQTT integration via DigitalOutputIoT
- Remote control via DigitalOutputRTU
- Callback system for change notifications
- Pin remapping support

### Class Hierarchy
```
ExpansionCard (base class)
    └── DigitalOutputCard
```

---

## Hardware Specifications

### Electrical Characteristics

| Parameter | Specification |
|-----------|---------------|
| Output Voltage | 12V DC |
| Output Topology | Push-Pull |
| PWM Resolution | 12-bit (0-4095) |
| Number of Outputs | 16 |
| Current per Pin | 0.6A maximum |
| Current per Group | 1.2A maximum |
| Number of Groups | 4 groups of 4 outputs |

### Output Groups

The 16 outputs are divided into 4 groups with shared current limits:

- **Group 0**: Pins 0-3 (total 1.2A max)
- **Group 1**: Pins 4-7 (total 1.2A max)
- **Group 2**: Pins 8-11 (total 1.2A max)
- **Group 3**: Pins 12-15 (total 1.2A max)

### Important Warnings

1. **Per-Pin Current**: Each pin can handle up to 0.6A
2. **Group Current**: The sum of all currents in a group must not exceed 1.2A
3. **Heat Dissipation**: High current loads may require adequate ventilation
4. **Voltage**: Outputs are designed for 12V loads only

### Example Current Calculations

**Safe Configuration:**
- Group 0: Pin 0 (0.3A) + Pin 1 (0.4A) + Pin 2 (0.2A) + Pin 3 (0.1A) = 1.0A ✓

**Unsafe Configuration:**
- Group 0: Pin 0 (0.6A) + Pin 1 (0.6A) + Pin 2 (0.3A) = 1.5A ✗ (exceeds 1.2A group limit)

---

## PWM vs Digital Mode

The DigitalOutputCard supports both digital (ON/OFF) and PWM (analog) modes seamlessly.

### Digital Mode

In digital mode, outputs are either fully ON (12V) or fully OFF (0V).

```cpp
// Turn output fully ON
card.digitalWrite(0, HIGH);  // Sets value to 4095 internally

// Turn output fully OFF
card.digitalWrite(0, LOW);   // Sets value to 0 internally
```

### PWM Mode

PWM mode allows variable output levels for dimming LEDs, controlling motor speed, etc.

```cpp
// 25% brightness/speed
card.analogWrite(0, 1024);   // 1024/4095 ≈ 25%

// 50% brightness/speed
card.analogWrite(0, 2048);   // 2048/4095 ≈ 50%

// 75% brightness/speed
card.analogWrite(0, 3072);   // 3072/4095 ≈ 75%

// 100% brightness/speed
card.analogWrite(0, 4095);   // Full output
```

### PWM Resolution

The card uses a PCA9685 PWM driver chip providing:
- **12-bit resolution** (4096 steps: 0-4095)
- **Frequency**: Configurable via PCA9685 driver
- **Smooth dimming** for LED applications
- **Precise control** for motor speed applications

---

## State vs Value

The DigitalOutputCard maintains two separate properties for each pin: **state** and **value**.

### State (bool)

The **state** represents whether the output is enabled or disabled:
- `true` (1): Output is enabled
- `false` (0): Output is disabled

### Value (uint16_t)

The **value** represents the PWM duty cycle when enabled:
- Range: `0` to `4095`
- `0`: 0% duty cycle (OFF)
- `2048`: 50% duty cycle
- `4095`: 100% duty cycle (fully ON)

### How State and Value Interact

The **actual output** is calculated as:
```
Actual Output = State × Value
```

| State | Value | Actual Output | Result |
|-------|-------|---------------|--------|
| 0 (OFF) | 4095 | 0 × 4095 = 0 | OFF |
| 1 (ON) | 4095 | 1 × 4095 = 4095 | Fully ON |
| 1 (ON) | 2048 | 1 × 2048 = 2048 | 50% brightness |
| 0 (OFF) | 2048 | 0 × 2048 = 0 | OFF |

### Use Cases

**Scenario 1: Light Switch with Dimmer**

You want a light that can be turned ON/OFF while remembering its brightness level:

```cpp
// Set brightness to 50%
card.setValue(0, 2048);  // Value = 2048 (50%)

// Turn ON (uses stored brightness)
card.setState(0, true);  // Output = 2048 (50% brightness)

// Turn OFF (brightness value preserved)
card.setState(0, false); // Output = 0, but value still = 2048

// Turn ON again (returns to 50% brightness)
card.setState(0, true);  // Output = 2048
```

**Scenario 2: Simple ON/OFF Control**

For simple ON/OFF control, use `digitalWrite`:

```cpp
card.digitalWrite(0, HIGH);  // Sets state=true, value=4095
card.digitalWrite(0, LOW);   // Sets state=false, value=0
```

**Scenario 3: PWM Control**

For PWM dimming without state tracking:

```cpp
card.analogWrite(0, 1024);   // Sets state=true, value=1024
card.analogWrite(0, 2048);   // Sets state=true, value=2048
card.analogWrite(0, 0);      // Sets state=false, value=0
```

---

## API Reference

### Constructors

#### DigitalOutputCard(uint8_t address)
Creates a new DigitalOutputCard with the specified I2C address.

**Parameters:**
- `address`: I2C address of the card (typically 0x40-0x5F)

**Example:**
```cpp
DigitalOutputCard card(0x40);
```

---

#### DigitalOutputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4)
Creates a new DigitalOutputCard using DIP switch configuration.

**Parameters:**
- `bit0` to `bit4`: DIP switch positions (true = ON, false = OFF)

**Note:** Bit 0 is leftmost on the DIP switch

**Example:**
```cpp
// DIP switch: ON-OFF-OFF-ON-OFF
DigitalOutputCard card(true, false, false, true, false);
```

---

### Initialization

#### bool begin()
Initializes the card and the PWM driver.

**Returns:**
- `true`: Initialization successful (always returns true as output card doesn't send ACK)

**Example:**
```cpp
if (card.begin()) {
    Serial.println("Card initialized");
}
```

---

### Digital Output Control

#### void digitalWrite(uint8_t pin, bool state)
Sets an output to HIGH (12V) or LOW (0V).

**Parameters:**
- `pin`: Pin number (0-15)
- `state`: `HIGH` (true) or `LOW` (false)

**Side Effects:**
- Sets state to the specified value
- Sets value to 4095 if HIGH, 0 if LOW
- Triggers change callbacks
- Auto-saves to FRAM if enabled

**Example:**
```cpp
card.digitalWrite(0, HIGH);  // Turn on pin 0
card.digitalWrite(1, LOW);   // Turn off pin 1
```

---

#### void analogWrite(uint8_t pin, uint16_t value)
Sets the PWM value of an output.

**Parameters:**
- `pin`: Pin number (0-15)
- `value`: PWM value (0-4095)

**Side Effects:**
- Sets value to the specified value (clamped to 4095)
- Sets state to true if value > 0, false if value == 0
- Triggers change callbacks
- Auto-saves to FRAM if enabled

**Example:**
```cpp
card.analogWrite(0, 2048);  // 50% brightness
card.analogWrite(1, 1024);  // 25% brightness
card.analogWrite(2, 4095);  // Full brightness
```

---

### State and Value Control

#### void setState(uint8_t pin, bool state)
Sets the state of a pin without changing its value.

**Parameters:**
- `pin`: Pin number (0-15)
- `state`: `true` (enabled) or `false` (disabled)

**Note:** Actual output = state × value

**Example:**
```cpp
card.setValue(0, 2048);   // Set brightness to 50%
card.setState(0, true);   // Turn on at 50%
card.setState(0, false);  // Turn off (brightness preserved)
card.setState(0, true);   // Turn on again at 50%
```

---

#### void setValue(uint8_t pin, uint16_t value)
Sets the value of a pin without changing its state.

**Parameters:**
- `pin`: Pin number (0-15)
- `value`: PWM value (0-4095)

**Note:** Actual output = state × value

**Example:**
```cpp
card.setState(0, true);   // Turn on
card.setValue(0, 1024);   // Dim to 25%
card.setValue(0, 2048);   // Dim to 50%
card.setValue(0, 4095);   // Brighten to 100%
```

---

#### bool getState(uint8_t pin)
Gets the current state of a pin.

**Parameters:**
- `pin`: Pin number (0-15)

**Returns:**
- `true`: Pin is enabled
- `false`: Pin is disabled

**Example:**
```cpp
if (card.getState(0)) {
    Serial.println("Pin 0 is ON");
}
```

---

#### uint16_t getValue(uint8_t pin)
Gets the current PWM value of a pin.

**Parameters:**
- `pin`: Pin number (0-15)

**Returns:**
- PWM value (0-4095)

**Example:**
```cpp
uint16_t brightness = card.getValue(0);
Serial.printf("Brightness: %d\n", brightness);
```

---

#### void toggleState(uint8_t pin)
Toggles the state of a pin.

**Parameters:**
- `pin`: Pin number (0-15)

**Example:**
```cpp
card.toggleState(0);  // If ON, turn OFF; if OFF, turn ON
```

---

### Callback System

#### uint8_t registerChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback)
Registers a callback function to be called when any pin changes.

**Parameters:**
- `callback`: Function with signature `void callback(uint8_t pin, bool state, uint16_t value)`

**Returns:**
- Handler ID for unregistering the callback

**Example:**
```cpp
void onChange(uint8_t pin, bool state, uint16_t value) {
    Serial.printf("Pin %d changed: state=%d, value=%d\n", pin, state, value);
}

uint8_t handler = card.registerChangeCallback(onChange);
```

---

#### void unregisterChangeCallback(uint8_t handler)
Unregisters a previously registered callback.

**Parameters:**
- `handler`: Handler ID returned by registerChangeCallback

**Example:**
```cpp
card.unregisterChangeCallback(handler);
```

---

### Pin Mapping

#### void loadPinMap(uint8_t pinMap[16])
Loads a custom pin mapping.

**Parameters:**
- `pinMap`: Array of 16 elements mapping virtual pins to physical pins

**Use Case:** Remap pin numbers to match your project's wiring

**Example:**
```cpp
uint8_t customMap[16] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
card.loadPinMap(customMap);  // Reverse pin order
```

---

### FRAM Persistence

#### void bindFRAM(FRAM *fram, uint16_t address)
Binds a FRAM object to the card for persistent storage.

**Parameters:**
- `fram`: Pointer to FRAM object
- `address`: Starting address in FRAM (requires 34 bytes)

**Memory Layout:**
- Bytes 0-1: State bitmap (16 bits)
- Bytes 2-33: Values (16 × 2 bytes)

**Example:**
```cpp
FRAM fram;
card.bindFRAM(&fram, 0x0000);  // Use FRAM starting at address 0
```

---

#### void saveToFRAM()
Saves all states and values to FRAM.

**Example:**
```cpp
card.saveToFRAM();
```

---

#### void loadFromFRAM()
Loads all states and values from FRAM.

**Example:**
```cpp
card.loadFromFRAM();  // Restore previous state after power loss
```

---

#### void setAutoSaveToFRAM(bool autoSave)
Enables or disables automatic saving to FRAM on every change.

**Parameters:**
- `autoSave`: `true` to enable, `false` to disable

**Warning:** Auto-save increases FRAM wear. Use with caution for frequently changing outputs.

**Example:**
```cpp
card.setAutoSaveToFRAM(true);   // Auto-save enabled
card.digitalWrite(0, HIGH);      // Automatically saved to FRAM
```

---

#### void saveStateToFRAM()
Saves only the state bitmap to FRAM.

**Example:**
```cpp
card.saveStateToFRAM();
```

---

#### void savePinValueToFRAM(uint8_t pin)
Saves a single pin's value to FRAM.

**Parameters:**
- `pin`: Pin number (0-15)

**Example:**
```cpp
card.savePinValueToFRAM(0);  // Save only pin 0's value
```

---

### Utility Methods

#### void loop()
Loop function (not used, required by ExpansionCard interface).

---

#### uint8_t getType()
Gets the card type.

**Returns:**
- `CARD_TYPE_DIGITAL_OUTPUT` (0x00)

---

## Callback System

The callback system allows you to be notified whenever a pin's state or value changes.

### Callback Signature

```cpp
void callbackFunction(uint8_t pin, bool state, uint16_t value)
```

**Parameters:**
- `pin`: The pin that changed (0-15)
- `state`: New state of the pin
- `value`: New value of the pin

### Registering Callbacks

```cpp
void myCallback(uint8_t pin, bool state, uint16_t value) {
    Serial.printf("Pin %d: state=%d, value=%d\n", pin, state, value);
}

void setup() {
    uint8_t handler = card.registerChangeCallback(myCallback);
}
```

### Multiple Callbacks

You can register multiple callbacks:

```cpp
uint8_t handler1 = card.registerChangeCallback(callback1);
uint8_t handler2 = card.registerChangeCallback(callback2);
uint8_t handler3 = card.registerChangeCallback(callback3);
```

### Unregistering Callbacks

```cpp
card.unregisterChangeCallback(handler1);
```

### Lambda Callbacks

```cpp
card.registerChangeCallback([](uint8_t pin, bool state, uint16_t value) {
    if (pin == 0) {
        Serial.println("Pin 0 changed!");
    }
});
```

### When Callbacks Are Triggered

Callbacks are triggered by:
- `digitalWrite()`
- `analogWrite()`
- `setState()`
- `setValue()`
- `loadFromFRAM()` (called for each pin)
- `unpackStates()` (internal method)

---

## FRAM Persistence

FRAM (Ferroelectric RAM) provides non-volatile storage for pin states and values, allowing the card to restore its previous state after power loss.

### Memory Layout

The card uses 34 bytes of FRAM:

| Address | Size | Content |
|---------|------|---------|
| 0-1 | 2 bytes | State bitmap (16 bits, one per pin) |
| 2-3 | 2 bytes | Value for pin 0 |
| 4-5 | 2 bytes | Value for pin 1 |
| ... | ... | ... |
| 32-33 | 2 bytes | Value for pin 15 |

### Basic Usage

```cpp
#include <FRAM.h>
#include <DigitalOutputCard.hpp>

FRAM fram;
DigitalOutputCard card(0x40);

void setup() {
    fram.begin();
    card.begin();

    // Bind FRAM to card
    card.bindFRAM(&fram, 0x0000);

    // Load previous state
    card.loadFromFRAM();
}
```

### Manual Save/Load

```cpp
// Change some outputs
card.digitalWrite(0, HIGH);
card.analogWrite(1, 2048);

// Save to FRAM
card.saveToFRAM();

// Later, after power cycle...
card.loadFromFRAM();  // Restore previous state
```

### Auto-Save Mode

```cpp
void setup() {
    card.bindFRAM(&fram, 0x0000);
    card.setAutoSaveToFRAM(true);  // Enable auto-save

    // Every change is now automatically saved
    card.digitalWrite(0, HIGH);    // Saved
    card.analogWrite(1, 2048);     // Saved
}
```

### Selective Saving

```cpp
// Save only states
card.saveStateToFRAM();

// Save only one pin's value
card.savePinValueToFRAM(5);

// Save everything
card.saveToFRAM();
```

### FRAM Wear Considerations

FRAM has excellent endurance (10^14 read/write cycles), but you should still consider:

1. **Auto-Save**: Use only for infrequently changing outputs
2. **Rapid Changes**: For rapidly changing PWM, disable auto-save and save periodically
3. **Critical Changes**: Use `saveStateToFRAM()` for important state changes only

**Good Practice:**
```cpp
// Disable auto-save for rapid PWM changes
card.setAutoSaveToFRAM(false);

// Animate brightness
for (int i = 0; i <= 4095; i += 100) {
    card.analogWrite(0, i);
    delay(50);
}

// Save final value
card.savePinValueToFRAM(0);
```

---

## IoT Integration (MQTT)

The **DigitalOutputIoT** class provides MQTT integration for remote control and monitoring.

### MQTT Topic Structure

All topics are relative to the card's base topic: `<base_topic>/<card_id>/`

#### Control Topics (Subscribe)

| Topic Pattern | Payload | Description |
|---------------|---------|-------------|
| `<pin>/set/state` | `0` or `1` | Set pin state |
| `<pin>/set/value` | `0-4095` | Set pin PWM value |
| `requeststate` | N/A | Request publishing all states |
| `publish_enable` | `0` or `1` | Enable/disable auto-publishing |

**Note:** `<pin>` is always 2 digits, zero-padded (e.g., `00`, `01`, `15`)

#### Status Topics (Publish)

| Topic Pattern | Payload | Description |
|---------------|---------|-------------|
| `<pin>/state` | `0` or `1` | Current pin state |
| `<pin>/value` | `0-4095` | Current pin PWM value |

### MQTT Examples

#### Setting Pin State via MQTT

**Topic:** `espmega/dout/00/set/state`
**Payload:** `1`
**Result:** Pin 0 turns ON

**Topic:** `espmega/dout/00/set/state`
**Payload:** `0`
**Result:** Pin 0 turns OFF

#### Setting PWM Value via MQTT

**Topic:** `espmega/dout/00/set/value`
**Payload:** `2048`
**Result:** Pin 0 set to 50% brightness

**Topic:** `espmega/dout/05/set/value`
**Payload:** `4095`
**Result:** Pin 5 set to 100% brightness

#### Requesting State

**Topic:** `espmega/dout/requeststate`
**Payload:** (empty)
**Result:** All 16 pins publish their state and value

#### Enable/Disable Publishing

**Topic:** `espmega/dout/publish_enable`
**Payload:** `1`
**Result:** Auto-publishing enabled

**Topic:** `espmega/dout/publish_enable`
**Payload:** `0`
**Result:** Auto-publishing disabled

### Auto-Publishing

When auto-publishing is enabled (default), the card automatically publishes state and value whenever a pin changes:

```
# User changes pin 3 to 75% brightness
PUBLISH: espmega/dout/03/state -> "1"
PUBLISH: espmega/dout/03/value -> "3072"
```

### Integration Example

```cpp
#include <DigitalOutputCard.hpp>
#include <DigitalOutputIoT.hpp>

DigitalOutputCard card(0x40);
DigitalOutputIoT iot;
PubSubClient mqtt;

void setup() {
    card.begin();

    // Initialize IoT component
    iot.begin(0, &card, &mqtt, "espmega/dout");

    // Subscribe to MQTT topics
    iot.subscribe();
}

void loop() {
    mqtt.loop();  // Handle MQTT
}
```

### Full Topic Examples

Assuming base topic is `home/automation/outputs/card0`:

| Action | Topic | Payload |
|--------|-------|---------|
| Turn on pin 0 | `home/automation/outputs/card0/00/set/state` | `1` |
| Set pin 5 to 25% | `home/automation/outputs/card0/05/set/value` | `1024` |
| Get all states | `home/automation/outputs/card0/requeststate` | - |

---

## RTU Integration

The **DigitalOutputRTU** class allows you to control a remote DigitalOutputCard over MQTT as if it were local.

### Use Case

Control a DigitalOutputCard on another ESPMegaPRO device via MQTT:

```
[Device A] DigitalOutputRTU ----MQTT----> [Device B] DigitalOutputCard + DigitalOutputIoT
```

### RTU API

The RTU class provides the same API as DigitalOutputCard:

```cpp
DigitalOutputRTU rtu;

void setup() {
    rtu.begin("remote/device/outputs", 0, &iot);
    rtu.subscribe();
}

void loop() {
    // Control remote card as if it were local
    rtu.digitalWrite(0, HIGH);
    rtu.analogWrite(1, 2048);
    rtu.setState(2, true);
    rtu.setValue(3, 1024);

    // Read remote state
    bool state = rtu.getState(0);
    uint16_t value = rtu.getValue(1);
}
```

### RTU Topic Format

The RTU publishes to:
```
<base_topic>/<card_slot>/<pin>/set/state
<base_topic>/<card_slot>/<pin>/set/value
```

And subscribes to:
```
<base_topic>/<card_slot>/<pin>/state
<base_topic>/<card_slot>/<pin>/value
```

### RTU Callbacks

Like the local card, RTU supports callbacks when remote state changes:

```cpp
void onRemoteChange(uint8_t pin, bool state, uint16_t value) {
    Serial.printf("Remote pin %d changed: %d, %d\n", pin, state, value);
}

void setup() {
    rtu.registerChangeCallback(onRemoteChange);
}
```

### RTU Limitations

- **Latency**: Commands are subject to network latency
- **Reliability**: Depends on MQTT broker availability
- **State Sync**: State is updated only when remote device publishes
- **No FRAM**: FRAM operations are not available via RTU

---

## Code Examples

### Example 1: Basic Digital Output

```cpp
#include <DigitalOutputCard.hpp>

DigitalOutputCard card(0x40);

void setup() {
    Serial.begin(115200);
    card.begin();

    // Turn on pins 0-3
    for (int i = 0; i < 4; i++) {
        card.digitalWrite(i, HIGH);
    }
}

void loop() {
    // Blink pin 0
    card.digitalWrite(0, HIGH);
    delay(1000);
    card.digitalWrite(0, LOW);
    delay(1000);
}
```

### Example 2: PWM Dimming

```cpp
#include <DigitalOutputCard.hpp>

DigitalOutputCard card(0x40);

void setup() {
    card.begin();
}

void loop() {
    // Fade in
    for (int brightness = 0; brightness <= 4095; brightness += 50) {
        card.analogWrite(0, brightness);
        delay(10);
    }

    // Fade out
    for (int brightness = 4095; brightness >= 0; brightness -= 50) {
        card.analogWrite(0, brightness);
        delay(10);
    }
}
```

### Example 3: State/Value Control

```cpp
#include <DigitalOutputCard.hpp>

DigitalOutputCard card(0x40);

void setup() {
    card.begin();

    // Set brightness level
    card.setValue(0, 2048);  // 50% when ON

    // Turn OFF (brightness preserved)
    card.setState(0, false);
}

void loop() {
    // Toggle ON/OFF every second (maintains 50% brightness when ON)
    card.toggleState(0);
    delay(1000);
}
```

### Example 4: Callbacks

```cpp
#include <DigitalOutputCard.hpp>

DigitalOutputCard card(0x40);

void onChange(uint8_t pin, bool state, uint16_t value) {
    Serial.printf("Pin %d changed: state=%d, value=%d\n", pin, state, value);
}

void setup() {
    Serial.begin(115200);
    card.begin();

    card.registerChangeCallback(onChange);

    card.digitalWrite(0, HIGH);   // Triggers callback
    card.analogWrite(1, 2048);    // Triggers callback
}

void loop() {
    // Do nothing
}
```

### Example 5: FRAM Persistence

```cpp
#include <DigitalOutputCard.hpp>
#include <FRAM.h>

DigitalOutputCard card(0x40);
FRAM fram;

void setup() {
    fram.begin();
    card.begin();

    // Bind FRAM
    card.bindFRAM(&fram, 0x0000);

    // Load previous state
    card.loadFromFRAM();

    // Enable auto-save
    card.setAutoSaveToFRAM(true);
}

void loop() {
    // All changes are automatically saved
    card.digitalWrite(0, HIGH);
    delay(5000);
    card.digitalWrite(0, LOW);
    delay(5000);
}
```

### Example 6: MQTT Control

```cpp
#include <DigitalOutputCard.hpp>
#include <DigitalOutputIoT.hpp>
#include <PubSubClient.h>
#include <WiFi.h>

DigitalOutputCard card(0x40);
DigitalOutputIoT iot;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup() {
    WiFi.begin("SSID", "password");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    mqtt.setServer("mqtt.example.com", 1883);
    mqtt.connect("espmega");

    card.begin();
    iot.begin(0, &card, &mqtt, "home/outputs");
    iot.subscribe();
}

void loop() {
    mqtt.loop();
}
```

### Example 7: Pattern Generation

```cpp
#include <DigitalOutputCard.hpp>

DigitalOutputCard card(0x40);

void setup() {
    card.begin();
}

void loop() {
    // Knight Rider pattern
    for (int i = 0; i < 16; i++) {
        card.digitalWrite(i, HIGH);
        delay(50);
        card.digitalWrite(i, LOW);
    }
    for (int i = 14; i > 0; i--) {
        card.digitalWrite(i, HIGH);
        delay(50);
        card.digitalWrite(i, LOW);
    }
}
```

---

## Troubleshooting

### Outputs Not Working

1. **Check I2C Address**: Verify DIP switch settings match your code
2. **Check Power**: Ensure 12V supply is connected
3. **Check Current**: Verify group current limits are not exceeded
4. **Check Wiring**: Ensure load is connected correctly

### PWM Not Smooth

1. **Increase Steps**: Use smaller increments for smoother transitions
2. **Adjust Delay**: Reduce delay between steps
3. **Check Load**: Some loads (e.g., motors) may not respond smoothly to PWM

### FRAM Not Persisting

1. **Check Binding**: Ensure `bindFRAM()` was called
2. **Check Saving**: Call `saveToFRAM()` before power loss
3. **Check Address**: Ensure FRAM address doesn't overlap other cards

### MQTT Not Working

1. **Check Subscription**: Ensure `iot.subscribe()` was called
2. **Check Topics**: Verify topic format matches documentation
3. **Check Connection**: Ensure MQTT broker is connected
4. **Enable Publishing**: Send `publish_enable` with payload `1`

---

## Best Practices

1. **Current Management**: Always calculate group current before connecting loads
2. **FRAM Usage**: Use auto-save sparingly; prefer manual save for frequent changes
3. **PWM Frequency**: For LEDs, higher frequency reduces flicker
4. **Error Handling**: Always check return values and handle errors
5. **Documentation**: Document your pin assignments for future reference
6. **Testing**: Test with low-current loads before connecting high-current devices

---

## License

This documentation is part of the ESPMegaPRO library.

---

## Support

For issues, questions, or contributions, please visit:
https://github.com/SiwatINC/ESPMegaPRO3-library
