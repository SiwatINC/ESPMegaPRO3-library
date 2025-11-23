# ESPMegaPRO R3 Library

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Library-orange)](https://platformio.org/lib/show/xxxxx/ESPMegaPROR3)
[![Version](https://img.shields.io/badge/version-2.10.0-blue)](https://github.com/SiwatINC/ESPMegaPRO3-library)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

**Comprehensive hardware library for the Siwat INC ESPMegaPRO R3 Industrial IoT Controller**

The ESPMegaPRO R3 is a powerful, industrial-grade ESP32-based controller with expandable I/O cards, built-in networking, and IoT capabilities. This library provides both Object-Oriented and Functional programming interfaces for controlling all aspects of the board.

---

## 📋 Table of Contents

- [Features](#-features)
- [Hardware Overview](#-hardware-overview)
- [Quick Start](#-quick-start)
- [Programming Styles](#-programming-styles)
- [Expansion Cards](#-expansion-cards)
- [IoT & Networking](#-iot--networking)
- [Documentation](#-documentation)
- [Examples](#-examples)
- [Installation](#-installation)
- [Dependencies](#-dependencies)
- [Contributing](#-contributing)
- [License](#-license)

---

## ✨ Features

### Core Features
- **Dual Programming Interface**: Choose between OOP or Functional programming styles
- **Expansion Card System**: Modular design with hot-swappable expansion cards
- **Industrial I/O**: 16 Digital Inputs, 16 PWM Outputs (built-in)
- **IoT Ready**: Built-in MQTT, WiFi, and Ethernet support
- **Web Interface**: Integrated web server for remote configuration
- **Touch Display**: Optional 3.5" TFT LCD with resistive touch (select models)
- **Real-Time Clock**: DS1307 RTC with battery backup
- **Non-Volatile Storage**: I2C FRAM for reliable data persistence
- **OTA Updates**: Over-the-Air firmware updates via web interface

### Supported Expansion Cards
- **Digital Input Card** - 16 channels with debouncing
- **Digital Output Card** - 16 PWM channels (12V push-pull)
- **Analog Card** - 8 ADC inputs + 4 DAC outputs
- **Climate Card** - IR blaster + DHT22/DS18B20 sensor
- **Current Transformer Card** - Energy monitoring and power measurement

---

## 🔧 Hardware Overview

### ESPMegaPRO R3 Board Specifications

| Component | Specification |
|-----------|--------------|
| **Microcontroller** | ESP32-WROOM-32 |
| **Digital Inputs** | 16x isolated inputs (built-in) |
| **PWM Outputs** | 16x channels, 12V, 0.6A per pin, 1.2A per group |
| **Expansion Slots** | Up to 255 addressable cards |
| **Networking** | WiFi 802.11 b/g/n, Ethernet (10/100 Mbps) |
| **Display** | 3.5" TFT LCD (optional, model-dependent) |
| **Storage** | FRAM I2C non-volatile memory |
| **RTC** | DS1307 with battery backup |
| **Power Supply** | 12V or 24V DC (model-dependent) |

### I2C Address Map

| Component | Address | Notes |
|-----------|---------|-------|
| Input Bank A | 0x21 | Built-in digital inputs 0-7 |
| Input Bank B | 0x22 | Built-in digital inputs 8-15 |
| PWM Bank | 0x5F | Built-in PWM outputs 0-15 |
| RTC | 0x68 | DS1307 Real-Time Clock |
| FRAM | 0x56 | Non-volatile storage |
| Analog Input A | 0x48 | Analog Card channels 0-3 |
| Analog Input B | 0x49 | Analog Card channels 4-7 |
| DAC 0-3 | 0x60-0x63 | Analog Card DAC outputs |

---

## 🚀 Quick Start

### Functional Style (Simple & Direct)

Perfect for simple applications and quick prototyping:

```cpp
#include <ESPMegaPRO.h>

void setup() {
    ESPMega_begin();  // Initialize the board

    // Set PWM output on pin 5
    ESPMega_analogWrite(5, 2048);  // 50% duty cycle (0-4095)
}

void loop() {
    ESPMega_loop();  // Must be called regularly

    // Read digital input and mirror to output
    bool input = ESPMega_digitalRead(0);
    ESPMega_digitalWrite(0, input);
}
```

### Object-Oriented Style (Advanced & Scalable)

Recommended for complex applications with multiple cards and IoT features:

```cpp
#include <ESPMegaProOS.hpp>
#include <ETH.h>

ESPMegaPRO espmega = ESPMegaPRO();

void setup() {
    // Initialize board
    espmega.begin();

    // Enable IoT module
    espmega.enableIotModule();

    // Configure and connect to network
    ETH.begin();
    espmega.iot->bindEthernetInterface(&ETH);
    espmega.iot->loadNetworkConfig();
    espmega.iot->connectNetwork();

    // Connect to MQTT
    espmega.iot->loadMqttConfig();
    espmega.iot->connectToMqtt();

    // Register built-in cards with IoT
    espmega.iot->registerCard(0);  // Digital inputs
    espmega.iot->registerCard(1);  // Digital outputs

    // Register callback for input changes
    espmega.inputs.registerCallback([](uint8_t pin, bool state) {
        Serial.printf("Input %d changed to %d\n", pin, state);
    });
}

void loop() {
    espmega.loop();  // Handles all components
}
```

---

## 📚 Programming Styles

### Functional Style (ESPMegaPRO.h)

The functional interface provides simple, Arduino-style functions:

```cpp
#include <ESPMegaPRO.h>

// Digital I/O
bool state = ESPMega_digitalRead(pin);     // Read input pin (0-15)
ESPMega_digitalWrite(pin, HIGH);            // Write to output pin (0-15)

// Analog/PWM I/O
ESPMega_analogWrite(pin, value);            // PWM output (0-4095)
int16_t reading = ESPMega_analogRead(pin);  // Analog input (0-4095)
ESPMega_dacWrite(pin, value);               // DAC output (0-4095)

// RTC
rtctime_t time = ESPMega_getTime();
ESPMega_setTime(hours, minutes, seconds, day, month, year);
ESPMega_updateTimeFromNTP();

// System
ESPMega_begin();  // Initialize
ESPMega_loop();   // Must be called regularly
```

**Limitations**: Only supports built-in I/O. Does not support expansion cards, IoT, or display.

### Object-Oriented Style (ESPMegaProOS.hpp)

The OOP interface provides full access to all features:

```cpp
#include <ESPMegaProOS.hpp>

ESPMegaPRO espmega;

// Access built-in cards
espmega.inputs.digitalRead(pin);
espmega.outputs.digitalWrite(pin, state);
espmega.outputs.analogWrite(pin, value);

// Install expansion cards
espmega.installCard(slot, cardPointer);

// IoT features
espmega.enableIotModule();
espmega.iot->connectNetwork();
espmega.iot->connectToMqtt();

// Display features
espmega.enableInternalDisplay(&Serial);
espmega.display->bindInputCard(&inputCard);

// Web server
espmega.enableWebServer(port);

// FRAM and RTC
espmega.fram.read(address);
rtctime_t time = espmega.getTime();
```

---

## 🔌 Expansion Cards

### Digital Input Card

16 channels of isolated digital inputs with debouncing and callbacks.

```cpp
#include <DigitalInputCard.hpp>

// Create card (built-in uses addresses 0x21, 0x22)
DigitalInputCard inputs(0x21, 0x22);

void setup() {
    inputs.begin();

    // Set debounce time (ms)
    inputs.setDebounceTime(0, 50);

    // Register callback for all pin changes
    inputs.registerCallback([](uint8_t pin, bool state) {
        Serial.printf("Pin %d: %s\n", pin, state ? "HIGH" : "LOW");
    });
}

void loop() {
    inputs.loop();  // Process debouncing and callbacks

    // Read specific pin
    bool state = inputs.digitalRead(5);
}
```

**Features**:
- Hardware debouncing (configurable per pin)
- Change detection callbacks
- Custom pin mapping
- 16-bit buffer access

### Digital Output Card

16 channels of PWM-capable outputs (12V push-pull).

```cpp
#include <DigitalOutputCard.hpp>

// Create card (built-in uses address 0x5F)
DigitalOutputCard outputs(0x5F);

void setup() {
    outputs.begin();

    // Bind to FRAM for state persistence
    outputs.bindFRAM(&fram, 500);  // FRAM address 500
    outputs.setAutoSaveToFRAM(true);
    outputs.loadFromFRAM();  // Restore previous state

    // Register change callback
    outputs.registerChangeCallback([](uint8_t pin, bool state, uint16_t value) {
        Serial.printf("Output %d: state=%d, pwm=%d\n", pin, state, value);
    });
}

void loop() {
    // Digital write
    outputs.digitalWrite(0, HIGH);

    // PWM write (0-4095)
    outputs.analogWrite(1, 2048);  // 50% duty cycle

    // Advanced control
    outputs.setState(2, true);      // Turn on
    outputs.setValue(2, 3000);      // Set PWM value
    outputs.toggleState(3);         // Toggle pin 3

    // Read state
    bool state = outputs.getState(0);
    uint16_t pwm = outputs.getValue(1);
}
```

**Specifications**:
- Current: 0.6A per pin, 1.2A per group (4 pins)
- Groups: 0-3, 4-7, 8-11, 12-15
- PWM Resolution: 12-bit (0-4095)
- Voltage: 12V push-pull

### Analog Card

8 ADC inputs (16-bit) and 4 DAC outputs (12-bit).

```cpp
#include <AnalogCard.hpp>

AnalogCard analogCard;

void setup() {
    analogCard.begin();

    // Register DAC change callback
    analogCard.registerDACChangeCallback([](uint8_t pin, bool state, uint16_t value) {
        Serial.printf("DAC %d: %s, value=%d\n", pin, state ? "ON" : "OFF", value);
    });
}

void loop() {
    // Read ADC (0-7) - Returns 0-4095
    uint16_t reading = analogCard.analogRead(0);
    float voltage = reading * (3.3 / 4095.0);

    // Write DAC (0-3)
    analogCard.dacWrite(0, 2048);  // Output ~1.65V

    // Advanced DAC control
    analogCard.setDACState(1, true);   // Enable DAC
    analogCard.setDACValue(1, 4095);   // Max output

    // Read DAC state
    bool state = analogCard.getDACState(0);
    uint16_t value = analogCard.getDACValue(0);
}
```

### Climate Card

IR blaster for air conditioner control with temperature/humidity monitoring.

```cpp
#include <ClimateCard.hpp>

// Define AC characteristics
const char* mode_names[] = {"off", "fan", "cool"};
const char* fan_speed_names[] = {"auto", "low", "medium", "high"};

size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temp, const uint16_t** code) {
    // Return IR timing array for your AC model
    static const uint16_t ir_codes[3][4][16][200] = { /* Your IR codes */ };
    *code = &ir_codes[mode][fan_speed][temp - 16][0];
    return 200;  // Number of timings
}

AirConditioner ac = {
    .max_temperature = 30,
    .min_temperature = 16,
    .modes = 3,
    .mode_names = mode_names,
    .fan_speeds = 4,
    .fan_speed_names = fan_speed_names,
    .getInfraredCode = &getInfraredCode
};

// Create card (IR on pin 14, DHT22 on pin 15)
ClimateCard climate(14, ac, AC_SENSOR_TYPE_DHT22, 15, RMT_CHANNEL_0);

void setup() {
    climate.begin();

    // Bind to FRAM for state persistence
    climate.bindFRAM(&fram, 1000);
    climate.setFRAMAutoSave(true);
    climate.loadStateFromFRAM();

    // Register callbacks
    climate.registerChangeCallback([](uint8_t mode, uint8_t fan, uint8_t temp) {
        Serial.printf("AC: mode=%d, fan=%d, temp=%d\n", mode, fan, temp);
    });

    climate.registerSensorCallback([](float temp, float humidity) {
        Serial.printf("Room: %.1f°C, %.1f%%\n", temp, humidity);
    });
}

void loop() {
    climate.loop();

    // Control AC
    climate.setTemperature(24);
    climate.setMode(2);  // Cool mode
    climate.setFanSpeed(1);  // Low

    // Or set all at once
    climate.setState(2, 1, 24);

    // Read sensor
    float temp = climate.getRoomTemperature();
    float humidity = climate.getHumidity();
}
```

**Supported Sensors**:
- `AC_SENSOR_TYPE_DHT22` - Temperature + Humidity
- `AC_SENSOR_TYPE_DS18B20` - Temperature only
- `AC_SENSOR_TYPE_NONE` - No sensor

### Current Transformer Card

Power and energy monitoring using current transformers.

```cpp
#include <CurrentTransformerCard.hpp>
#include <AnalogCard.hpp>

AnalogCard analogCard;
float voltage = 220.0;  // Your mains voltage

// ADC to current conversion function
float adcToCurrent(uint16_t adc) {
    // Example: SCT-013-030 (0-30A -> 0-1V)
    float volts = adc * (3.3 / 4095.0);
    return volts * 30.0;  // 30A at 1V
}

// Create CT card (using analog pin 0, 1000ms conversion interval)
CurrentTransformerCard ct(&analogCard, 0, &voltage, adcToCurrent, 1000);

void setup() {
    analogCard.begin();
    ct.begin();

    // Bind to FRAM for energy persistence
    ct.bindFRAM(&fram, 2000);
    ct.setEnergyAutoSave(true);
    ct.loadEnergy();

    // Register callback
    ct.registerCallback([](float current, double energy) {
        Serial.printf("Current: %.2fA, Energy: %.3fkWh\n", current, energy / 1000.0);
    });
}

void loop() {
    ct.loop();

    // Read measurements
    float current = ct.getCurrent();        // Amperes
    double energy = ct.getEnergy();         // Watt-hours
    float power = ct.getPower();            // Watts
    float voltage = ct.getVoltage();        // Volts

    // Reset energy counter
    ct.resetEnergy();
}
```

---

## 🌐 IoT & Networking

### Network Configuration

```cpp
#include <ESPMegaIoT.hpp>

espmega.enableIotModule();

// Option 1: Use Ethernet
ETH.begin();
espmega.iot->bindEthernetInterface(&ETH);

// Option 2: Use WiFi
espmega.iot->connectToWifi("SSID", "password");

// Configure network settings
NetworkConfig netConfig = {
    .ip = {192, 168, 1, 100},
    .gateway = {192, 168, 1, 1},
    .subnet = {255, 255, 255, 0},
    .dns1 = {8, 8, 8, 8},
    .dns2 = {8, 8, 4, 4},
    .useStaticIp = true,
    .useWifi = false,
    .wifiUseAuth = true
};
strcpy(netConfig.hostname, "espmega-device");
strcpy(netConfig.ssid, "MyNetwork");
strcpy(netConfig.password, "MyPassword");

espmega.iot->setNetworkConfig(netConfig);
espmega.iot->saveNetworkConfig();  // Save to FRAM
espmega.iot->connectNetwork();
```

### MQTT Integration

```cpp
// Configure MQTT
MqttConfig mqttConfig = {
    .mqtt_port = 1883,
    .mqtt_useauth = true
};
strcpy(mqttConfig.mqtt_server, "mqtt.example.com");
strcpy(mqttConfig.mqtt_user, "username");
strcpy(mqttConfig.mqtt_password, "password");
strcpy(mqttConfig.base_topic, "home/espmega");

espmega.iot->setMqttConfig(mqttConfig);
espmega.iot->saveMqttConfig();
espmega.iot->connectToMqtt();

// Register cards for auto-publishing
espmega.iot->registerCard(0);  // Digital inputs
espmega.iot->registerCard(1);  // Digital outputs

// Custom MQTT callback
espmega.iot->registerMqttCallback([](char* topic, char* payload) {
    Serial.printf("MQTT: %s = %s\n", topic, payload);
});

// Relative topic callback (strips base topic)
espmega.iot->registerRelativeMqttCallback([](char* topic, char* payload) {
    if (strcmp(topic, "custom/command") == 0) {
        // Handle custom command
    }
});

// Manual publish
espmega.iot->publish("full/topic/path", "message");
espmega.iot->publishRelative("subtopic", "message");  // Publishes to base_topic/subtopic

// Subscribe to topics
espmega.iot->subscribe("external/topic");
espmega.iot->subscribeRelative("commands");  // Subscribes to base_topic/commands
```

**MQTT Topic Structure** (when cards are registered):

Digital Input Card (slot 0):
```
{base_topic}/input/00/state → "0" or "1"
{base_topic}/input/01/state → "0" or "1"
...
{base_topic}/requeststate → Request state publish
```

Digital Output Card (slot 1):
```
{base_topic}/output/00/state → "0" or "1"
{base_topic}/output/00/value → "0" to "4095"
{base_topic}/output/00/set/state → Set state
{base_topic}/output/00/set/value → Set PWM value
```

---

## 🖥️ Web Server

```cpp
espmega.enableWebServer(80);

// Set credentials (saved to FRAM)
espmega.webServer->setWebUsername("admin");
espmega.webServer->setWebPassword("secretpass");
espmega.webServer->saveCredentialsToFRAM();

// Access custom server for endpoints
AsyncWebServer* server = espmega.webServer->getServer();
server->on("/custom", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello from custom endpoint!");
});
```

**Built-in Web Interface**:
- Dashboard: `http://device-ip/`
- Configuration: `http://device-ip/config`
- OTA Update: `http://device-ip/ota`
- REST API: `http://device-ip/rest`

---

## 🖼️ Internal Display

For models with built-in display (3.5" TFT LCD):

```cpp
espmega.enableInternalDisplay(&Serial);

// Bind cards to display
espmega.display->bindInputCard(&espmega.inputs);
espmega.display->bindOutputCard(&espmega.outputs);
espmega.display->bindClimateCard(&climateCard);
```

The display provides:
- Real-time I/O status
- Network and MQTT configuration
- PWM output adjustment
- Climate control interface
- OTA update interface

---

## 💾 FRAM (Non-Volatile Storage)

```cpp
#include <FRAM.h>

// Access FRAM
espmega.fram.write8(address, value);
uint8_t val = espmega.fram.read8(address);

espmega.fram.write16(address, value);
espmega.fram.write32(address, value);

// Write arrays
uint8_t data[] = {1, 2, 3, 4};
espmega.fram.write(address, data, sizeof(data));

// Read arrays
uint8_t buffer[4];
espmega.fram.read(address, buffer, sizeof(buffer));
```

**FRAM Memory Map**:

| Address Range | Component | Size | Purpose |
|--------------|-----------|------|---------|
| 0-33 | Reserved | 34 bytes | System reserved |
| 34-300 | IoT Module | 267 bytes | Network & MQTT config |
| 301-400 | Web Server | 100 bytes | Credentials |
| 400+ | User | Variable | Custom use / Card state |

**Best Practices**:
- Use higher addresses (>400) for custom storage
- Cards auto-save state when FRAM binding enabled
- Always check address conflicts

---

## 🕐 Real-Time Clock

```cpp
// Get time
rtctime_t time = espmega.getTime();
Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
    time.year, time.month, time.day,
    time.hours, time.minutes, time.seconds);

// Set time manually
espmega.setTime(14, 30, 0, 15, 6, 2024);  // 14:30:00, June 15, 2024

// Sync from NTP
espmega.setTimezone("UTC-7");  // Set timezone
if (espmega.updateTimeFromNTP()) {
    Serial.println("Time synced from NTP");
}
```

---

## 📖 Documentation

### Core Documentation
- [Getting Started Guide](docs/GETTING_STARTED.md) - Step-by-step setup
- [API Reference](docs/API_REFERENCE.md) - Complete API documentation
- [Hardware Guide](docs/HARDWARE.md) - Pinouts and specifications
- [MQTT Protocol](docs/MQTT_PROTOCOL.md) - MQTT topic structure

### Examples
- [Basic Examples](examples/) - Simple usage examples
- [Advanced Examples](examples/advanced/) - Complex applications
- [Integration Examples](examples/integration/) - Third-party integration

---

## 🔧 Installation

### PlatformIO (Recommended)

Add to your `platformio.ini`:

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    SiwatINC/ESPMegaPROR3@^2.10.0
```

### Arduino IDE

1. Open Library Manager (Sketch → Include Library → Manage Libraries)
2. Search for "ESPMegaPROR3"
3. Click Install

### Manual Installation

1. Download the latest release from [GitHub](https://github.com/SiwatINC/ESPMegaPRO3-library)
2. Extract to your libraries folder:
   - Arduino: `~/Documents/Arduino/libraries/`
   - PlatformIO: `~/.platformio/lib/`

---

## 📦 Dependencies

This library automatically installs required dependencies:

- Adafruit PWM Servo Driver Library (^2.4.1)
- PCF8574 (^0.3.7)
- Adafruit ADS1X15 (^2.4.0)
- Adafruit BusIO (^1.14.3)
- MCP4725 (^0.3.7)
- FRAM_I2C (^0.6.1)
- Time (^1.6.1)
- DS1307RTC (0.0.0-alpha+sha.c2590c0033)
- PubSubClient (^2.8.0)
- ArduinoJson (^6.21.4)
- DS18B20 (^0.2.1)
- DHTNEW (^0.4.18)
- ESPAsyncWebServer

---

## 🎯 Examples

### Example 1: Simple Input/Output Control

```cpp
#include <ESPMegaPRO.h>

void setup() {
    ESPMega_begin();
}

void loop() {
    ESPMega_loop();

    // Read all inputs and mirror to outputs
    for (int i = 0; i < 16; i++) {
        bool state = ESPMega_digitalRead(i);
        ESPMega_digitalWrite(i, state);
    }
}
```

### Example 2: PWM Dimming

```cpp
#include <ESPMegaPRO.h>

void setup() {
    ESPMega_begin();
}

void loop() {
    ESPMega_loop();

    // Fade PWM output 0 in and out
    static uint16_t pwm = 0;
    static int8_t direction = 1;

    ESPMega_analogWrite(0, pwm);

    pwm += direction * 10;
    if (pwm >= 4095 || pwm <= 0) direction *= -1;

    delay(10);
}
```

### Example 3: MQTT-Controlled Outputs

See [examples/OOP_Firmware/basic_firmware.cpp](examples/OOP_Firmware/basic_firmware.cpp) for a complete example.

### Example 4: Temperature Monitoring with MQTT

```cpp
#include <ESPMegaProOS.hpp>
#include <ClimateCard.hpp>

ESPMegaPRO espmega;
ClimateCard climate(14, ac, AC_SENSOR_TYPE_DHT22, 15, RMT_CHANNEL_0);

void setup() {
    espmega.begin();
    espmega.installCard(2, &climate);

    espmega.enableIotModule();
    espmega.iot->connectNetwork();
    espmega.iot->connectToMqtt();

    // Publish temperature every 30 seconds
    climate.registerSensorCallback([](float temp, float humidity) {
        char buffer[50];
        sprintf(buffer, "{\"temp\":%.1f,\"humidity\":%.1f}", temp, humidity);
        espmega.iot->publishRelative("climate/sensor", buffer);
    });
}

void loop() {
    espmega.loop();
}
```

---

## 🏗️ Project Structure

```
ESPMegaPRO3-library/
├── examples/
│   ├── Functional_PinOperation/    # Simple examples
│   ├── OOP_Firmware/                # Advanced OOP examples
│   └── ...
├── docs/
│   ├── GETTING_STARTED.md
│   ├── API_REFERENCE.md
│   ├── HARDWARE.md
│   └── ...
├── src/                             # Library source files
│   ├── ESPMegaPRO.h                # Functional interface
│   ├── ESPMegaProOS.hpp            # OOP interface
│   ├── DigitalInputCard.hpp
│   ├── DigitalOutputCard.hpp
│   ├── AnalogCard.hpp
│   ├── ClimateCard.hpp
│   ├── CurrentTransformerCard.hpp
│   ├── ESPMegaIoT.hpp
│   ├── ESPMegaWebServer.hpp
│   ├── InternalDisplay.hpp
│   └── ...
├── library.json
├── library.properties
└── README.md
```

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This library is licensed under the MIT License. See [LICENSE](LICENSE) file for details.

---

## 🆘 Support

- **Documentation**: [docs/](docs/)
- **Examples**: [examples/](examples/)
- **Issues**: [GitHub Issues](https://github.com/SiwatINC/ESPMegaPRO3-library/issues)
- **Email**: siwat@siwatinc.com
- **Website**: [https://siwatinc.com](https://siwatinc.com)

---

## 🔄 Version History

### v2.10.0 (Current)
- Added default static IP configuration
- Fixed networking stack crash
- Added uptime counter in web UI
- Improved stability

### Previous Versions
See [CHANGELOG.md](CHANGELOG.md) for complete history.

---

**Made with ❤️ by Siwat INC**
