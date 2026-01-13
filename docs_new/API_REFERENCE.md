# ESPMegaPRO R3 Complete API Reference

Complete API reference for all classes and functions in the ESPMegaPRO R3 library.

> **Note:** This is a high-level overview. For detailed documentation of each component, see the individual component documentation files.

## Table of Contents

- [Functional API (ESPMegaPRO.h)](#functional-api)
- [OOP API (ESPMegaProOS.hpp)](#oop-api)
- [Expansion Cards](#expansion-cards)
  - [DigitalInputCard](#digitalinputcard)
  - [DigitalOutputCard](#digitaloutputcard)
  - [AnalogCard](#analogcard)
  - [ClimateCard](#climatecard)
  - [CurrentTransformerCard](#currenttransformercard)
- [IoT & Networking](#iot--networking)
  - [ESPMegaIoT](#espmegaiot)
  - [ESPMegaWebServer](#espmegawebserver)
- [Display](#display)
  - [InternalDisplay](#internaldisplay)
- [Supporting Classes](#supporting-classes)

---

## Functional API

> **File:** `ESPMegaPRO.h`
> **Use Case:** Simple projects, quick prototyping
> **Limitations:** Built-in I/O only, no expansion cards

### Initialization

```cpp
void ESPMega_begin();
void ESPMega_loop();
```

### Digital I/O

```cpp
bool ESPMega_digitalRead(int pin);           // Read digital input (0-15)
void ESPMega_digitalWrite(int pin, bool state);  // Write digital output (0-15)
```

### PWM Output

```cpp
void ESPMega_analogWrite(int pin, int value);  // Write PWM (0-4095)
```

### Analog I/O (Requires Analog Card)

```cpp
int16_t ESPMega_analogRead(int pin);        // Read ADC (0-7)
void ESPMega_dacWrite(int pin, int value);  // Write DAC (0-3)
```

### Real-Time Clock

```cpp
rtctime_t ESPMega_getTime();
void ESPMega_setTime(int h, int m, int s, int day, int month, int year);
bool ESPMega_updateTimeFromNTP();
```

---

## OOP API

> **File:** `ESPMegaProOS.hpp`
> **Use Case:** Complex projects, full feature access
> **Features:** Expansion cards, IoT, web server, display

### ESPMegaPRO Class

#### Initialization

```cpp
ESPMegaPRO espmega;
bool begin();
void loop();
```

#### Card Management

```cpp
bool installCard(uint8_t slot, ExpansionCard* card);
ExpansionCard* getCard(uint8_t slot);
```

#### Built-in Cards

```cpp
DigitalInputCard inputs;   // Slot 0 (built-in)
DigitalOutputCard outputs; // Slot 1 (built-in)
```

#### IoT Module

```cpp
void enableIotModule();
ESPMegaIoT* iot;
```

#### Web Server

```cpp
void enableWebServer(uint16_t port);
ESPMegaWebServer* webServer;
```

#### Display

```cpp
void enableInternalDisplay(HardwareSerial* serial);
InternalDisplay* display;
```

#### RTC & FRAM

```cpp
rtctime_t getTime();
void setTime(int h, int m, int s, int day, int month, int year);
bool updateTimeFromNTP();
void setTimezone(const char* offset);
FRAM fram;
void dumpFRAMtoSerial(uint16_t start, uint16_t end);
```

---

## Expansion Cards

### DigitalInputCard

> **Detailed Docs:** [DigitalInputCard.md](DigitalInputCard.md)
> **Card Type:** `0x01`
> **Features:** 16 inputs, debouncing, callbacks, pin mapping

#### Constructor

```cpp
DigitalInputCard(uint8_t address_a, uint8_t address_b);
DigitalInputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5);
```

#### Core Methods

```cpp
bool begin();
void loop();
bool digitalRead(uint8_t pin);
bool digitalRead(uint8_t pin, bool refresh);
uint8_t getInputBufferA();
uint8_t getInputBufferB();
```

#### Debouncing

```cpp
void setDebounceTime(uint8_t pin, uint32_t debounceTime);
```

#### Callbacks

```cpp
uint8_t registerCallback(std::function<void(uint8_t, bool)> callback);
void unregisterCallback(uint8_t handler);
```

#### Utilities

```cpp
void loadPinMap(uint8_t pinMap[16]);
void preloadInputBuffer();
bool getStatus();
uint8_t getType();
```

---

### DigitalOutputCard

> **Detailed Docs:** [DigitalOutputCard.md](DigitalOutputCard.md)
> **Card Type:** `0x00`
> **Features:** 16 PWM outputs, FRAM persistence, callbacks

#### Constructor

```cpp
DigitalOutputCard(uint8_t address);
DigitalOutputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4);
```

#### Core Methods

```cpp
bool begin();
void loop();
void digitalWrite(uint8_t pin, bool state);
void analogWrite(uint8_t pin, uint16_t value);
```

#### State Control

```cpp
void setState(uint8_t pin, bool state);
bool getState(uint8_t pin);
void toggleState(uint8_t pin);
```

#### Value Control

```cpp
void setValue(uint8_t pin, uint16_t value);
uint16_t getValue(uint8_t pin);
```

#### Callbacks

```cpp
uint8_t registerChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback);
void unregisterChangeCallback(uint8_t handler);
```

#### FRAM Persistence

```cpp
void bindFRAM(FRAM* fram, uint16_t address);
void saveToFRAM();
void loadFromFRAM();
void setAutoSaveToFRAM(bool autoSave);
void savePinValueToFRAM(uint8_t pin);
```

#### Utilities

```cpp
void loadPinMap(uint8_t pinMap[16]);
uint8_t getType();
```

---

### AnalogCard

> **Detailed Docs:** [AnalogCard.md](AnalogCard.md)
> **Card Type:** `0x02`
> **Features:** 8 ADC inputs (16-bit), 4 DAC outputs (12-bit)

#### Constructor

```cpp
AnalogCard();
```

#### Core Methods

```cpp
bool begin();
void loop();
uint16_t analogRead(uint8_t pin);  // 0-7
void dacWrite(uint8_t pin, uint16_t value);  // 0-3
```

#### DAC Control

```cpp
void setDACState(uint8_t pin, bool state);
bool getDACState(uint8_t pin);
void setDACValue(uint8_t pin, uint16_t value);
uint16_t getDACValue(uint8_t pin);
void sendDataToDAC(uint8_t pin, uint16_t value);
```

#### Callbacks

```cpp
uint8_t registerDACChangeCallback(std::function<void(uint8_t, bool, uint16_t)> callback);
void unregisterDACChangeCallback(uint8_t handler);
```

#### Utilities

```cpp
uint8_t getType();
```

---

### ClimateCard

> **Detailed Docs:** [ClimateCard.md](ClimateCard.md)
> **Card Type:** `0x03`
> **Features:** IR AC control, DHT22/DS18B20 sensors, FRAM persistence

#### Constructor

```cpp
ClimateCard(uint8_t ir_pin, AirConditioner ac, uint8_t sensor_type, uint8_t sensor_pin, rmt_channel_t channel);
ClimateCard(uint8_t ir_pin, AirConditioner ac, rmt_channel_t channel);
```

#### Core Methods

```cpp
bool begin();
void loop();
```

#### AC Control

```cpp
void setTemperature(uint8_t temperature);
uint8_t getTemperature();
void setMode(uint8_t mode);
void setModeByName(const char* mode_name);
uint8_t getMode();
char* getModeName();
void setFanSpeed(uint8_t fan_speed);
void setFanSpeedByName(const char* fan_speed_name);
uint8_t getFanSpeed();
char* getFanSpeedName();
void setState(uint8_t mode, uint8_t fan_speed, uint8_t temperature);
```

#### Sensor Reading

```cpp
float getRoomTemperature();
float getHumidity();
uint8_t getSensorType();
```

#### FRAM Persistence

```cpp
void bindFRAM(FRAM* fram, uint16_t fram_address);
void setFRAMAutoSave(bool autoSave);
void saveStateToFRAM();
void loadStateFromFRAM();
```

#### Callbacks

```cpp
uint8_t registerChangeCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback);
uint8_t registerSensorCallback(std::function<void(float, float)> callback);
void unregisterChangeCallback(uint8_t handler);
void unregisterSensorCallback(uint8_t handler);
```

#### Utilities

```cpp
uint8_t getType();
```

---

### CurrentTransformerCard

> **Detailed Docs:** [CurrentTransformerCard.md](CurrentTransformerCard.md)
> **Card Type:** `0x04`
> **Features:** Current measurement, energy accumulation, FRAM persistence

#### Constructor

```cpp
CurrentTransformerCard(AnalogCard* analogCard, uint8_t pin, float* voltage,
                      std::function<float(uint16_t)> adcToCurrent,
                      uint32_t conversionInterval);
```

#### Core Methods

```cpp
bool begin();
void loop();
void beginConversion();
```

#### Measurements

```cpp
float getCurrent();      // Amperes
double getEnergy();      // Watt-hours
float getPower();        // Watts
float getVoltage();      // Volts
```

#### Energy Management

```cpp
void setEnergy(float energy);
void resetEnergy();
```

#### FRAM Persistence

```cpp
void bindFRAM(FRAM* fram, uint32_t framAddress);
void saveEnergy();
void loadEnergy();
void setEnergyAutoSave(bool autoSave);
```

#### Callbacks

```cpp
uint8_t registerCallback(std::function<void(float, double)> callback);
void unregisterCallback(uint8_t handler);
```

#### Utilities

```cpp
uint8_t getType();
```

---

## IoT & Networking

### ESPMegaIoT

> **Detailed Docs:** [ESPMegaIoT.md](ESPMegaIoT.md)
> **Features:** WiFi, Ethernet, MQTT, card integration

#### Initialization

```cpp
void intr_begin(ExpansionCard* cards[]);
void loop();
void bindFRAM(FRAM* fram);
```

#### Network Configuration

```cpp
void connectToWifi(const char* ssid, const char* password);
void connectToWifi(const char* ssid);
void disconnectFromWifi();
bool wifiConnected();
void ethernetBegin();
void bindEthernetInterface(ETHClass* ethernetIface);
void connectNetwork();
bool networkConnected();
IPAddress getETHIp();
IPAddress getWifiIp();
IPAddress getIp();
String getETHMac();
String getWifiMac();
String getMac();
```

#### Network Config Management

```cpp
void setNetworkConfig(NetworkConfig network_config);
void saveNetworkConfig();
void loadNetworkConfig();
NetworkConfig* getNetworkConfig();
```

#### MQTT Configuration

```cpp
void setMqttConfig(MqttConfig mqtt_config);
void saveMqttConfig();
void loadMqttConfig();
MqttConfig* getMqttConfig();
```

#### MQTT Connection

```cpp
void connectToMqtt();
bool connectToMqtt(char* client_id, char* mqtt_server, uint16_t mqtt_port, char* mqtt_user, char* mqtt_password);
bool connectToMqtt(char* client_id, char* mqtt_server, uint16_t mqtt_port);
void disconnectFromMqtt();
bool mqttConnected();
```

#### Publishing & Subscribing

```cpp
void publish(const char* topic, const char* payload);
void publish(const char* topic, const char* payload, unsigned int length);
void publishRelative(const char* topic, const char* payload);
void publishRelative(const char* topic, const char* payload, unsigned int length);
void subscribe(const char* topic);
void subscribeRelative(const char* topic);
void unsubscribeFromTopic(const char* topic);
```

#### Card Integration

```cpp
void registerCard(uint8_t card_id);
void unregisterCard(uint8_t card_id);
void publishCard(uint8_t card_id);
IoTComponent* getComponent(uint8_t card_id);
```

#### Callbacks

```cpp
uint16_t registerMqttCallback(std::function<void(char*, char*)> callback);
void unregisterMqttCallback(uint16_t handler);
uint16_t registerRelativeMqttCallback(std::function<void(char*, char*)> callback);
void unregisterRelativeMqttCallback(uint16_t handler);
uint16_t registerSubscribeCallback(std::function<void(void)> callback);
void unregisterSubscribeCallback(uint16_t handler);
```

#### Utilities

```cpp
void setBaseTopic(char* base_topic);
void publishSystemSummary();
```

---

### ESPMegaWebServer

> **Detailed Docs:** [ESPMegaWebServer.md](ESPMegaWebServer.md)
> **Features:** Web UI, REST API, OTA updates, authentication

#### Constructor

```cpp
ESPMegaWebServer(uint16_t port, ESPMegaIoT* iot);
```

#### Core Methods

```cpp
void begin();
void loop();
```

#### Credentials Management

```cpp
void setWebUsername(const char* username);
void setWebPassword(const char* password);
char* getWebUsername();
char* getWebPassword();
void resetCredentials();
```

#### FRAM Persistence

```cpp
void bindFRAM(FRAM* fram);
void saveCredentialsToFRAM();
void loadCredentialsFromFRAM();
```

#### Server Access

```cpp
AsyncWebServer* getServer();
bool checkAuthentication(AsyncWebServerRequest* request);
```

#### Built-in Handlers

```cpp
void dashboardHandler(AsyncWebServerRequest* request);
void configHandler(AsyncWebServerRequest* request);
void getConfigHandler(AsyncWebServerRequest* request);
void saveConfigJSONHandler(AsyncWebServerRequest* request, JsonVariant& json);
void getDeviceInfoHandler(AsyncWebServerRequest* request);
void otaRequestHandler(AsyncWebServerRequest* request);
void otaUploadHandler(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
void restAPIHandler(AsyncWebServerRequest* request);
void rebootHandler(AsyncWebServerRequest* request);
```

---

## Display

### InternalDisplay

> **Detailed Docs:** [InternalDisplay.md](InternalDisplay.md)
> **Features:** 3.5" TFT LCD, touch interface, card binding

#### Constructor

```cpp
InternalDisplay(HardwareSerial* displayAdapter);
```

#### Initialization

```cpp
void begin(ESPMegaIoT* iot, std::function<rtctime_t()> getRtcTime);
void loop();
```

#### Card Binding

```cpp
void bindInputCard(DigitalInputCard* inputCard);
void bindOutputCard(DigitalOutputCard* outputCard);
void bindClimateCard(ClimateCard* climateCard);
void unbindInputCard();
void unbindOutputCard();
void unbindClimateCard();
```

---

## Supporting Classes

### ExpansionCard (Base Class)

```cpp
class ExpansionCard {
public:
    virtual bool begin();
    virtual void loop();
    virtual uint8_t getType();
};
```

### IoTComponent (Base Class)

```cpp
class IoTComponent {
public:
    virtual bool begin(uint8_t card_id, ExpansionCard* card, PubSubClient* mqtt, char* base_topic);
    virtual void handleMqttMessage(char* topic, char* payload);
    virtual void publishReport();
    virtual uint8_t getType();
    virtual void subscribe();
    void loop();
protected:
    void publishRelative(const char* topic, const char* payload);
    void subscribeRelative(const char* topic);
};
```

---

## Data Structures

### rtctime_t

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

### NetworkConfig

```cpp
struct NetworkConfig {
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
    char hostname[32];
    bool useStaticIp;
    bool useWifi;
    bool wifiUseAuth;
    char ssid[32];
    char password[32];
};
```

### MqttConfig

```cpp
struct MqttConfig {
    char mqtt_server[32];
    uint16_t mqtt_port;
    char mqtt_user[32];
    char mqtt_password[32];
    bool mqtt_useauth;
    char base_topic[32];
};
```

### AirConditioner

```cpp
struct AirConditioner {
    uint8_t max_temperature;
    uint8_t min_temperature;
    uint8_t modes;
    const char** mode_names;
    uint8_t fan_speeds;
    const char** fan_speed_names;
    size_t (*getInfraredCode)(uint8_t, uint8_t, uint8_t, const uint16_t**);
};
```

### ClimateCardData

```cpp
struct ClimateCardData {
    uint8_t ac_temperature;
    uint8_t ac_mode;
    uint8_t ac_fan_speed;
};
```

---

## Constants

### Card Types

```cpp
#define CARD_TYPE_DIGITAL_OUTPUT 0x00
#define CARD_TYPE_DIGITAL_INPUT  0x01
#define CARD_TYPE_ANALOG         0x02
#define CARD_TYPE_CLIMATE        0x03
#define CARD_TYPE_CT             0x04
```

### Sensor Types

```cpp
#define AC_SENSOR_TYPE_NONE     0x00
#define AC_SENSOR_TYPE_DHT22    0x01
#define AC_SENSOR_TYPE_DS18B20  0x02
```

### I2C Addresses

```cpp
#define INPUT_BANK_A_ADDRESS           0x21
#define INPUT_BANK_B_ADDRESS           0x22
#define PWM_BANK_ADDRESS               0x5F
#define RTC_ADDRESS                    0x68
#define FRAM_ADDRESS                   0x56
#define ANALOG_INPUT_BANK_A_ADDRESS    0x48
#define ANALOG_INPUT_BANK_B_ADDRESS    0x49
#define DAC0_ADDRESS                   0x60
#define DAC1_ADDRESS                   0x61
#define DAC2_ADDRESS                   0x62
#define DAC3_ADDRESS                   0x63
```

---

## Quick Reference by Use Case

### I want to...

**...read a digital input**
- Functional: `ESPMega_digitalRead(pin)`
- OOP: `espmega.inputs.digitalRead(pin)`

**...control an output**
- Functional: `ESPMega_digitalWrite(pin, state)`
- OOP: `espmega.outputs.digitalWrite(pin, state)`

**...use PWM**
- Functional: `ESPMega_analogWrite(pin, value)`
- OOP: `espmega.outputs.analogWrite(pin, value)`

**...read an analog input**
- Functional: `ESPMega_analogRead(pin)`
- OOP: `analogCard.analogRead(pin)`

**...connect to WiFi**
- `espmega.iot->connectToWifi(ssid, password)`

**...connect to MQTT**
- `espmega.iot->connectToMqtt()`

**...publish to MQTT**
- `espmega.iot->publish(topic, message)`

**...control AC**
- `climateCard.setTemperature(24)`
- `climateCard.setMode(2)`

**...measure power**
- `float current = ct.getCurrent()`
- `float power = ct.getPower()`

**...save state to FRAM**
- `outputs.bindFRAM(&fram, address)`
- `outputs.setAutoSaveToFRAM(true)`

**...create a web server**
- `espmega.enableWebServer(80)`

**...update firmware OTA**
- Navigate to `http://device-ip/ota`

---

For detailed information on any component, refer to the individual documentation files linked throughout this reference.

**Last Updated:** November 2024
**Library Version:** 2.10.0
