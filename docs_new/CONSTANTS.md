# ESPMegaPRO3 Constants Reference

## Table of Contents
1. [SDK Version](#sdk-version)
2. [Hardware - I2C Addresses](#hardware---i2c-addresses)
3. [Hardware - Pin Definitions](#hardware---pin-definitions)
4. [Hardware - Interrupt Pins](#hardware---interrupt-pins)
5. [Expansion Cards - Card Type IDs](#expansion-cards---card-type-ids)
6. [Expansion Cards - FRAM Layout](#expansion-cards---fram-layout)
7. [Networking - Timeouts and Intervals](#networking---timeouts-and-intervals)
8. [Networking - TCP/MQTT](#networking---tcpmqtt)
9. [IoT - FRAM Memory Map](#iot---fram-memory-map)
10. [MQTT Topics - Digital Output Card](#mqtt-topics---digital-output-card)
11. [MQTT Topics - Digital Input Card](#mqtt-topics---digital-input-card)
12. [MQTT Topics - Analog Card](#mqtt-topics---analog-card)
13. [MQTT Topics - Climate Card](#mqtt-topics---climate-card)
14. [MQTT Topics - Current Transformer Card](#mqtt-topics---current-transformer-card)
15. [Display - Page IDs](#display---page-ids)
16. [Display - Picture IDs](#display---picture-ids)
17. [Display - Timeouts and Intervals](#display---timeouts-and-intervals)
18. [Display - Touch Types](#display---touch-types)
19. [Display - Messages](#display---messages)
20. [Climate Card - Sensor Types](#climate-card---sensor-types)
21. [Climate Card - AC Settings](#climate-card---ac-settings)
22. [Climate Card - Timing](#climate-card---timing)
23. [Recovery Mode](#recovery-mode)
24. [IR Receiver - Buffer Sizes](#ir-receiver---buffer-sizes)
25. [Global Extern Variables](#global-extern-variables)
26. [Data Structures](#data-structures)

---

## SDK Version

| Constant | Value | Type | Description | Location |
|----------|-------|------|-------------|----------|
| `SDK_VESRION` | `"2.10.0"` | String | Current SDK version identifier | ESPMegaCommon.hpp |

**Note:** There is a typo in the constant name (VESRION instead of VERSION).

---

## Hardware - I2C Addresses

All I2C addresses used by the ESPMegaPRO3 board and its components.

| Constant | Value | Description | Device Type | Used By |
|----------|-------|-------------|-------------|---------|
| `FRAM_ADDRESS` | `0x56` | FRAM memory module I2C address | FRAM (Ferroelectric RAM) | ESPMegaPRO.h, ESPMegaProOS.hpp |
| `INPUT_BANK_A_ADDRESS` | `0x21` | First bank of digital inputs | PCF8574 I/O Expander | ESPMegaPRO.h, ESPMegaProOS.hpp |
| `INPUT_BANK_B_ADDRESS` | `0x22` | Second bank of digital inputs | PCF8574 I/O Expander | ESPMegaPRO.h, ESPMegaProOS.hpp |
| `PWM_BANK_ADDRESS` | `0x5F` | PWM/Digital output controller | PCA9685 PWM Driver | ESPMegaPRO.h, ESPMegaProOS.hpp |
| `RTC_ADDRESS` | `0x68` | Real-time clock module | DS1307 RTC | ESPMegaPRO.h, ESPMegaProOS.hpp |
| `ANALOG_INPUT_BANK_A_ADDRESS` | `0x48` | First analog input bank (pins A0-A3) | ADS1115 ADC | ESPMegaPRO.h, AnalogCard.hpp |
| `ANALOG_INPUT_BANK_B_ADDRESS` | `0x49` | Second analog input bank (pins A4-A7) | ADS1115 ADC | ESPMegaPRO.h, AnalogCard.hpp |
| `DAC0_ADDRESS` | `0x60` | Digital-to-analog converter 0 | MCP4725 DAC | ESPMegaPRO.h, AnalogCard.hpp |
| `DAC1_ADDRESS` | `0x61` | Digital-to-analog converter 1 | MCP4725 DAC | ESPMegaPRO.h, AnalogCard.hpp |
| `DAC2_ADDRESS` | `0x62` | Digital-to-analog converter 2 | MCP4725 DAC | ESPMegaPRO.h, AnalogCard.hpp |
| `DAC3_ADDRESS` | `0x63` | Digital-to-analog converter 3 | MCP4725 DAC | ESPMegaPRO.h, AnalogCard.hpp |

### Notes:
- All addresses are 7-bit I2C addresses
- The FRAM, RTC, and I/O expanders are built into the base ESPMegaPRO board
- Analog components (ADCs and DACs) are only present if ANALOG_CARD_ENABLE is defined
- Each DAC is 12-bit resolution (0-4095)
- Each ADC bank supports 4 differential or 4 single-ended inputs

---

## Hardware - Pin Definitions

Physical pin numbers used by the ESPMegaPRO3 board.

| Constant | Value | Type | Description | Valid Range |
|----------|-------|------|-------------|-------------|
| `INPUT_BANK_A_INTERRUPT` | `36` | GPIO Pin | Interrupt pin for input bank A | ESP32 GPIO |
| `INPUT_BANK_B_INTERRUPT` | `39` | GPIO Pin | Interrupt pin for input bank B | ESP32 GPIO |

### Pin Ranges:
- **Digital Inputs:** I0-I15 (16 pins total, 8 per bank)
- **Digital Outputs/PWM:** P0-P15 (16 pins total)
- **Analog Inputs:** A0-A7 (8 pins total, 4 per bank)
- **Analog Outputs (DAC):** AO0-AO3 (4 pins total)

### Digital Input Pin Mapping:
- Bank A: I0-I7
- Bank B: I8-I15

### Digital Output Current Limits:
- Per pin: 0.6A maximum
- Per group (4 pins): 1.2A maximum
  - Group 0: P0-P3
  - Group 1: P4-P7
  - Group 2: P8-P11
  - Group 3: P12-P15

---

## Hardware - Interrupt Pins

| Mode | Status | Description |
|------|--------|-------------|
| `USE_INTERRUPT` | Commented out by default | When defined, enables interrupt-driven input reading |

**Note:** Interrupt mode is currently disabled by default. Input polling is used instead.

---

## Expansion Cards - Card Type IDs

Each expansion card has a unique type ID returned by `getType()`.

| Constant | Value | Card Type | Description | Header File |
|----------|-------|-----------|-------------|-------------|
| `CARD_TYPE_DIGITAL_OUTPUT` | `0x00` | Digital Output Card | 16-channel PWM output card (built-in) | DigitalOutputCard.hpp |
| `CARD_TYPE_DIGITAL_INPUT` | `0x01` | Digital Input Card | 16-channel digital input card (built-in) | DigitalInputCard.hpp |
| `CARD_TYPE_ANALOG` | `0x02` | Analog Card | 8 ADC inputs + 4 DAC outputs | AnalogCard.hpp |
| `CARD_TYPE_CLIMATE` | `0x03` | Climate Card | IR-based climate control with temp/humidity sensor | ClimateCard.hpp |
| `CARD_TYPE_CT` | `4` | Current Transformer Card | Current and energy monitoring | CurrentTransformerCard.hpp |

### Card Slot Assignment:
- Slot 0: Built-in Digital Input Card (default)
- Slot 1: Built-in Digital Output Card (default)
- Slots 2-254: Available for expansion cards

**Note:** Maximum 255 cards can be installed (slots 0-254).

---

## Expansion Cards - FRAM Layout

### Digital Output Card FRAM Layout
**Size:** 34 bytes per card

| Offset | Size | Description | Data Type |
|--------|------|-------------|-----------|
| 0-1 | 2 bytes | Pin states (16 bits, one per pin) | uint16_t |
| 2-33 | 32 bytes | PWM values (16 × 2 bytes) | uint16_t[16] |

**Usage:**
```cpp
card->bindFRAM(fram, address);  // Bind to FRAM at specified address
card->setAutoSaveToFRAM(true);   // Auto-save on changes
card->saveToFRAM();              // Manual save
card->loadFromFRAM();            // Restore from FRAM
```

### Climate Card FRAM Layout
**Size:** 3 bytes per card

| Offset | Size | Description | Data Type | Range |
|--------|------|-------------|-----------|-------|
| 0 | 1 byte | AC temperature setting | uint8_t | ac.min_temperature - ac.max_temperature |
| 1 | 1 byte | AC mode | uint8_t | 0 - (ac.modes - 1) |
| 2 | 1 byte | AC fan speed | uint8_t | 0 - (ac.fan_speeds - 1) |

**Structure:** `ClimateCardData`

### Current Transformer Card FRAM Layout
**Size:** 16 bytes per card

| Offset | Size | Description | Data Type |
|--------|------|-------------|-----------|
| 0-15 | 16 bytes | Energy accumulator | long double |

---

## Networking - Timeouts and Intervals

### NTP (Network Time Protocol)

| Constant | Value | Unit | Description | Usage |
|----------|-------|------|-------------|-------|
| `NTP_TIMEOUT_MS` | `5000` | milliseconds | Timeout for NTP server response | ESPMegaProOS.hpp |
| `NTP_UPDATE_INTERVAL_MS` | `60000` | milliseconds (1 minute) | Interval between automatic NTP updates | ESPMegaProOS.hpp |
| `NTP_INITIAL_SYNC_DELAY_MS` | `15000` | milliseconds | Initial delay before first NTP sync | ESPMegaProOS.hpp |

**Configuration:**
```cpp
ESPMega_configNTP(gmtOffset_sec, daylightOffset_sec, ntpServer);
bool success = espmega.updateTimeFromNTP();
```

---

## Networking - TCP/MQTT

| Constant | Value | Unit | Description | Location |
|----------|-------|------|-------------|----------|
| `TCP_TIMEOUT_SEC` | `5` | seconds | TCP connection timeout | ESPMegaIoT.hpp |
| `MQTT_RECONNECT_INTERVAL` | `30000` | milliseconds (30 sec) | Time between MQTT reconnection attempts | ESPMegaIoT.hpp |

### Network Configuration Structure

**Structure:** `NetworkConfig` (defined in ESPMegaIoT.hpp)

| Field | Type | Size | Description | Default/Range |
|-------|------|------|-------------|---------------|
| `ip` | IPAddress | 4 bytes | Static IP address | - |
| `gateway` | IPAddress | 4 bytes | Gateway address | - |
| `subnet` | IPAddress | 4 bytes | Subnet mask | - |
| `dns1` | IPAddress | 4 bytes | Primary DNS server | - |
| `dns2` | IPAddress | 4 bytes | Secondary DNS server | - |
| `hostname` | char[32] | 32 bytes | Device hostname | Max 31 chars + null |
| `useStaticIp` | bool | 1 byte | Use static IP vs DHCP | true/false |
| `useWifi` | bool | 1 byte | Use WiFi vs Ethernet | true/false |
| `wifiUseAuth` | bool | 1 byte | WiFi requires authentication | true/false |
| `ssid` | char[32] | 32 bytes | WiFi SSID | Max 31 chars + null |
| `password` | char[32] | 32 bytes | WiFi password | Max 31 chars + null |

**Total Size:** ~107 bytes

### MQTT Configuration Structure

**Structure:** `MqttConfig` (defined in ESPMegaIoT.hpp)

| Field | Type | Size | Description | Default/Range |
|-------|------|------|-------------|---------------|
| `mqtt_server` | char[32] | 32 bytes | MQTT broker address | Max 31 chars + null |
| `mqtt_port` | uint16_t | 2 bytes | MQTT broker port | 1883 (default) |
| `mqtt_user` | char[32] | 32 bytes | MQTT username | Max 31 chars + null |
| `mqtt_password` | char[32] | 32 bytes | MQTT password | Max 31 chars + null |
| `mqtt_useauth` | bool | 1 byte | Use MQTT authentication | true/false |
| `base_topic` | char[32] | 32 bytes | Base MQTT topic | Max 31 chars + null |

**Total Size:** ~131 bytes

---

## IoT - FRAM Memory Map

### ESPMegaIoT Module FRAM Allocation

| Address Range | Size | Component | Description |
|---------------|------|-----------|-------------|
| 0-33 | 34 bytes | Reserved | (Not used by IoT module) |
| 34-300 | 267 bytes | ESPMegaIoT | Network and MQTT configuration |
| 301-400 | 100 bytes | ESPMegaWebServer | Web server credentials |
| 401+ | - | User/Cards | Available for expansion cards and user data |

**Constants:**

| Constant | Value | Description | Location |
|----------|-------|-------------|----------|
| `IOT_FRAM_ADDRESS` | `34` | Start address for IoT configuration | ESPMegaIoT.hpp |

**Breakdown of IoT FRAM (34-300):**
- NetworkConfig: ~107 bytes
- MqttConfig: ~131 bytes
- Additional metadata: ~29 bytes

### ESPMegaWebServer FRAM Allocation

**Address Range:** 301-400 (100 bytes total)

| Field | Size | Description |
|-------|------|-------------|
| `webUsername` | 32 bytes | Web interface username |
| `webPassword` | 32 bytes | Web interface password |
| Additional space | 36 bytes | Reserved for future use |

**Usage:**
```cpp
webServer->bindFRAM(fram);
webServer->saveCredentialsToFRAM();
webServer->loadCredentialsFromFRAM();
webServer->resetCredentials();
```

---

## MQTT Topics - Digital Output Card

All topics are relative to `{base_topic}/card/{slot}/`.

| Constant | Value | Direction | Payload | Description |
|----------|-------|-----------|---------|-------------|
| `SET_STATE_TOPIC` | `/set/state` | Subscribe | `0` or `1` | Set pin digital state |
| `SET_VALUE_TOPIC` | `/set/value` | Subscribe | `0-4095` | Set pin PWM value |
| `STATE_TOPIC` | `/state` | Publish | `0` or `1` | Pin digital state |
| `VALUE_TOPIC` | `/value` | Publish | `0-4095` | Pin PWM value |
| `REQUEST_STATE_TOPIC` | `requeststate` | Subscribe | N/A | Request state publication |
| `PUBLISH_ENABLE_TOPIC` | `publish_enable` | Subscribe | `0` or `1` | Enable/disable auto-publish |

### Topic Examples:
```
{base_topic}/card/1/00/set/state     → Set pin 0 state
{base_topic}/card/1/00/set/value     → Set pin 0 PWM value
{base_topic}/card/1/00/state         ← Pin 0 state
{base_topic}/card/1/00/value         ← Pin 0 PWM value
{base_topic}/card/1/requeststate     → Request all states
{base_topic}/card/1/publish_enable   → Enable publishing
```

**Pin Format:** Always 2 digits, zero-padded (00-15)

---

## MQTT Topics - Digital Input Card

All topics are relative to `{base_topic}/card/{slot}/`.

| Constant | Value | Direction | Payload | Description |
|----------|-------|-----------|---------|-------------|
| `PUBLISH_ENABLE_TOPIC` | `publish_enable` | Subscribe | `0` or `1` | Enable/disable state publishing |
| `INPUT_REQUEST_STATE_TOPIC` | `requeststate` | Subscribe | N/A | Request current input states |

### Topic Examples:
```
{base_topic}/card/0/{pin}            ← Pin state (0 or 1)
{base_topic}/card/0/requeststate     → Request all states
{base_topic}/card/0/publish_enable   → Enable publishing
```

---

## MQTT Topics - Analog Card

All topics are relative to `{base_topic}/card/{slot}/`.

### DAC (Digital-to-Analog Converter) Topics

| Constant | Value | Direction | Payload | Description |
|----------|-------|-----------|---------|-------------|
| `DAC_SET_STATE_TOPIC` | `/set/state` | Subscribe | `0` or `1` | Set DAC channel on/off |
| `DAC_SET_VALUE_TOPIC` | `/set/value` | Subscribe | `0-4095` | Set DAC output value |
| `DAC_STATE_TOPIC` | `/dac/00/state` | Publish | `0` or `1` | DAC channel state |
| `DAC_VALUE_TOPIC` | `/dac/00/value` | Publish | `0-4095` | DAC output value |
| `DAC_PUBLISH_ENABLE_TOPIC` | `/publish_enable` | Subscribe | `0` or `1` | Enable/disable DAC publishing |
| `REQUEST_STATE_TOPIC` | `requeststate` | Subscribe | N/A | Request all ADC/DAC states |

### Topic Examples:
```
{base_topic}/card/2/dac/00/set/state → Set DAC 0 state
{base_topic}/card/2/dac/00/set/value → Set DAC 0 value (0-4095)
{base_topic}/card/2/dac/00/state     ← DAC 0 state
{base_topic}/card/2/dac/00/value     ← DAC 0 value
{base_topic}/card/2/adc/00           ← ADC 0 reading (0-4095)
{base_topic}/card/2/requeststate     → Request all states
```

**Pin Format:** Always 2 digits, zero-padded (00-03 for DAC, 00-07 for ADC)

---

## MQTT Topics - Climate Card

All topics are relative to `{base_topic}/card/{slot}/`.

| Constant | Value | Direction | Payload | Description |
|----------|-------|-----------|---------|-------------|
| `AC_MODE_REPORT_TOPIC` | `mode` | Publish | Mode name string | Current AC mode |
| `AC_MODE_SET_TOPIC` | `set/mode` | Subscribe | Mode name or index | Set AC mode |
| `AC_TEMPERATURE_REPORT_TOPIC` | `temperature` | Publish | Integer | Target temperature |
| `AC_TEMPERATURE_SET_TOPIC` | `set/temperature` | Subscribe | Integer | Set target temperature |
| `AC_FAN_SPEED_REPORT_TOPIC` | `fan_speed` | Publish | Speed name string | Current fan speed |
| `AC_FAN_SPEED_SET_TOPIC` | `set/fan_speed` | Subscribe | Speed name or index | Set fan speed |
| `AC_ROOM_TEMPERATURE_REPORT_TOPIC` | `room_temperature` | Publish | Float | Room temperature reading |
| `AC_HUMIDITY_REPORT_TOPIC` | `humidity` | Publish | Float | Room humidity reading (%) |
| `AC_REQUEST_STATE_TOPIC` | `requeststate` | Subscribe | N/A | Request all climate states |

### Topic Examples:
```
{base_topic}/card/3/mode                → "off", "fan_only", "cool"
{base_topic}/card/3/set/mode            ← "cool" or "2"
{base_topic}/card/3/temperature         → "24"
{base_topic}/card/3/set/temperature     ← "22"
{base_topic}/card/3/fan_speed           → "auto", "low", "medium", "high"
{base_topic}/card/3/set/fan_speed       ← "low" or "1"
{base_topic}/card/3/room_temperature    → "25.5"
{base_topic}/card/3/humidity            → "60.2"
{base_topic}/card/3/requeststate        → Request all values
```

---

## MQTT Topics - Current Transformer Card

All topics are relative to `{base_topic}/card/{slot}/`.

| Constant | Value | Direction | Payload | Description |
|----------|-------|-----------|---------|-------------|
| `CT_REQUESTSTATE_TOPIC` | `requeststate` | Subscribe | N/A | Request current readings |
| `CT_SET_ENERGY_TOPIC` | `energy/set` | Subscribe | Float | Set energy accumulator value |
| `CT_RESET_ENERGY_TOPIC` | `energy/reset` | Subscribe | N/A | Reset energy accumulator to 0 |
| `CT_ENERGY_TOPIC` | `energy` | Publish | Double | Total energy consumed (kWh) |
| `CT_POWER_TOPIC` | `power` | Publish | Float | Current power (W) |
| `CT_CURRENT_TOPIC` | `current` | Publish | Float | Current (A) |

### Topic Examples:
```
{base_topic}/card/4/current          ← Current in Amps
{base_topic}/card/4/power            ← Power in Watts
{base_topic}/card/4/energy           ← Energy in kWh
{base_topic}/card/4/energy/set       → Set energy value
{base_topic}/card/4/energy/reset     → Reset energy to 0
{base_topic}/card/4/requeststate     → Request current readings
```

---

## Display - Page IDs

Page IDs for the internal display navigation.

| Constant | Value | Page Name | Description |
|----------|-------|-----------|-------------|
| `INTERNAL_DISPLAY_BOOT_PAGE` | `0` | Boot Screen | Shown during device initialization |
| `INTERNAL_DISPLAY_DASHBOARD_PAGE` | `1` | Dashboard | Main status overview page |
| `INTERNAL_DISPLAY_INPUT_PAGE` | `2` | Input Status | Digital input monitoring |
| `INTERNAL_DISPLAY_OUTPUT_PAGE` | `3` | Output Control | Digital output control |
| `INTERNAL_DISPLAY_AC_PAGE` | `4` | Climate Control | Air conditioner control |
| `INTERNAL_DISPLAY_PWM_ADJUSTMENT_PAGE` | `5` | PWM Adjustment | Fine-tune PWM output values |
| `INTERNAL_DISPLAY_NETWORK_CONFIG_PAGE` | `6` | Network Config | Network settings configuration |
| `INTERNAL_DISPLAY_OTA_PAGE` | `9` | OTA Update | Over-the-air firmware update |
| `INTERNAL_DISPLAY_CLIMATE_NULL_PTR_PAGE` | `10` | Climate Error | Shown when climate card not installed |
| `INTERNAL_DISPLAY_MQTT_CONFIG_PAGE` | `11` | MQTT Config | MQTT broker configuration |
| `INTERNAL_DISPLAY_INPUT_NULL_PTR_PAGE` | `12` | Input Error | Shown when input card not bound |
| `INTERNAL_DISPLAY_OUTPUT_NULL_PTR_PAGE` | `13` | Output Error | Shown when output card not bound |

**Location:** InternalDisplay.hpp

---

## Display - Picture IDs

Picture/icon resource IDs used by the internal display.

### Network Status Icons

| Constant | Value | Description |
|----------|-------|-------------|
| `PIC_LAN_DISCONNECTED` | `2` | Network disconnected icon |
| `PIC_LAN_CONNECTED` | `3` | Network connected icon |
| `PIC_MQTT_DISCONNECTED` | `4` | MQTT disconnected icon |
| `PIC_MQTT_CONNECTED` | `5` | MQTT connected icon |

### PWM Bar Graphics

| Constant | Value | Description |
|----------|-------|-------------|
| `PIC_PWM_BAR_ON` | `33` | PWM bar segment (active) |
| `PIC_PWM_BAR_OFF` | `48` | PWM bar segment (inactive) |

### AC Mode Icons

| Constant | Value | Description |
|----------|-------|-------------|
| `PIC_AC_MODE_OFF_ACTIVE` | `24` | AC off mode (selected) |
| `PIC_AC_MODE_OFF_INACTIVE` | `25` | AC off mode (unselected) |
| `PIC_AC_MODE_FAN_ACTIVE` | `22` | AC fan mode (selected) |
| `PIC_AC_MODE_FAN_INACTIVE` | `23` | AC fan mode (unselected) |
| `PIC_AC_MODE_COOL_ACTIVE` | `12` | AC cool mode (selected) |
| `PIC_AC_MODE_COOL_INACTIVE` | `13` | AC cool mode (unselected) |

### AC Fan Speed Icons

| Constant | Value | Description |
|----------|-------|-------------|
| `PIC_AC_FAN_SPEED_AUTO_ACTIVE` | `14` | Auto fan speed (selected) |
| `PIC_AC_FAN_SPEED_AUTO_INACTIVE` | `15` | Auto fan speed (unselected) |
| `PIC_AC_FAN_SPEED_LOW_ACTIVE` | `18` | Low fan speed (selected) |
| `PIC_AC_FAN_SPEED_LOW_INACTIVE` | `19` | Low fan speed (unselected) |
| `PIC_AC_FAN_SPEED_MEDIUM_ACTIVE` | `20` | Medium fan speed (selected) |
| `PIC_AC_FAN_SPEED_MEDIUM_INACTIVE` | `21` | Medium fan speed (unselected) |
| `PIC_AC_FAN_SPEED_HIGH_ACTIVE` | `16` | High fan speed (selected) |
| `PIC_AC_FAN_SPEED_HIGH_INACTIVE` | `17` | High fan speed (unselected) |

**Location:** InternalDisplay.hpp

---

## Display - Timeouts and Intervals

| Constant | Value | Unit | Description | Location |
|----------|-------|------|-------------|----------|
| `DISPLAY_MUTEX_TAKE_TIMEOUT` | `1000` | milliseconds | Timeout for acquiring display serial mutex | ESPMegaDisplay.hpp |
| `OTA_WAIT_TIMEOUT` | `1000` | milliseconds | Timeout for OTA operations | ESPMegaDisplay.hpp |
| `DISPLAY_FETCH_TIMEOUT` | `100` | milliseconds | Timeout for fetching display data | ESPMegaDisplay.hpp |
| `DISPLAY_FETCH_RETRY_COUNT` | `5` | count | Number of retries for failed fetch operations | ESPMegaDisplay.hpp |
| `INTERNAL_DISPLAY_CLOCK_REFRESH_INTERVAL` | `15000` | milliseconds (15 sec) | Clock update interval | InternalDisplay.hpp |
| `INTERNAL_DISPLAY_TOP_BAR_REFRESH_INTERVAL` | `5000` | milliseconds (5 sec) | Status bar refresh interval | InternalDisplay.hpp |

### Display Buffer Sizes

| Buffer | Size | Description | Location |
|--------|------|-------------|----------|
| `rx_buffer` | 256 bytes | Serial receive buffer | ESPMegaDisplay.hpp |
| `tx_buffer` | 256 bytes | Serial transmit buffer | ESPMegaDisplay.hpp |

---

## Display - Touch Types

Touch event type identifiers.

| Constant | Value | Description | Location |
|----------|-------|-------------|----------|
| `TOUCH_TYPE_PRESS` | `0x01` | Touch press event | InternalDisplay.hpp |
| `TOUCH_TYPE_RELEASE` | `0x0` | Touch release event | InternalDisplay.hpp |

**Usage:** Received in touch callbacks to distinguish between press and release events.

---

## Display - Messages

Display message string constants.

| Constant | Value | Description | Location |
|----------|-------|-------------|----------|
| `MSG_MQTT_CONNECTED` | `"BMS Managed"` | Display text when MQTT connected | InternalDisplay.hpp |
| `MSG_MQTT_DISCONNECTED` | `"Standalone"` | Display text when MQTT disconnected | InternalDisplay.hpp |
| `MSG_PWM_ADJUSTMENT_STATE_ON` | `"ON"` | PWM state on text | InternalDisplay.hpp |
| `MSG_PWM_ADJUSTMENT_STATE_OFF` | `"OFF"` | PWM state off text | InternalDisplay.hpp |
| `PASSWORD_OBFUSCATION_STRING` | `"********"` | Password masking string | InternalDisplay.hpp |

---

## Climate Card - Sensor Types

Sensor type identifiers for the Climate Card.

| Constant | Value | Sensor Type | Description | Location |
|----------|-------|-------------|-------------|----------|
| `AC_SENSOR_TYPE_NONE` | `0x00` | No Sensor | Climate card without temperature sensor | ClimateCard.hpp |
| `AC_SENSOR_TYPE_DHT22` | `0x01` | DHT22 | Temperature and humidity sensor | ClimateCard.hpp |
| `AC_SENSOR_TYPE_DS18B20` | `0x02` | DS18B20 | Temperature-only sensor (1-Wire) | ClimateCard.hpp |

**Usage:**
```cpp
ClimateCard climate(ir_pin, ac_config, AC_SENSOR_TYPE_DHT22, sensor_pin, channel);
uint8_t type = climate.getSensorType();
```

---

## Climate Card - AC Settings

### AC Fan Speed Indices

These are position assumptions for standard AC units.

| Constant | Value | Fan Speed | Description |
|----------|-------|-----------|-------------|
| `AC_FAN_SPEED_AUTO` | `0` | Auto | Automatic fan speed control |
| `AC_FAN_SPEED_LOW` | `1` | Low | Low fan speed |
| `AC_FAN_SPEED_MEDIUM` | `2` | Medium | Medium fan speed |
| `AC_FAN_SPEED_HIGH` | `3` | High | High fan speed |

### AC Mode Indices

These are position assumptions for standard AC units.

| Constant | Value | Mode | Description |
|----------|-------|------|-------------|
| `AC_MODE_OFF` | `0` | Off | Air conditioner off |
| `AC_MODE_FAN_ONLY` | `1` | Fan Only | Fan circulation without cooling |
| `AC_MODE_COOL` | `2` | Cool | Cooling mode |

**Note:** Actual mode and fan speed configurations depend on the specific AC unit's `AirConditioner` structure. These constants are used by the InternalDisplay UI and may not match all AC models.

**Location:** InternalDisplay.hpp

---

## Climate Card - Timing

| Constant | Value | Unit | Description | Location |
|----------|-------|------|-------------|----------|
| `AC_SENSOR_READ_INTERVAL` | `5000` | milliseconds (5 sec) | Interval between sensor readings | ClimateCard.hpp |
| `AC_SENSOR_READ_TIMEOUT` | `250` | milliseconds | Timeout for sensor read operation | ClimateCard.hpp |

**Usage:** Controls how often the climate card reads from DHT22/DS18B20 sensors.

---

## Recovery Mode

Recovery mode is activated when the device is in a bootloop.

| Constant | Value | Unit | Description | Location |
|----------|-------|------|-------------|----------|
| `RECOVERY_WATCHDOG_TIMEOUT` | `15` | seconds | Time before restart is considered a bootloop | ESPMegaRecovery.hpp |

**Behavior:**
- If device restarts before this timeout expires
- And this happens 5 consecutive times
- Device enters recovery mode
- Recovery mode blocks normal operation and starts OTA server
- Exit by uploading new firmware or power cycling

**FRAM Usage:** 1 byte to store bootloop counter

---

## IR Receiver - Buffer Sizes

| Buffer | Size | Type | Description | Location |
|--------|------|------|-------------|----------|
| `irBuffer` | 1000 | unsigned int | IR timing data buffer | IRReceiver.hpp |

**Note:** Buffer is marked `volatile` as it's modified by interrupt service routine (ISR).

---

## Global Extern Variables

Global variables accessible across the library.

| Variable | Type | Description | Location | Scope |
|----------|------|-------------|----------|-------|
| `ESPMega_FRAM` | `FRAM` | Global FRAM object | ESPMegaPRO.h | External linkage |

**Usage:**
```cpp
extern FRAM ESPMega_FRAM;
ESPMega_FRAM.read(address, &data, size);
ESPMega_FRAM.write(address, &data, size);
```

---

## Data Structures

### rtctime_t

Time structure used throughout the library.

```cpp
struct rtctime_t {
    uint8_t hours;      // 0-23
    uint8_t minutes;    // 0-59
    uint8_t seconds;    // 0-59
    uint8_t day;        // 1-31
    uint8_t month;      // 1-12
    uint8_t year;       // Full year (e.g., 2024)
};
```

**Location:** ESPMegaPRO.h, TimeStructure.hpp

**Note:** Not compatible with Arduino Time library.

### NetworkConfig

Network configuration structure (107 bytes total).

```cpp
struct NetworkConfig {
    IPAddress ip;               // Static IP address
    IPAddress gateway;          // Gateway address
    IPAddress subnet;           // Subnet mask
    IPAddress dns1;             // Primary DNS
    IPAddress dns2;             // Secondary DNS
    char hostname[32];          // Device hostname
    bool useStaticIp;           // true = static IP, false = DHCP
    bool useWifi;               // true = WiFi, false = Ethernet
    bool wifiUseAuth;           // true = use SSID/password
    char ssid[32];              // WiFi SSID
    char password[32];          // WiFi password
};
```

**Location:** ESPMegaIoT.hpp

**FRAM Storage:** Saved to FRAM address 34-300

### MqttConfig

MQTT broker configuration structure (131 bytes total).

```cpp
struct MqttConfig {
    char mqtt_server[32];       // MQTT broker address/hostname
    uint16_t mqtt_port;         // MQTT broker port (default: 1883)
    char mqtt_user[32];         // MQTT username
    char mqtt_password[32];     // MQTT password
    bool mqtt_useauth;          // Enable MQTT authentication
    char base_topic[32];        // Base topic prefix
};
```

**Location:** ESPMegaIoT.hpp

**FRAM Storage:** Saved to FRAM address 34-300

### ClimateCardData

Air conditioner state structure (3 bytes).

```cpp
struct ClimateCardData {
    uint8_t ac_temperature;     // Target temperature
    uint8_t ac_mode;            // Operating mode index
    uint8_t ac_fan_speed;       // Fan speed index
};
```

**Location:** ClimateCard.hpp

**FRAM Storage:** 3 bytes when bindFRAM() is used

### AirConditioner

Air conditioner model definition structure.

```cpp
struct AirConditioner {
    uint8_t max_temperature;              // Maximum temperature setting
    uint8_t min_temperature;              // Minimum temperature setting
    uint8_t modes;                        // Number of operating modes
    const char **mode_names;              // Array of mode name strings
    uint8_t fan_speeds;                   // Number of fan speed levels
    const char **fan_speed_names;         // Array of fan speed name strings
    size_t (*getInfraredCode)(            // Function to get IR codes
        uint8_t mode,
        uint8_t fan_speed,
        uint8_t temperature,
        const uint16_t** code
    );
};
```

**Location:** ClimateCard.hpp

**Usage:** Define AC model characteristics and IR code generation function.

### ir_data_t

Infrared signal data container.

```cpp
struct ir_data_t {
    unsigned int *data;         // Pointer to timing data array
    size_t size;                // Number of timing values
};
```

**Location:** IRReceiver.hpp

**Usage:** Returned by `IRReceiver::end_long_receive()` containing recorded IR signal.

---

## Public Member Variables in Major Classes

### ESPMegaPRO Class

**Location:** ESPMegaProOS.hpp

| Variable | Type | Description | Default Slot |
|----------|------|-------------|--------------|
| `fram` | `FRAM` | FRAM memory object | - |
| `inputs` | `DigitalInputCard` | Built-in digital input card | Slot 0 |
| `outputs` | `DigitalOutputCard` | Built-in digital output card | Slot 1 |
| `display` | `InternalDisplay*` | Internal display pointer | - |
| `iot` | `ESPMegaIoT*` | IoT module pointer | - |
| `webServer` | `ESPMegaWebServer*` | Web server pointer | - |
| `recovery` | `ESPMegaRecovery` | Recovery mode handler | - |

**Notes:**
- `inputs` initialized with addresses `INPUT_BANK_A_ADDRESS`, `INPUT_BANK_B_ADDRESS`
- `outputs` initialized with address `PWM_BANK_ADDRESS`
- Pointers (`display`, `iot`, `webServer`) must be enabled via respective `enable*()` methods

### ESPMegaDisplay Class

**Location:** ESPMegaDisplay.hpp

| Variable | Type | Visibility | Description |
|----------|------|------------|-------------|
| `serialMutex` | `SemaphoreHandle_t` | Public | RTOS mutex for serial access |

**Protected Variables:**

| Variable | Type | Description | Size/Range |
|----------|------|-------------|------------|
| `currentPage` | `uint8_t` | Currently displayed page | 0-255 |
| `rx_buffer` | `char[256]` | Serial receive buffer | 256 bytes |
| `tx_buffer` | `char[256]` | Serial transmit buffer | 256 bytes |
| `rx_buffer_index` | `uint8_t` | Current position in rx_buffer | 0-255 |
| `otaBytesWritten` | `size_t` | OTA update progress | - |
| `baudRate` | `uint32_t` | Normal operation baud rate | - |
| `uploadBaudRate` | `uint32_t` | OTA upload baud rate | - |
| `txPin` | `uint8_t` | TX pin number | - |
| `rxPin` | `uint8_t` | RX pin number | - |

---

## PWM Value Ranges

### Digital Output Card (PWM)

| Parameter | Min | Max | Resolution | Description |
|-----------|-----|-----|------------|-------------|
| PWM Value | `0` | `4095` | 12-bit | 0 = fully off, 4095 = fully on |
| Digital State | `0` (LOW) | `1` (HIGH) | Boolean | On/off state |

### Analog Card (DAC)

| Parameter | Min | Max | Resolution | Description |
|-----------|-----|-----|------------|-------------|
| DAC Value | `0` | `4095` | 12-bit | 0V to Vref output |
| ADC Reading | `0` | `4095` | 12-bit | Input voltage reading |

**Note:** Both PWM and DAC use 12-bit resolution (4096 discrete levels).

---

## Compilation Flags

| Flag | Default | Description | Location |
|------|---------|-------------|----------|
| `ANALOG_CARD_ENABLE` | Defined | Enables analog card support | ESPMegaPRO.h |
| `USE_INTERRUPT` | Commented out | Enables interrupt-driven input reading | ESPMegaPRO.h |

**Usage:**
```cpp
#define ANALOG_CARD_ENABLE      // Include before ESPMegaPRO.h
#define USE_INTERRUPT           // Enable interrupt mode
```

---

## Array and Map Sizes

### ESPMegaPRO Class

| Array | Size | Type | Description |
|-------|------|------|-------------|
| `cards` | 255 | `ExpansionCard*[]` | Expansion card pointers |
| `cardInstalled` | 255 | `bool[]` | Card installation status |

### ESPMegaIoT Class

| Container | Size/Type | Description |
|-----------|-----------|-------------|
| `components` | 255 × `IoTComponent*` | IoT component array |
| `payload_buffer` | 200 bytes | MQTT payload buffer |

### Pin Mappings

Each card maintains pin mapping arrays:

| Array | Size | Description |
|-------|------|-------------|
| `pinMap` | 16 | Physical to virtual pin mapping |
| `virtualPinMap` | 16 | Virtual to physical pin mapping |

---

## MQTT Payload Buffer

| Constant | Value | Description | Location |
|----------|-------|-------------|----------|
| Payload Buffer Size | 200 bytes | Maximum MQTT message payload | ESPMegaIoT.hpp (implicit) |

**Note:** This limits MQTT message size. For larger payloads, messages must be split.

---

## Card Addressing

### DIP Switch to I2C Address Conversion

Digital cards can be addressed by DIP switch position:

```cpp
DigitalInputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5);
DigitalOutputCard(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4);
```

**Note:** Refer to hardware documentation for DIP switch to I2C address mapping.

---

## Search Index

**Quick Reference by Category:**

- **Hardware:** I2C Addresses, Pin Definitions
- **Cards:** Card Type IDs, FRAM Layouts
- **Networking:** Timeouts, Network/MQTT Config
- **MQTT:** Topics for each card type
- **Display:** Page IDs, Picture IDs, Touch Types, Messages
- **Climate:** Sensor Types, AC Settings, Timing
- **Memory:** FRAM Memory Map
- **Recovery:** Watchdog Timeout
- **Version:** SDK Version

**Quick Reference by Component:**

- **ESPMegaPRO Board:** Hardware addresses, pin definitions
- **Digital Input Card:** Card type 0x01, MQTT topics
- **Digital Output Card:** Card type 0x00, PWM ranges, FRAM layout, MQTT topics
- **Analog Card:** Card type 0x02, ADC/DAC addresses, MQTT topics
- **Climate Card:** Card type 0x03, sensor types, FRAM layout, MQTT topics
- **Current Transformer:** Card type 4, MQTT topics, FRAM layout
- **IoT Module:** Network/MQTT config, FRAM 34-300
- **Web Server:** Credentials, FRAM 301-400
- **Display:** Page IDs, picture IDs, refresh intervals
- **Recovery:** Bootloop detection

---

**Document Version:** 1.0
**SDK Version:** 2.10.0
**Last Updated:** 2025-11-23
