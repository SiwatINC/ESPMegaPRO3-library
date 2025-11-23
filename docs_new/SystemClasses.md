# System Classes Documentation

This document covers the core system classes for display management, OTA updates, and recovery mode in the ESPMegaPRO3 library.

## Table of Contents
- [ESPMegaDisplay Base Class](#espmegadisplay-base-class)
- [ESPMegaDisplayOTA Class](#espmegadisplayota-class)
- [ESPMegaRecovery Class](#espmmegarecovery-class)

---

## ESPMegaDisplay Base Class

The `ESPMegaDisplay` class provides a comprehensive interface for controlling UART-based displays. It implements a serial communication protocol with thread-safe access, event callbacks, and OTA update capabilities for the display firmware.

### Hardware Connection

Connect the display to the ESPMega board:
- Display TX pin → ESPMega RX pin
- Display RX pin → ESPMega TX pin

### Constructor

```cpp
ESPMegaDisplay(HardwareSerial *displayAdapter, uint32_t baudRate,
               uint32_t uploadBaudRate, uint8_t txPin, uint8_t rxPin)
```

**Parameters:**
- `displayAdapter` - Pointer to the HardwareSerial interface
- `baudRate` - Normal operation baud rate (e.g., 115200)
- `uploadBaudRate` - Baud rate for OTA uploads (e.g., 921600)
- `txPin` - TX pin number
- `rxPin` - RX pin number

**Example:**
```cpp
ESPMegaDisplay display(&Serial2, 115200, 921600, 17, 16);
```

---

### Core Methods

#### Initialization and Main Loop

##### `void begin()`
Initializes the display connection, configures the serial port, and resets the display.

**Example:**
```cpp
void setup() {
    display.begin();
}
```

##### `void loop()`
Must be called repeatedly in the main loop to process incoming serial data and trigger callbacks.

**Example:**
```cpp
void loop() {
    display.loop();
}
```

##### `void reset()`
Restarts the display by sending the reset command.

**Example:**
```cpp
display.reset();
```

---

### Display Control Methods

#### `void setBrightness(int value)`
Sets the display brightness.

**Parameters:**
- `value` - Brightness level (display-dependent range, typically 0-100)

**Example:**
```cpp
display.setBrightness(80);  // Set to 80% brightness
```

#### `void setVolume(int value)`
Sets the display speaker volume.

**Parameters:**
- `value` - Volume level (display-dependent range, typically 0-100)

**Example:**
```cpp
display.setVolume(50);  // Set to 50% volume
```

#### `void jumpToPage(int page)`
Navigates to the specified page on the display.

**Parameters:**
- `page` - Page number to jump to

**Example:**
```cpp
display.jumpToPage(2);  // Jump to page 2
```

---

### Component Value Methods

#### `void setString(const char* component, const char* value)`
Sets the text value of a string component on the display.

**Parameters:**
- `component` - Component name/identifier
- `value` - String value to set

**Example:**
```cpp
display.setString("statusText", "System Ready");
display.setString("t0", "Temperature: 25°C");
```

#### `void setNumber(const char* component, int value)`
Sets the numeric value of a number component on the display.

**Parameters:**
- `component` - Component name/identifier
- `value` - Numeric value to set

**Example:**
```cpp
display.setNumber("progress", 75);
display.setNumber("n0", 42);
```

#### `const char* getString(const char* component)`
Retrieves the string value from a display component.

**Parameters:**
- `component` - Component name/identifier

**Returns:**
- Pointer to a dynamically allocated string containing the value
- `nullptr` if the operation fails

**Warning:**
- This function is blocking (waits up to 500ms for response)
- The returned string must be freed after use with `free()`

**Example:**
```cpp
const char* value = display.getString("textInput");
if (value != nullptr) {
    Serial.println(value);
    free((void*)value);  // Must free the allocated memory
}
```

#### `bool getStringToBuffer(const char* component, char* buffer, uint8_t buffer_size)`
Retrieves a string value and stores it in a pre-allocated buffer.

**Parameters:**
- `component` - Component name/identifier
- `buffer` - Pre-allocated buffer to store the string
- `buffer_size` - Size of the buffer

**Returns:**
- `true` if successful
- `false` if buffer too small or operation failed

**Warning:**
- This function is blocking (waits up to 500ms for response)

**Example:**
```cpp
char buffer[64];
if (display.getStringToBuffer("username", buffer, sizeof(buffer))) {
    Serial.printf("Username: %s\n", buffer);
}
```

#### `uint32_t getNumber(const char* component)`
Retrieves a numeric value from a display component.

**Parameters:**
- `component` - Component name/identifier

**Returns:**
- 32-bit unsigned integer value
- 0 if the operation fails

**Warning:**
- This function is blocking (waits up to 500ms for response)

**Example:**
```cpp
uint32_t sliderValue = display.getNumber("slider1");
Serial.printf("Slider value: %d\n", sliderValue);
```

---

### Event Callback System

The display supports three types of events: touch events, page changes, and custom payloads.

#### Touch Events

##### `uint16_t registerTouchCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback)`
Registers a callback for touch events.

**Parameters:**
- `callback` - Function with signature `void(uint8_t page, uint8_t component, uint8_t event)`
  - `page` - Page number where touch occurred
  - `component` - Component ID that was touched
  - `event` - Event type (0x01 = press, 0x00 = release)

**Returns:**
- Handle (ID) for the callback registration

**Example:**
```cpp
void onTouch(uint8_t page, uint8_t component, uint8_t event) {
    Serial.printf("Touch: page=%d, component=%d, event=%s\n",
                  page, component, event ? "press" : "release");

    if (page == 0 && component == 1 && event == 1) {
        // Button 1 on page 0 was pressed
        handleButton1Press();
    }
}

void setup() {
    uint16_t handle = display.registerTouchCallback(onTouch);
}
```

##### `void unregisterTouchCallback(uint16_t handle)`
Removes a previously registered touch callback.

**Parameters:**
- `handle` - Handle returned from `registerTouchCallback()`

**Example:**
```cpp
uint16_t touchHandle = display.registerTouchCallback(onTouch);
// Later...
display.unregisterTouchCallback(touchHandle);
```

#### Page Change Events

##### `uint16_t registerPageChangeCallback(std::function<void(uint8_t)> callback)`
Registers a callback for page change events.

**Parameters:**
- `callback` - Function with signature `void(uint8_t page)`
  - `page` - New page number

**Returns:**
- Handle (ID) for the callback registration

**Example:**
```cpp
void onPageChange(uint8_t page) {
    Serial.printf("Page changed to: %d\n", page);

    switch(page) {
        case 0:
            initializeHomePage();
            break;
        case 1:
            initializeSettingsPage();
            break;
    }
}

void setup() {
    uint16_t handle = display.registerPageChangeCallback(onPageChange);
}
```

##### `void unregisterPageChangeCallback(uint16_t handle)`
Removes a previously registered page change callback.

**Parameters:**
- `handle` - Handle returned from `registerPageChangeCallback()`

#### Custom Payload Events

##### `uint16_t registerPayloadCallback(std::function<void(uint8_t, uint8_t*, uint8_t)> callback)`
Registers a callback for custom payload types not handled by the standard protocol.

**Parameters:**
- `callback` - Function with signature `void(uint8_t type, uint8_t* payload, uint8_t length)`
  - `type` - Payload type identifier (first byte of payload)
  - `payload` - Pointer to payload data (excluding type byte and stop bytes)
  - `length` - Length of payload data

**Returns:**
- Handle (ID) for the callback registration

**Example:**
```cpp
void onCustomPayload(uint8_t type, uint8_t* payload, uint8_t length) {
    Serial.printf("Custom payload: type=0x%02X, length=%d\n", type, length);

    if (type == 0x80) {  // Custom sensor data
        uint16_t sensorValue = (payload[1] << 8) | payload[0];
        processSensorData(sensorValue);
    }
}

void setup() {
    uint16_t handle = display.registerPayloadCallback(onCustomPayload);
}
```

##### `void unregisterPayloadCallback(uint16_t handle)`
Removes a previously registered payload callback.

**Parameters:**
- `handle` - Handle returned from `registerPayloadCallback()`

---

### Thread Safety / Mutex Control

The display uses a FreeRTOS mutex to ensure thread-safe access to the serial port.

#### `bool takeSerialMutex()`
Acquires the serial mutex for direct serial access.

**Returns:**
- `true` if mutex acquired successfully
- `false` if timeout (1000ms)

**Note:** Only needed when accessing `displayAdapter` directly. All built-in methods handle mutex automatically.

**Example:**
```cpp
if (display.takeSerialMutex()) {
    // Direct serial access
    Serial2.print("custom_command");
    display.giveSerialMutex();
}
```

#### `void giveSerialMutex()`
Releases the serial mutex.

**Example:**
```cpp
display.takeSerialMutex();
// ... perform serial operations ...
display.giveSerialMutex();
```

#### `SemaphoreHandle_t serialMutex`
Public member providing direct access to the FreeRTOS semaphore handle.

---

### Display OTA Update Methods

These methods enable firmware updates for the display itself over the serial connection.

#### `bool beginUpdate(size_t size)`
Initiates a display firmware update.

**Parameters:**
- `size` - Total size of the firmware file in bytes

**Returns:**
- `true` if update started successfully
- `false` if display didn't respond or initialization failed

**Behavior:**
1. Attempts to connect at configured `baudRate`
2. If that fails, retries at 9600 baud
3. Sends reset and connect commands
4. Sends update size and switches to `uploadBaudRate`
5. Waits for 0x05 ready byte from display

**Example:**
```cpp
size_t firmwareSize = 524288;  // 512KB
if (display.beginUpdate(firmwareSize)) {
    Serial.println("Display update started");
} else {
    Serial.println("Failed to start display update");
}
```

#### `bool beginUpdate(size_t size, uint32_t baudRate)`
Initiates a display firmware update with a specific baud rate.

**Parameters:**
- `size` - Total size of the firmware file in bytes
- `baudRate` - Baud rate to use for initial connection

**Returns:**
- `true` if update started successfully
- `false` if display didn't respond or initialization failed

**Example:**
```cpp
if (!display.beginUpdate(firmwareSize, 115200)) {
    // Try at 9600 baud
    display.beginUpdate(firmwareSize, 9600);
}
```

#### `bool writeUpdate(uint8_t* data, size_t size)`
Writes a chunk of firmware data to the display.

**Parameters:**
- `data` - Pointer to firmware data
- `size` - Size of this chunk (maximum 4096 bytes)

**Returns:**
- `true` if data written successfully
- `false` if size too large or display not ready

**Behavior:**
- After every 4096 bytes, waits for 0x05 acknowledgment from display
- Blocks until acknowledgment received or timeout (1000ms)

**Example:**
```cpp
uint8_t buffer[512];
size_t bytesRead = file.read(buffer, sizeof(buffer));
if (!display.writeUpdate(buffer, bytesRead)) {
    Serial.println("Write failed");
}
```

#### `void endUpdate()`
Completes the display firmware update.

**Behavior:**
1. Releases the serial mutex
2. Resets the display
3. Restarts the ESP32

**Example:**
```cpp
display.endUpdate();
// ESP32 will restart after this call
```

#### `size_t getUpdateBytesWritten()`
Returns the total number of bytes written during the current update.

**Returns:**
- Number of bytes written so far

**Example:**
```cpp
size_t written = display.getUpdateBytesWritten();
Serial.printf("Progress: %d / %d bytes\n", written, totalSize);
```

---

### Display Communication Protocol

The display uses a simple serial protocol with the following characteristics:

#### Message Format

All messages are terminated with three `0xFF` bytes (stop bytes):

```
[COMMAND/DATA] 0xFF 0xFF 0xFF
```

#### Command Messages (ESP32 → Display)

**Setting Values:**
```
component=value 0xFF 0xFF 0xFF
```

**Setting Strings:**
```
component="string value" 0xFF 0xFF 0xFF
```

**Getting Values:**
```
get component 0xFF 0xFF 0xFF
```

**Page Navigation:**
```
page N 0xFF 0xFF 0xFF
```

**Brightness:**
```
dim=value 0xFF 0xFF 0xFF
```

**Volume:**
```
vol=value 0xFF 0xFF 0xFF
```

**Reset:**
```
rest 0xFF 0xFF 0xFF
```

#### Response Messages (Display → ESP32)

**Touch Event (0x65):**
```
0x65 [page] [component] [event] 0xFF 0xFF 0xFF
```
- Total length: 7 bytes
- `event`: 0x01 = press, 0x00 = release

**Page Report (0x66):**
```
0x66 [page] 0xFF 0xFF 0xFF
```
- Total length: 5 bytes
- Sent when page changes

**String Response (0x70):**
```
0x70 [string bytes...] 0xFF 0xFF 0xFF
```
- Variable length
- String is not null-terminated in transmission

**Number Response (0x71):**
```
0x71 [byte0] [byte1] [byte2] [byte3] 0xFF 0xFF 0xFF
```
- Total length: 8 bytes
- 32-bit little-endian value

#### Protocol Constants

```cpp
#define DISPLAY_MUTEX_TAKE_TIMEOUT 1000    // Mutex timeout in ms
#define OTA_WAIT_TIMEOUT 1000              // OTA acknowledgment timeout
#define DISPLAY_FETCH_TIMEOUT 100          // Response timeout per attempt
#define DISPLAY_FETCH_RETRY_COUNT 5        // Number of retry attempts
```

#### Payload Validation

Valid payloads must:
1. Be at least 3 bytes long
2. End with three consecutive `0xFF` bytes
3. Have valid type identifier as first byte

---

### Protected Members

These members are available to derived classes:

```cpp
protected:
    uint32_t baudRate;                // Normal operation baud rate
    uint32_t uploadBaudRate;          // OTA upload baud rate
    uint8_t txPin;                    // TX pin number
    uint8_t rxPin;                    // RX pin number
    size_t otaBytesWritten;           // OTA progress counter
    uint8_t currentPage;              // Current display page
    uint8_t rx_buffer_index;          // Receive buffer position
    char rx_buffer[256];              // Receive buffer
    char tx_buffer[256];              // Transmit buffer
    HardwareSerial *displayAdapter;   // Serial interface pointer
```

#### Protected Methods

```cpp
bool recieveSerialCommand();                    // Receive and process serial data
bool recieveSerialCommand(bool process);        // Receive with optional processing
void processSerialCommand();                    // Process received command
void processTouchPayload();                     // Handle touch event
void processPageReportPayload();                // Handle page change event
void sendStopBytes();                           // Send 0xFF 0xFF 0xFF
void sendCommand(char* command);                // Send command with stop bytes
bool payloadIsValid();                          // Check if buffer has valid payload
bool waitForValidPayload(uint32_t timeout);     // Wait for complete payload
```

---

### Creating Custom Display Classes

You can extend `ESPMegaDisplay` to create custom display implementations with additional features.

#### Example: Custom Display with Logging

```cpp
#pragma once
#include <ESPMegaDisplay.hpp>

class CustomDisplay : public ESPMegaDisplay {
public:
    CustomDisplay(HardwareSerial *displayAdapter, uint32_t baudRate,
                  uint32_t uploadBaudRate, uint8_t txPin, uint8_t rxPin)
        : ESPMegaDisplay(displayAdapter, baudRate, uploadBaudRate, txPin, rxPin) {
    }

    // Override to add logging
    void jumpToPage(int page) {
        Serial.printf("Jumping to page %d\n", page);
        ESPMegaDisplay::jumpToPage(page);
    }

    // Add custom command
    void sendCustomCommand(const char* cmd) {
        if (takeSerialMutex()) {
            displayAdapter->print(cmd);
            sendStopBytes();
            giveSerialMutex();
        }
    }

    // Access protected buffer for custom processing
    void dumpRxBuffer() {
        Serial.printf("RX Buffer Index: %d\n", rx_buffer_index);
        for (int i = 0; i < rx_buffer_index; i++) {
            Serial.printf("%02X ", rx_buffer[i]);
        }
        Serial.println();
    }
};
```

#### Example: Display with Auto-Retry

```cpp
class ReliableDisplay : public ESPMegaDisplay {
private:
    int maxRetries = 3;

public:
    ReliableDisplay(HardwareSerial *displayAdapter, uint32_t baudRate,
                    uint32_t uploadBaudRate, uint8_t txPin, uint8_t rxPin)
        : ESPMegaDisplay(displayAdapter, baudRate, uploadBaudRate, txPin, rxPin) {
    }

    // Retry setString with verification
    bool setStringVerified(const char* component, const char* value) {
        for (int attempt = 0; attempt < maxRetries; attempt++) {
            setString(component, value);
            delay(50);

            char buffer[256];
            if (getStringToBuffer(component, buffer, sizeof(buffer))) {
                if (strcmp(buffer, value) == 0) {
                    return true;  // Verification successful
                }
            }
            Serial.printf("Retry %d/%d\n", attempt + 1, maxRetries);
        }
        return false;  // All retries failed
    }

    void setMaxRetries(int retries) {
        maxRetries = retries;
    }
};
```

---

### Complete Usage Example

```cpp
#include <ESPMegaDisplay.hpp>

ESPMegaDisplay display(&Serial2, 115200, 921600, 17, 16);

void onTouch(uint8_t page, uint8_t component, uint8_t event) {
    if (event == 1) {  // Button press
        Serial.printf("Button %d pressed on page %d\n", component, page);

        switch(page) {
            case 0:  // Home page
                if (component == 1) {
                    // Settings button
                    display.jumpToPage(1);
                } else if (component == 2) {
                    // Toggle button
                    toggleState = !toggleState;
                    display.setNumber("toggleBtn", toggleState);
                }
                break;

            case 1:  // Settings page
                if (component == 1) {
                    // Save button
                    char buffer[64];
                    if (display.getStringToBuffer("textInput", buffer, sizeof(buffer))) {
                        saveSettings(buffer);
                    }
                    display.jumpToPage(0);
                }
                break;
        }
    }
}

void onPageChange(uint8_t page) {
    Serial.printf("Now on page %d\n", page);

    if (page == 0) {
        // Update home page with current data
        display.setString("statusText", "System Ready");
        display.setNumber("temperature", getCurrentTemp());
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize display
    display.begin();

    // Register callbacks
    display.registerTouchCallback(onTouch);
    display.registerPageChangeCallback(onPageChange);

    // Initial setup
    display.setBrightness(80);
    display.setVolume(50);
    display.jumpToPage(0);
}

void loop() {
    display.loop();  // Process display events

    // Update display periodically
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();

        display.setNumber("temperature", getCurrentTemp());
        display.setString("time", getTimeString());
    }
}
```

---

## ESPMegaDisplayOTA Class

The `ESPMegaDisplayOTA` class provides web-based over-the-air (OTA) firmware updates for Nextion/TJC displays through a REST API. It integrates with `ESPMegaWebServer` to provide authenticated access to display firmware updates.

### Overview

Display OTA updates work through a three-phase process:
1. **Begin** - Initialize update with total firmware size
2. **Write** - Transfer firmware data in chunks (max 4096 bytes)
3. **End** - Finalize update and restart ESP32

### Constructor

```cpp
ESPMegaDisplayOTA()
```

Default constructor. Call `begin()` to initialize.

**Example:**
```cpp
ESPMegaDisplayOTA displayOTA;
```

---

### Methods

#### `void begin(const char* base_path, ESPMegaDisplay *display, ESPMegaWebServer *webServer)`
Initializes the display OTA system and registers web endpoints.

**Parameters:**
- `base_path` - URL base path for OTA endpoints (e.g., "/display")
- `display` - Pointer to ESPMegaDisplay instance
- `webServer` - Pointer to ESPMegaWebServer instance

**Registered Endpoints:**
- `POST base_path/ota/begin` - Start update
- `POST base_path/ota/write` - Write firmware chunk
- `POST base_path/ota/end` - Finalize update
- `GET base_path/index.html` - OTA web interface

**Example:**
```cpp
ESPMegaDisplay display(&Serial2, 115200, 921600, 17, 16);
ESPMegaWebServer webServer(80, &iot);
ESPMegaDisplayOTA displayOTA;

void setup() {
    display.begin();
    webServer.begin();

    displayOTA.begin("/display", &display, &webServer);
    // Endpoints now available at:
    // POST /display/ota/begin
    // POST /display/ota/write
    // POST /display/ota/end
    // GET  /display/index.html
}
```

---

### REST API Reference

All endpoints require authentication (username/password from web server configuration).

#### POST /base_path/ota/begin

Initiates a display firmware update.

**Request Body (JSON):**
```json
{
    "size": 524288
}
```

**Fields:**
- `size` (number, required) - Total firmware file size in bytes

**Response (Success):**
```json
{
    "status": "success"
}
```

**Response (Error):**
```json
{
    "status": "error"
}
```

**Status Codes:**
- `200` - Update started successfully
- `500` - Failed to start update (display not responding)

**Example (curl):**
```bash
curl -X POST http://192.168.1.100/display/ota/begin \
  -u admin:password \
  -H "Content-Type: application/json" \
  -d '{"size": 524288}'
```

---

#### POST /base_path/ota/write

Writes a chunk of firmware data to the display.

**Request Body (JSON):**
```json
{
    "size": 512,
    "data": [0x4E, 0x45, 0x58, ...]
}
```

**Fields:**
- `size` (number, required) - Number of bytes in this chunk (max 4096)
- `data` (array of numbers, required) - Array of byte values (0-255)

**Response (Success):**
```json
{
    "status": "success",
    "bytes_written": 2048
}
```

**Response (Error):**
```json
{
    "status": "error",
    "message": "The size of the update is too big"
}
```

**Error Messages:**
- "The size of the update is too big" - Chunk size > 4096 bytes
- "The update chunk is too big" - Would exceed total size specified in begin

**Status Codes:**
- `200` - Chunk written successfully
- `500` - Write failed

**Example (curl):**
```bash
# Example with small data array
curl -X POST http://192.168.1.100/display/ota/write \
  -u admin:password \
  -H "Content-Type: application/json" \
  -d '{"size": 3, "data": [0x4E, 0x45, 0x58]}'
```

---

#### POST /base_path/ota/end

Finalizes the display firmware update and restarts the ESP32.

**Request Body (JSON):**
```json
{}
```

**Response:**
```json
{
    "status": "success"
}
```

**Status Codes:**
- `200` - Update completed successfully

**Behavior:**
- Calls `display->endUpdate()`
- Resets the display
- Restarts the ESP32

**Example (curl):**
```bash
curl -X POST http://192.168.1.100/display/ota/end \
  -u admin:password \
  -H "Content-Type: application/json" \
  -d '{}'
```

---

### Update Process Flow

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │
       │ 1. POST /ota/begin with total size
       ├─────────────────────────────────────┐
       │                                     │
       │ 2. Receive success confirmation    │
       │◄────────────────────────────────────┤
       │                                     │
       │ 3. POST /ota/write (chunk 1)       │
       ├─────────────────────────────────────┤
       │                                     │
       │ 4. Receive bytes_written count     │
       │◄────────────────────────────────────┤
       │                                     │
       │ 5. POST /ota/write (chunk 2)       │
       ├─────────────────────────────────────┤
       │                                     │
       │ 6. Receive bytes_written count     │
       │◄────────────────────────────────────┤
       │                                     │
       │    ... repeat for all chunks ...   │
       │                                     │
       │ 7. POST /ota/end                   │
       ├─────────────────────────────────────┤
       │                                     │
       │ 8. Receive success                 │
       │◄────────────────────────────────────┤
       │                                     │
       │    [ESP32 restarts]                │
       │                                     │
```

---

### Complete Usage Example

#### Arduino Code

```cpp
#include <ESPMegaDisplay.hpp>
#include <ESPMegaDisplayOTA.hpp>
#include <ESPMegaWebServer.hpp>
#include <ESPMegaIoT.hpp>

ESPMegaIoT iot;
ESPMegaDisplay display(&Serial2, 115200, 921600, 17, 16);
ESPMegaWebServer webServer(80, &iot);
ESPMegaDisplayOTA displayOTA;

void setup() {
    Serial.begin(115200);

    // Initialize networking
    iot.begin();
    iot.connectNetwork();

    // Initialize display
    display.begin();

    // Initialize web server
    webServer.begin();

    // Initialize display OTA
    displayOTA.begin("/display", &display, &webServer);

    Serial.println("Display OTA ready!");
    Serial.printf("Access at: http://%s/display/index.html\n",
                  WiFi.localIP().toString().c_str());
}

void loop() {
    display.loop();
    iot.loop();
}
```

#### Python OTA Upload Script

```python
#!/usr/bin/env python3
import requests
import json
import sys

def upload_display_firmware(ip, username, password, firmware_file, base_path="/display"):
    """Upload firmware to display via OTA"""

    base_url = f"http://{ip}{base_path}/ota"
    auth = (username, password)

    # Read firmware file
    with open(firmware_file, 'rb') as f:
        firmware = f.read()

    total_size = len(firmware)
    print(f"Firmware size: {total_size} bytes")

    # Step 1: Begin update
    print("Starting update...")
    response = requests.post(
        f"{base_url}/begin",
        auth=auth,
        json={"size": total_size}
    )

    if response.status_code != 200:
        print(f"Failed to start update: {response.text}")
        return False

    print("Update started")

    # Step 2: Write firmware in chunks
    chunk_size = 4096
    offset = 0

    while offset < total_size:
        chunk = firmware[offset:offset + chunk_size]
        chunk_len = len(chunk)

        # Convert bytes to array of integers
        data_array = list(chunk)

        print(f"Writing chunk: {offset}/{total_size} ({offset*100//total_size}%)")

        response = requests.post(
            f"{base_url}/write",
            auth=auth,
            json={
                "size": chunk_len,
                "data": data_array
            }
        )

        if response.status_code != 200:
            print(f"Failed to write chunk: {response.text}")
            return False

        result = response.json()
        print(f"  Bytes written: {result.get('bytes_written', 'unknown')}")

        offset += chunk_len

    # Step 3: End update
    print("Finalizing update...")
    response = requests.post(
        f"{base_url}/end",
        auth=auth,
        json={}
    )

    if response.status_code != 200:
        print(f"Failed to finalize update: {response.text}")
        return False

    print("Update completed! ESP32 will restart.")
    return True

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python display_ota.py <ip> <username> <password> <firmware.tft>")
        sys.exit(1)

    ip = sys.argv[1]
    username = sys.argv[2]
    password = sys.argv[3]
    firmware = sys.argv[4]

    success = upload_display_firmware(ip, username, password, firmware)
    sys.exit(0 if success else 1)
```

**Usage:**
```bash
python display_ota.py 192.168.1.100 admin password display_firmware.tft
```

#### JavaScript Web Client Example

```javascript
async function uploadDisplayFirmware(baseUrl, username, password, file) {
    const auth = 'Basic ' + btoa(username + ':' + password);
    const headers = {
        'Authorization': auth,
        'Content-Type': 'application/json'
    };

    // Read file
    const arrayBuffer = await file.arrayBuffer();
    const firmware = new Uint8Array(arrayBuffer);
    const totalSize = firmware.length;

    console.log(`Firmware size: ${totalSize} bytes`);

    // Step 1: Begin update
    console.log('Starting update...');
    let response = await fetch(`${baseUrl}/ota/begin`, {
        method: 'POST',
        headers: headers,
        body: JSON.stringify({ size: totalSize })
    });

    if (!response.ok) {
        throw new Error('Failed to start update');
    }

    // Step 2: Write chunks
    const chunkSize = 4096;
    let offset = 0;

    while (offset < totalSize) {
        const chunk = firmware.slice(offset, offset + chunkSize);
        const dataArray = Array.from(chunk);

        const progress = Math.floor(offset * 100 / totalSize);
        console.log(`Writing chunk: ${offset}/${totalSize} (${progress}%)`);

        response = await fetch(`${baseUrl}/ota/write`, {
            method: 'POST',
            headers: headers,
            body: JSON.stringify({
                size: chunk.length,
                data: dataArray
            })
        });

        if (!response.ok) {
            throw new Error('Failed to write chunk');
        }

        const result = await response.json();
        console.log(`  Bytes written: ${result.bytes_written}`);

        offset += chunk.length;
    }

    // Step 3: End update
    console.log('Finalizing update...');
    response = await fetch(`${baseUrl}/ota/end`, {
        method: 'POST',
        headers: headers,
        body: JSON.stringify({})
    });

    if (!response.ok) {
        throw new Error('Failed to finalize update');
    }

    console.log('Update completed! ESP32 will restart.');
}

// Usage in HTML
document.getElementById('uploadBtn').addEventListener('click', async () => {
    const fileInput = document.getElementById('firmwareFile');
    const file = fileInput.files[0];

    try {
        await uploadDisplayFirmware(
            'http://192.168.1.100/display',
            'admin',
            'password',
            file
        );
        alert('Update successful!');
    } catch (error) {
        alert('Update failed: ' + error.message);
    }
});
```

---

### Error Handling

#### Common Errors and Solutions

| Error | Cause | Solution |
|-------|-------|----------|
| 500 on begin | Display not responding | Check connections, verify display is powered |
| "Size too big" on write | Chunk > 4096 bytes | Reduce chunk size to 4096 or less |
| "Chunk too big" on write | Total exceeds begin size | Verify firmware size matches begin request |
| Timeout during write | Display communication issue | Check baud rate, connections, retry |
| ESP32 doesn't restart | Network issue during end | Power cycle the ESP32 manually |

#### Retry Logic Example

```cpp
bool uploadWithRetry(const char* ip, const uint8_t* firmware, size_t size) {
    const int maxRetries = 3;

    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        Serial.printf("Upload attempt %d/%d\n", attempt, maxRetries);

        if (uploadFirmware(ip, firmware, size)) {
            return true;
        }

        if (attempt < maxRetries) {
            Serial.println("Retrying in 5 seconds...");
            delay(5000);
        }
    }

    Serial.println("All upload attempts failed");
    return false;
}
```

---

### Security Considerations

1. **Authentication Required**: All endpoints check web server credentials
2. **HTTPS Recommended**: Use HTTPS in production to encrypt firmware data
3. **Network Security**: Ensure device is on a trusted network during updates
4. **Size Validation**: Maximum chunk size enforced (4096 bytes)
5. **Total Size Validation**: Prevents oversized uploads

---

### Best Practices

1. **Verify Firmware**: Always verify firmware file integrity before upload
2. **Stable Power**: Ensure stable power during display updates
3. **Monitor Progress**: Track bytes_written to monitor update progress
4. **Error Recovery**: Implement retry logic for network issues
5. **Backup**: Keep a working firmware backup before updating
6. **Test First**: Test new firmware on a development display first
7. **Chunk Size**: Use 4096-byte chunks for optimal performance

---

## ESPMegaRecovery Class

The `ESPMegaRecovery` class provides automatic boot loop detection and recovery mode functionality. It protects the ESP32 from becoming unresponsive due to bad firmware by detecting repeated quick restarts and entering a minimal recovery mode that allows firmware to be uploaded via OTA.

### Overview

Recovery mode is triggered when the device restarts 5 consecutive times within 15 seconds (configurable). When in recovery mode:
- All normal application code is blocked
- A minimal web server starts
- Only OTA firmware upload is available
- The device waits indefinitely for new firmware

This prevents "OTA bricking" scenarios where bad firmware causes immediate crashes.

### How It Works

```
┌─────────────────────────────────────────────────┐
│                 Power On / Reset                │
└───────────────────┬─────────────────────────────┘
                    │
                    ▼
        ┌───────────────────────┐
        │  ESPMegaRecovery      │
        │  begin()              │
        └───────┬───────────────┘
                │
                ▼
        ┌───────────────────────┐
        │ Read bootloop counter │
        │ from FRAM             │
        └───────┬───────────────┘
                │
                ▼
        ┌───────────────────────┐
        │ Increment counter     │
        └───────┬───────────────┘
                │
                ▼
        ┌───────────────────────┐
        │ Counter > 5?          │
        └───┬───────────┬───────┘
            │           │
          NO│           │YES
            │           │
            ▼           ▼
    ┌───────────┐  ┌──────────────────┐
    │  Normal   │  │ Reset counter    │
    │  Boot     │  │ Enter Recovery   │
    └─────┬─────┘  │ Mode (blocking)  │
          │        └──────────────────┘
          │
          ▼
    ┌────────────────────┐
    │ Application setup()│
    │ and loop()         │
    └─────┬──────────────┘
          │
          │ Every loop iteration
          ▼
    ┌────────────────────┐
    │ recovery.loop()    │
    │ Watchdog timer     │
    └─────┬──────────────┘
          │
          │ After 15 seconds
          ▼
    ┌────────────────────┐
    │ Reset counter to 0 │
    │ (successful boot)  │
    └────────────────────┘
```

### Constructor

```cpp
ESPMegaRecovery()
```

Default constructor. Initialize all pointers to null and set recovery mode flag to false.

**Example:**
```cpp
ESPMegaRecovery recovery;
```

---

### Methods

#### `void bindFRAM(FRAM* fram, uint32_t address)`
Binds a FRAM storage device to persist the bootloop counter across reboots.

**Parameters:**
- `fram` - Pointer to FRAM instance
- `address` - Memory address in FRAM to store the counter (1 byte)

**Example:**
```cpp
FRAM fram;
ESPMegaRecovery recovery;

void setup() {
    fram.begin();
    recovery.bindFRAM(&fram, 0x0100);  // Store counter at address 0x0100
}
```

#### `void begin()`
Initializes the recovery system and checks for boot loop conditions.

**Behavior:**
1. Reads bootloop counter from FRAM (if bound)
2. Increments the counter
3. If counter > 5:
   - Resets counter (prevents re-entry on next reboot in recovery)
   - Enters recovery mode
   - Blocks forever waiting for OTA upload
4. If counter ≤ 5:
   - Returns normally to continue boot

**Must be called** in `setup()` before other initialization code.

**Example:**
```cpp
void setup() {
    Serial.begin(115200);

    fram.begin();
    recovery.bindFRAM(&fram, 0x0100);
    recovery.begin();  // Will block here if in boot loop

    // Normal application code only runs if NOT in boot loop
    initializeApp();
}
```

#### `void loop()`
Must be called in the main loop to manage the watchdog timer.

**Behavior:**
- If in recovery mode: Blocks forever (infinite loop)
- If normal mode: Monitors uptime
  - After 15 seconds, resets bootloop counter (successful boot)
  - Only resets once per power cycle

**Example:**
```cpp
void loop() {
    recovery.loop();  // Call first in loop

    // Rest of application code
    doApplicationTasks();
}
```

#### `void enterRecoveryMode()`
Manually enters recovery mode and starts minimal OTA server.

**Behavior:**
1. Sets recovery mode flag
2. Initializes networking (ETH)
3. Loads network configuration from FRAM
4. Connects to network
5. Starts web server with only these endpoints:
   - `GET /` - Dashboard
   - `POST /ota_update` - Firmware upload
   - `GET /reboot` - Reboot device
   - `GET /get_device_info` - Device information
   - `GET /config` - Shows "Recovery Mode" message

**Example:**
```cpp
// Manually trigger recovery (e.g., via button press)
if (digitalRead(RECOVERY_BUTTON) == LOW) {
    recovery.enterRecoveryMode();
    recovery.loop();  // Block here
}
```

#### `uint8_t getBootloopCounter()`
Returns the current bootloop counter value.

**Returns:**
- Counter value (0-255)

**Example:**
```cpp
uint8_t count = recovery.getBootloopCounter();
Serial.printf("Boot attempts: %d\n", count);
```

#### `void inclementBootloopCounter()`
Increments the bootloop counter and saves to FRAM.

**Note:** Called automatically by `begin()`. Rarely needs manual calling.

**Example:**
```cpp
// Manually increment if detecting critical error
if (criticalErrorDetected()) {
    recovery.inclementBootloopCounter();
    ESP.restart();
}
```

#### `void resetBootloopCounter()`
Resets the bootloop counter to 0 and saves to FRAM.

**Note:** Called automatically after successful boot (15 seconds uptime).

**Example:**
```cpp
// Manually reset after successful recovery action
if (firmwareRepaired()) {
    recovery.resetBootloopCounter();
}
```

#### `bool isRecoveryMode()`
Checks if the system is currently in recovery mode.

**Returns:**
- `true` if in recovery mode
- `false` if in normal operation

**Example:**
```cpp
if (recovery.isRecoveryMode()) {
    Serial.println("Running in recovery mode - limited functionality");
} else {
    Serial.println("Normal operation");
}
```

---

### Configuration

#### Watchdog Timeout

The watchdog timeout determines how long the device must run without restart to be considered "stable":

```cpp
#define RECOVERY_WATCHDOG_TIMEOUT 15  // seconds
```

To customize, modify the constant in `ESPMegaRecovery.hpp`:

```cpp
#define RECOVERY_WATCHDOG_TIMEOUT 30  // 30 seconds for slower boot
```

#### Boot Loop Threshold

The threshold is hardcoded to 5 consecutive quick reboots. To customize, modify in `begin()`:

```cpp
if(this->getBootloopCounter() > 5) {  // Change this value
    // Enter recovery mode
}
```

---

### Recovery Mode Features

When in recovery mode, the device provides:

#### Limited Web Interface

**Dashboard (GET /)**
- Standard dashboard interface
- Shows "Recovery Mode" status

**Device Info (GET /get_device_info)**
Returns JSON with recovery mode indicators:
```json
{
    "hostname": "espmega",
    "ip_address": "192.168.1.100",
    "mac_address": "AA:BB:CC:DD:EE:FF",
    "model": "ESPMegaPRO R3.3b",
    "mqtt_server": "Recovery",
    "mqtt_port": "Mode",
    "base_topic": "Recovery Mode",
    "mqtt_connected": "Recovery Mode",
    "software_version": "EMG-SAFE-1.0.0",
    "sdk_version": "1.0.0",
    "idf_version": "v4.4.x"
}
```

**Config (GET /config)**
Shows message: "RECOVERY MODE - Configuration is not available in recovery mode"
Redirects to dashboard after 1.5 seconds.

**OTA Upload (POST /ota_update)**
Standard firmware upload functionality - allows uploading new firmware to fix the issue.

**Reboot (GET /reboot)**
Restarts the device after upload.

---

### Complete Usage Example

#### Basic Setup

```cpp
#include <ESPMegaRecovery.hpp>
#include <FRAM.h>

#define RECOVERY_FRAM_ADDRESS 0x0100

FRAM fram;
ESPMegaRecovery recovery;

void setup() {
    Serial.begin(115200);
    Serial.println("ESPMega Starting...");

    // Initialize FRAM
    fram.begin();

    // Initialize recovery system
    recovery.bindFRAM(&fram, RECOVERY_FRAM_ADDRESS);
    recovery.begin();  // Will block here if boot loop detected

    Serial.println("Boot successful, initializing application...");

    // Normal application initialization
    initializeHardware();
    connectNetwork();
    startServices();

    Serial.println("Initialization complete");
}

void loop() {
    // Recovery watchdog (resets counter after 15 seconds)
    recovery.loop();

    // Normal application code
    processData();
    handleCommands();
}
```

#### Advanced: Manual Recovery Button

```cpp
#include <ESPMegaRecovery.hpp>

#define RECOVERY_BUTTON 0  // Boot button

ESPMegaRecovery recovery;
bool recoveryButtonPressed = false;

void checkRecoveryButton() {
    // Hold button during boot to enter recovery
    pinMode(RECOVERY_BUTTON, INPUT_PULLUP);
    delay(100);

    if (digitalRead(RECOVERY_BUTTON) == LOW) {
        Serial.println("Recovery button pressed during boot");
        recoveryButtonPressed = true;
    }
}

void setup() {
    Serial.begin(115200);

    checkRecoveryButton();

    fram.begin();
    recovery.bindFRAM(&fram, 0x0100);

    if (recoveryButtonPressed) {
        Serial.println("Entering manual recovery mode");
        recovery.enterRecoveryMode();
        recovery.loop();  // Block forever
    }

    recovery.begin();  // Check for automatic recovery

    // Normal initialization
    initializeApp();
}

void loop() {
    recovery.loop();
    doWork();
}
```

#### Advanced: Recovery Status Display

```cpp
void displayRecoveryStatus() {
    uint8_t bootCount = recovery.getBootloopCounter();

    if (bootCount > 0) {
        Serial.printf("WARNING: Boot attempt #%d/5\n", bootCount);
        Serial.printf("If this persists, recovery mode will activate\n");
    }

    if (recovery.isRecoveryMode()) {
        Serial.println("====================================");
        Serial.println("         RECOVERY MODE ACTIVE       ");
        Serial.println("====================================");
        Serial.println("Device is in safe mode");
        Serial.println("Upload new firmware via web interface");
        Serial.printf("Connect to: http://%s\n", WiFi.localIP().toString().c_str());
        Serial.println("====================================");
    }
}

void setup() {
    Serial.begin(115200);

    fram.begin();
    recovery.bindFRAM(&fram, 0x0100);

    displayRecoveryStatus();

    recovery.begin();

    if (!recovery.isRecoveryMode()) {
        initializeApp();
    }
}
```

#### Advanced: Graceful Error Recovery

```cpp
enum ErrorSeverity {
    ERROR_MINOR,      // Log and continue
    ERROR_MAJOR,      // Reset subsystem
    ERROR_CRITICAL    // Increment boot counter and restart
};

void handleError(ErrorSeverity severity, const char* message) {
    Serial.printf("ERROR [%d]: %s\n", severity, message);

    switch(severity) {
        case ERROR_MINOR:
            // Just log it
            logError(message);
            break;

        case ERROR_MAJOR:
            // Try to recover
            resetSubsystem();
            break;

        case ERROR_CRITICAL:
            // This is serious - might be bad firmware
            Serial.println("CRITICAL ERROR - Incrementing boot counter");
            recovery.inclementBootloopCounter();
            delay(1000);
            ESP.restart();
            break;
    }
}

void loop() {
    recovery.loop();

    if (!checkSystemHealth()) {
        handleError(ERROR_CRITICAL, "System health check failed");
    }

    doWork();
}
```

---

### Boot Loop Scenarios

#### Example 1: Crash in Setup

```cpp
void setup() {
    recovery.begin();  // Counter: 1

    // This code crashes immediately
    String* ptr = nullptr;
    ptr->charAt(0);  // CRASH!

    // Device restarts...
}

// After 5 restarts:
// - Counter reaches 6
// - Recovery mode activates
// - User can upload fixed firmware
```

#### Example 2: Network Configuration Error

```cpp
void setup() {
    recovery.begin();  // Counter: 1

    // Bad network config causes watchdog timeout
    while(!WiFi.connect("wrong_ssid", "wrong_pass")) {
        delay(100);  // Watchdog timeout -> restart
    }
}

// After 5 watchdog timeouts:
// - Recovery mode activates
// - User can fix network config via web interface
```

#### Example 3: Successful Boot

```cpp
void setup() {
    recovery.begin();  // Counter: 1
    initializeApp();   // Success!
}

void loop() {
    recovery.loop();   // After 15 seconds, counter -> 0
    doWork();
}

// After 15 seconds:
// - Counter reset to 0
// - Next restart will start fresh
```

---

### Troubleshooting

#### Device Stuck in Recovery Mode

**Symptom:** Device always boots into recovery mode

**Causes:**
- Counter stuck at high value in FRAM
- FRAM corruption
- Actual boot loop condition

**Solutions:**
1. Upload working firmware via recovery interface
2. Manually reset FRAM counter:
```cpp
fram.write8(RECOVERY_FRAM_ADDRESS, 0);
ESP.restart();
```

#### Recovery Mode Not Triggering

**Symptom:** Device crashes repeatedly but recovery doesn't activate

**Causes:**
- FRAM not bound before `begin()`
- Counter address conflict
- Watchdog not resetting device

**Solutions:**
1. Verify FRAM binding:
```cpp
recovery.bindFRAM(&fram, address);  // Before begin()
recovery.begin();
```

2. Check counter manually:
```cpp
uint8_t count = fram.read8(RECOVERY_FRAM_ADDRESS);
Serial.printf("Counter: %d\n", count);
```

#### Cannot Access Recovery Web Interface

**Symptom:** Recovery mode active but can't connect to web interface

**Causes:**
- Network configuration invalid
- FRAM network config corrupted
- Ethernet not connected

**Solutions:**
1. Check serial output for IP address
2. Try default IP if static config used
3. Connect via Ethernet cable
4. Reset network config in FRAM if possible

---

### Best Practices

1. **Always Call in Order:**
   ```cpp
   recovery.bindFRAM(&fram, address);  // First
   recovery.begin();                   // Second
   // Then normal initialization
   ```

2. **Check Recovery Status:**
   ```cpp
   if (!recovery.isRecoveryMode()) {
       // Only initialize complex features in normal mode
       initializeComplexFeatures();
   }
   ```

3. **Use Unique FRAM Address:**
   - Don't conflict with other FRAM usage
   - Document the address in your code
   ```cpp
   // FRAM Memory Map:
   // 0x0000-0x00FF: Network config
   // 0x0100: Recovery counter
   // 0x0101-0x01FF: Application config
   ```

4. **Test Recovery Mode:**
   - Deliberately create boot loop to test
   - Verify web interface works
   - Test firmware upload in recovery

5. **Monitor Boot Count:**
   ```cpp
   Serial.printf("Boot count: %d\n", recovery.getBootloopCounter());
   ```

6. **Graceful Degradation:**
   - Don't crash on minor errors
   - Use watchdog timer appropriately
   - Implement error recovery before restarting

---

### FRAM Integration

The recovery system requires FRAM to persist the bootloop counter across reboots.

#### Memory Layout Example

```cpp
// FRAM Address Map
#define FRAM_NETWORK_CONFIG     0x0000  // 256 bytes
#define FRAM_RECOVERY_COUNTER   0x0100  // 1 byte
#define FRAM_WEB_CREDENTIALS    0x0101  // 128 bytes
#define FRAM_APP_CONFIG         0x0181  // 128 bytes
```

#### Complete FRAM Setup

```cpp
#include <FRAM.h>
#include <ESPMegaRecovery.hpp>

FRAM fram;
ESPMegaRecovery recovery;

void setup() {
    // Initialize I2C FRAM
    Wire.begin(21, 22);  // SDA, SCL
    fram.begin();

    // Bind recovery to FRAM
    recovery.bindFRAM(&fram, FRAM_RECOVERY_COUNTER);
    recovery.begin();

    if (!recovery.isRecoveryMode()) {
        // Normal initialization
        initializeApp();
    }
}
```

---

## Summary

### ESPMegaDisplay
- UART-based display control with thread-safe access
- Serial protocol using 0xFF 0xFF 0xFF stop bytes
- Event callbacks for touch, page changes, and custom payloads
- Built-in OTA update capability for display firmware
- Extensible through inheritance for custom implementations

### ESPMegaDisplayOTA
- Web-based OTA updates for display firmware
- Three-phase update process (begin, write, end)
- Authenticated REST API
- Maximum 4096-byte chunks
- Automatic progress tracking

### ESPMegaRecovery
- Automatic boot loop detection
- Safe recovery mode with minimal web interface
- FRAM-based persistence across reboots
- 15-second watchdog timer
- Prevents OTA bricking scenarios

Together, these classes provide a robust system for display management with comprehensive error recovery capabilities.
