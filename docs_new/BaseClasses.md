# Base Classes and Common Structures

This document covers the fundamental base classes and common structures used throughout the ESPMegaPRO library. Understanding these classes is essential for creating custom expansion cards and IoT components.

## Table of Contents

- [ExpansionCard Base Class](#expansioncard-base-class)
- [IoTComponent Base Class](#iotcomponent-base-class)
- [TimeStructure](#timestructure)
- [ESPMegaCommon](#espmegacommon)
- [ESPMegaTCP](#espmegatcp)
- [ESPMegaRTU](#espmegartu)

---

## ExpansionCard Base Class

### Overview

`ExpansionCard` is the abstract base class for all hardware expansion cards in the ESPMegaPRO ecosystem. It provides a common interface that all expansion cards must implement, allowing the system to manage different types of cards uniformly.

### Purpose and Inheritance

The `ExpansionCard` class serves as:
- A common interface for hardware expansion cards
- An abstraction layer for different card types
- A contract that defines the minimum functionality required by all cards

All expansion cards (DigitalInputCard, DigitalOutputCard, AnalogCard, ClimateCard, etc.) inherit from this base class.

### Class Definition

```cpp
class ExpansionCard {
    public:
        ExpansionCard() {}
        virtual bool begin();
        virtual void loop();
        virtual uint8_t getType();
};
```

### Virtual Methods to Implement

When creating a custom expansion card, you must implement the following virtual methods:

#### `begin()`

```cpp
virtual bool begin();
```

**Purpose:** Initialize the expansion card hardware and prepare it for operation.

**Returns:** `true` if initialization succeeds, `false` otherwise.

**Implementation Requirements:**
- Initialize all hardware components (I2C devices, GPIO pins, etc.)
- Set up initial state variables
- Perform any necessary hardware configuration
- Return `true` on success, `false` on failure

**Example:**
```cpp
bool MyCustomCard::begin() {
    // Initialize I2C device
    if (!myDevice.begin(DEVICE_ADDRESS)) {
        return false;
    }

    // Configure initial settings
    myDevice.setMode(NORMAL_MODE);

    // Initialize state variables
    initialized = true;

    return true;
}
```

#### `loop()`

```cpp
virtual void loop();
```

**Purpose:** Perform periodic operations that need to be executed in the main loop.

**Implementation Requirements:**
- Read sensor data or input buffers
- Process state changes
- Trigger callbacks for detected events
- Perform any time-sensitive operations
- Should execute quickly and not block

**Example:**
```cpp
void MyCustomCard::loop() {
    // Read current sensor values
    uint16_t currentValue = myDevice.readValue();

    // Check for changes
    if (currentValue != previousValue) {
        // Trigger callbacks
        for (auto& callback : callbacks) {
            callback.second(currentValue);
        }
        previousValue = currentValue;
    }
}
```

#### `getType()`

```cpp
virtual uint8_t getType();
```

**Purpose:** Return a unique identifier for this card type.

**Returns:** A unique 8-bit value identifying the card type (0-255).

**Implementation Requirements:**
- Return a unique card type ID
- Use a consistent value across all instances of the same card type
- Define the type as a constant for clarity

**Example:**
```cpp
#define CARD_TYPE_CUSTOM 0x10

uint8_t MyCustomCard::getType() {
    return CARD_TYPE_CUSTOM;
}
```

### Card Type IDs

The following card type IDs are already in use:

| Card Type | ID | Description |
|-----------|----|-----------  |
| Digital Input | 0x01 | 16-channel digital input card |
| Analog | 0x02 | 8 ADC inputs + 4 DAC outputs |
| Digital Output | 0x03 | 16-channel digital output card |
| Climate | 0x04 | Temperature/humidity sensor card |
| Current Transformer | 0x05 | Energy monitoring card |

Choose a unique ID for your custom card (e.g., 0x10 and above for user-defined cards).

### How to Create Custom Expansion Cards

Follow this step-by-step guide to create a custom expansion card:

#### Step 1: Define Your Card Class

Create a header file (e.g., `MyCustomCard.hpp`):

```cpp
#pragma once
#include <ExpansionCard.hpp>

// Define unique card type ID
#define CARD_TYPE_CUSTOM 0x10

class MyCustomCard : public ExpansionCard {
    public:
        MyCustomCard(uint8_t address);
        bool begin() override;
        void loop() override;
        uint8_t getType() override;

        // Custom methods specific to your card
        uint16_t readSensor();
        void writeOutput(uint8_t value);
        uint8_t registerCallback(std::function<void(uint16_t)> callback);
        void unregisterCallback(uint8_t handler);

    private:
        uint8_t device_address;
        uint16_t currentValue;
        uint16_t previousValue;
        bool initialized;

        // Callback management
        uint8_t callback_index;
        std::map<uint8_t, std::function<void(uint16_t)>> callbacks;
};
```

#### Step 2: Implement the Virtual Methods

Create an implementation file (e.g., `MyCustomCard.cpp`):

```cpp
#include "MyCustomCard.hpp"

MyCustomCard::MyCustomCard(uint8_t address)
    : device_address(address),
      currentValue(0),
      previousValue(0),
      initialized(false),
      callback_index(0) {
}

bool MyCustomCard::begin() {
    // Initialize your hardware
    Wire.begin();

    // Test communication
    Wire.beginTransmission(device_address);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    // Perform initial configuration
    currentValue = readSensor();
    previousValue = currentValue;
    initialized = true;

    return true;
}

void MyCustomCard::loop() {
    if (!initialized) return;

    // Read current value
    currentValue = readSensor();

    // Detect changes and trigger callbacks
    if (currentValue != previousValue) {
        for (auto& callback : callbacks) {
            callback.second(currentValue);
        }
        previousValue = currentValue;
    }
}

uint8_t MyCustomCard::getType() {
    return CARD_TYPE_CUSTOM;
}
```

#### Step 3: Implement Card-Specific Methods

```cpp
uint16_t MyCustomCard::readSensor() {
    // Implement sensor reading logic
    Wire.beginTransmission(device_address);
    Wire.write(0x00); // Register address
    Wire.endTransmission();

    Wire.requestFrom(device_address, (uint8_t)2);
    uint16_t value = Wire.read() << 8;
    value |= Wire.read();

    return value;
}

void MyCustomCard::writeOutput(uint8_t value) {
    // Implement output writing logic
    Wire.beginTransmission(device_address);
    Wire.write(0x01); // Register address
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t MyCustomCard::registerCallback(std::function<void(uint16_t)> callback) {
    callbacks[callback_index] = callback;
    return callback_index++;
}

void MyCustomCard::unregisterCallback(uint8_t handler) {
    callbacks.erase(handler);
}
```

### Complete Custom Card Example

Here's a complete example of a custom temperature sensor card:

**TemperatureSensorCard.hpp:**

```cpp
#pragma once
#include <ExpansionCard.hpp>
#include <Wire.h>
#include <map>

#define CARD_TYPE_TEMPERATURE 0x10
#define TEMP_SENSOR_ADDRESS 0x48

class TemperatureSensorCard : public ExpansionCard {
    public:
        TemperatureSensorCard();
        bool begin() override;
        void loop() override;
        uint8_t getType() override;

        // Temperature reading methods
        float getTemperature();
        float getTemperatureF();

        // Callback for temperature changes
        uint8_t registerTemperatureCallback(std::function<void(float)> callback);
        void unregisterTemperatureCallback(uint8_t handler);

        // Configuration
        void setUpdateInterval(uint32_t interval);
        void setTemperatureThreshold(float threshold);

    private:
        float currentTemp;
        float previousTemp;
        float threshold;
        uint32_t updateInterval;
        uint32_t lastUpdate;
        bool initialized;

        uint8_t callback_index;
        std::map<uint8_t, std::function<void(float)>> callbacks;

        float readTemperature();
};
```

**TemperatureSensorCard.cpp:**

```cpp
#include "TemperatureSensorCard.hpp"

TemperatureSensorCard::TemperatureSensorCard()
    : currentTemp(0.0),
      previousTemp(0.0),
      threshold(0.5),
      updateInterval(1000),
      lastUpdate(0),
      initialized(false),
      callback_index(0) {
}

bool TemperatureSensorCard::begin() {
    Wire.begin();

    // Test sensor communication
    Wire.beginTransmission(TEMP_SENSOR_ADDRESS);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    // Configure sensor (example: set resolution)
    Wire.beginTransmission(TEMP_SENSOR_ADDRESS);
    Wire.write(0x01); // Configuration register
    Wire.write(0x60); // 12-bit resolution
    Wire.endTransmission();

    // Read initial temperature
    currentTemp = readTemperature();
    previousTemp = currentTemp;
    initialized = true;
    lastUpdate = millis();

    return true;
}

void TemperatureSensorCard::loop() {
    if (!initialized) return;

    // Check if it's time to update
    if (millis() - lastUpdate < updateInterval) {
        return;
    }

    // Read current temperature
    currentTemp = readTemperature();

    // Check if change exceeds threshold
    if (abs(currentTemp - previousTemp) >= threshold) {
        // Trigger all registered callbacks
        for (auto& callback : callbacks) {
            callback.second(currentTemp);
        }
        previousTemp = currentTemp;
    }

    lastUpdate = millis();
}

uint8_t TemperatureSensorCard::getType() {
    return CARD_TYPE_TEMPERATURE;
}

float TemperatureSensorCard::readTemperature() {
    Wire.beginTransmission(TEMP_SENSOR_ADDRESS);
    Wire.write(0x00); // Temperature register
    Wire.endTransmission();

    Wire.requestFrom(TEMP_SENSOR_ADDRESS, (uint8_t)2);
    uint16_t raw = Wire.read() << 8;
    raw |= Wire.read();

    // Convert to Celsius (example conversion)
    return (float)raw * 0.0625;
}

float TemperatureSensorCard::getTemperature() {
    return currentTemp;
}

float TemperatureSensorCard::getTemperatureF() {
    return (currentTemp * 9.0 / 5.0) + 32.0;
}

uint8_t TemperatureSensorCard::registerTemperatureCallback(
    std::function<void(float)> callback) {
    callbacks[callback_index] = callback;
    return callback_index++;
}

void TemperatureSensorCard::unregisterTemperatureCallback(uint8_t handler) {
    callbacks.erase(handler);
}

void TemperatureSensorCard::setUpdateInterval(uint32_t interval) {
    updateInterval = interval;
}

void TemperatureSensorCard::setTemperatureThreshold(float newThreshold) {
    threshold = newThreshold;
}
```

**Usage Example:**

```cpp
#include <ESPMegaPRO.h>
#include "TemperatureSensorCard.hpp"

ESPMegaPRO espmega;
TemperatureSensorCard tempCard;

void temperatureChanged(float newTemp) {
    Serial.print("Temperature changed to: ");
    Serial.print(newTemp);
    Serial.println(" C");
}

void setup() {
    Serial.begin(115200);

    // Initialize temperature card
    if (!tempCard.begin()) {
        Serial.println("Failed to initialize temperature card!");
        return;
    }

    // Register callback
    tempCard.registerTemperatureCallback(temperatureChanged);

    // Configure update interval (2 seconds)
    tempCard.setUpdateInterval(2000);

    // Set threshold for triggering callbacks (0.2°C)
    tempCard.setTemperatureThreshold(0.2);

    // Install card in ESPMega
    espmega.installCard(0, &tempCard);

    Serial.println("Temperature sensor card initialized!");
}

void loop() {
    espmega.loop();

    // You can also read temperature directly
    float temp = tempCard.getTemperature();
    // Do something with temp...
}
```

---

## IoTComponent Base Class

### Overview

`IoTComponent` is the abstract base class for creating MQTT-enabled interfaces to expansion cards. It wraps an `ExpansionCard` and provides MQTT communication capabilities, allowing remote control and monitoring of expansion cards through MQTT.

### Purpose and Inheritance

The `IoTComponent` class:
- Provides MQTT integration for expansion cards
- Handles message routing and topic subscriptions
- Offers helper methods for publishing and subscribing to relative topics
- Manages the connection between expansion cards and the IoT module

### Class Definition

```cpp
class IoTComponent {
    public:
        virtual bool begin(uint8_t card_id, ExpansionCard *card,
                          PubSubClient *mqtt, char *base_topic);
        virtual void handleMqttMessage(char *topic, char *payload);
        void setMqttClient(PubSubClient *mqtt);
        virtual void publishReport();
        virtual uint8_t getType();
        virtual void subscribe();
        void loop();

    protected:
        char *base_topic;
        void publishRelative(const char *topic, const char *payload);
        void subscribeRelative(const char *topic);
        PubSubClient *mqtt;
        uint8_t card_id;
};
```

### Virtual Methods to Implement

#### `begin()`

```cpp
virtual bool begin(uint8_t card_id, ExpansionCard *card,
                   PubSubClient *mqtt, char *base_topic);
```

**Purpose:** Initialize the IoT component and link it to an expansion card.

**Parameters:**
- `card_id`: The slot number where the card is installed (0-254)
- `card`: Pointer to the underlying ExpansionCard
- `mqtt`: Pointer to the PubSubClient for MQTT communication
- `base_topic`: The base MQTT topic for this device

**Returns:** `true` if initialization succeeds, `false` otherwise.

**Implementation Requirements:**
- Store references to the card, MQTT client, and base topic
- Cast the ExpansionCard pointer to the specific card type
- Register callbacks with the expansion card
- Perform any necessary initialization

**Example:**
```cpp
bool MyCustomIoT::begin(uint8_t card_id, ExpansionCard *card,
                        PubSubClient *mqtt, char *base_topic) {
    this->card_id = card_id;
    this->mqtt = mqtt;
    this->base_topic = base_topic;

    // Cast to specific card type
    this->card = (MyCustomCard*)card;

    // Register callback with the card
    this->card->registerCallback([this](uint16_t value) {
        this->handleValueChange(value);
    });

    return true;
}
```

#### `handleMqttMessage()`

```cpp
virtual void handleMqttMessage(char *topic, char *payload);
```

**Purpose:** Process incoming MQTT messages for this component.

**Parameters:**
- `topic`: The MQTT topic (relative to base_topic/card_id)
- `payload`: The message payload as a string

**Implementation Requirements:**
- Parse the topic to determine the requested action
- Validate the payload
- Execute the appropriate card operation
- Publish acknowledgment if needed

**Example:**
```cpp
void MyCustomIoT::handleMqttMessage(char *topic, char *payload) {
    // Handle "set" command
    if (strcmp(topic, "set") == 0) {
        uint8_t value = atoi(payload);
        card->writeOutput(value);
        publishValue(); // Confirm the change
    }
    // Handle "requeststate" command
    else if (strcmp(topic, "requeststate") == 0) {
        publishReport();
    }
}
```

#### `publishReport()`

```cpp
virtual void publishReport();
```

**Purpose:** Publish the current state of all card values to MQTT.

**Implementation Requirements:**
- Read current values from the expansion card
- Publish each relevant value to its corresponding topic
- Use `publishRelative()` helper for publishing

**Example:**
```cpp
void MyCustomIoT::publishReport() {
    // Publish current sensor value
    uint16_t value = card->readSensor();
    char payload[10];
    itoa(value, payload, 10);
    publishRelative("value", payload);

    // Publish status
    publishRelative("status", "online");
}
```

#### `getType()`

```cpp
virtual uint8_t getType();
```

**Purpose:** Return the card type of the underlying expansion card.

**Returns:** The card type ID from the wrapped ExpansionCard.

**Example:**
```cpp
uint8_t MyCustomIoT::getType() {
    return CARD_TYPE_CUSTOM;
}
```

#### `subscribe()`

```cpp
virtual void subscribe();
```

**Purpose:** Subscribe to all MQTT topics that this component needs to monitor.

**Implementation Requirements:**
- Use `subscribeRelative()` to subscribe to topics
- Subscribe to all command topics the component supports
- Typically called during initialization

**Example:**
```cpp
void MyCustomIoT::subscribe() {
    subscribeRelative("set");
    subscribeRelative("requeststate");
    subscribeRelative("config");
}
```

### Helper Methods

The `IoTComponent` base class provides two essential helper methods:

#### `publishRelative()`

```cpp
void publishRelative(const char *topic, const char *payload);
```

**Purpose:** Publish a message to a topic relative to the base topic and card ID.

**Topic Format:** `{base_topic}/{card_id:02d}/{topic}`

**Example:**
```cpp
// If base_topic = "espmega/main" and card_id = 5
publishRelative("temperature", "25.5");
// Publishes to: "espmega/main/05/temperature"
```

#### `subscribeRelative()`

```cpp
void subscribeRelative(const char *topic);
```

**Purpose:** Subscribe to a topic relative to the base topic and card ID.

**Topic Format:** `{base_topic}/{card_id:02d}/{topic}`

**Example:**
```cpp
// If base_topic = "espmega/main" and card_id = 5
subscribeRelative("set");
// Subscribes to: "espmega/main/05/set"
```

### MQTT Integration Patterns

#### Pattern 1: State Publishing

Publish state changes automatically when they occur:

```cpp
void MyCustomIoT::handleValueChange(uint16_t value) {
    char payload[10];
    itoa(value, payload, 10);
    publishRelative("value", payload);
}
```

#### Pattern 2: Command Processing

Process incoming commands and publish confirmations:

```cpp
void MyCustomIoT::handleMqttMessage(char *topic, char *payload) {
    if (strcmp(topic, "set/output") == 0) {
        uint8_t value = atoi(payload);
        card->writeOutput(value);

        // Publish confirmation
        publishRelative("output", payload);
    }
}
```

#### Pattern 3: Periodic Reporting

Use the loop() method for periodic state publishing:

```cpp
void MyCustomIoT::loop() {
    static uint32_t lastPublish = 0;

    if (millis() - lastPublish >= publishInterval) {
        publishReport();
        lastPublish = millis();
    }
}
```

#### Pattern 4: Request-Response

Respond to state requests:

```cpp
void MyCustomIoT::handleMqttMessage(char *topic, char *payload) {
    if (strcmp(topic, "requeststate") == 0) {
        publishReport();
    }
}
```

### How to Create Custom IoT Components

#### Step 1: Define Your IoT Component Class

Create a header file (e.g., `MyCustomIoT.hpp`):

```cpp
#pragma once
#include <IoTComponent.hpp>
#include "MyCustomCard.hpp"

// MQTT Topics
#define VALUE_TOPIC "value"
#define SET_TOPIC "set"
#define REQUEST_STATE_TOPIC "requeststate"

class MyCustomIoT : public IoTComponent {
    public:
        bool begin(uint8_t card_id, ExpansionCard *card,
                  PubSubClient *mqtt, char *base_topic) override;
        void handleMqttMessage(char *topic, char *payload) override;
        void publishReport() override;
        uint8_t getType() override;
        void subscribe() override;
        void loop() override;

    private:
        MyCustomCard *card;
        void handleValueChange(uint16_t value);
        void publishValue();

        uint32_t publishInterval;
        uint32_t lastPublish;
};
```

#### Step 2: Implement the Virtual Methods

Create an implementation file (e.g., `MyCustomIoT.cpp`):

```cpp
#include "MyCustomIoT.hpp"

bool MyCustomIoT::begin(uint8_t card_id, ExpansionCard *card,
                        PubSubClient *mqtt, char *base_topic) {
    this->card_id = card_id;
    this->mqtt = mqtt;
    this->base_topic = base_topic;
    this->card = (MyCustomCard*)card;

    // Register callback for value changes
    this->card->registerCallback([this](uint16_t value) {
        this->handleValueChange(value);
    });

    publishInterval = 60000; // 1 minute
    lastPublish = 0;

    return true;
}

void MyCustomIoT::handleMqttMessage(char *topic, char *payload) {
    // Handle set command
    if (strcmp(topic, SET_TOPIC) == 0) {
        uint8_t value = atoi(payload);
        card->writeOutput(value);
        publishValue();
    }
    // Handle state request
    else if (strcmp(topic, REQUEST_STATE_TOPIC) == 0) {
        publishReport();
    }
}

void MyCustomIoT::publishReport() {
    publishValue();
}

void MyCustomIoT::subscribe() {
    subscribeRelative(SET_TOPIC);
    subscribeRelative(REQUEST_STATE_TOPIC);
}

uint8_t MyCustomIoT::getType() {
    return CARD_TYPE_CUSTOM;
}

void MyCustomIoT::loop() {
    // Periodic publishing
    if (millis() - lastPublish >= publishInterval) {
        publishReport();
        lastPublish = millis();
    }
}

void MyCustomIoT::handleValueChange(uint16_t value) {
    // Publish when value changes
    publishValue();
}

void MyCustomIoT::publishValue() {
    uint16_t value = card->readSensor();
    char payload[10];
    itoa(value, payload, 10);
    publishRelative(VALUE_TOPIC, payload);
}
```

### Complete IoT Component Example

Here's a complete example building on the TemperatureSensorCard:

**TemperatureSensorIoT.hpp:**

```cpp
#pragma once
#include <IoTComponent.hpp>
#include "TemperatureSensorCard.hpp"

// MQTT Topics
#define TEMP_TOPIC "temperature"
#define TEMP_F_TOPIC "temperature_f"
#define INTERVAL_SET_TOPIC "set/interval"
#define THRESHOLD_SET_TOPIC "set/threshold"
#define REQUEST_STATE_TOPIC "requeststate"

class TemperatureSensorIoT : public IoTComponent {
    public:
        bool begin(uint8_t card_id, ExpansionCard *card,
                  PubSubClient *mqtt, char *base_topic) override;
        void handleMqttMessage(char *topic, char *payload) override;
        void publishReport() override;
        uint8_t getType() override;
        void subscribe() override;
        void loop() override;

        // Publishing control
        void setAutoPublish(bool enabled);
        void setPublishInterval(uint32_t interval);

    private:
        TemperatureSensorCard *card;

        // Callback handlers
        void handleTemperatureChange(float temp);

        // Publishing methods
        void publishTemperature();
        void publishConfiguration();

        // Configuration
        bool autoPublish;
        uint32_t publishInterval;
        uint32_t lastPublish;
};
```

**TemperatureSensorIoT.cpp:**

```cpp
#include "TemperatureSensorIoT.hpp"

bool TemperatureSensorIoT::begin(uint8_t card_id, ExpansionCard *card,
                                 PubSubClient *mqtt, char *base_topic) {
    this->card_id = card_id;
    this->mqtt = mqtt;
    this->base_topic = base_topic;
    this->card = (TemperatureSensorCard*)card;

    // Register temperature change callback
    this->card->registerTemperatureCallback([this](float temp) {
        this->handleTemperatureChange(temp);
    });

    // Initialize configuration
    autoPublish = true;
    publishInterval = 60000; // 1 minute
    lastPublish = 0;

    return true;
}

void TemperatureSensorIoT::handleMqttMessage(char *topic, char *payload) {
    // Handle interval configuration
    if (strcmp(topic, INTERVAL_SET_TOPIC) == 0) {
        uint32_t interval = atoi(payload);
        card->setUpdateInterval(interval);
        publishConfiguration();
    }
    // Handle threshold configuration
    else if (strcmp(topic, THRESHOLD_SET_TOPIC) == 0) {
        float threshold = atof(payload);
        card->setTemperatureThreshold(threshold);
        publishConfiguration();
    }
    // Handle state request
    else if (strcmp(topic, REQUEST_STATE_TOPIC) == 0) {
        publishReport();
    }
}

void TemperatureSensorIoT::publishReport() {
    publishTemperature();
    publishConfiguration();
}

void TemperatureSensorIoT::subscribe() {
    subscribeRelative(INTERVAL_SET_TOPIC);
    subscribeRelative(THRESHOLD_SET_TOPIC);
    subscribeRelative(REQUEST_STATE_TOPIC);
}

uint8_t TemperatureSensorIoT::getType() {
    return CARD_TYPE_TEMPERATURE;
}

void TemperatureSensorIoT::loop() {
    // Auto-publish at intervals if enabled
    if (autoPublish && (millis() - lastPublish >= publishInterval)) {
        publishTemperature();
        lastPublish = millis();
    }
}

void TemperatureSensorIoT::handleTemperatureChange(float temp) {
    // Publish immediately when temperature changes significantly
    publishTemperature();
}

void TemperatureSensorIoT::publishTemperature() {
    // Publish temperature in Celsius
    float tempC = card->getTemperature();
    char payload[10];
    dtostrf(tempC, 1, 2, payload);
    publishRelative(TEMP_TOPIC, payload);

    // Publish temperature in Fahrenheit
    float tempF = card->getTemperatureF();
    dtostrf(tempF, 1, 2, payload);
    publishRelative(TEMP_F_TOPIC, payload);
}

void TemperatureSensorIoT::publishConfiguration() {
    // Publish current configuration
    // This could include interval, threshold, etc.
    char payload[20];
    sprintf(payload, "{\"interval\":%d}", publishInterval);
    publishRelative("config", payload);
}

void TemperatureSensorIoT::setAutoPublish(bool enabled) {
    autoPublish = enabled;
}

void TemperatureSensorIoT::setPublishInterval(uint32_t interval) {
    publishInterval = interval;
}
```

**Usage Example:**

```cpp
#include <ESPMegaPRO.h>
#include <ESPMegaIoT.hpp>
#include "TemperatureSensorCard.hpp"
#include "TemperatureSensorIoT.hpp"

ESPMegaPRO espmega;
ESPMegaIoT iot;
TemperatureSensorCard tempCard;
TemperatureSensorIoT tempIoT;

void setup() {
    Serial.begin(115200);

    // Initialize ESPMega
    espmega.begin();

    // Initialize temperature card
    tempCard.begin();
    espmega.installCard(0, &tempCard);

    // Initialize IoT
    iot.intr_begin(espmega.getCards());
    iot.connectNetwork();
    iot.connectToMqtt();

    // Register IoT component
    // Note: ESPMegaIoT will automatically create and manage the
    // TemperatureSensorIoT component when you register the card
    iot.registerCard(0);

    Serial.println("Temperature sensor with IoT initialized!");
}

void loop() {
    espmega.loop();
    iot.loop();
}
```

### MQTT Topic Structure

IoT components follow this topic structure:

```
{base_topic}/{card_id:02d}/{topic}
```

**Example Topics:**

For `base_topic = "espmega/device1"` and `card_id = 3`:

- Temperature reading: `espmega/device1/03/temperature`
- Set threshold: `espmega/device1/03/set/threshold`
- Request state: `espmega/device1/03/requeststate`
- Configuration: `espmega/device1/03/config`

### Best Practices for IoT Components

1. **Always validate payload data** before using it
2. **Publish confirmations** after executing commands
3. **Use JSON** for complex data structures
4. **Implement requeststate** for on-demand state reporting
5. **Keep loop() lightweight** - avoid blocking operations
6. **Use callbacks** to respond to card events immediately
7. **Cache topic lengths** for frequently used topics
8. **Provide configuration options** via MQTT when applicable

---

## TimeStructure

### Overview

The `rtctime_t` structure is used throughout the ESPMegaPRO library to represent date and time values. It provides a simple, lightweight structure for time storage and manipulation.

### Structure Definition

```cpp
struct rtctime_t {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};
```

### Field Documentation

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `hours` | uint8_t | 0-23 | Hours in 24-hour format |
| `minutes` | uint8_t | 0-59 | Minutes |
| `seconds` | uint8_t | 0-59 | Seconds |
| `day` | uint8_t | 1-31 | Day of month |
| `month` | uint8_t | 1-12 | Month (1=January, 12=December) |
| `year` | uint16_t | 0-65535 | Full year (e.g., 2024) |

### Important Notes

- This structure uses **24-hour time format** (0-23 hours)
- The year is stored as a **full 4-digit year** (e.g., 2024, not 24)
- Month values are **1-indexed** (1=January, unlike some C libraries that use 0=January)
- Day values are **1-indexed** (1-31)
- This structure is **not compatible** with the Arduino Time library

### Usage Examples

#### Creating a Time Structure

```cpp
rtctime_t currentTime;
currentTime.hours = 14;      // 2:00 PM
currentTime.minutes = 30;
currentTime.seconds = 0;
currentTime.day = 23;
currentTime.month = 11;      // November
currentTime.year = 2024;
```

#### Initializing with Designated Initializers

```cpp
rtctime_t eventTime = {
    .hours = 9,
    .minutes = 0,
    .seconds = 0,
    .day = 1,
    .month = 1,
    .year = 2025
};
```

#### Reading Time from InternalDisplay

```cpp
#include <InternalDisplay.hpp>

InternalDisplay display;
rtctime_t currentTime;

void setup() {
    display.begin();
}

void loop() {
    // Get current time from display's RTC
    currentTime = display.getTime();

    // Use the time
    Serial.print("Current time: ");
    Serial.print(currentTime.hours);
    Serial.print(":");
    Serial.print(currentTime.minutes);
    Serial.print(":");
    Serial.println(currentTime.seconds);
}
```

#### Setting Time on InternalDisplay

```cpp
rtctime_t newTime;
newTime.hours = 10;
newTime.minutes = 30;
newTime.seconds = 0;
newTime.day = 15;
newTime.month = 6;
newTime.year = 2024;

display.setTime(newTime);
```

#### Comparing Times

```cpp
bool isTimeBefore(rtctime_t t1, rtctime_t t2) {
    if (t1.year != t2.year) return t1.year < t2.year;
    if (t1.month != t2.month) return t1.month < t2.month;
    if (t1.day != t2.day) return t1.day < t2.day;
    if (t1.hours != t2.hours) return t1.hours < t2.hours;
    if (t1.minutes != t2.minutes) return t1.minutes < t2.minutes;
    return t1.seconds < t2.seconds;
}

rtctime_t time1 = {10, 30, 0, 15, 6, 2024};
rtctime_t time2 = {14, 45, 0, 15, 6, 2024};

if (isTimeBefore(time1, time2)) {
    Serial.println("time1 is before time2");
}
```

#### Formatting Time as String

```cpp
String formatTime(rtctime_t time) {
    char buffer[20];
    sprintf(buffer, "%02d:%02d:%02d",
            time.hours, time.minutes, time.seconds);
    return String(buffer);
}

String formatDate(rtctime_t time) {
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d",
            time.year, time.month, time.day);
    return String(buffer);
}

String formatDateTime(rtctime_t time) {
    char buffer[40];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            time.year, time.month, time.day,
            time.hours, time.minutes, time.seconds);
    return String(buffer);
}

// Usage
rtctime_t now = display.getTime();
Serial.println(formatTime(now));      // "14:30:00"
Serial.println(formatDate(now));      // "2024-11-23"
Serial.println(formatDateTime(now));  // "2024-11-23 14:30:00"
```

#### Time-Based Scheduling

```cpp
bool isTimeForAction(rtctime_t current, uint8_t targetHour, uint8_t targetMinute) {
    return (current.hours == targetHour && current.minutes == targetMinute);
}

void loop() {
    rtctime_t now = display.getTime();

    // Execute action at 9:00 AM
    if (isTimeForAction(now, 9, 0)) {
        // Perform morning routine
        performMorningRoutine();
        delay(60000); // Wait 1 minute to avoid repeating
    }

    // Execute action at 6:00 PM
    if (isTimeForAction(now, 18, 0)) {
        // Perform evening routine
        performEveningRoutine();
        delay(60000);
    }
}
```

#### Converting to Unix Timestamp

```cpp
uint32_t toUnixTimestamp(rtctime_t time) {
    // Simple conversion (doesn't account for leap years perfectly)
    uint32_t timestamp = 0;

    // Years since 1970
    for (uint16_t y = 1970; y < time.year; y++) {
        timestamp += 365 * 86400;
        if (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) {
            timestamp += 86400; // Leap year
        }
    }

    // Months
    uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool isLeapYear = (time.year % 4 == 0 &&
                      (time.year % 100 != 0 || time.year % 400 == 0));
    if (isLeapYear) daysInMonth[1] = 29;

    for (uint8_t m = 1; m < time.month; m++) {
        timestamp += daysInMonth[m - 1] * 86400;
    }

    // Days
    timestamp += (time.day - 1) * 86400;

    // Hours, minutes, seconds
    timestamp += time.hours * 3600;
    timestamp += time.minutes * 60;
    timestamp += time.seconds;

    return timestamp;
}
```

#### Time Difference Calculation

```cpp
struct TimeDifference {
    uint16_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

TimeDifference calculateDifference(rtctime_t t1, rtctime_t t2) {
    uint32_t ts1 = toUnixTimestamp(t1);
    uint32_t ts2 = toUnixTimestamp(t2);

    uint32_t diff = (ts2 > ts1) ? (ts2 - ts1) : (ts1 - ts2);

    TimeDifference result;
    result.days = diff / 86400;
    diff %= 86400;
    result.hours = diff / 3600;
    diff %= 3600;
    result.minutes = diff / 60;
    result.seconds = diff % 60;

    return result;
}
```

---

## ESPMegaCommon

### Overview

`ESPMegaCommon.hpp` contains common definitions and constants used throughout the ESPMegaPRO library.

### Contents

#### SDK Version

```cpp
#define SDK_VESRION "2.10.0"
```

**Note:** There is a typo in the macro name (`VESRION` instead of `VERSION`). This is maintained for backward compatibility.

**Purpose:** Identifies the version of the ESPMegaPRO SDK being used.

**Usage:**

```cpp
#include <ESPMegaCommon.hpp>

void setup() {
    Serial.begin(115200);
    Serial.print("ESPMegaPRO SDK Version: ");
    Serial.println(SDK_VESRION);
}
```

### Best Practices

When creating your own libraries or firmware:

1. **Include ESPMegaCommon.hpp** to access version information
2. **Check SDK version** if your code requires specific features
3. **Document version dependencies** in your code

```cpp
#include <ESPMegaCommon.hpp>

// Check SDK version at compile time
#if strcmp(SDK_VESRION, "2.10.0") < 0
    #error "This code requires ESPMegaPRO SDK 2.10.0 or later"
#endif
```

---

## ESPMegaTCP

### Overview

`ESPMegaTCP` is a class that provides TCP/IP API functionality for the ESPMegaPRO. It enables network communication and remote control capabilities.

### Class Definition

```cpp
class ESPMegaTCP {
    public:
        void begin();
        void loop();
};
```

### Methods

#### `begin()`

```cpp
void begin();
```

**Purpose:** Initialize the TCP/IP interface.

**Usage:**

```cpp
#include <ESPMegaTCP.hpp>

ESPMegaTCP tcp;

void setup() {
    tcp.begin();
}
```

#### `loop()`

```cpp
void loop();
```

**Purpose:** Handle TCP/IP operations in the main loop.

**Usage:**

```cpp
void loop() {
    tcp.loop();
}
```

### Purpose and Usage

The ESPMegaTCP class provides:
- TCP/IP server functionality
- Remote command processing
- Network-based control interface

**Typical Usage Pattern:**

```cpp
#include <ESPMegaPRO.h>
#include <ESPMegaTCP.hpp>

ESPMegaPRO espmega;
ESPMegaTCP tcp;

void setup() {
    // Initialize ESPMega
    espmega.begin();

    // Initialize TCP interface
    tcp.begin();

    Serial.println("TCP interface ready");
}

void loop() {
    espmega.loop();
    tcp.loop();
}
```

### Notes

- ESPMegaTCP is typically used for local network control
- For IoT/cloud connectivity, use ESPMegaIoT instead
- Both ESPMegaTCP and ESPMegaIoT can be used simultaneously

---

## ESPMegaRTU

### Overview

`ESPMegaRTU` (Remote Terminal Unit) is a base class for creating remote interfaces to IoT components on other ESPMegaPRO devices. It allows one ESPMegaPRO to control expansion cards on another ESPMegaPRO over MQTT.

### Class Definition

```cpp
class ESPMegaRTU {
    public:
        ESPMegaRTU();
        ~ESPMegaRTU();
        virtual void subscribe();
        virtual void begin(char* remote_base_topic, uint8_t remote_card_slot,
                          ESPMegaIoT* iot);
    protected:
        char* remoteBaseTopic;
        uint8_t remoteBaseTopicLength;
        uint8_t cardSlot;
        ESPMegaIoT* iot;
};
```

### How RTU Works

The RTU (Remote Terminal Unit) pattern allows you to:

1. **Access remote cards** on another ESPMegaPRO device
2. **Control remote devices** through MQTT
3. **Create virtual representations** of remote expansion cards
4. **Build distributed systems** with multiple ESPMegaPRO devices

**Architecture:**

```
[Device A]                          [Device B]
ESPMegaPRO                          ESPMegaPRO
├─ DigitalOutputCard (Slot 0)       ├─ AnalogCard (Slot 0)
├─ AnalogCardRTU ──────MQTT────────>│  (Remote access)
└─ ESPMegaIoT                       └─ ESPMegaIoT
   (base: device-a)                    (base: device-b)
```

Device A can control Device B's AnalogCard through MQTT using AnalogCardRTU.

### Virtual Methods

#### `begin()`

```cpp
virtual void begin(char* remote_base_topic, uint8_t remote_card_slot,
                   ESPMegaIoT* iot);
```

**Purpose:** Initialize the RTU connection to a remote card.

**Parameters:**
- `remote_base_topic`: The base MQTT topic of the remote device
- `remote_card_slot`: The slot number of the remote card
- `iot`: Pointer to the local ESPMegaIoT instance

#### `subscribe()`

```cpp
virtual void subscribe();
```

**Purpose:** Subscribe to MQTT topics for receiving remote card state updates.

### Complete API

#### Constructor and Destructor

```cpp
ESPMegaRTU();
~ESPMegaRTU();
```

**Usage:**

```cpp
ESPMegaRTU* rtu = new ESPMegaRTU();
```

#### Protected Members

These members are available to derived RTU classes:

```cpp
char* remoteBaseTopic;          // Base topic of remote device
uint8_t remoteBaseTopicLength;  // Length of base topic
uint8_t cardSlot;               // Remote card slot number
ESPMegaIoT* iot;                // Local IoT instance
```

### Creating Custom RTU Classes

Here's how to create an RTU class for remote card access:

**AnalogCardRTU.hpp:**

```cpp
#pragma once
#include <ESPMegaRTU.hpp>

class AnalogCardRTU : public ESPMegaRTU {
    public:
        void begin(char* remote_base_topic, uint8_t remote_card_slot,
                  ESPMegaIoT* iot) override;
        void subscribe() override;

        // Remote control methods
        void setDACValue(uint8_t pin, uint16_t value);
        void setDACState(uint8_t pin, bool state);
        void requestADCValue(uint8_t pin);

        // State storage (from remote)
        uint16_t getLastADCValue(uint8_t pin);
        uint16_t getDACValue(uint8_t pin);
        bool getDACState(uint8_t pin);

        // Callbacks for remote state changes
        uint8_t registerADCCallback(std::function<void(uint8_t, uint16_t)> callback);
        void unregisterADCCallback(uint8_t handler);

    private:
        void handleADCUpdate(char* topic, char* payload);
        void handleDACUpdate(char* topic, char* payload);
        void publishToRemote(const char* topic, const char* payload);

        // Cached remote state
        uint16_t adcValues[8];
        uint16_t dacValues[4];
        bool dacStates[4];

        // Callbacks
        uint8_t callbackIndex;
        std::map<uint8_t, std::function<void(uint8_t, uint16_t)>> adcCallbacks;
};
```

**AnalogCardRTU.cpp:**

```cpp
#include "AnalogCardRTU.hpp"

void AnalogCardRTU::begin(char* remote_base_topic, uint8_t remote_card_slot,
                         ESPMegaIoT* iot) {
    this->remoteBaseTopic = remote_base_topic;
    this->remoteBaseTopicLength = strlen(remote_base_topic);
    this->cardSlot = remote_card_slot;
    this->iot = iot;

    // Initialize cached state
    for (int i = 0; i < 8; i++) {
        adcValues[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
        dacValues[i] = 0;
        dacStates[i] = false;
    }

    callbackIndex = 0;

    // Register MQTT callback for remote updates
    iot->registerRelativeMqttCallback([this](char* topic, char* payload) {
        // Parse topic to determine if it's for our remote card
        // Handle ADC and DAC updates
        if (strstr(topic, "/adc/") != nullptr) {
            handleADCUpdate(topic, payload);
        } else if (strstr(topic, "/dac/") != nullptr) {
            handleDACUpdate(topic, payload);
        }
    });
}

void AnalogCardRTU::subscribe() {
    // Subscribe to all ADC value topics
    for (uint8_t pin = 0; pin < 8; pin++) {
        char topic[50];
        sprintf(topic, "%s/%02d/adc/%02d/value",
                remoteBaseTopic, cardSlot, pin);
        iot->subscribe(topic);
    }

    // Subscribe to all DAC state and value topics
    for (uint8_t pin = 0; pin < 4; pin++) {
        char topic[50];
        sprintf(topic, "%s/%02d/dac/%02d/state",
                remoteBaseTopic, cardSlot, pin);
        iot->subscribe(topic);

        sprintf(topic, "%s/%02d/dac/%02d/value",
                remoteBaseTopic, cardSlot, pin);
        iot->subscribe(topic);
    }
}

void AnalogCardRTU::setDACValue(uint8_t pin, uint16_t value) {
    char topic[50];
    char payload[10];

    sprintf(topic, "%s/%02d/dac/%02d/set/value",
            remoteBaseTopic, cardSlot, pin);
    itoa(value, payload, 10);

    iot->publish(topic, payload);
}

void AnalogCardRTU::setDACState(uint8_t pin, bool state) {
    char topic[50];

    sprintf(topic, "%s/%02d/dac/%02d/set/state",
            remoteBaseTopic, cardSlot, pin);

    iot->publish(topic, state ? "1" : "0");
}

void AnalogCardRTU::requestADCValue(uint8_t pin) {
    char topic[50];
    sprintf(topic, "%s/%02d/adc/%02d/request",
            remoteBaseTopic, cardSlot, pin);
    iot->publish(topic, "1");
}

uint16_t AnalogCardRTU::getLastADCValue(uint8_t pin) {
    if (pin < 8) {
        return adcValues[pin];
    }
    return 0;
}

uint16_t AnalogCardRTU::getDACValue(uint8_t pin) {
    if (pin < 4) {
        return dacValues[pin];
    }
    return 0;
}

bool AnalogCardRTU::getDACState(uint8_t pin) {
    if (pin < 4) {
        return dacStates[pin];
    }
    return false;
}

uint8_t AnalogCardRTU::registerADCCallback(
    std::function<void(uint8_t, uint16_t)> callback) {
    adcCallbacks[callbackIndex] = callback;
    return callbackIndex++;
}

void AnalogCardRTU::unregisterADCCallback(uint8_t handler) {
    adcCallbacks.erase(handler);
}

void AnalogCardRTU::handleADCUpdate(char* topic, char* payload) {
    // Parse pin number from topic
    // Format: {base}/{slot}/adc/{pin}/value
    char* pinStr = strstr(topic, "/adc/") + 5;
    uint8_t pin = atoi(pinStr);

    if (pin < 8) {
        uint16_t value = atoi(payload);
        adcValues[pin] = value;

        // Trigger callbacks
        for (auto& callback : adcCallbacks) {
            callback.second(pin, value);
        }
    }
}

void AnalogCardRTU::handleDACUpdate(char* topic, char* payload) {
    // Parse pin number and type (state/value) from topic
    char* pinStr = strstr(topic, "/dac/") + 5;
    uint8_t pin = atoi(pinStr);

    if (pin < 4) {
        if (strstr(topic, "/state") != nullptr) {
            dacStates[pin] = (atoi(payload) != 0);
        } else if (strstr(topic, "/value") != nullptr) {
            dacValues[pin] = atoi(payload);
        }
    }
}
```

### Usage Example

**Device A (Controller):**

```cpp
#include <ESPMegaPRO.h>
#include <ESPMegaIoT.hpp>
#include "AnalogCardRTU.hpp"

ESPMegaPRO espmega;
ESPMegaIoT iot;
AnalogCardRTU remoteAnalog;

void onRemoteADCChange(uint8_t pin, uint16_t value) {
    Serial.print("Remote ADC ");
    Serial.print(pin);
    Serial.print(" changed to: ");
    Serial.println(value);
}

void setup() {
    Serial.begin(115200);

    // Initialize local device
    espmega.begin();
    iot.intr_begin(espmega.getCards());
    iot.setBaseTopic("device-a");
    iot.connectNetwork();
    iot.connectToMqtt();

    // Initialize RTU connection to remote device
    remoteAnalog.begin("device-b", 0, &iot);
    remoteAnalog.subscribe();

    // Register callback for remote changes
    remoteAnalog.registerADCCallback(onRemoteADCChange);

    Serial.println("RTU initialized!");
}

void loop() {
    espmega.loop();
    iot.loop();

    // Control remote DAC
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        // Set DAC value on remote device every 5 seconds
        static uint16_t value = 0;
        remoteAnalog.setDACValue(0, value);
        value = (value + 100) % 4096;

        lastUpdate = millis();
    }
}
```

**Device B (Remote):**

```cpp
#include <ESPMegaPRO.h>
#include <ESPMegaIoT.hpp>
#include <AnalogCard.hpp>

ESPMegaPRO espmega;
ESPMegaIoT iot;
AnalogCard analogCard;

void setup() {
    Serial.begin(115200);

    // Initialize device
    espmega.begin();

    // Initialize and install analog card
    analogCard.begin();
    espmega.installCard(0, &analogCard);

    // Initialize IoT
    iot.intr_begin(espmega.getCards());
    iot.setBaseTopic("device-b");
    iot.connectNetwork();
    iot.connectToMqtt();

    // Register card for IoT control
    iot.registerCard(0);

    Serial.println("Remote device ready!");
}

void loop() {
    espmega.loop();
    iot.loop();
}
```

### RTU Use Cases

1. **Distributed Control:** Control multiple ESPMegaPRO devices from a central controller
2. **Redundancy:** Monitor remote devices and switch control if primary fails
3. **Aggregation:** Collect data from multiple remote devices
4. **Remote I/O:** Extend I/O capabilities across network

### Best Practices for RTU

1. **Handle connection failures** gracefully
2. **Cache remote state** locally for quick access
3. **Implement timeouts** for remote operations
4. **Validate received data** before using it
5. **Use QoS levels** appropriately for critical commands
6. **Implement state synchronization** on reconnection
7. **Consider network latency** in time-critical applications

---

## Summary

This document covered the essential base classes and structures in the ESPMegaPRO library:

- **ExpansionCard**: Base class for hardware expansion cards
- **IoTComponent**: Base class for MQTT-enabled card interfaces
- **TimeStructure**: Simple time/date representation
- **ESPMegaCommon**: Common definitions and SDK version
- **ESPMegaTCP**: TCP/IP communication interface
- **ESPMegaRTU**: Remote Terminal Unit for distributed systems

Understanding these base classes enables you to:
- Create custom expansion cards
- Build IoT-enabled components
- Develop distributed control systems
- Extend the ESPMegaPRO ecosystem

For specific card implementations, refer to the individual card documentation in this directory.
