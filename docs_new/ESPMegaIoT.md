# ESPMegaIoT Documentation

## Table of Contents
1. [Overview](#overview)
2. [NetworkConfig Struct](#networkconfig-struct)
3. [MqttConfig Struct](#mqttconfig-struct)
4. [Network Configuration](#network-configuration)
5. [MQTT Connection](#mqtt-connection)
6. [Card Registration System](#card-registration-system)
7. [Topic Structure](#topic-structure)
8. [Callback Systems](#callback-systems)
9. [Publishing and Subscribing](#publishing-and-subscribing)
10. [FRAM Storage](#fram-storage)
11. [API Reference](#api-reference)
12. [Code Examples](#code-examples)

---

## Overview

The `ESPMegaIoT` class provides comprehensive IoT functionality for the ESPMegaPRO platform. It manages:

- **Network connectivity** (WiFi or Ethernet)
- **MQTT communication** with automatic reconnection
- **Expansion card integration** through the IoTComponent system
- **Configuration persistence** using FRAM storage
- **Flexible topic structure** and callback systems

The class handles all the complexity of network management, allowing you to focus on your application logic.

### Key Features

- Support for both WiFi and Ethernet connectivity
- Static IP or DHCP configuration
- MQTT with optional authentication
- Automatic MQTT reconnection
- Card-based topic routing
- Multiple callback systems for flexibility
- Non-volatile configuration storage
- LWT (Last Will and Testament) support

---

## NetworkConfig Struct

The `NetworkConfig` struct stores all network-related configuration parameters.

### Structure Definition

```cpp
struct NetworkConfig {
    IPAddress ip;           // The IP address
    IPAddress gateway;      // The gateway address
    IPAddress subnet;       // The subnet mask
    IPAddress dns1;         // The primary DNS server
    IPAddress dns2;         // The secondary DNS server
    char hostname[32];      // The hostname
    bool useStaticIp;       // Whether to use static IP (false = DHCP)
    bool useWifi;           // Whether to use WiFi (false = Ethernet)
    bool wifiUseAuth;       // Whether WiFi requires password
    char ssid[32];          // The WiFi SSID
    char password[32];      // The WiFi password
};
```

### Field Details

#### IP Configuration
- **ip**: The static IP address for the device (e.g., `IPAddress(192, 168, 1, 100)`)
- **gateway**: The network gateway (e.g., `IPAddress(192, 168, 1, 1)`)
- **subnet**: The subnet mask (e.g., `IPAddress(255, 255, 255, 0)`)
- **dns1**: Primary DNS server (e.g., `IPAddress(1, 1, 1, 1)` for Cloudflare DNS)
- **dns2**: Secondary DNS server (e.g., `IPAddress(8, 8, 8, 8)` for Google DNS)

#### Device Identification
- **hostname**: The device hostname (max 31 characters + null terminator)
  - Used as MQTT client ID
  - Used for network identification
  - Shown in router DHCP tables

#### Network Mode Selection
- **useStaticIp**:
  - `true`: Use static IP configuration (ip, gateway, subnet must be set)
  - `false`: Use DHCP (IP assigned automatically by router)

- **useWifi**:
  - `true`: Connect via WiFi interface
  - `false`: Connect via Ethernet interface

#### WiFi Configuration
- **wifiUseAuth**:
  - `true`: WiFi network requires password (use ssid and password fields)
  - `false`: Open WiFi network (password field ignored)

- **ssid**: WiFi network name (max 31 characters + null terminator)
- **password**: WiFi password (max 31 characters + null terminator)

### Default Values

If FRAM is empty (all zeros), the following defaults are applied:

```cpp
network_config.ip = IPAddress(192, 168, 0, 99);
network_config.gateway = IPAddress(192, 168, 0, 1);
network_config.subnet = IPAddress(255, 255, 255, 0);
network_config.dns1 = IPAddress(1, 1, 1, 1);
network_config.useStaticIp = true;
```

### Example Configurations

#### Ethernet with Static IP
```cpp
NetworkConfig config;
config.ip = IPAddress(192, 168, 1, 100);
config.gateway = IPAddress(192, 168, 1, 1);
config.subnet = IPAddress(255, 255, 255, 0);
config.dns1 = IPAddress(1, 1, 1, 1);
config.dns2 = IPAddress(8, 8, 8, 8);
strcpy(config.hostname, "espmega-01");
config.useStaticIp = true;
config.useWifi = false;
```

#### WiFi with DHCP
```cpp
NetworkConfig config;
strcpy(config.hostname, "espmega-wifi");
strcpy(config.ssid, "MyNetwork");
strcpy(config.password, "MyPassword");
config.useStaticIp = false;  // Use DHCP
config.useWifi = true;
config.wifiUseAuth = true;
```

#### WiFi (Open Network) with Static IP
```cpp
NetworkConfig config;
config.ip = IPAddress(10, 0, 0, 50);
config.gateway = IPAddress(10, 0, 0, 1);
config.subnet = IPAddress(255, 255, 255, 0);
config.dns1 = IPAddress(1, 1, 1, 1);
strcpy(config.hostname, "espmega-guest");
strcpy(config.ssid, "GuestNetwork");
config.useStaticIp = true;
config.useWifi = true;
config.wifiUseAuth = false;  // Open network
```

---

## MqttConfig Struct

The `MqttConfig` struct stores all MQTT-related configuration parameters.

### Structure Definition

```cpp
struct MqttConfig {
    char mqtt_server[32];      // The MQTT server address
    uint16_t mqtt_port;        // The MQTT server port
    char mqtt_user[32];        // The MQTT username
    char mqtt_password[32];    // The MQTT password
    bool mqtt_useauth;         // Whether to use MQTT authentication
    char base_topic[32];       // The base topic for MQTT messages
};
```

### Field Details

#### Server Configuration
- **mqtt_server**: MQTT broker address
  - Can be IP address (e.g., "192.168.1.5")
  - Can be hostname (e.g., "mqtt.example.com")
  - Max 31 characters + null terminator

- **mqtt_port**: MQTT broker port
  - Standard MQTT: `1883`
  - MQTT over TLS: `8883` (not supported by PubSubClient by default)
  - Custom ports as configured on your broker

#### Authentication
- **mqtt_useauth**:
  - `true`: Connect with username/password authentication
  - `false`: Connect without authentication (mqtt_user and mqtt_password ignored)

- **mqtt_user**: Username for MQTT authentication (max 31 characters + null terminator)
- **mqtt_password**: Password for MQTT authentication (max 31 characters + null terminator)

#### Topic Configuration
- **base_topic**: The root topic for all MQTT messages from this device
  - All card topics are published under: `{base_topic}/{card_id}/{subtopic}`
  - General topics are published under: `{base_topic}/{topic}`
  - Should not include leading or trailing slashes
  - Example: "home/espmega"

### Example Configurations

#### Local MQTT Broker (No Auth)
```cpp
MqttConfig config;
strcpy(config.mqtt_server, "192.168.1.5");
config.mqtt_port = 1883;
config.mqtt_useauth = false;
strcpy(config.base_topic, "home/espmega");
```

#### Cloud MQTT Broker (With Auth)
```cpp
MqttConfig config;
strcpy(config.mqtt_server, "mqtt.example.com");
config.mqtt_port = 1883;
strcpy(config.mqtt_user, "myusername");
strcpy(config.mqtt_password, "mypassword");
config.mqtt_useauth = true;
strcpy(config.base_topic, "devices/espmega-01");
```

#### Home Assistant
```cpp
MqttConfig config;
strcpy(config.mqtt_server, "192.168.1.100");
config.mqtt_port = 1883;
strcpy(config.mqtt_user, "homeassistant");
strcpy(config.mqtt_password, "ha_password");
config.mqtt_useauth = true;
strcpy(config.base_topic, "homeassistant/espmega");
```

---

## Network Configuration

### WiFi vs Ethernet

The ESPMegaIoT supports two network interfaces:

#### Ethernet (Recommended)
- More reliable and stable connection
- Lower latency
- No signal interference issues
- Requires physical cable connection
- Supported on ESPMegaPRO R3 hardware

```cpp
NetworkConfig config;
config.useWifi = false;  // Use Ethernet
// Configure other network settings...
```

#### WiFi
- Wireless connectivity
- Easier physical installation
- May experience signal interference
- Requires WiFi credentials

```cpp
NetworkConfig config;
config.useWifi = true;  // Use WiFi
strcpy(config.ssid, "MyNetwork");
strcpy(config.password, "MyPassword");
config.wifiUseAuth = true;  // Password-protected network
// Configure other network settings...
```

### Static IP vs DHCP

#### Static IP Configuration
Use static IP when you need a predictable, fixed IP address:

**Advantages:**
- Consistent IP address across reboots
- Easier for port forwarding and firewall rules
- Faster connection (no DHCP negotiation)
- Suitable for server/controller applications

**Configuration:**
```cpp
NetworkConfig config;
config.useStaticIp = true;
config.ip = IPAddress(192, 168, 1, 100);
config.gateway = IPAddress(192, 168, 1, 1);
config.subnet = IPAddress(255, 255, 255, 0);
config.dns1 = IPAddress(1, 1, 1, 1);      // Cloudflare DNS
config.dns2 = IPAddress(8, 8, 8, 8);      // Google DNS
```

#### DHCP Configuration
Use DHCP for dynamic IP assignment:

**Advantages:**
- Automatic IP configuration
- No manual IP management
- Easy to move device between networks
- Suitable for client applications

**Configuration:**
```cpp
NetworkConfig config;
config.useStaticIp = false;  // Use DHCP
// IP, gateway, subnet, DNS are assigned automatically
// Hostname is still used for identification
strcpy(config.hostname, "espmega-01");
```

### Network Connection Flow

```cpp
// 1. Set network configuration
ESPMegaIoT iot;
NetworkConfig netConfig;
// ... configure netConfig ...
iot.setNetworkConfig(netConfig);

// 2. Connect to network
iot.connectNetwork();  // Uses settings from NetworkConfig

// 3. Check connection status
if (iot.networkConnected()) {
    Serial.println("Network connected!");
    Serial.print("IP Address: ");
    Serial.println(iot.getIp());
    Serial.print("MAC Address: ");
    Serial.println(iot.getMac());
}
```

---

## MQTT Connection

### Connection Process

The ESPMegaIoT class provides multiple ways to connect to MQTT:

#### Method 1: Using MqttConfig (Recommended)
```cpp
ESPMegaIoT iot;

// Configure MQTT
MqttConfig mqttConfig;
strcpy(mqttConfig.mqtt_server, "192.168.1.5");
mqttConfig.mqtt_port = 1883;
mqttConfig.mqtt_useauth = true;
strcpy(mqttConfig.mqtt_user, "username");
strcpy(mqttConfig.mqtt_password, "password");
strcpy(mqttConfig.base_topic, "home/espmega");

iot.setMqttConfig(mqttConfig);

// Connect
iot.connectToMqtt();  // Uses settings from MqttConfig
```

#### Method 2: Direct Connection (With Auth)
```cpp
char client_id[] = "espmega-01";
char mqtt_server[] = "192.168.1.5";
uint16_t mqtt_port = 1883;
char mqtt_user[] = "username";
char mqtt_password[] = "password";

bool success = iot.connectToMqtt(client_id, mqtt_server, mqtt_port,
                                  mqtt_user, mqtt_password);
```

#### Method 3: Direct Connection (No Auth)
```cpp
char client_id[] = "espmega-01";
char mqtt_server[] = "192.168.1.5";
uint16_t mqtt_port = 1883;

bool success = iot.connectToMqtt(client_id, mqtt_server, mqtt_port);
```

### Authentication

MQTT brokers can require authentication:

#### With Authentication
```cpp
MqttConfig config;
config.mqtt_useauth = true;
strcpy(config.mqtt_user, "myusername");
strcpy(config.mqtt_password, "mypassword");
```

#### Without Authentication
```cpp
MqttConfig config;
config.mqtt_useauth = false;
// mqtt_user and mqtt_password are ignored
```

### Automatic Reconnection

ESPMegaIoT automatically handles MQTT reconnection:

- Checks connection status every 1 second
- Attempts reconnection every 30 seconds (configurable via `MQTT_RECONNECT_INTERVAL`)
- Automatically resubscribes to all topics after reconnection
- Publishes all card states after reconnection

**No manual intervention required** - just call `iot.loop()` regularly:

```cpp
void loop() {
    iot.loop();  // Handles automatic reconnection
}
```

### Connection Status

Check MQTT connection status:

```cpp
if (iot.mqttConnected()) {
    Serial.println("MQTT is connected");
} else {
    Serial.println("MQTT is disconnected");
}
```

### Last Will and Testament (LWT)

ESPMegaIoT automatically sets up LWT for connection monitoring:

- **Availability Topic**: `{base_topic}/availability`
- **Online Message**: `"online"` (published when connected)
- **Offline Message**: `"offline"` (published by broker on disconnect)
- **Retained**: Yes

Subscribe to this topic to monitor device online status:

```
{base_topic}/availability
```

---

## Card Registration System

The card registration system integrates expansion cards with MQTT functionality.

### Overview

Each expansion card installed in ESPMegaPRO can be registered for IoT functionality:

1. **Install the card** using `ESPMegaPRO::installCard()`
2. **Register for IoT** using `ESPMegaIoT::registerCard()`
3. **Automatic MQTT integration** - the card's state is published/subscribed automatically

### Supported Card Types

- `CARD_TYPE_ANALOG` - AnalogCard → AnalogIoT
- `CARD_TYPE_DIGITAL_INPUT` - DigitalInputCard → DigitalInputIoT
- `CARD_TYPE_DIGITAL_OUTPUT` - DigitalOutputCard → DigitalOutputIoT
- `CARD_TYPE_CLIMATE` - ClimateCard → ClimateIoT
- `CARD_TYPE_CT` - CurrentTransformerCard → CurrentTransformerIoT

### Registration Process

```cpp
// Example: Register digital output card at slot 0
espmega.installCard(0, new DigitalOutputCard());
espmega.iot.registerCard(0);

// The card is now accessible via MQTT at:
// {base_topic}/00/...
```

### Card ID Format

Card IDs are always formatted as 2-digit numbers with leading zeros:

- Card 0 → `00`
- Card 5 → `05`
- Card 15 → `15`

### What Happens During Registration

When you call `registerCard(card_id)`:

1. **Card Type Detection**: Determines the expansion card type
2. **IoTComponent Creation**: Creates appropriate IoTComponent wrapper
3. **MQTT Setup**: Initializes MQTT pub/sub for the card
4. **Auto-Subscribe**: If MQTT is connected, subscribes to card topics
5. **State Publishing**: If MQTT is connected, publishes initial state

### Unregistering Cards

```cpp
espmega.iot.unregisterCard(0);  // Unregister card 0
```

This:
- Stops MQTT communication for the card
- Frees memory used by IoTComponent
- Does NOT uninstall the physical card

### Publishing Card States

```cpp
// Publish state for a specific card
espmega.iot.publishCard(0);

// Request state for all cards via MQTT
// Publish to: {base_topic}/requeststate
```

### Example: Multi-Card Setup

```cpp
void setup() {
    // Initialize ESPMegaPRO
    ESPMegaPRO espmega;
    espmega.begin();

    // Install cards
    espmega.installCard(0, new DigitalOutputCard());
    espmega.installCard(1, new DigitalInputCard());
    espmega.installCard(2, new AnalogCard());

    // Setup IoT
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.loadNetworkConfig();
    espmega.iot.loadMqttConfig();
    espmega.iot.connectNetwork();
    espmega.iot.connectToMqtt();

    // Register all cards for IoT
    espmega.iot.registerCard(0);  // Topics: {base_topic}/00/...
    espmega.iot.registerCard(1);  // Topics: {base_topic}/01/...
    espmega.iot.registerCard(2);  // Topics: {base_topic}/02/...
}
```

### Getting IoTComponent Reference

```cpp
IoTComponent* component = espmega.iot.getComponent(0);
if (component != NULL) {
    uint8_t type = component->getType();
    // Use component...
}
```

---

## Topic Structure

ESPMegaIoT uses a hierarchical topic structure for organization and routing.

### Base Topic

All MQTT topics are organized under a **base topic** defined in `MqttConfig`:

```cpp
strcpy(mqtt_config.base_topic, "home/espmega");
```

### Topic Types

#### 1. Card Topics
Format: `{base_topic}/{card_id}/{subtopic}`

Example with base topic `"home/espmega"` and card 0:
```
home/espmega/00/state
home/espmega/00/set
home/espmega/00/value
```

Card-specific topics depend on the card type (see individual card documentation).

#### 2. System Topics
Format: `{base_topic}/{topic}`

**Built-in System Topics:**

| Topic | Type | Description |
|-------|------|-------------|
| `{base_topic}/availability` | Status | Online/offline status (LWT) |
| `{base_topic}/requeststate` | Command | Request all cards to publish state |
| `{base_topic}/requestinfo` | Command | Request system information |
| `{base_topic}/info` | Data | System information (JSON) |

#### 3. Custom Topics
You can publish to any topic under the base topic:

```cpp
// Publish to: home/espmega/custom/sensor
iot.publishRelative("custom/sensor", "23.5");

// Subscribe to: home/espmega/custom/command
iot.subscribeRelative("custom/command");
```

### Topic Hierarchy Example

```
home/espmega/                      (base topic)
├── availability                   (LWT status)
├── requeststate                   (request all states)
├── requestinfo                    (request system info)
├── info                          (system info JSON)
├── 00/                           (Digital Output Card)
│   ├── state                     (outputs state)
│   └── set                       (set output)
├── 01/                           (Digital Input Card)
│   └── input                     (input states)
├── 02/                           (Analog Card)
│   ├── adc/00                    (ADC value 0)
│   ├── adc/01                    (ADC value 1)
│   ├── dac/00/set                (Set DAC 0)
│   └── dac/00/state              (DAC 0 state)
└── custom/                       (Custom topics)
    ├── temperature
    └── humidity
```

### System Information Format

When you publish to `{base_topic}/requestinfo`, the system responds on `{base_topic}/info` with JSON:

```json
{
    "ip": "192.168.1.100",
    "firmware": "2.10.0",
    "sdk_version": "1.0.0",
    "board_model": "ESPMegaPRO R3",
    "cards": [
        {"id": 0, "type": 2},
        {"id": 1, "type": 1},
        {"id": 2, "type": 0}
    ]
}
```

---

## Callback Systems

ESPMegaIoT provides three distinct callback systems for maximum flexibility.

### 1. MQTT Callbacks (Absolute Topics)

These callbacks are triggered for **ALL** MQTT messages received, regardless of topic.

#### Use Cases
- Monitoring all MQTT traffic
- Logging all messages
- Custom topic handling outside base topic
- Debugging

#### Registration
```cpp
uint16_t handler = iot.registerMqttCallback([](char* topic, char* payload) {
    Serial.print("Received on ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);
});

// Later, unregister if needed
iot.unregisterMqttCallback(handler);
```

#### Callback Signature
```cpp
void callback(char* topic, char* payload)
```
- `topic`: Full absolute topic (e.g., `"home/espmega/00/state"`)
- `payload`: Null-terminated string payload

#### Example
```cpp
void setup() {
    iot.registerMqttCallback([](char* topic, char* payload) {
        // Log all messages to SD card
        logFile.print(millis());
        logFile.print(",");
        logFile.print(topic);
        logFile.print(",");
        logFile.println(payload);
    });
}
```

### 2. Relative MQTT Callbacks

These callbacks are triggered only for topics **under the base topic**, with the base topic removed.

#### Use Cases
- Handling custom topics under your base topic
- Simplifying topic parsing
- Application-specific MQTT handlers

#### Registration
```cpp
uint16_t handler = iot.registerRelativeMqttCallback([](char* topic, char* payload) {
    // topic has base topic removed
    // E.g., "custom/sensor" instead of "home/espmega/custom/sensor"

    if (strcmp(topic, "custom/command") == 0) {
        handleCustomCommand(payload);
    }
});

// Later, unregister if needed
iot.unregisterRelativeMqttCallback(handler);
```

#### Callback Signature
```cpp
void callback(char* topic, char* payload)
```
- `topic`: Relative topic with base topic removed (e.g., `"00/state"` or `"custom/sensor"`)
- `payload`: Null-terminated string payload

#### Example
```cpp
void setup() {
    iot.registerRelativeMqttCallback([](char* topic, char* payload) {
        // Handle custom temperature sensor
        if (strcmp(topic, "sensors/temperature") == 0) {
            float temp = atof(payload);
            updateTemperatureDisplay(temp);
        }

        // Handle custom commands
        if (strncmp(topic, "commands/", 9) == 0) {
            executeCommand(topic + 9, payload);
        }
    });
}
```

### 3. Subscribe Callbacks

These callbacks are triggered when the MQTT client is **subscribing to topics** (at connection and reconnection).

#### Use Cases
- Subscribing to custom topics
- Dynamic topic subscription
- Re-subscribing after reconnection

#### Registration
```cpp
uint16_t handler = iot.registerSubscribeCallback([]() {
    // Subscribe to custom topics
    iot.subscribeRelative("custom/sensor");
    iot.subscribe("external/topic");
});

// Later, unregister if needed
iot.unregisterSubscribeCallback(handler);
```

#### Callback Signature
```cpp
void callback(void)
```
- No parameters
- Called during MQTT subscription phase

#### When Are Subscribe Callbacks Called?
- Initial MQTT connection
- After automatic reconnection
- Before card subscriptions

#### Example
```cpp
void setup() {
    iot.registerSubscribeCallback([]() {
        // Subscribe to weather data from external source
        iot.subscribe("weather/outdoor/temperature");
        iot.subscribe("weather/outdoor/humidity");

        // Subscribe to custom control topics
        iot.subscribeRelative("control/mode");
        iot.subscribeRelative("control/setpoint");
    });

    // Handle the subscribed topics
    iot.registerMqttCallback([](char* topic, char* payload) {
        if (strcmp(topic, "weather/outdoor/temperature") == 0) {
            outdoorTemp = atof(payload);
        }
    });
}
```

### Callback Management

#### Handler IDs
All callback registration functions return a unique handler ID:

```cpp
uint16_t handler1 = iot.registerMqttCallback(callback1);
uint16_t handler2 = iot.registerMqttCallback(callback2);
uint16_t handler3 = iot.registerRelativeMqttCallback(callback3);
```

#### Unregistering Callbacks
Use the handler ID to unregister:

```cpp
iot.unregisterMqttCallback(handler1);
iot.unregisterRelativeMqttCallback(handler3);
```

#### Multiple Callbacks
You can register multiple callbacks of each type:

```cpp
// All three will be called for each message
iot.registerMqttCallback(logger);
iot.registerMqttCallback(debugger);
iot.registerMqttCallback(customHandler);
```

### Callback Execution Order

When an MQTT message is received:

1. **All absolute MQTT callbacks** are called first (with full topic)
2. **Topic is checked** against base topic
3. If topic matches base topic:
   - **All relative MQTT callbacks** are called (with relative topic)
   - **Card-specific handlers** are called if applicable

---

## Publishing and Subscribing

### Publishing Messages

#### Absolute Publishing
Publish to any topic:

```cpp
iot.publish("sensor/temperature", "23.5");

// With explicit length (for binary data)
iot.publish("sensor/data", binaryData, dataLength);
```

#### Relative Publishing
Publish relative to base topic:

```cpp
// Publishes to: {base_topic}/custom/sensor
iot.publishRelative("custom/sensor", "23.5");

// With explicit length
iot.publishRelative("custom/data", binaryData, dataLength);
```

### Subscribing to Topics

#### Absolute Subscribing
Subscribe to any topic:

```cpp
iot.subscribe("external/sensor/temperature");
iot.subscribe("weather/+/temperature");  // Wildcard supported
iot.subscribe("home/#");                 // Multi-level wildcard
```

#### Relative Subscribing
Subscribe relative to base topic:

```cpp
// Subscribes to: {base_topic}/custom/command
iot.subscribeRelative("custom/command");

// Subscribes to: {base_topic}/sensors/+
iot.subscribeRelative("sensors/+");
```

### Unsubscribing

```cpp
iot.unsubscribeFromTopic("sensor/temperature");
```

### Publishing Best Practices

#### 1. Use Relative Publishing for Device Topics
```cpp
// Good - organized under your base topic
iot.publishRelative("status/uptime", String(millis()).c_str());

// Avoid - publishing outside your namespace
iot.publish("random/topic", "data");
```

#### 2. Publish JSON for Complex Data
```cpp
#include <ArduinoJson.h>

StaticJsonDocument<200> doc;
doc["temperature"] = 23.5;
doc["humidity"] = 45.2;
doc["pressure"] = 1013.25;

char buffer[200];
serializeJson(doc, buffer);
iot.publishRelative("sensors/environment", buffer);
```

#### 3. Use Retained Messages for State
```cpp
// Not available directly in ESPMegaIoT
// Use PubSubClient directly for retained messages:
iot.publish("state/output", "ON");  // Not retained

// For retained, you need to access the underlying mqtt client
// This is not exposed in current API
```

### Subscribing Best Practices

#### 1. Use Subscribe Callbacks
```cpp
iot.registerSubscribeCallback([]() {
    iot.subscribeRelative("control/+");
    iot.subscribeRelative("settings/#");
});
```

#### 2. Wildcard Subscriptions
```cpp
// Subscribe to all card topics
iot.subscribeRelative("+/state");

// Subscribe to all under a subtree
iot.subscribeRelative("sensors/#");
```

---

## FRAM Storage

ESPMegaIoT uses FRAM (Ferroelectric RAM) for non-volatile configuration storage.

### Overview

FRAM provides:
- **Non-volatile storage** - data persists across power cycles
- **Fast writes** - no wear leveling needed like EEPROM
- **High endurance** - millions of write cycles
- **No write delays** - instant write completion

### FRAM Address Allocation

ESPMegaIoT uses FRAM addresses **34 to 300** (267 bytes total):

| Address Range | Size | Content |
|---------------|------|---------|
| 34-37 | 4 bytes | IP address |
| 38-41 | 4 bytes | Gateway address |
| 42-45 | 4 bytes | Subnet mask |
| 46-49 | 4 bytes | DNS1 address |
| 50-53 | 4 bytes | DNS2 address |
| 54-85 | 32 bytes | Hostname |
| 86 | 1 byte | useStaticIp flag |
| 87 | 1 byte | useWifi flag |
| 88 | 1 byte | wifiUseAuth flag |
| 89-120 | 32 bytes | WiFi SSID |
| 121-152 | 32 bytes | WiFi Password |
| 153-161 | 9 bytes | Reserved |
| 162-163 | 2 bytes | MQTT Port |
| 164-195 | 32 bytes | MQTT Server |
| 196-227 | 32 bytes | MQTT Username |
| 228-259 | 32 bytes | MQTT Password |
| 260 | 1 byte | MQTT useauth flag |
| 261-292 | 32 bytes | MQTT Base Topic |
| 293-300 | 8 bytes | Reserved |

### Binding FRAM

```cpp
#include <FRAM.h>

FRAM fram;

void setup() {
    fram.begin();  // Initialize FRAM hardware

    espmega.iot.bindFRAM(&fram);  // Bind to IoT
}
```

### Saving Configuration

#### Save Network Configuration
```cpp
NetworkConfig config;
config.ip = IPAddress(192, 168, 1, 100);
config.gateway = IPAddress(192, 168, 1, 1);
config.subnet = IPAddress(255, 255, 255, 0);
config.dns1 = IPAddress(1, 1, 1, 1);
strcpy(config.hostname, "espmega-01");
config.useStaticIp = true;
config.useWifi = false;

espmega.iot.setNetworkConfig(config);
espmega.iot.saveNetworkConfig();  // Save to FRAM
```

#### Save MQTT Configuration
```cpp
MqttConfig config;
strcpy(config.mqtt_server, "192.168.1.5");
config.mqtt_port = 1883;
strcpy(config.mqtt_user, "username");
strcpy(config.mqtt_password, "password");
config.mqtt_useauth = true;
strcpy(config.base_topic, "home/espmega");

espmega.iot.setMqttConfig(config);
espmega.iot.saveMqttConfig();  // Save to FRAM
```

### Loading Configuration

#### Load Network Configuration
```cpp
espmega.iot.loadNetworkConfig();  // Load from FRAM

// Access loaded config
NetworkConfig* config = espmega.iot.getNetworkConfig();
Serial.print("Loaded IP: ");
Serial.println(config->ip);
```

#### Load MQTT Configuration
```cpp
espmega.iot.loadMqttConfig();  // Load from FRAM

// Access loaded config
MqttConfig* config = espmega.iot.getMqttConfig();
Serial.print("Loaded MQTT Server: ");
Serial.println(config->mqtt_server);
```

### Typical Startup Sequence

```cpp
void setup() {
    // Initialize FRAM
    fram.begin();
    espmega.iot.bindFRAM(&fram);

    // Load configurations
    espmega.iot.loadNetworkConfig();
    espmega.iot.loadMqttConfig();

    // Connect using loaded config
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.connectNetwork();
    espmega.iot.connectToMqtt();
}
```

### Accessing Configuration

Get pointers to current configuration:

```cpp
NetworkConfig* netConfig = espmega.iot.getNetworkConfig();
MqttConfig* mqttConfig = espmega.iot.getMqttConfig();

// Read values
Serial.println(netConfig->hostname);
Serial.println(mqttConfig->base_topic);

// Modify and save
strcpy(netConfig->hostname, "new-hostname");
espmega.iot.saveNetworkConfig();
```

**Warning**: The returned pointers point to internal structures. Modifications are reflected immediately. Always call `save*Config()` to persist changes.

---

## API Reference

### Constructor and Initialization

#### `ESPMegaIoT()`
Constructor for ESPMegaIoT object.

**Note**: You should not create this object directly. Use `ESPMegaPRO::iot` instead.

```cpp
// Don't do this:
// ESPMegaIoT myIot;

// Do this:
ESPMegaPRO espmega;
espmega.iot.begin();  // Called automatically by ESPMegaPRO
```

#### `void intr_begin(ExpansionCard *cards[])`
Internal initialization function.

**Parameters:**
- `cards`: Array of ExpansionCard pointers

**Note**: Called automatically by `ESPMegaPRO::begin()`. Do not call directly.

#### `void loop()`
Main loop function for IoT operations.

**Note**: Called automatically by `ESPMegaPRO::loop()`. Do not call directly unless you know what you're doing.

**Responsibilities:**
- Calls loop() for all registered IoTComponents
- Processes MQTT messages
- Handles automatic reconnection

---

### Network Configuration

#### `void setNetworkConfig(NetworkConfig network_config)`
Set the network configuration.

**Parameters:**
- `network_config`: NetworkConfig struct with network settings

**Example:**
```cpp
NetworkConfig config;
config.ip = IPAddress(192, 168, 1, 100);
config.gateway = IPAddress(192, 168, 1, 1);
config.subnet = IPAddress(255, 255, 255, 0);
config.dns1 = IPAddress(1, 1, 1, 1);
strcpy(config.hostname, "espmega-01");
config.useStaticIp = true;
config.useWifi = false;

iot.setNetworkConfig(config);
```

#### `NetworkConfig* getNetworkConfig()`
Get pointer to current network configuration.

**Returns:** Pointer to internal NetworkConfig struct

**Warning**: Do not modify the returned struct directly without calling `saveNetworkConfig()` to persist changes.

**Example:**
```cpp
NetworkConfig* config = iot.getNetworkConfig();
Serial.print("Hostname: ");
Serial.println(config->hostname);
```

#### `void loadNetworkConfig()`
Load network configuration from FRAM.

**Requires:** FRAM must be bound using `bindFRAM()` first

**Example:**
```cpp
iot.bindFRAM(&fram);
iot.loadNetworkConfig();
```

#### `void saveNetworkConfig()`
Save current network configuration to FRAM.

**Requires:** FRAM must be bound using `bindFRAM()` first

**Example:**
```cpp
NetworkConfig config;
// ... configure settings ...
iot.setNetworkConfig(config);
iot.saveNetworkConfig();
```

#### `void bindEthernetInterface(ETHClass *ethernetIface)`
Bind the Ethernet interface to the IoT object.

**Parameters:**
- `ethernetIface`: Pointer to ETHClass object (typically `&ETH`)

**Example:**
```cpp
iot.bindEthernetInterface(&ETH);
```

#### `void connectNetwork()`
Connect to the network using current configuration.

**Behavior:**
- Uses settings from NetworkConfig
- Connects via WiFi or Ethernet based on `useWifi` flag
- Applies static IP or uses DHCP based on `useStaticIp` flag

**Example:**
```cpp
iot.loadNetworkConfig();
iot.connectNetwork();
```

#### `bool networkConnected()`
Check if network is connected.

**Returns:** `true` if connected, `false` otherwise

**Example:**
```cpp
if (iot.networkConnected()) {
    Serial.println("Network is connected");
}
```

---

### WiFi Functions

#### `void connectToWifi(const char *ssid, const char *password)`
Connect to a WiFi network with password.

**Parameters:**
- `ssid`: WiFi network name
- `password`: WiFi password

**Example:**
```cpp
iot.connectToWifi("MyNetwork", "MyPassword");
```

#### `void connectToWifi(const char *ssid)`
Connect to an open WiFi network (no password).

**Parameters:**
- `ssid`: WiFi network name

**Example:**
```cpp
iot.connectToWifi("OpenNetwork");
```

#### `void disconnectFromWifi()`
Disconnect from WiFi network.

**Example:**
```cpp
iot.disconnectFromWifi();
```

#### `bool wifiConnected()`
Check if WiFi is connected.

**Returns:** `true` if WiFi connected, `false` otherwise

**Example:**
```cpp
if (iot.wifiConnected()) {
    Serial.println("WiFi connected");
}
```

---

### Ethernet Functions

#### `void ethernetBegin()`
Initialize Ethernet interface.

**Behavior:**
- Sets hostname from NetworkConfig
- Does not configure IP (use `connectNetwork()` for full setup)

**Example:**
```cpp
iot.ethernetBegin();
```

---

### MQTT Configuration

#### `void setMqttConfig(MqttConfig mqtt_config)`
Set the MQTT configuration.

**Parameters:**
- `mqtt_config`: MqttConfig struct with MQTT settings

**Example:**
```cpp
MqttConfig config;
strcpy(config.mqtt_server, "192.168.1.5");
config.mqtt_port = 1883;
config.mqtt_useauth = true;
strcpy(config.mqtt_user, "username");
strcpy(config.mqtt_password, "password");
strcpy(config.base_topic, "home/espmega");

iot.setMqttConfig(config);
```

#### `MqttConfig* getMqttConfig()`
Get pointer to current MQTT configuration.

**Returns:** Pointer to internal MqttConfig struct

**Warning**: Do not modify directly without calling `saveMqttConfig()`.

**Example:**
```cpp
MqttConfig* config = iot.getMqttConfig();
Serial.print("MQTT Server: ");
Serial.println(config->mqtt_server);
```

#### `void loadMqttConfig()`
Load MQTT configuration from FRAM.

**Requires:** FRAM must be bound first

**Example:**
```cpp
iot.loadMqttConfig();
```

#### `void saveMqttConfig()`
Save current MQTT configuration to FRAM.

**Requires:** FRAM must be bound first

**Example:**
```cpp
MqttConfig config;
// ... configure settings ...
iot.setMqttConfig(config);
iot.saveMqttConfig();
```

#### `void setBaseTopic(char *base_topic)`
Set the MQTT base topic.

**Parameters:**
- `base_topic`: The base topic string (max 31 characters)

**Example:**
```cpp
iot.setBaseTopic("home/espmega");
```

---

### MQTT Connection

#### `void connectToMqtt()`
Connect to MQTT using current MqttConfig.

**Requires:** MqttConfig must be set first

**Behavior:**
- Uses hostname as client ID
- Uses credentials if `mqtt_useauth` is true
- Sets up LWT on `{base_topic}/availability`
- Automatically subscribes to topics
- Publishes initial states

**Example:**
```cpp
iot.setMqttConfig(mqttConfig);
iot.connectToMqtt();
```

#### `bool connectToMqtt(char *client_id, char *mqtt_server, uint16_t mqtt_port, char *mqtt_user, char *mqtt_password)`
Connect to MQTT with authentication (direct method).

**Parameters:**
- `client_id`: MQTT client ID
- `mqtt_server`: MQTT broker address
- `mqtt_port`: MQTT broker port
- `mqtt_user`: MQTT username
- `mqtt_password`: MQTT password

**Returns:** `true` if connection successful, `false` otherwise

**Example:**
```cpp
bool success = iot.connectToMqtt("espmega-01", "192.168.1.5", 1883,
                                  "username", "password");
```

#### `bool connectToMqtt(char *client_id, char *mqtt_server, uint16_t mqtt_port)`
Connect to MQTT without authentication (direct method).

**Parameters:**
- `client_id`: MQTT client ID
- `mqtt_server`: MQTT broker address
- `mqtt_port`: MQTT broker port

**Returns:** `true` if connection successful, `false` otherwise

**Example:**
```cpp
bool success = iot.connectToMqtt("espmega-01", "192.168.1.5", 1883);
```

#### `void disconnectFromMqtt()`
Disconnect from MQTT broker.

**Example:**
```cpp
iot.disconnectFromMqtt();
```

#### `bool mqttConnected()`
Check if MQTT is connected.

**Returns:** `true` if MQTT connected, `false` otherwise

**Example:**
```cpp
if (iot.mqttConnected()) {
    Serial.println("MQTT connected");
}
```

---

### Publishing

#### `void publish(const char *topic, const char *payload)`
Publish message to absolute topic.

**Parameters:**
- `topic`: Full topic path
- `payload`: Null-terminated string payload

**Example:**
```cpp
iot.publish("sensor/temperature", "23.5");
```

#### `void publish(const char *topic, const char *payload, unsigned int length)`
Publish message with explicit length (for binary data).

**Parameters:**
- `topic`: Full topic path
- `payload`: Payload data
- `length`: Payload length in bytes

**Example:**
```cpp
uint8_t data[] = {0x01, 0x02, 0x03};
iot.publish("sensor/data", (const char*)data, 3);
```

#### `void publishRelative(const char *topic, const char *payload)`
Publish message relative to base topic.

**Parameters:**
- `topic`: Topic relative to base topic
- `payload`: Null-terminated string payload

**Resulting Topic:** `{base_topic}/{topic}`

**Example:**
```cpp
// Publishes to: home/espmega/sensors/temperature
iot.publishRelative("sensors/temperature", "23.5");
```

#### `void publishRelative(const char *topic, const char *payload, unsigned int length)`
Publish message relative to base topic with explicit length.

**Parameters:**
- `topic`: Topic relative to base topic
- `payload`: Payload data
- `length`: Payload length in bytes

**Example:**
```cpp
uint8_t data[] = {0x01, 0x02, 0x03};
iot.publishRelative("sensors/data", (const char*)data, 3);
```

---

### Subscribing

#### `void subscribe(const char *topic)`
Subscribe to absolute topic.

**Parameters:**
- `topic`: Full topic path (wildcards supported: `+` and `#`)

**Example:**
```cpp
iot.subscribe("external/sensor/temperature");
iot.subscribe("weather/+/temperature");
iot.subscribe("home/#");
```

#### `void subscribeRelative(const char *topic)`
Subscribe to topic relative to base topic.

**Parameters:**
- `topic`: Topic relative to base topic

**Resulting Subscription:** `{base_topic}/{topic}`

**Example:**
```cpp
// Subscribes to: home/espmega/control/+
iot.subscribeRelative("control/+");
```

#### `void unsubscribeFromTopic(const char *topic)`
Unsubscribe from topic.

**Parameters:**
- `topic`: Full topic path

**Example:**
```cpp
iot.unsubscribeFromTopic("sensor/temperature");
```

---

### Callbacks

#### `uint16_t registerMqttCallback(std::function<void(char*, char*)> callback)`
Register callback for all MQTT messages.

**Parameters:**
- `callback`: Function called for each MQTT message

**Callback Signature:** `void(char* topic, char* payload)`
- `topic`: Full absolute topic
- `payload`: Null-terminated payload string

**Returns:** Handler ID for unregistering

**Example:**
```cpp
uint16_t handler = iot.registerMqttCallback([](char* topic, char* payload) {
    Serial.print("MQTT: ");
    Serial.print(topic);
    Serial.print(" = ");
    Serial.println(payload);
});
```

#### `void unregisterMqttCallback(uint16_t handler)`
Unregister MQTT callback.

**Parameters:**
- `handler`: Handler ID from registration

**Example:**
```cpp
iot.unregisterMqttCallback(handler);
```

#### `uint16_t registerRelativeMqttCallback(std::function<void(char*, char*)> callback)`
Register callback for MQTT messages under base topic.

**Parameters:**
- `callback`: Function called for messages under base topic

**Callback Signature:** `void(char* topic, char* payload)`
- `topic`: Relative topic (base topic removed)
- `payload`: Null-terminated payload string

**Returns:** Handler ID for unregistering

**Example:**
```cpp
uint16_t handler = iot.registerRelativeMqttCallback([](char* topic, char* payload) {
    if (strcmp(topic, "custom/command") == 0) {
        executeCommand(payload);
    }
});
```

#### `void unregisterRelativeMqttCallback(uint16_t handler)`
Unregister relative MQTT callback.

**Parameters:**
- `handler`: Handler ID from registration

**Example:**
```cpp
iot.unregisterRelativeMqttCallback(handler);
```

#### `uint16_t registerSubscribeCallback(std::function<void(void)> callback)`
Register callback for MQTT subscription events.

**Parameters:**
- `callback`: Function called during subscription

**Callback Signature:** `void(void)`

**Returns:** Handler ID for unregistering

**Called When:**
- Initial MQTT connection
- After automatic reconnection

**Example:**
```cpp
uint16_t handler = iot.registerSubscribeCallback([]() {
    iot.subscribeRelative("custom/topic");
    iot.subscribe("external/topic");
});
```

#### `void unregisterSubscribeCallback(uint16_t handler)`
Unregister subscribe callback.

**Parameters:**
- `handler`: Handler ID from registration

**Example:**
```cpp
iot.unregisterSubscribeCallback(handler);
```

---

### Card Management

#### `void registerCard(uint8_t card_id)`
Register expansion card for IoT functionality.

**Parameters:**
- `card_id`: ID of the card (0-254)

**Requirements:**
- Card must be installed using `ESPMegaPRO::installCard()` first
- Card type must be supported

**Behavior:**
- Creates appropriate IoTComponent for card type
- Subscribes to card topics if MQTT connected
- Publishes initial state if MQTT connected

**Example:**
```cpp
espmega.installCard(0, new DigitalOutputCard());
espmega.iot.registerCard(0);
```

#### `void unregisterCard(uint8_t card_id)`
Unregister card from IoT functionality.

**Parameters:**
- `card_id`: ID of the card

**Behavior:**
- Stops MQTT communication for card
- Frees IoTComponent memory
- Does NOT uninstall physical card

**Example:**
```cpp
espmega.iot.unregisterCard(0);
```

#### `void publishCard(uint8_t card_id)`
Publish state for specific card.

**Parameters:**
- `card_id`: ID of the card

**Example:**
```cpp
espmega.iot.publishCard(0);
```

#### `IoTComponent* getComponent(uint8_t card_id)`
Get IoTComponent for a card.

**Parameters:**
- `card_id`: ID of the card

**Returns:** Pointer to IoTComponent, or `NULL` if card not registered

**Example:**
```cpp
IoTComponent* component = iot.getComponent(0);
if (component != NULL) {
    uint8_t type = component->getType();
}
```

---

### Network Information

#### `IPAddress getIp()`
Get IP address of active network interface.

**Returns:** Current IP address (WiFi or Ethernet based on config)

**Example:**
```cpp
Serial.print("IP Address: ");
Serial.println(iot.getIp());
```

#### `IPAddress getWifiIp()`
Get WiFi IP address.

**Returns:** WiFi interface IP address

**Example:**
```cpp
Serial.print("WiFi IP: ");
Serial.println(iot.getWifiIp());
```

#### `IPAddress getETHIp()`
Get Ethernet IP address.

**Returns:** Ethernet interface IP address

**Example:**
```cpp
Serial.print("Ethernet IP: ");
Serial.println(iot.getETHIp());
```

#### `String getMac()`
Get MAC address of active network interface.

**Returns:** MAC address string (format: "AA:BB:CC:DD:EE:FF")

**Example:**
```cpp
Serial.print("MAC Address: ");
Serial.println(iot.getMac());
```

#### `String getWifiMac()`
Get WiFi MAC address.

**Returns:** WiFi MAC address string

**Example:**
```cpp
Serial.print("WiFi MAC: ");
Serial.println(iot.getWifiMac());
```

#### `String getETHMac()`
Get Ethernet MAC address.

**Returns:** Ethernet MAC address string

**Example:**
```cpp
Serial.print("Ethernet MAC: ");
Serial.println(iot.getETHMac());
```

---

### System Functions

#### `void bindFRAM(FRAM *fram)`
Bind FRAM object for configuration storage.

**Parameters:**
- `fram`: Pointer to initialized FRAM object

**FRAM Usage:** Addresses 34-300 (267 bytes)

**Example:**
```cpp
FRAM fram;
fram.begin();
iot.bindFRAM(&fram);
```

#### `void publishSystemSummary()`
Publish system information as JSON.

**Published To:** `{base_topic}/info`

**JSON Format:**
```json
{
    "ip": "192.168.1.100",
    "firmware": "2.10.0",
    "sdk_version": "1.0.0",
    "board_model": "ESPMegaPRO R3",
    "cards": [
        {"id": 0, "type": 2},
        {"id": 1, "type": 1}
    ]
}
```

**Example:**
```cpp
iot.publishSystemSummary();

// Or trigger via MQTT:
// Publish to: {base_topic}/requestinfo
```

---

## Code Examples

### Basic Network Connection

```cpp
#include <ESPMegaPRO.h>
#include <ETH.h>

ESPMegaPRO espmega;

void setup() {
    Serial.begin(115200);

    // Initialize ESPMegaPRO
    espmega.begin();

    // Configure network
    NetworkConfig netConfig;
    netConfig.ip = IPAddress(192, 168, 1, 100);
    netConfig.gateway = IPAddress(192, 168, 1, 1);
    netConfig.subnet = IPAddress(255, 255, 255, 0);
    netConfig.dns1 = IPAddress(1, 1, 1, 1);
    strcpy(netConfig.hostname, "espmega-01");
    netConfig.useStaticIp = true;
    netConfig.useWifi = false;  // Use Ethernet

    espmega.iot.setNetworkConfig(netConfig);
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.connectNetwork();

    // Wait for connection
    while (!espmega.iot.networkConnected()) {
        delay(100);
    }

    Serial.println("Network connected!");
    Serial.print("IP: ");
    Serial.println(espmega.iot.getIp());
}

void loop() {
    espmega.loop();
}
```

### Basic MQTT Connection

```cpp
#include <ESPMegaPRO.h>
#include <ETH.h>

ESPMegaPRO espmega;

void setup() {
    Serial.begin(115200);
    espmega.begin();

    // Setup network
    NetworkConfig netConfig;
    netConfig.ip = IPAddress(192, 168, 1, 100);
    netConfig.gateway = IPAddress(192, 168, 1, 1);
    netConfig.subnet = IPAddress(255, 255, 255, 0);
    netConfig.dns1 = IPAddress(1, 1, 1, 1);
    strcpy(netConfig.hostname, "espmega-01");
    netConfig.useStaticIp = true;
    netConfig.useWifi = false;

    espmega.iot.setNetworkConfig(netConfig);
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.connectNetwork();

    // Setup MQTT
    MqttConfig mqttConfig;
    strcpy(mqttConfig.mqtt_server, "192.168.1.5");
    mqttConfig.mqtt_port = 1883;
    mqttConfig.mqtt_useauth = false;
    strcpy(mqttConfig.base_topic, "home/espmega");

    espmega.iot.setMqttConfig(mqttConfig);

    // Wait for network
    while (!espmega.iot.networkConnected()) {
        delay(100);
    }

    // Connect MQTT
    espmega.iot.connectToMqtt();

    Serial.println("Setup complete!");
}

void loop() {
    espmega.loop();

    if (espmega.iot.mqttConnected()) {
        // MQTT operations here
    }
}
```

### Using FRAM for Configuration

```cpp
#include <ESPMegaPRO.h>
#include <ETH.h>
#include <FRAM.h>

ESPMegaPRO espmega;
FRAM fram;

void setup() {
    Serial.begin(115200);
    espmega.begin();

    // Initialize FRAM
    fram.begin();
    espmega.iot.bindFRAM(&fram);

    // Load configurations from FRAM
    espmega.iot.loadNetworkConfig();
    espmega.iot.loadMqttConfig();

    // Connect using loaded config
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.connectNetwork();

    while (!espmega.iot.networkConnected()) {
        delay(100);
    }

    espmega.iot.connectToMqtt();

    Serial.println("Loaded config and connected!");
}

void loop() {
    espmega.loop();
}
```

### Registering Cards for IoT

```cpp
#include <ESPMegaPRO.h>
#include <ETH.h>
#include <DigitalOutputCard.hpp>
#include <DigitalInputCard.hpp>

ESPMegaPRO espmega;

void setup() {
    Serial.begin(115200);
    espmega.begin();

    // Install physical cards
    espmega.installCard(0, new DigitalOutputCard());
    espmega.installCard(1, new DigitalInputCard());

    // Setup network and MQTT
    NetworkConfig netConfig;
    netConfig.ip = IPAddress(192, 168, 1, 100);
    netConfig.gateway = IPAddress(192, 168, 1, 1);
    netConfig.subnet = IPAddress(255, 255, 255, 0);
    netConfig.dns1 = IPAddress(1, 1, 1, 1);
    strcpy(netConfig.hostname, "espmega-01");
    netConfig.useStaticIp = true;
    netConfig.useWifi = false;

    espmega.iot.setNetworkConfig(netConfig);
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.connectNetwork();

    MqttConfig mqttConfig;
    strcpy(mqttConfig.mqtt_server, "192.168.1.5");
    mqttConfig.mqtt_port = 1883;
    mqttConfig.mqtt_useauth = false;
    strcpy(mqttConfig.base_topic, "home/espmega");

    espmega.iot.setMqttConfig(mqttConfig);

    while (!espmega.iot.networkConnected()) {
        delay(100);
    }

    espmega.iot.connectToMqtt();

    // Register cards for IoT
    espmega.iot.registerCard(0);  // Topics: home/espmega/00/...
    espmega.iot.registerCard(1);  // Topics: home/espmega/01/...

    Serial.println("Cards registered!");
}

void loop() {
    espmega.loop();
}
```

### Using Callbacks

```cpp
#include <ESPMegaPRO.h>
#include <ETH.h>

ESPMegaPRO espmega;

void setup() {
    Serial.begin(115200);
    espmega.begin();

    // Setup network and MQTT (abbreviated)
    // ... setup code here ...

    // Register MQTT callback for all topics
    espmega.iot.registerMqttCallback([](char* topic, char* payload) {
        Serial.print("Message on: ");
        Serial.print(topic);
        Serial.print(" -> ");
        Serial.println(payload);
    });

    // Register relative callback for topics under base topic
    espmega.iot.registerRelativeMqttCallback([](char* topic, char* payload) {
        if (strcmp(topic, "custom/command") == 0) {
            Serial.print("Custom command: ");
            Serial.println(payload);
        }
    });

    // Register subscribe callback
    espmega.iot.registerSubscribeCallback([]() {
        Serial.println("Subscribing to custom topics...");
        espmega.iot.subscribeRelative("custom/command");
        espmega.iot.subscribeRelative("sensors/+");
    });

    espmega.iot.connectToMqtt();
}

void loop() {
    espmega.loop();
}
```

### Publishing Custom Data

```cpp
#include <ESPMegaPRO.h>
#include <ETH.h>
#include <ArduinoJson.h>

ESPMegaPRO espmega;
unsigned long lastPublish = 0;

void setup() {
    Serial.begin(115200);
    espmega.begin();

    // Setup network and MQTT (abbreviated)
    // ... setup code here ...
}

void loop() {
    espmega.loop();

    // Publish sensor data every 5 seconds
    if (millis() - lastPublish > 5000) {
        lastPublish = millis();

        if (espmega.iot.mqttConnected()) {
            // Simple value
            float temperature = 23.5;
            espmega.iot.publishRelative("sensors/temperature",
                                        String(temperature).c_str());

            // JSON data
            StaticJsonDocument<200> doc;
            doc["temperature"] = 23.5;
            doc["humidity"] = 45.2;
            doc["pressure"] = 1013.25;

            char buffer[200];
            serializeJson(doc, buffer);
            espmega.iot.publishRelative("sensors/environment", buffer);
        }
    }
}
```

---

## Complete Example: Production Setup

```cpp
#include <ESPMegaPRO.h>
#include <ETH.h>
#include <FRAM.h>
#include <DigitalOutputCard.hpp>
#include <DigitalInputCard.hpp>
#include <AnalogCard.hpp>

ESPMegaPRO espmega;
FRAM fram;

bool firstBoot = false;

void setup() {
    Serial.begin(115200);
    Serial.println("ESPMegaPRO IoT Starting...");

    // Initialize core
    espmega.begin();

    // Initialize FRAM
    fram.begin();
    espmega.iot.bindFRAM(&fram);

    // Load or set default configuration
    espmega.iot.loadNetworkConfig();
    espmega.iot.loadMqttConfig();

    NetworkConfig* netConfig = espmega.iot.getNetworkConfig();

    // Check if this is first boot (hostname empty)
    if (strlen(netConfig->hostname) == 0) {
        firstBoot = true;
        Serial.println("First boot detected, setting defaults...");

        // Set default network config
        netConfig->ip = IPAddress(192, 168, 1, 100);
        netConfig->gateway = IPAddress(192, 168, 1, 1);
        netConfig->subnet = IPAddress(255, 255, 255, 0);
        netConfig->dns1 = IPAddress(1, 1, 1, 1);
        netConfig->dns2 = IPAddress(8, 8, 8, 8);
        strcpy(netConfig->hostname, "espmega-01");
        netConfig->useStaticIp = true;
        netConfig->useWifi = false;

        espmega.iot.saveNetworkConfig();

        // Set default MQTT config
        MqttConfig mqttConfig;
        strcpy(mqttConfig.mqtt_server, "192.168.1.5");
        mqttConfig.mqtt_port = 1883;
        mqttConfig.mqtt_useauth = false;
        strcpy(mqttConfig.base_topic, "home/espmega");

        espmega.iot.setMqttConfig(mqttConfig);
        espmega.iot.saveMqttConfig();
    }

    // Install expansion cards
    espmega.installCard(0, new DigitalOutputCard());
    espmega.installCard(1, new DigitalInputCard());
    espmega.installCard(2, new AnalogCard());

    // Connect network
    espmega.iot.bindEthernetInterface(&ETH);
    espmega.iot.connectNetwork();

    Serial.println("Waiting for network...");
    unsigned long startTime = millis();
    while (!espmega.iot.networkConnected()) {
        delay(100);
        if (millis() - startTime > 30000) {
            Serial.println("Network connection timeout!");
            return;
        }
    }

    Serial.println("Network connected!");
    Serial.print("IP Address: ");
    Serial.println(espmega.iot.getIp());
    Serial.print("MAC Address: ");
    Serial.println(espmega.iot.getMac());

    // Connect MQTT
    espmega.iot.connectToMqtt();

    Serial.println("Waiting for MQTT...");
    startTime = millis();
    while (!espmega.iot.mqttConnected()) {
        delay(100);
        espmega.loop();  // Allow reconnection attempts
        if (millis() - startTime > 30000) {
            Serial.println("MQTT connection timeout!");
            break;
        }
    }

    if (espmega.iot.mqttConnected()) {
        Serial.println("MQTT connected!");

        // Register cards for IoT
        espmega.iot.registerCard(0);
        espmega.iot.registerCard(1);
        espmega.iot.registerCard(2);

        // Register callbacks
        espmega.iot.registerRelativeMqttCallback([](char* topic, char* payload) {
            Serial.print("Relative message: ");
            Serial.print(topic);
            Serial.print(" = ");
            Serial.println(payload);
        });

        espmega.iot.registerSubscribeCallback([]() {
            Serial.println("Subscribing to custom topics...");
            espmega.iot.subscribeRelative("custom/#");
        });

        // Publish system info
        espmega.iot.publishSystemSummary();
    }

    Serial.println("Setup complete!");
}

void loop() {
    espmega.loop();

    // Your application code here

    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 60000) {  // Every minute
        lastHeartbeat = millis();

        if (espmega.iot.mqttConnected()) {
            char buffer[50];
            sprintf(buffer, "%lu", millis() / 1000);
            espmega.iot.publishRelative("uptime", buffer);
        }
    }
}
```

---

## IoTComponent Class Reference

The `IoTComponent` class is the base class for card-specific IoT implementations.

### Overview

Each expansion card type has a corresponding IoTComponent subclass:
- AnalogCard → AnalogIoT
- DigitalInputCard → DigitalInputIoT
- DigitalOutputCard → DigitalOutputIoT
- ClimateCard → ClimateIoT
- CurrentTransformerCard → CurrentTransformerIoT

### Virtual Methods

Subclasses must implement:

```cpp
// Initialize the component
virtual bool begin(uint8_t card_id, ExpansionCard *card,
                   PubSubClient *mqtt, char *base_topic);

// Handle incoming MQTT messages
virtual void handleMqttMessage(char *topic, char *payload);

// Publish current state
virtual void publishReport();

// Get card type
virtual uint8_t getType();

// Subscribe to MQTT topics
virtual void subscribe();

// Main loop (optional)
virtual void loop();
```

### Protected Helper Methods

Available to subclasses:

```cpp
// Publish relative to: {base_topic}/{card_id}/{topic}
void publishRelative(const char *topic, const char *payload);

// Subscribe relative to: {base_topic}/{card_id}/{topic}
void subscribeRelative(const char *topic);
```

### Protected Members

```cpp
char *base_topic;           // MQTT base topic
PubSubClient *mqtt;         // MQTT client
uint8_t card_id;            // Card ID (0-254)
```

### Example Usage (Creating Custom IoTComponent)

```cpp
class MyCustomIoT : public IoTComponent {
public:
    bool begin(uint8_t card_id, ExpansionCard *card,
               PubSubClient *mqtt, char *base_topic) override {
        this->card_id = card_id;
        this->card = (MyCustomCard*)card;
        this->mqtt = mqtt;
        this->base_topic = base_topic;
        return true;
    }

    void subscribe() override {
        subscribeRelative("set");
        subscribeRelative("command");
    }

    void handleMqttMessage(char *topic, char *payload) override {
        if (strcmp(topic, "set") == 0) {
            // Handle set command
        }
    }

    void publishReport() override {
        char buffer[50];
        sprintf(buffer, "%d", card->getValue());
        publishRelative("state", buffer);
    }

    uint8_t getType() override {
        return CARD_TYPE_CUSTOM;
    }

private:
    MyCustomCard *card;
};
```

---

## Troubleshooting

### Network Won't Connect

**Ethernet:**
- Check cable connection
- Verify static IP is in correct subnet
- Check gateway address is correct
- Try DHCP instead of static IP

**WiFi:**
- Verify SSID and password
- Check WiFi signal strength
- Try open network first to test
- Verify `wifiUseAuth` flag matches network type

### MQTT Won't Connect

- Verify MQTT broker is running (`mosquitto -v` or check broker logs)
- Check firewall isn't blocking port 1883
- Verify authentication credentials if using auth
- Check base topic doesn't have leading/trailing slashes
- Monitor broker logs for connection attempts

### MQTT Connects but No Messages

- Verify topics are correct (check with MQTT client like MQTT Explorer)
- Check callbacks are registered before connection
- Ensure `loop()` is called regularly
- Verify cards are registered

### Configuration Not Persisting

- Verify FRAM is initialized and bound
- Call `saveNetworkConfig()` or `saveMqttConfig()` after changes
- Check FRAM addresses aren't used by other code
- Verify FRAM hardware is working

### Random Disconnections

- Check network stability
- Verify power supply is adequate
- Increase `MQTT_RECONNECT_INTERVAL` if broker is slow
- Check TCP timeout setting

---

## Advanced Topics

### Custom Topic Namespaces

Organize your topics into namespaces:

```cpp
// Sensors namespace
iot.publishRelative("sensors/temperature", temp);
iot.publishRelative("sensors/humidity", humid);

// Controls namespace
iot.publishRelative("controls/mode", mode);
iot.publishRelative("controls/setpoint", setpoint);

// Status namespace
iot.publishRelative("status/uptime", uptime);
iot.publishRelative("status/memory", freeHeap);
```

### Retained Messages

PubSubClient supports retained messages, but ESPMegaIoT doesn't expose this directly. Access the underlying MQTT client if needed:

```cpp
// Not recommended, but possible
// (ESPMegaIoT doesn't expose mqtt client publicly)
```

### QoS Levels

Default QoS is 0 (at most once). ESPMegaIoT doesn't currently support QoS 1 or 2.

### Large Payloads

Payload buffer is 200 bytes. For larger payloads:
- Split into multiple messages
- Use external storage
- Increase `payload_buffer` size (requires modifying library)

---

## See Also

- [DigitalOutputIoT Documentation](DigitalOutputIoT.md)
- [DigitalInputIoT Documentation](DigitalInputIoT.md)
- [AnalogIoT Documentation](AnalogIoT.md)
- [ClimateIoT Documentation](ClimateIoT.md)
- [ESPMegaPRO Documentation](ESPMegaPRO.md)
- [PubSubClient Library](https://github.com/knolleary/pubsubclient)

---

**End of ESPMegaIoT Documentation**
