# Internal Display Documentation

## Table of Contents
1. [Overview](#overview)
2. [Class Hierarchy](#class-hierarchy)
3. [ESPMegaDisplay Base Class](#espmegadisplay-base-class)
4. [InternalDisplay Class](#internaldisplay-class)
5. [Display Pages](#display-pages)
6. [Binding Cards to Display](#binding-cards-to-display)
7. [Touch Interface](#touch-interface)
8. [Display Protocol](#display-protocol)
9. [Creating Custom Displays](#creating-custom-displays)
10. [Display OTA Updates](#display-ota-updates)
11. [Code Examples](#code-examples)

---

## Overview

The ESPMegaPRO display system provides a powerful interface for monitoring and controlling your ESPMegaPRO device. It consists of a base class (`ESPMegaDisplay`) for UART-controlled displays and a concrete implementation (`InternalDisplay`) designed for the 3.5" TFT LCD with resistive touch screen that comes with some ESPMegaPRO chassis.

### Key Features
- Real-time status monitoring (network, MQTT, time)
- Digital input visualization
- PWM output control and adjustment
- Climate/AC control interface
- Network and MQTT configuration
- OTA display firmware updates
- Event-driven architecture with callbacks
- Thread-safe serial communication

---

## Class Hierarchy

```
ESPMegaDisplay (Base Class)
    └── InternalDisplay (Concrete Implementation)
    └── YourCustomDisplay (Your Implementation)
```

---

## ESPMegaDisplay Base Class

The `ESPMegaDisplay` class provides the foundation for all display implementations. It handles low-level serial communication and provides a standard API for display control.

### Constructor

```cpp
ESPMegaDisplay(HardwareSerial *displayAdapter, uint32_t baudRate,
               uint32_t uploadBaudRate, uint8_t txPin, uint8_t rxPin)
```

**Parameters:**
- `displayAdapter`: Pointer to the HardwareSerial object (e.g., `&Serial`)
- `baudRate`: Communication baud rate (typically 115200)
- `uploadBaudRate`: Baud rate for OTA updates (typically 921600)
- `txPin`: TX pin number
- `rxPin`: RX pin number

### Core Methods

#### Initialization and Control

##### `void begin()`
Initializes the display adapter and resets the display.

**Usage:**
```cpp
display->begin();
```

##### `void loop()`
Must be called in the main loop to process incoming serial data.

**Usage:**
```cpp
void loop() {
    display->loop();
}
```

##### `void reset()`
Sends a reset command to the display.

**Usage:**
```cpp
display->reset();
```

##### `void jumpToPage(int page)`
Switches to the specified page number.

**Parameters:**
- `page`: Page number (0-255)

**Usage:**
```cpp
display->jumpToPage(1); // Jump to dashboard
```

##### `void setBrightness(int value)`
Sets the display brightness.

**Parameters:**
- `value`: Brightness level (0-100)

**Usage:**
```cpp
display->setBrightness(75);
```

##### `void setVolume(int value)`
Sets the display volume/beep level.

**Parameters:**
- `value`: Volume level (0-100)

**Usage:**
```cpp
display->setVolume(50);
```

#### Component Manipulation

##### `void setNumber(const char* component, int value)`
Sets a numeric value on a display component.

**Parameters:**
- `component`: Component name or property (e.g., "slider.val")
- `value`: Integer value to set

**Usage:**
```cpp
display->setNumber("temp.val", 25);
display->setNumber("slider.val", 50);
```

##### `void setString(const char* component, const char* value)`
Sets a string value on a display component.

**Parameters:**
- `component`: Component name or property (e.g., "status.txt")
- `value`: String value to set

**Usage:**
```cpp
display->setString("status.txt", "Connected");
display->setString("ip.txt", "192.168.1.100");
```

##### `uint32_t getNumber(const char* component)`
Gets a numeric value from a display component.

**Parameters:**
- `component`: Component name or property

**Returns:**
- `uint32_t`: The numeric value

**Warning:** This is a blocking function with timeout.

**Usage:**
```cpp
uint32_t sliderValue = display->getNumber("slider.val");
```

##### `const char* getString(const char* component)`
Gets a string value from a display component.

**Parameters:**
- `component`: Component name or property

**Returns:**
- `const char*`: The string value (must be freed after use)

**Warning:** This is a blocking function with timeout.

**Usage:**
```cpp
const char* text = display->getString("textbox.txt");
// Use the text...
free((void*)text); // Don't forget to free!
```

##### `bool getStringToBuffer(const char* component, char* buffer, uint8_t buffer_size)`
Gets a string value and stores it in a provided buffer.

**Parameters:**
- `component`: Component name or property
- `buffer`: Destination buffer
- `buffer_size`: Size of the buffer

**Returns:**
- `bool`: true if successful, false otherwise

**Usage:**
```cpp
char ipAddress[16];
if (display->getStringToBuffer("ip_set.txt", ipAddress, 16)) {
    Serial.println(ipAddress);
}
```

#### Serial Mutex Management

##### `bool takeSerialMutex()`
Acquires the serial communication mutex.

**Returns:**
- `bool`: true if mutex acquired, false if timeout

**Usage:**
```cpp
if (display->takeSerialMutex()) {
    // Direct serial operations
    displayAdapter->print("custom command");
    display->sendStopBytes();
    display->giveSerialMutex();
}
```

##### `void giveSerialMutex()`
Releases the serial communication mutex.

#### Callback Registration

##### `uint16_t registerTouchCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback)`
Registers a callback for touch events.

**Parameters:**
- `callback`: Function with signature `void(uint8_t page, uint8_t component, uint8_t type)`
  - `page`: Page number where touch occurred
  - `component`: Component ID that was touched
  - `type`: Touch type (0x01 = press, 0x00 = release)

**Returns:**
- `uint16_t`: Handle for later unregistration

**Usage:**
```cpp
auto touchCallback = [](uint8_t page, uint8_t component, uint8_t type) {
    Serial.printf("Touch: page=%d, component=%d, type=%d\n", page, component, type);
};
uint16_t handle = display->registerTouchCallback(touchCallback);
```

##### `void unregisterTouchCallback(uint16_t handle)`
Unregisters a touch callback.

**Parameters:**
- `handle`: Handle returned from registerTouchCallback

##### `uint16_t registerPageChangeCallback(std::function<void(uint8_t)> callback)`
Registers a callback for page change events.

**Parameters:**
- `callback`: Function with signature `void(uint8_t page)`

**Returns:**
- `uint16_t`: Handle for later unregistration

**Usage:**
```cpp
auto pageCallback = [](uint8_t page) {
    Serial.printf("Changed to page %d\n", page);
};
uint16_t handle = display->registerPageChangeCallback(pageCallback);
```

##### `void unregisterPageChangeCallback(uint16_t handle)`
Unregisters a page change callback.

##### `uint16_t registerPayloadCallback(std::function<void(uint8_t, uint8_t*, uint8_t)> callback)`
Registers a callback for custom payloads.

**Parameters:**
- `callback`: Function with signature `void(uint8_t type, uint8_t* payload, uint8_t length)`

**Returns:**
- `uint16_t`: Handle for later unregistration

**Usage:**
```cpp
auto payloadCallback = [](uint8_t type, uint8_t* payload, uint8_t length) {
    Serial.printf("Payload type 0x%02X, length %d\n", type, length);
};
uint16_t handle = display->registerPayloadCallback(payloadCallback);
```

##### `void unregisterPayloadCallback(uint16_t handle)`
Unregisters a payload callback.

#### OTA Update Methods

##### `bool beginUpdate(size_t size)`
Begins a display firmware update.

**Parameters:**
- `size`: Total size of the update file in bytes

**Returns:**
- `bool`: true if update initiated successfully

**Usage:**
```cpp
File updateFile = SD.open("/display.tft");
size_t fileSize = updateFile.size();
if (display->beginUpdate(fileSize)) {
    // Proceed with update
}
```

##### `bool writeUpdate(uint8_t* data, size_t size)`
Writes a chunk of update data to the display.

**Parameters:**
- `data`: Pointer to data buffer
- `size`: Size of data (max 4096 bytes per chunk)

**Returns:**
- `bool`: true if write successful

**Usage:**
```cpp
uint8_t buffer[4096];
size_t bytesRead = updateFile.read(buffer, sizeof(buffer));
if (!display->writeUpdate(buffer, bytesRead)) {
    Serial.println("Update write failed!");
}
```

##### `void endUpdate()`
Completes the display update and restarts the system.

##### `size_t getUpdateBytesWritten()`
Gets the number of bytes written during the current update.

**Returns:**
- `size_t`: Bytes written

---

## InternalDisplay Class

The `InternalDisplay` class extends `ESPMegaDisplay` to provide a complete user interface for the ESPMegaPRO system.

### Constructor

```cpp
InternalDisplay(HardwareSerial *displayAdapter)
```

**Parameters:**
- `displayAdapter`: Pointer to HardwareSerial object

**Note:** The constructor automatically sets the correct baud rates and pins for the internal display.

### Initialization

##### `void begin(ESPMegaIoT *iot, std::function<rtctime_t()> getRtcTime)`
Initializes the internal display with ESPMegaIoT integration.

**Parameters:**
- `iot`: Pointer to ESPMegaIoT object
- `getRtcTime`: Function that returns current RTC time

**Usage:**
```cpp
InternalDisplay display(&Serial);
display.begin(espmega.iot, []() { return espmega.rtc.getTime(); });
```

**Note:** You should use `ESPMegaPRO::enableInternalDisplay()` instead of calling this directly.

### Card Binding Methods

##### `void bindInputCard(DigitalInputCard *inputCard)`
Binds a digital input card to the display for visualization on the input page.

**Parameters:**
- `inputCard`: Pointer to DigitalInputCard

**Features:**
- Real-time input state visualization
- Automatic callback registration
- Handles card state changes

**Usage:**
```cpp
display->bindInputCard(&espmega.inputs);
```

##### `void unbindInputCard()`
Unbinds the currently bound input card.

##### `void bindOutputCard(DigitalOutputCard *outputCard)`
Binds a digital output card to the display for control and visualization.

**Parameters:**
- `outputCard`: Pointer to DigitalOutputCard

**Features:**
- PWM value display with bars
- State color indication
- PWM adjustment interface

**Usage:**
```cpp
display->bindOutputCard(&espmega.outputs);
```

##### `void unbindOutputCard()`
Unbinds the currently bound output card.

##### `void bindClimateCard(ClimateCard *climateCard)`
Binds a climate/AC card to the display for control and monitoring.

**Parameters:**
- `climateCard`: Pointer to ClimateCard

**Features:**
- Temperature control
- Mode selection (Off, Fan, Cool)
- Fan speed control (Auto, Low, Medium, High)
- Room temperature and humidity display (if sensor available)

**Assumptions:**
- Modes in order: [off, fan_only, cool]
- Fan speeds in order: [auto, low, medium, high]

**Usage:**
```cpp
display->bindClimateCard(&climateCard);
```

##### `void unbindClimateCard()`
Unbinds the currently bound climate card.

---

## Display Pages

The InternalDisplay implements multiple pages, each with a specific purpose:

### Page IDs

| Page ID | Name | Purpose |
|---------|------|---------|
| 0 | Boot Page | Displayed during system startup |
| 1 | Dashboard | Main status overview |
| 2 | Input Page | Digital input visualization |
| 3 | Output Page | PWM output control |
| 4 | AC Page | Climate control |
| 5 | PWM Adjustment | Detailed PWM control |
| 6 | Network Config | Network settings |
| 9 | OTA Page | Firmware updates |
| 10 | Climate Null Ptr | Error page (no climate card) |
| 11 | MQTT Config | MQTT settings |
| 12 | Input Null Ptr | Error page (no input card) |
| 13 | Output Null Ptr | Error page (no output card) |

### Boot Page (Page 0)

**Purpose:** Displays during system initialization

**Components:**
- `boot_state.txt`: Status text showing boot progress

**Usage:**
```cpp
// Displayed automatically during boot
// Status is updated via ESPMegaPRO initialization
```

### Dashboard Page (Page 1)

**Purpose:** Main overview of system status

**Components:**
- `hostname.txt`: Device hostname
- `ip_address.txt`: Current IP address
- `server_address.txt`: MQTT server address
- `status_txt.txt`: MQTT connection status ("BMS Managed" or "Standalone")
- `server.pic`: MQTT status icon (connected/disconnected)
- `lan.pic`: Network status icon (connected/disconnected)
- `time.txt`: Current time in 12-hour format

**Auto-refresh:**
- Status icons: Every 5 seconds
- Clock: Every 15 seconds

**Displayed Information:**
```
┌─────────────────────────────┐
│ [LAN] [MQTT]      12:30 PM  │
├─────────────────────────────┤
│ Hostname: espmega-001       │
│ IP: 192.168.1.100           │
│ MQTT: mqtt.example.com      │
│ Status: BMS Managed         │
└─────────────────────────────┘
```

### Input Page (Page 2)

**Purpose:** Visualize digital input states

**Requirements:**
- Input card must be bound with `bindInputCard()`

**Components:**
- `I0.val` to `I15.val`: Input state indicators (0 or 1)

**Features:**
- Real-time updates on input state changes
- Shows all 16 input channels
- Visual indicators for HIGH/LOW states

**Visualization:**
```
┌─────────────────────────────┐
│ Digital Inputs              │
├─────────────────────────────┤
│ I0  ●  I1  ○  I2  ●  I3  ○ │
│ I4  ○  I5  ●  I6  ○  I7  ● │
│ I8  ●  I9  ○  I10 ●  I11 ○ │
│ I12 ○  I13 ●  I14 ○  I15 ● │
└─────────────────────────────┘
● = HIGH, ○ = LOW
```

### Output Page (Page 3)

**Purpose:** Display and control PWM outputs

**Requirements:**
- Output card must be bound with `bindOutputCard()`

**Components:**
- `j0.val` to `j15.val`: PWM bar values (0-100%)
- `j0.ppic` to `j15.ppic`: Bar color (ON/OFF state)

**Features:**
- Real-time PWM value display as progress bars
- Color indication for ON/OFF state
- Shows all 16 output channels
- Tap to enter detailed adjustment mode

**Visualization:**
```
┌─────────────────────────────┐
│ PWM Outputs                 │
├─────────────────────────────┤
│ P0  [████████░░] 75%  [ON] │
│ P1  [██████████] 100% [ON] │
│ P2  [░░░░░░░░░░] 0%   [OFF]│
│ P3  [█████░░░░░] 50%  [ON] │
│ ...                         │
└─────────────────────────────┘
```

### AC Page (Page 4)

**Purpose:** Climate/Air Conditioner control

**Requirements:**
- Climate card must be bound with `bindClimateCard()`

**Components:**
- `temp.txt`: Target temperature display
- `roomtemp.txt`: Current room temperature
- `roomhumid.txt`: Current humidity (if sensor supports it)
- `mode_off.pic`, `mode_fan.pic`, `mode_cool.pic`: Mode buttons
- `fan_auto.pic`, `fan_low.pic`, `fan_mid.pic`, `fan_high.pic`: Fan speed buttons
- `b0` (component 17): Decrease temperature button
- `b1` (component 18): Increase temperature button

**Touch Components:**
- Component 4: Fan speed AUTO
- Component 5: Fan speed LOW
- Component 6: Fan speed MEDIUM
- Component 7: Fan speed HIGH
- Component 8: Mode COOL
- Component 9: Mode FAN ONLY
- Component 10: Mode OFF
- Component 17: Temperature DOWN
- Component 18: Temperature UP

**Features:**
- Temperature adjustment (± buttons)
- Mode selection (Off, Fan Only, Cool)
- Fan speed control (Auto, Low, Medium, High)
- Room temperature/humidity display
- Visual feedback for active settings

**Display Layout:**
```
┌─────────────────────────────┐
│ Air Conditioner             │
├─────────────────────────────┤
│ Set Temp:  [-]  25°C  [+]  │
│                             │
│ Mode:   [OFF] [FAN] [COOL] │
│ Fan:    [AUTO] [LOW]        │
│         [MED]  [HIGH]       │
│                             │
│ Room: 24°C   Humidity: 55% │
└─────────────────────────────┘
```

### PWM Adjustment Page (Page 5)

**Purpose:** Detailed control of a single PWM output

**Requirements:**
- Output card must be bound

**Components:**
- `pwm_id.txt`: Current PWM channel (e.g., "P0")
- `pwm_value.val`: Slider for PWM value (0-4095)
- `pwm_state.txt`: State text ("ON" or "OFF")

**Touch Components:**
- Component 1: PWM value slider
- Component 4: Toggle PWM state button
- Component 5: Previous channel (P-1)
- Component 6: Next channel (P+1)

**Features:**
- Fine-grained PWM control (0-4095)
- Channel selection with < > buttons
- State toggle (ON/OFF)
- Real-time value display

**Display Layout:**
```
┌─────────────────────────────┐
│ PWM Adjustment              │
├─────────────────────────────┤
│ Channel: [<]  P5  [>]      │
│                             │
│ Value:  [═════░░░░] 2048   │
│                             │
│ State:  [  ON  ]           │
└─────────────────────────────┘
```

### Network Config Page (Page 6)

**Purpose:** Configure network settings

**Components:**
- `ip_set.txt`: IP address input
- `netmask_set.txt`: Subnet mask input
- `gateway_set.txt`: Gateway address input
- `dns_set.txt`: DNS server input
- `host_set.txt`: Hostname input
- Component 7: Save button

**Touch Handling:**
- Component 7 (Release): Saves config and restarts

**Features:**
- Static IP configuration
- Hostname setting
- Validation of IP addresses
- Auto-restart after save

**Warning:** Always sets static IP mode and Ethernet mode (not WiFi).

### MQTT Config Page (Page 11)

**Purpose:** Configure MQTT connection

**Components:**
- `mqttsv_set.txt`: MQTT server address
- `port_set.val`: MQTT port number
- `user_set.txt`: MQTT username
- `password_set.txt`: MQTT password (obfuscated)
- `topic_set.txt`: Base topic
- `use_auth.val`: Use authentication checkbox
- Component 2: Save button

**Touch Handling:**
- Component 2 (Release): Saves config and restarts

**Features:**
- MQTT server configuration
- Authentication settings
- Base topic configuration
- Password obfuscation (shows "********")
- Preserves existing password if not changed

**Password Handling:**
- Password displayed as "********"
- Only updated if changed from "********"
- Preserves security

---

## Binding Cards to Display

### Input Card Binding

```cpp
// Bind input card
display->bindInputCard(&espmega.inputs);

// The display will now:
// - Show real-time input states on page 2
// - Update automatically when inputs change
// - Handle null pointer gracefully (shows error page)

// Unbind if needed
display->unbindInputCard();
```

### Output Card Binding

```cpp
// Bind output card
display->bindOutputCard(&espmega.outputs);

// The display will now:
// - Show PWM values as bars on page 3
// - Allow PWM adjustment on page 5
// - Update in real-time when outputs change
// - Color-code based on ON/OFF state

// Unbind if needed
display->unbindOutputCard();
```

### Climate Card Binding

```cpp
// Bind climate card
display->bindClimateCard(&climateCard);

// The display will now:
// - Allow AC control on page 4
// - Show temperature, mode, fan speed
// - Display sensor readings if available
// - Update when AC state changes

// Important: Climate card must have modes and fan speeds
// in this specific order:
// Modes: [off, fan_only, cool]
// Fan speeds: [auto, low, medium, high]

// Unbind if needed
display->unbindClimateCard();
```

### Multiple Card Example

```cpp
// Complete setup with all cards
void setupDisplay() {
    // Enable display
    espmega.enableInternalDisplay(&Serial);

    // Bind all cards
    espmega.display->bindInputCard(&espmega.inputs);
    espmega.display->bindOutputCard(&espmega.outputs);
    espmega.display->bindClimateCard(&climateCard);

    // Display is now fully functional
    // All pages will work correctly
}
```

---

## Touch Interface

### Touch Event Structure

Touch events are delivered via callbacks with three parameters:

```cpp
void handleTouch(uint8_t page, uint8_t component, uint8_t type) {
    // page: The page where touch occurred
    // component: The component ID that was touched
    // type: 0x01 for press, 0x00 for release
}
```

### Touch Types

```cpp
#define TOUCH_TYPE_PRESS   0x01  // Finger pressed down
#define TOUCH_TYPE_RELEASE 0x00  // Finger lifted
```

### Registering Touch Handlers

```cpp
// Method 1: Lambda function
auto touchHandler = [](uint8_t page, uint8_t component, uint8_t type) {
    if (page == 1 && component == 5 && type == TOUCH_TYPE_RELEASE) {
        Serial.println("Button 5 on dashboard released!");
    }
};
display->registerTouchCallback(touchHandler);

// Method 2: Member function
class MyHandler {
    void handleTouch(uint8_t page, uint8_t component, uint8_t type) {
        // Handle touch
    }
};

MyHandler handler;
auto boundCallback = std::bind(&MyHandler::handleTouch, &handler,
                               std::placeholders::_1,
                               std::placeholders::_2,
                               std::placeholders::_3);
display->registerTouchCallback(boundCallback);
```

### Touch Handling Best Practices

1. **Debouncing:** Handle both press and release to avoid double-triggering
2. **Page Checking:** Always verify the current page before acting
3. **Non-Blocking:** Keep touch handlers fast
4. **State Validation:** Check if required objects are available

```cpp
void handleTouch(uint8_t page, uint8_t component, uint8_t type) {
    // Only handle release to avoid double-trigger
    if (type != TOUCH_TYPE_RELEASE) return;

    // Page-specific handling
    switch (page) {
        case 1: // Dashboard
            if (component == 10) {
                // Handle dashboard button 10
            }
            break;

        case 4: // AC page
            if (component == 18) {
                // Increase temperature
                if (climateCard != nullptr) {
                    climateCard->setTemperature(
                        climateCard->getTemperature() + 1
                    );
                }
            }
            break;
    }
}
```

---

## Display Protocol

The display uses a UART-based protocol with specific command formats.

### Serial Communication

**Baud Rate:** 115200 (normal), 921600 (OTA upload)

**Format:** 8N1 (8 data bits, no parity, 1 stop bit)

**Stop Bytes:** All commands end with three 0xFF bytes

### Command Format

#### Setting Values

```cpp
// Set number
"component.property=value\xFF\xFF\xFF"
// Example: "temp.val=25\xFF\xFF\xFF"

// Set string
"component.property=\"value\"\xFF\xFF\xFF"
// Example: "status.txt=\"Connected\"\xFF\xFF\xFF"

// Change page
"page N\xFF\xFF\xFF"
// Example: "page 1\xFF\xFF\xFF"
```

#### Getting Values

```cpp
// Get value
"get component.property\xFF\xFF\xFF"
// Response: [Type][Data...]\xFF\xFF\xFF

// Type 0x70: String response
// Type 0x71: Number response (4 bytes, little-endian)
```

### Payload Types

| Type | Description | Length |
|------|-------------|--------|
| 0x00 | Invalid | Variable |
| 0x01 | Success | 4 bytes |
| 0x65 | Touch Event | 7 bytes |
| 0x66 | Page Report | 5 bytes |
| 0x70 | String Data | Variable |
| 0x71 | Number Data | 8 bytes |

### Touch Event Payload

```
[0x65][Page][Component][Event][0xFF][0xFF][0xFF]
```

- **Byte 0:** 0x65 (touch event marker)
- **Byte 1:** Page number
- **Byte 2:** Component ID
- **Byte 3:** Event type (0x01=press, 0x00=release)
- **Bytes 4-6:** Stop bytes

### Page Report Payload

```
[0x66][Page][0xFF][0xFF][0xFF]
```

- **Byte 0:** 0x66 (page report marker)
- **Byte 1:** Current page number
- **Bytes 2-4:** Stop bytes

### Direct Serial Access

For advanced users who need direct serial access:

```cpp
// Acquire mutex
if (display->takeSerialMutex()) {
    // Direct serial operations
    displayAdapter->print("custom_command");
    displayAdapter->write(data_byte);

    // Always send stop bytes
    display->sendStopBytes();  // Sends 0xFF 0xFF 0xFF

    // Release mutex
    display->giveSerialMutex();
}
```

**Important:**
- Always acquire the mutex first
- Always send stop bytes after commands
- Always release the mutex
- Keep operations brief to avoid blocking

---

## Creating Custom Displays

You can create your own display implementation by extending `ESPMegaDisplay`.

### Basic Custom Display

```cpp
#include <ESPMegaDisplay.hpp>

class MyCustomDisplay : public ESPMegaDisplay {
public:
    MyCustomDisplay(HardwareSerial *adapter)
        : ESPMegaDisplay(adapter, 115200, 921600, 1, 3) {
        // Constructor
    }

    void begin() {
        // Call base class begin
        ESPMegaDisplay::begin();

        // Register callbacks
        auto touchCallback = std::bind(&MyCustomDisplay::handleTouch,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3);
        this->registerTouchCallback(touchCallback);

        auto pageCallback = std::bind(&MyCustomDisplay::handlePageChange,
                                      this,
                                      std::placeholders::_1);
        this->registerPageChangeCallback(pageCallback);

        // Initialize your display
        this->jumpToPage(0);
        this->setBrightness(75);
    }

    void loop() {
        // Call base class loop to process serial
        ESPMegaDisplay::loop();

        // Add your periodic updates here
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 1000) {
            lastUpdate = millis();
            updateDisplay();
        }
    }

private:
    void handleTouch(uint8_t page, uint8_t component, uint8_t type) {
        // Handle touch events
        if (type == 0x01) { // Press
            Serial.printf("Touch: page=%d, component=%d\n", page, component);
        }
    }

    void handlePageChange(uint8_t page) {
        // Handle page changes
        currentPage = page;
        refreshPage(page);
    }

    void refreshPage(uint8_t page) {
        // Refresh display elements for the page
        switch(page) {
            case 0:
                // Update home page
                this->setString("title.txt", "My Display");
                break;
            case 1:
                // Update page 1
                break;
        }
    }

    void updateDisplay() {
        // Periodic updates
        this->setNumber("uptime.val", millis() / 1000);
    }
};
```

### Advanced Custom Display with IoT Integration

```cpp
class IoTDisplay : public ESPMegaDisplay {
public:
    IoTDisplay(HardwareSerial *adapter)
        : ESPMegaDisplay(adapter, 115200, 921600, 1, 3) {}

    void begin(ESPMegaIoT *iot) {
        this->iot = iot;
        ESPMegaDisplay::begin();

        // Register callbacks
        registerCallbacks();

        // Initial display setup
        setupPages();
    }

    void bindSensor(AnalogCard *sensor, uint8_t channel) {
        this->sensor = sensor;
        this->sensorChannel = channel;

        // Register sensor callback
        auto callback = [this](uint8_t pin, uint16_t value) {
            this->updateSensorDisplay(pin, value);
        };
        sensor->registerCallback(callback);
    }

private:
    ESPMegaIoT *iot;
    AnalogCard *sensor;
    uint8_t sensorChannel;

    void registerCallbacks() {
        auto touchCb = std::bind(&IoTDisplay::handleTouch, this,
                                std::placeholders::_1,
                                std::placeholders::_2,
                                std::placeholders::_3);
        registerTouchCallback(touchCb);
    }

    void setupPages() {
        jumpToPage(1);
        this->setString("title.txt", "IoT Monitor");
    }

    void handleTouch(uint8_t page, uint8_t component, uint8_t type) {
        if (page == 1 && component == 5 && type == 0x00) {
            // Toggle something via MQTT
            if (iot && iot->mqttConnected()) {
                iot->publish("device/command", "toggle");
            }
        }
    }

    void updateSensorDisplay(uint8_t pin, uint16_t value) {
        if (pin == sensorChannel && currentPage == 2) {
            this->setNumber("sensor.val", value);
        }
    }
};
```

### Custom Display Design Guidelines

1. **Always call base class methods:**
   ```cpp
   void begin() {
       ESPMegaDisplay::begin();  // Important!
       // Your initialization
   }
   ```

2. **Handle page changes:**
   - Refresh display elements when page changes
   - Only update visible components

3. **Use mutex for direct access:**
   ```cpp
   if (takeSerialMutex()) {
       // Direct operations
       giveSerialMutex();
   }
   ```

4. **Implement loop efficiently:**
   - Call base loop() first
   - Use timers for periodic updates
   - Keep updates non-blocking

5. **Null pointer checks:**
   - Always verify pointers before use
   - Handle missing cards gracefully

---

## Display OTA Updates

The `ESPMegaDisplayOTA` class provides web-based OTA updates for the display firmware.

### Setup

```cpp
#include <ESPMegaDisplayOTA.hpp>

ESPMegaDisplayOTA displayOta;
ESPMegaDisplay display(&Serial);
ESPMegaWebServer webServer;

void setup() {
    display.begin();
    webServer.begin();

    // Initialize OTA at /display path
    displayOta.begin("/display", &display, &webServer);

    // Access OTA interface at:
    // http://device-ip/display/index.html
}
```

### Web Interface Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/display/index.html` | GET | OTA web interface |
| `/display/ota/begin` | POST | Start update |
| `/display/ota/write` | POST | Write data chunk |
| `/display/ota/end` | POST | Finish update |

### OTA Begin Request

```json
POST /display/ota/begin
Content-Type: application/json

{
    "size": 1048576
}
```

**Response:**
```json
{
    "status": "success"
}
```

### OTA Write Request

```json
POST /display/ota/write
Content-Type: application/json

{
    "size": 4096,
    "data": [0xFF, 0xAA, 0x55, ...]
}
```

**Response:**
```json
{
    "status": "success",
    "bytes_written": 4096
}
```

### OTA End Request

```json
POST /display/ota/end
Content-Type: application/json

{}
```

**Response:**
```json
{
    "status": "success"
}
```

### Programmatic OTA Update

```cpp
void updateDisplayFromSD() {
    File updateFile = SD.open("/display_fw.tft");
    if (!updateFile) {
        Serial.println("Update file not found");
        return;
    }

    size_t fileSize = updateFile.size();

    // Begin update
    if (!display.beginUpdate(fileSize)) {
        Serial.println("Failed to begin update");
        updateFile.close();
        return;
    }

    // Write data in chunks
    uint8_t buffer[4096];
    while (updateFile.available()) {
        size_t bytesRead = updateFile.read(buffer, sizeof(buffer));

        if (!display.writeUpdate(buffer, bytesRead)) {
            Serial.println("Write failed");
            updateFile.close();
            return;
        }

        // Show progress
        Serial.printf("Progress: %d%%\n",
            (display.getUpdateBytesWritten() * 100) / fileSize);
    }

    updateFile.close();

    // Finish update (will restart)
    display.endUpdate();
}
```

---

## Code Examples

See the example files in `/examples/Display/`:

- **basic_display.ino** - Basic display setup and usage
- **with_climate.ino** - Display with climate control
- **full_integration.ino** - Complete system with all features

### Quick Start Example

```cpp
#include <ESPMegaProOS.hpp>
#include <InternalDisplay.hpp>

ESPMegaPRO espmega;

void setup() {
    // Initialize ESPMega
    espmega.begin();

    // Enable IoT
    espmega.enableIotModule();
    espmega.iot->connectNetwork();
    espmega.iot->connectToMqtt();

    // Enable and configure display
    espmega.enableInternalDisplay(&Serial);
    espmega.display->bindInputCard(&espmega.inputs);
    espmega.display->bindOutputCard(&espmega.outputs);
}

void loop() {
    espmega.loop();  // Handles display loop automatically
}
```

### Custom Touch Handler Example

```cpp
void setup() {
    espmega.enableInternalDisplay(&Serial);

    // Add custom touch handler
    auto customTouch = [](uint8_t page, uint8_t component, uint8_t type) {
        if (page == 1 && component == 20 && type == 0x00) {
            Serial.println("Custom button pressed!");
            // Your custom action
        }
    };

    espmega.display->registerTouchCallback(customTouch);
}
```

### Dynamic Value Updates Example

```cpp
void loop() {
    espmega.loop();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        lastUpdate = millis();

        // Update custom value on display
        float temperature = readTemperature();
        espmega.display->setNumber("custom_temp.val", (int)temperature);

        // Update status message
        if (espmega.iot->mqttConnected()) {
            espmega.display->setString("custom_status.txt", "Online");
        } else {
            espmega.display->setString("custom_status.txt", "Offline");
        }
    }
}
```

---

## API Reference Summary

### ESPMegaDisplay Methods

| Method | Purpose |
|--------|---------|
| `begin()` | Initialize display |
| `loop()` | Process serial data |
| `reset()` | Reset display |
| `jumpToPage(page)` | Change page |
| `setBrightness(value)` | Set brightness |
| `setVolume(value)` | Set volume |
| `setNumber(comp, val)` | Set numeric value |
| `setString(comp, val)` | Set string value |
| `getNumber(comp)` | Get numeric value |
| `getString(comp)` | Get string value |
| `getStringToBuffer(comp, buf, size)` | Get string to buffer |
| `registerTouchCallback(cb)` | Register touch handler |
| `registerPageChangeCallback(cb)` | Register page handler |
| `registerPayloadCallback(cb)` | Register payload handler |
| `takeSerialMutex()` | Acquire serial lock |
| `giveSerialMutex()` | Release serial lock |
| `beginUpdate(size)` | Start OTA update |
| `writeUpdate(data, size)` | Write OTA data |
| `endUpdate()` | Finish OTA update |
| `getUpdateBytesWritten()` | Get OTA progress |

### InternalDisplay Methods

| Method | Purpose |
|--------|---------|
| `begin(iot, getRtcTime)` | Initialize with IoT |
| `bindInputCard(card)` | Bind input card |
| `bindOutputCard(card)` | Bind output card |
| `bindClimateCard(card)` | Bind climate card |
| `unbindInputCard()` | Unbind input card |
| `unbindOutputCard()` | Unbind output card |
| `unbindClimateCard()` | Unbind climate card |

---

## Troubleshooting

### Display Not Responding

1. **Check wiring:**
   - Display TX → ESP32 RX
   - Display RX → ESP32 TX
   - Common ground

2. **Verify baud rate:**
   ```cpp
   // Should be 115200 for normal operation
   display.begin();
   ```

3. **Check mutex timeout:**
   - If operations fail, mutex might be held too long
   - Increase timeout or optimize serial access

### Touch Not Working

1. **Verify callback registration:**
   ```cpp
   auto cb = [](uint8_t p, uint8_t c, uint8_t t) {
       Serial.printf("Touch: %d/%d/%d\n", p, c, t);
   };
   display->registerTouchCallback(cb);
   ```

2. **Check page number:**
   - Touch events include page number
   - Ensure you're on the expected page

### Values Not Updating

1. **Call loop() regularly:**
   ```cpp
   void loop() {
       display->loop();  // Required!
   }
   ```

2. **Check component names:**
   - Names are case-sensitive
   - Use exact component names from display editor

3. **Verify page visibility:**
   - Components only update on current page
   - Use page change callback to refresh

### OTA Update Fails

1. **Check file size:**
   - Must not exceed display memory
   - Typical limit: 16-32MB

2. **Verify baud rate:**
   - Upload uses high speed (921600)
   - May need to try 9600 if stuck

3. **Power supply:**
   - Ensure stable power during update
   - Update draws more current

---

## Best Practices

1. **Always call loop():**
   ```cpp
   void loop() {
       display->loop();  // Essential!
   }
   ```

2. **Use timers for updates:**
   ```cpp
   static unsigned long lastUpdate = 0;
   if (millis() - lastUpdate > 1000) {
       lastUpdate = millis();
       updateDisplay();
   }
   ```

3. **Null pointer checks:**
   ```cpp
   if (climateCard != nullptr) {
       display->bindClimateCard(climateCard);
   }
   ```

4. **Minimal serial access:**
   - Keep mutex held time short
   - Batch updates when possible

5. **Handle errors gracefully:**
   ```cpp
   if (!display->takeSerialMutex()) {
       return;  // Failed to acquire, try later
   }
   // ... operations ...
   display->giveSerialMutex();
   ```

---

## Additional Resources

- **Nextion Display Editor:** For creating custom display layouts
- **Display Protocol:** Nextion instruction set documentation
- **ESPMegaPRO Documentation:** Main system documentation
- **Example Code:** See `/examples/Display/` directory

---

**Document Version:** 1.0
**Last Updated:** 2025-11-23
**Compatible with:** ESPMegaPRO Library v2.10.0+
