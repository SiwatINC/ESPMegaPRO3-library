# ESPMegaWebServer Documentation

## Table of Contents

1. [Overview](#overview)
2. [Class Reference](#class-reference)
3. [Built-in Web Interface Pages](#built-in-web-interface-pages)
4. [Authentication System](#authentication-system)
5. [Credentials Storage in FRAM](#credentials-storage-in-fram)
6. [Custom Endpoint Creation](#custom-endpoint-creation)
7. [REST API](#rest-api)
8. [OTA Update Process](#ota-update-process)
9. [Configuration via Web Interface](#configuration-via-web-interface)
10. [Complete Code Examples](#complete-code-examples)

---

## Overview

The `ESPMegaWebServer` class provides a comprehensive web server implementation for the ESPMegaPRO board. It is built on top of the `AsyncWebServer` library and provides:

- **Built-in web interface** for configuration and OTA updates
- **HTTP Basic Authentication** for security
- **FRAM storage** for persistent credentials
- **REST API** for device information and configuration
- **Custom endpoint support** for user-defined functionality
- **Over-The-Air (OTA) firmware updates** via web interface

### Key Features

- Asynchronous web server for high performance
- Authentication on all endpoints
- Integrated with ESPMegaIoT for network and MQTT configuration
- Simple API for adding custom endpoints
- JSON-based configuration
- Device information API

### FRAM Memory Usage

The ESPMegaWebServer uses **FRAM addresses 301-400** (100 bytes reserved, 64 bytes currently used):
- **301-332**: Web username (32 bytes, null-terminated string)
- **333-364**: Web password (32 bytes, null-terminated string)

---

## Class Reference

### Constructor

#### ESPMegaWebServer(uint16_t port, ESPMegaIoT *iot)

Creates a new web server instance.

**Parameters:**
- `port` - TCP port to listen on (typically 80 for HTTP)
- `iot` - Pointer to an initialized ESPMegaIoT object

**Example:**
```cpp
ESPMegaIoT iot;
ESPMegaWebServer webServer(80, &iot);
```

**Note:** It is recommended to use `ESPMegaPRO::enableWebServer()` instead of creating the object directly.

---

### Public Methods

#### void begin()

Starts the web server and registers all built-in endpoint handlers.

**Usage:**
```cpp
webServer.begin();
```

**Note:**
- Must be called after ESPMegaIoT has been initialized
- Automatically loads credentials from FRAM
- Automatically called by `ESPMegaPRO::enableWebServer()`

**Registered Endpoints:**
- `GET /` - Dashboard (OTA page)
- `GET /config` - Configuration page
- `POST /save_config` - Save configuration
- `GET /get_config` - Get current configuration
- `GET /get_device_info` - Get device information
- `POST /ota_update` - OTA firmware update
- `GET /reboot` - Reboot device

---

#### void loop()

Loop function for the web server.

**Usage:**
```cpp
void loop() {
    webServer.loop();
}
```

**Note:** This method is currently not used as AsyncWebServer handles requests asynchronously.

---

#### void bindFRAM(FRAM *fram)

Binds a FRAM object to the web server for credential storage.

**Parameters:**
- `fram` - Pointer to an initialized FRAM object

**Usage:**
```cpp
FRAM fram;
fram.begin(FRAM_ADDRESS);
webServer.bindFRAM(&fram);
```

**Note:** Must be called before `begin()`.

---

#### void loadCredentialsFromFRAM()

Loads web username and password from FRAM.

**Usage:**
```cpp
webServer.loadCredentialsFromFRAM();
```

**Behavior:**
- Reads credentials from FRAM addresses 301-364
- Validates that credentials are null-terminated strings
- Validates that credentials are at least 1 character long
- Calls `resetCredentials()` if validation fails
- Automatically called by `begin()`

---

#### void saveCredentialsToFRAM()

Saves web username and password to FRAM.

**Usage:**
```cpp
webServer.setWebUsername("admin");
webServer.setWebPassword("newpassword");
webServer.saveCredentialsToFRAM();
```

**Note:** Writes to FRAM addresses 301-364.

---

#### void resetCredentials()

Resets web username and password to default values.

**Usage:**
```cpp
webServer.resetCredentials();
```

**Default Credentials:**
- Username: `admin`
- Password: `admin`

**Note:** Automatically saves the default credentials to FRAM.

---

#### char* getWebUsername()

Returns the current web username.

**Returns:** Pointer to web username string

**Usage:**
```cpp
char* username = webServer.getWebUsername();
Serial.println(username);
```

**Warning:** Do not free or modify the returned pointer.

---

#### char* getWebPassword()

Returns the current web password.

**Returns:** Pointer to web password string

**Usage:**
```cpp
char* password = webServer.getWebPassword();
Serial.println(password);
```

**Warning:** Do not free or modify the returned pointer.

---

#### void setWebUsername(const char* username)

Sets the web username.

**Parameters:**
- `username` - New username (max 31 characters, null-terminated)

**Usage:**
```cpp
webServer.setWebUsername("admin");
webServer.saveCredentialsToFRAM(); // Don't forget to save!
```

**Note:** Must call `saveCredentialsToFRAM()` to persist the change.

---

#### void setWebPassword(const char* password)

Sets the web password.

**Parameters:**
- `password` - New password (max 31 characters, null-terminated)

**Usage:**
```cpp
webServer.setWebPassword("securepassword");
webServer.saveCredentialsToFRAM(); // Don't forget to save!
```

**Note:** Must call `saveCredentialsToFRAM()` to persist the change.

---

#### AsyncWebServer* getServer()

Returns a pointer to the underlying AsyncWebServer object.

**Returns:** Pointer to AsyncWebServer instance

**Usage:**
```cpp
AsyncWebServer* server = webServer.getServer();
// Add custom endpoints
server->on("/custom", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Custom endpoint");
});
```

**Use Cases:**
- Adding custom endpoints
- Advanced server configuration
- Custom route handlers

---

#### bool checkAuthentication(AsyncWebServerRequest *request)

Checks if a request is authenticated and requests authentication if not.

**Parameters:**
- `request` - Pointer to AsyncWebServerRequest object

**Returns:**
- `true` - Request is authenticated
- `false` - Request is not authenticated (authentication has been requested)

**Usage:**
```cpp
void customHandler(AsyncWebServerRequest *request) {
    if (!webServer.checkAuthentication(request)) {
        return; // Authentication requested
    }
    // Handle authenticated request
    request->send(200, "text/plain", "Authenticated!");
}
```

---

### Built-in Endpoint Handlers

These handlers are automatically registered by `begin()`. You typically don't need to call them directly, but they can be useful for understanding the web server's behavior.

#### void dashboardHandler(AsyncWebServerRequest *request)

Handles requests to the dashboard page (`/`).

**Endpoint:** `GET /`

**Response:** HTML page for OTA updates

**Authentication:** Required

---

#### void configHandler(AsyncWebServerRequest *request)

Handles requests to the configuration page (`/config`).

**Endpoint:** `GET /config`

**Response:** HTML configuration page

**Authentication:** Required

---

#### void saveConfigJSONHandler(AsyncWebServerRequest *request, JsonVariant &json)

Handles JSON POST requests to save configuration.

**Endpoint:** `POST /save_config`

**Request Body:** JSON object with configuration fields

**Authentication:** Required

**JSON Fields:**
- `ip_address` - Static IP address (string)
- `netmask` - Network mask (string)
- `gateway` - Gateway IP address (string)
- `dns` - DNS server IP address (string)
- `hostname` - Device hostname (string)
- `bms_ip` - MQTT broker IP address (string)
- `bms_port` - MQTT broker port (integer)
- `bms_useauth` - Use MQTT authentication (boolean)
- `bms_username` - MQTT username (string)
- `bms_password` - MQTT password (string)
- `bms_endpoint` - MQTT base topic (string)
- `web_username` - Web interface username (string)
- `web_password` - Web interface password (string)

**Response:**
- `200 OK` - Configuration saved successfully (device will reboot)
- `400 Bad Request` - Invalid configuration data

---

#### void getConfigHandler(AsyncWebServerRequest *request)

Handles requests to get current configuration.

**Endpoint:** `GET /get_config`

**Response:** JSON object with current configuration

**Authentication:** Required

**Response Fields:** Same as `saveConfigJSONHandler` request body

---

#### void getDeviceInfoHandler(AsyncWebServerRequest *request)

Handles requests to get device information.

**Endpoint:** `GET /get_device_info`

**Response:** JSON object with device information

**Authentication:** Required

**Response Fields:**
- `hostname` - Device hostname
- `ip_address` - Current IP address
- `mac_address` - MAC address
- `model` - Board model
- `mqtt_server` - MQTT broker address
- `mqtt_port` - MQTT broker port
- `base_topic` - MQTT base topic
- `mqtt_connected` - MQTT connection status ("Connected" or "Standalone")
- `software_version` - Firmware version
- `sdk_version` - SDK version
- `idf_version` - ESP-IDF version
- `uptime` - Device uptime in seconds

---

#### void otaRequestHandler(AsyncWebServerRequest *request)

Handles POST request completion for OTA updates.

**Endpoint:** `POST /ota_update` (request handler)

**Authentication:** Required

**Response:**
- `200 OK` with "OK" - Update successful (device will reboot)
- `200 OK` with "FAIL" - Update failed

---

#### void otaUploadHandler(...)

Handles firmware upload chunks for OTA updates.

**Endpoint:** `POST /ota_update` (upload handler)

**Parameters:**
- `request` - AsyncWebServerRequest object
- `filename` - Firmware filename
- `index` - Current byte position
- `data` - Firmware data chunk
- `len` - Length of data chunk
- `final` - True if this is the last chunk

**Authentication:** Required

**Behavior:**
- Receives firmware in chunks
- Writes to flash memory
- Validates firmware
- Logs progress to serial

---

#### void rebootHandler(AsyncWebServerRequest *request)

Handles requests to reboot the device.

**Endpoint:** `GET /reboot`

**Response:** `200 OK` with "Rebooting ESPMega PRO..."

**Authentication:** Required

**Behavior:** Device will reboot after sending response

---

## Built-in Web Interface Pages

### Dashboard (/)

The dashboard page provides:
- OTA firmware update interface
- Device status information
- Quick access to configuration

**Access:** Navigate to `http://<device-ip>/` in a web browser

**Features:**
- Drag-and-drop firmware upload
- Progress indication
- Automatic reboot after successful upload

### Configuration Page (/config)

The configuration page allows you to:
- Configure network settings (IP, gateway, DNS)
- Configure MQTT broker settings
- Change web interface credentials
- Set device hostname

**Access:** Navigate to `http://<device-ip>/config` in a web browser

**Features:**
- Form-based configuration
- Input validation
- Automatic save to FRAM
- Device reboot after save

---

## Authentication System

The ESPMegaWebServer uses **HTTP Basic Authentication** to protect all endpoints.

### How It Works

1. **Request without credentials**: Browser shows authentication dialog
2. **Request with credentials**: Server validates against stored username/password
3. **Valid credentials**: Request is processed normally
4. **Invalid credentials**: 401 Unauthorized response

### Default Credentials

- **Username:** `admin`
- **Password:** `admin`

**Security Warning:** Change the default credentials immediately in production!

### Changing Credentials

#### Method 1: Via Web Interface

1. Navigate to `http://<device-ip>/config`
2. Log in with current credentials
3. Enter new username and password in the form
4. Click "Save Configuration"
5. Device will reboot with new credentials

#### Method 2: Via Code

```cpp
void setup() {
    // ... initialization code ...

    espmega.enableWebServer(80);
    espmega.webServer->setWebUsername("myuser");
    espmega.webServer->setWebPassword("mypassword");
    espmega.webServer->saveCredentialsToFRAM();
}
```

**Warning:** If you set credentials in code, they will be reset every time the device boots unless you comment out the code after first boot.

### Protecting Custom Endpoints

Always check authentication in custom endpoint handlers:

```cpp
void customHandler(AsyncWebServerRequest *request) {
    // Method 1: Using checkAuthentication
    if (!espmega.webServer->checkAuthentication(request)) {
        return; // Authentication dialog shown to user
    }

    // Method 2: Manual check
    if (!request->authenticate(espmega.webServer->getWebUsername(),
                               espmega.webServer->getWebPassword())) {
        return request->requestAuthentication();
    }

    // Your authenticated code here
    request->send(200, "text/plain", "Authenticated!");
}
```

---

## Credentials Storage in FRAM

The web server stores credentials in **non-volatile FRAM** memory, ensuring they persist across power cycles and reboots.

### FRAM Memory Map

| Address Range | Size (bytes) | Content |
|--------------|-------------|---------|
| 301-332 | 32 | Web username (null-terminated) |
| 333-364 | 32 | Web password (null-terminated) |
| 365-400 | 36 | Reserved for future use |

### Validation

When loading credentials from FRAM, the web server validates:

1. **Null termination**: Strings must have a null terminator within 32 bytes
2. **Minimum length**: Username and password must be at least 1 character

If validation fails, credentials are reset to defaults (`admin`/`admin`).

### First Boot

On first boot (or if FRAM is corrupted):
- FRAM may contain random data
- Validation will fail
- Credentials automatically reset to `admin`/`admin`
- Reset credentials are saved to FRAM

### Manual Reset

```cpp
// Reset to default credentials (admin/admin)
espmega.webServer->resetCredentials();
```

---

## Custom Endpoint Creation

You can add custom endpoints to serve your own web pages, APIs, or functionality.

### Basic Custom Endpoint

```cpp
void setup() {
    espmega.enableWebServer(80);

    // Get the AsyncWebServer instance
    AsyncWebServer* server = espmega.webServer->getServer();

    // Add a simple GET endpoint
    server->on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Check authentication
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }
        request->send(200, "text/plain", "Hello, World!");
    });
}
```

### Endpoint with Parameters

```cpp
// GET /api/output?pin=5&state=1
server->on("/api/output", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    // Get URL parameters
    if (request->hasParam("pin") && request->hasParam("state")) {
        int pin = request->getParam("pin")->value().toInt();
        int state = request->getParam("state")->value().toInt();

        // Control output
        espmega.outputs.digitalWrite(pin, state);

        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Missing parameters");
    }
});
```

### JSON Response Endpoint

```cpp
server->on("/api/inputs", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    // Build JSON response
    StaticJsonDocument<512> doc;
    JsonArray inputs = doc.createNestedArray("inputs");

    for (int i = 0; i < 16; i++) {
        inputs.add(espmega.inputs.digitalRead(i));
    }

    // Serialize and send
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});
```

### JSON Request Handler

```cpp
// Add handler for POST /api/config
AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler(
    "/api/config",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }

        JsonObject obj = json.as<JsonObject>();

        // Process JSON data
        if (obj.containsKey("setting1")) {
            String value = obj["setting1"].as<String>();
            // Do something with value
        }

        request->send(200, "text/plain", "Config updated");
    }
);

AsyncWebServer* server = espmega.webServer->getServer();
server->addHandler(handler);
```

### Serving HTML Pages

```cpp
const char custom_page_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Custom Page</title>
</head>
<body>
    <h1>My Custom Page</h1>
    <p>This is a custom HTML page served from ESPMegaPRO.</p>
</body>
</html>
)rawliteral";

void setup() {
    espmega.enableWebServer(80);
    AsyncWebServer* server = espmega.webServer->getServer();

    server->on("/custom", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }
        request->send_P(200, "text/html", custom_page_html);
    });
}
```

### File Download Endpoint

```cpp
server->on("/download/log.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!espmega.webServer->checkAuthentication(request)) {
        return;
    }

    String logData = "Log entry 1\nLog entry 2\nLog entry 3\n";

    AsyncWebServerResponse *response = request->beginResponse(
        200,
        "text/plain",
        logData
    );
    response->addHeader("Content-Disposition", "attachment; filename=log.txt");
    request->send(response);
});
```

---

## REST API

The ESPMegaWebServer provides built-in REST API endpoints for device management.

### GET /get_device_info

Returns comprehensive device information.

**Request:**
```http
GET /get_device_info HTTP/1.1
Authorization: Basic YWRtaW46YWRtaW4=
```

**Response:**
```json
{
    "hostname": "espmega-pro",
    "ip_address": "192.168.1.100",
    "mac_address": "AA:BB:CC:DD:EE:FF",
    "model": "ESPMegaPRO R3",
    "mqtt_server": "192.168.1.10",
    "mqtt_port": 1883,
    "base_topic": "espmega/pro",
    "mqtt_connected": "Connected",
    "software_version": "2.10.0",
    "sdk_version": "1.0.0",
    "idf_version": "v4.4.3",
    "uptime": 3600
}
```

**Usage Example:**
```javascript
// JavaScript/Browser
fetch('http://192.168.1.100/get_device_info', {
    headers: {
        'Authorization': 'Basic ' + btoa('admin:admin')
    }
})
.then(response => response.json())
.then(data => console.log(data));
```

```python
# Python
import requests
from requests.auth import HTTPBasicAuth

response = requests.get(
    'http://192.168.1.100/get_device_info',
    auth=HTTPBasicAuth('admin', 'admin')
)
print(response.json())
```

---

### GET /get_config

Returns current device configuration.

**Request:**
```http
GET /get_config HTTP/1.1
Authorization: Basic YWRtaW46YWRtaW4=
```

**Response:**
```json
{
    "ip_address": "192.168.1.100",
    "netmask": "255.255.255.0",
    "gateway": "192.168.1.1",
    "dns": "8.8.8.8",
    "hostname": "espmega-pro",
    "bms_ip": "192.168.1.10",
    "bms_port": 1883,
    "bms_useauth": false,
    "bms_username": "",
    "bms_password": "",
    "bms_endpoint": "espmega/pro",
    "web_username": "admin",
    "web_password": "admin"
}
```

---

### POST /save_config

Saves device configuration.

**Request:**
```http
POST /save_config HTTP/1.1
Authorization: Basic YWRtaW46YWRtaW4=
Content-Type: application/json

{
    "ip_address": "192.168.1.100",
    "netmask": "255.255.255.0",
    "gateway": "192.168.1.1",
    "dns": "8.8.8.8",
    "hostname": "espmega-pro",
    "bms_ip": "192.168.1.10",
    "bms_port": 1883,
    "bms_useauth": false,
    "bms_username": "",
    "bms_password": "",
    "bms_endpoint": "espmega/pro",
    "web_username": "admin",
    "web_password": "newpassword"
}
```

**Response:**
```
OK
```

**Note:** Device will reboot after successful configuration save.

---

### GET /reboot

Reboots the device.

**Request:**
```http
GET /reboot HTTP/1.1
Authorization: Basic YWRtaW46YWRtaW4=
```

**Response:**
```
Rebooting ESPMega PRO...
```

**Note:** Device will reboot immediately after sending the response.

---

## OTA Update Process

Over-The-Air (OTA) firmware updates allow you to update the device firmware without physical access.

### Web Interface Method

This is the easiest method for most users.

#### Step-by-Step:

1. **Compile Firmware**
   - Build your firmware in Arduino IDE or PlatformIO
   - Locate the `.bin` file in the build output directory
   - For PlatformIO: `.pio/build/<env>/firmware.bin`
   - For Arduino IDE: Use "Sketch > Export Compiled Binary"

2. **Access Dashboard**
   - Navigate to `http://<device-ip>/` in web browser
   - Log in with web credentials

3. **Upload Firmware**
   - Click "Choose File" or drag-and-drop the `.bin` file
   - Click "Update" button
   - Wait for upload to complete (progress shown)
   - Device will reboot automatically

4. **Verify Update**
   - Wait for device to reboot (typically 10-30 seconds)
   - Refresh the page
   - Check device info to verify new firmware version

### Programmatic OTA Update

You can also upload firmware programmatically using HTTP POST.

#### Using curl:

```bash
curl -X POST \
  -u admin:admin \
  -F "file=@firmware.bin" \
  http://192.168.1.100/ota_update
```

#### Using Python:

```python
import requests
from requests.auth import HTTPBasicAuth

url = 'http://192.168.1.100/ota_update'
files = {'file': open('firmware.bin', 'rb')}
auth = HTTPBasicAuth('admin', 'admin')

response = requests.post(url, files=files, auth=auth)
print(response.text)
```

### OTA Update Process Details

1. **Upload Start**
   - Client sends POST request with firmware binary
   - Server authenticates request
   - Server calls `Update.begin()` to prepare flash memory

2. **Upload Progress**
   - Firmware is sent in chunks
   - Each chunk is written to flash using `Update.write()`
   - Progress is logged to serial monitor

3. **Upload Complete**
   - Final chunk triggers `Update.end()`
   - Server validates firmware integrity
   - Response sent to client ("OK" or "FAIL")

4. **Reboot**
   - Device reboots automatically
   - New firmware is executed
   - If new firmware is invalid, device may not boot (use recovery mode)

### Troubleshooting OTA Updates

**Upload fails immediately:**
- Check credentials
- Verify firmware file is valid `.bin` file
- Ensure device is reachable on network

**Upload succeeds but device won't boot:**
- Firmware may be corrupt or incompatible
- Use USB serial connection to flash recovery firmware
- Check compiler settings and board configuration

**Upload is very slow:**
- Normal for large firmware files
- Typical speed: 10-50 KB/s
- Be patient and don't interrupt the process

**Device reboots during upload:**
- Power supply may be unstable
- Network connection may be poor
- Try uploading smaller firmware or via USB

---

## Configuration via Web Interface

The web interface provides an easy way to configure device settings without writing code.

### Network Settings

Configure network parameters:

- **IP Address**: Static IP address for the device
- **Netmask**: Network subnet mask (typically 255.255.255.0)
- **Gateway**: Router/gateway IP address
- **DNS**: DNS server IP address

**Example:**
- IP Address: `192.168.1.100`
- Netmask: `255.255.255.0`
- Gateway: `192.168.1.1`
- DNS: `8.8.8.8`

### MQTT Settings

Configure MQTT broker connection:

- **BMS IP**: MQTT broker IP address or hostname
- **BMS Port**: MQTT broker port (typically 1883)
- **Use Authentication**: Enable if broker requires credentials
- **BMS Username**: MQTT username (if authentication enabled)
- **BMS Password**: MQTT password (if authentication enabled)
- **BMS Endpoint**: Base topic for MQTT messages

**Example:**
- BMS IP: `192.168.1.10`
- BMS Port: `1883`
- Use Authentication: `false`
- BMS Endpoint: `espmega/living-room`

### Web Server Settings

Configure web interface credentials:

- **Web Username**: Username for web interface login
- **Web Password**: Password for web interface login

**Security Recommendations:**
- Use strong, unique passwords
- Change default credentials immediately
- Don't use the same password as MQTT

### Hostname

Configure device hostname:

- **Hostname**: Friendly name for the device

**Example:** `espmega-living-room`

**Note:** Hostname is used for:
- Network identification
- mDNS (if enabled)
- MQTT client ID

---

## Complete Code Examples

### Example 1: Basic Web Server

Minimal setup to enable web server:

```cpp
#include <ESPMegaProOS.hpp>

ESPMegaPRO espmega;

void setup() {
    // Initialize ESPMega board
    espmega.begin();

    // Enable IoT module (required for web server)
    espmega.enableIotModule();

    // Enable web server on port 80
    espmega.enableWebServer(80);

    Serial.println("Web server started!");
    Serial.print("Access at: http://");
    Serial.println(espmega.iot->getIp());
}

void loop() {
    espmega.loop();
}
```

### Example 2: Custom Web Interface

Adding custom web pages:

```cpp
#include <ESPMegaProOS.hpp>

ESPMegaPRO espmega;

const char custom_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESPMega Control</title>
    <style>
        body { font-family: Arial; margin: 20px; }
        .button { padding: 10px 20px; margin: 5px; }
    </style>
</head>
<body>
    <h1>Output Control</h1>
    <button class="button" onclick="setOutput(0, 1)">Output 0 ON</button>
    <button class="button" onclick="setOutput(0, 0)">Output 0 OFF</button>

    <script>
        function setOutput(pin, state) {
            fetch(`/api/output?pin=${pin}&state=${state}`)
                .then(response => response.text())
                .then(data => alert(data));
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
    espmega.begin();
    espmega.enableIotModule();
    espmega.enableWebServer(80);

    // Add custom page
    AsyncWebServer* server = espmega.webServer->getServer();

    server->on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }
        request->send_P(200, "text/html", custom_html);
    });

    // Add API endpoint
    server->on("/api/output", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }

        if (request->hasParam("pin") && request->hasParam("state")) {
            int pin = request->getParam("pin")->value().toInt();
            int state = request->getParam("state")->value().toInt();

            espmega.outputs.digitalWrite(pin, state);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });
}

void loop() {
    espmega.loop();
}
```

### Example 3: RESTful API

Complete REST API for device control:

```cpp
#include <ESPMegaProOS.hpp>

ESPMegaPRO espmega;

void setupRestAPI() {
    AsyncWebServer* server = espmega.webServer->getServer();

    // GET /api/inputs - Get all input states
    server->on("/api/inputs", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }

        StaticJsonDocument<512> doc;
        JsonArray inputs = doc.createNestedArray("inputs");

        for (int i = 0; i < 16; i++) {
            JsonObject input = inputs.createNestedObject();
            input["pin"] = i;
            input["state"] = espmega.inputs.digitalRead(i);
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // GET /api/outputs - Get all output states
    server->on("/api/outputs", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }

        StaticJsonDocument<512> doc;
        JsonArray outputs = doc.createNestedArray("outputs");

        for (int i = 0; i < 16; i++) {
            JsonObject output = outputs.createNestedObject();
            output["pin"] = i;
            output["state"] = espmega.outputs.digitalRead(i);
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // POST /api/output/:pin/:state - Control single output
    server->on("^\\/api\\/output\\/([0-9]+)\\/([0-1])$", HTTP_POST,
        [](AsyncWebServerRequest *request) {
        if (!espmega.webServer->checkAuthentication(request)) {
            return;
        }

        String pinStr = request->pathArg(0);
        String stateStr = request->pathArg(1);

        int pin = pinStr.toInt();
        int state = stateStr.toInt();

        if (pin >= 0 && pin < 16) {
            espmega.outputs.digitalWrite(pin, state);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Invalid pin");
        }
    });
}

void setup() {
    espmega.begin();
    espmega.enableIotModule();
    espmega.enableWebServer(80);

    setupRestAPI();

    Serial.println("REST API ready!");
}

void loop() {
    espmega.loop();
}
```

### Example 4: Secure Configuration Change

Changing web credentials securely:

```cpp
#include <ESPMegaProOS.hpp>

ESPMegaPRO espmega;

void setup() {
    Serial.begin(115200);

    espmega.begin();
    espmega.enableIotModule();
    espmega.enableWebServer(80);

    // Display current credentials
    Serial.println("Current credentials:");
    Serial.print("Username: ");
    Serial.println(espmega.webServer->getWebUsername());
    Serial.print("Password: ");
    Serial.println(espmega.webServer->getWebPassword());

    // IMPORTANT: Uncomment these lines ONCE to set new credentials,
    // then comment them out again to prevent reset on every boot
    /*
    espmega.webServer->setWebUsername("myuser");
    espmega.webServer->setWebPassword("mySecurePassword123");
    espmega.webServer->saveCredentialsToFRAM();
    Serial.println("Credentials updated!");
    */
}

void loop() {
    espmega.loop();
}
```

---

## Best Practices

### Security

1. **Change Default Credentials**
   - Always change from `admin`/`admin` in production
   - Use strong passwords (8+ characters, mixed case, numbers, symbols)

2. **Network Security**
   - Use firewall rules to restrict access
   - Consider using VPN for remote access
   - Don't expose web server directly to internet

3. **Authentication**
   - Always check authentication in custom endpoints
   - Use HTTPS if handling sensitive data (requires additional setup)

### Performance

1. **Async Handlers**
   - Keep handler code fast and non-blocking
   - Avoid delays in request handlers
   - Use async operations for long tasks

2. **Memory Management**
   - Use PROGMEM for large HTML strings
   - Monitor heap usage when serving large responses
   - Limit concurrent connections if needed

3. **JSON Documents**
   - Size JsonDocument appropriately
   - Use StaticJsonDocument when size is known
   - Use DynamicJsonDocument for variable sizes

### Development

1. **Testing**
   - Test OTA updates thoroughly before deployment
   - Keep backup firmware for recovery
   - Test authentication on all custom endpoints

2. **Debugging**
   - Enable serial logging for troubleshooting
   - Use browser developer tools for API testing
   - Monitor serial output during OTA updates

3. **Version Control**
   - Track firmware versions
   - Document API changes
   - Maintain changelog

---

## Troubleshooting

### Cannot Access Web Interface

**Symptoms:** Browser cannot connect to device

**Solutions:**
- Verify device IP address using serial monitor
- Check network connectivity
- Ensure web server is enabled in code
- Verify firewall settings
- Try different browser

### Authentication Fails

**Symptoms:** Credentials rejected even when correct

**Solutions:**
- Reset credentials using `resetCredentials()`
- Check for typos in username/password
- Verify FRAM is working correctly
- Re-flash firmware if FRAM is corrupted

### OTA Update Fails

**Symptoms:** Upload fails or device won't boot after update

**Solutions:**
- Verify `.bin` file is not corrupted
- Check firmware size (must fit in flash)
- Ensure stable power supply
- Use USB serial if OTA completely fails
- Enter recovery mode if bootlooping

### Custom Endpoints Not Working

**Symptoms:** Custom endpoints return 404 or don't respond

**Solutions:**
- Verify endpoint is registered after `enableWebServer()`
- Check URL path matches exactly
- Ensure authentication is handled correctly
- Review serial logs for errors
- Test with simple endpoint first

---

## Additional Resources

### Related Documentation
- [ESPMegaIoT Documentation](ESPMegaIoT.md) - Network and MQTT
- [Getting Started Guide](GETTING_STARTED.md) - Board setup
- [AsyncWebServer Library](https://github.com/me-no-dev/ESPAsyncWebServer) - Underlying web server

### Example Projects
- `examples/WebServer/basic_webserver/` - Minimal web server setup
- `examples/WebServer/custom_endpoints/` - Custom page examples
- `examples/WebServer/rest_api/` - RESTful API implementation
- `examples/WebServer/ota_update/` - OTA update examples

### Support
- GitHub Issues: [ESPMegaPRO Repository](https://github.com/SiwatINC/ESPMegaPRO-v3-SDK)
- Documentation: `/docs_new/` directory
- Examples: `/examples/` directory
