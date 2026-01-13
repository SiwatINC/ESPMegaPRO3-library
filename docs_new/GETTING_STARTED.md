# Getting Started with ESPMegaPRO R3

This guide will walk you through setting up and programming your ESPMegaPRO R3 board from scratch.

## Table of Contents

1. [Hardware Setup](#hardware-setup)
2. [Software Installation](#software-installation)
3. [Your First Program](#your-first-program)
4. [Understanding the Board](#understanding-the-board)
5. [Next Steps](#next-steps)

---

## Hardware Setup

### What You Need

- ESPMegaPRO R3 board
- Power supply (12V or 24V DC, depending on your model)
- USB cable (for programming)
- Computer with Windows, macOS, or Linux

### Physical Installation

1. **Power Connection**
   - Connect your 12V or 24V DC power supply to the power input terminal
   - Observe correct polarity (+ and -)
   - The board should power on (check for LED indicators)

2. **USB Connection**
   - Connect the USB cable from your computer to the ESPMegaPRO board
   - The board should enumerate as a serial device

3. **Wiring I/O (Optional for testing)**
   - **Digital Inputs (I0-I15)**: Connect switches or sensors
     - Common ground inputs, active high (close to +12V to activate)
   - **Digital Outputs (O0-O15)**: Connect LEDs, relays, or loads
     - 12V push-pull outputs
     - Max 0.6A per pin, 1.2A per group of 4 pins
   - Always observe current limits!

### Network Connection (Optional)

- **Ethernet**: Connect RJ45 cable to the Ethernet port
- **WiFi**: Configure in software (covered later)

---

## Software Installation

### Option 1: PlatformIO (Recommended)

PlatformIO is a professional IDE for embedded development.

#### Install PlatformIO

1. **Install VS Code**
   - Download from [code.visualstudio.com](https://code.visualstudio.com/)
   - Install and open VS Code

2. **Install PlatformIO Extension**
   - Open Extensions view (Ctrl+Shift+X)
   - Search for "PlatformIO IDE"
   - Click Install
   - Restart VS Code

#### Create Your First Project

1. **Open PlatformIO Home**
   - Click PlatformIO icon in sidebar
   - Click "New Project"

2. **Project Configuration**
   - Name: `my-espmega-project`
   - Board: `Espressif ESP32 Dev Module`
   - Framework: `Arduino`
   - Click Finish

3. **Configure platformio.ini**

   Open `platformio.ini` and replace contents with:

   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   monitor_speed = 115200
   lib_deps =
       SiwatINC/ESPMegaPROR3@^2.10.0
   ```

4. **Write Code**

   Open `src/main.cpp` and write your program (see examples below)

5. **Upload**
   - Click the upload button (→) in PlatformIO toolbar
   - Or press Ctrl+Alt+U
   - Wait for compilation and upload

### Option 2: Arduino IDE

#### Install Arduino IDE

1. Download Arduino IDE from [arduino.cc](https://www.arduino.cc/en/software)
2. Install and open Arduino IDE

#### Add ESP32 Board Support

1. Go to **File → Preferences**
2. In "Additional Board Manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click OK
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32"
6. Install "esp32 by Espressif Systems"

#### Install ESPMegaPRO Library

1. Go to **Sketch → Include Library → Manage Libraries**
2. Search for "ESPMegaPROR3"
3. Click Install

#### Configure Board

1. Go to **Tools → Board**
2. Select **ESP32 Dev Module**
3. Configure settings:
   - Upload Speed: 115200
   - CPU Frequency: 240MHz
   - Flash Size: 4MB
   - Partition Scheme: Default

#### Write and Upload

1. Write your program in the editor
2. Click Upload button (→)
3. Wait for compilation and upload

---

## Your First Program

### Example 1: Blinking Output (Functional Style)

The simplest program - blink output pin 0:

```cpp
#include <ESPMegaPRO.h>

void setup() {
    Serial.begin(115200);
    ESPMega_begin();
    Serial.println("ESPMegaPRO Started!");
}

void loop() {
    ESPMega_loop();

    // Blink output 0
    ESPMega_digitalWrite(0, HIGH);
    delay(500);
    ESPMega_digitalWrite(0, LOW);
    delay(500);

    Serial.println("Blink!");
}
```

**Upload this and observe:**
- Output 0 should turn on/off every second
- Serial monitor should print "Blink!" every second

### Example 2: Input to Output Mirror (Functional Style)

Read inputs and copy to outputs:

```cpp
#include <ESPMegaPRO.h>

void setup() {
    Serial.begin(115200);
    ESPMega_begin();
    Serial.println("Input Mirror Started!");
}

void loop() {
    ESPMega_loop();

    // Mirror all 16 inputs to outputs
    for (int i = 0; i < 16; i++) {
        bool inputState = ESPMega_digitalRead(i);
        ESPMega_digitalWrite(i, inputState);
    }
}
```

**Test this by:**
- Connecting a switch to any input
- Observing the corresponding output turn on/off

### Example 3: PWM Dimming (Functional Style)

Fade an output using PWM:

```cpp
#include <ESPMegaPRO.h>

void setup() {
    Serial.begin(115200);
    ESPMega_begin();
    Serial.println("PWM Dimmer Started!");
}

void loop() {
    ESPMega_loop();

    // Fade in
    for (int pwm = 0; pwm <= 4095; pwm += 10) {
        ESPMega_analogWrite(0, pwm);
        delay(5);
    }

    // Fade out
    for (int pwm = 4095; pwm >= 0; pwm -= 10) {
        ESPMega_analogWrite(0, pwm);
        delay(5);
    }
}
```

**Connect a 12V LED to output 0 to see smooth dimming effect**

### Example 4: OOP Style with Callbacks

More advanced using object-oriented interface:

```cpp
#include <ESPMegaProOS.hpp>

ESPMegaPRO espmega = ESPMegaPRO();

void inputChanged(uint8_t pin, bool state) {
    Serial.printf("Input %d changed to %s\n", pin, state ? "HIGH" : "LOW");

    // Mirror to output
    espmega.outputs.digitalWrite(pin, state);
}

void setup() {
    Serial.begin(115200);

    // Initialize board
    espmega.begin();

    // Register callback for input changes
    espmega.inputs.registerCallback(inputChanged);

    Serial.println("OOP Example Started!");
}

void loop() {
    espmega.loop();
}
```

**This example shows:**
- Event-driven programming with callbacks
- Clean OOP interface
- Automatic input change detection

---

## Understanding the Board

### Built-in I/O

The ESPMegaPRO R3 has built-in I/O that's always available:

#### Digital Inputs (I0-I15)
- **Type**: Isolated inputs
- **Logic**: Active high (connect to +12V to activate)
- **Protection**: Built-in isolation and protection
- **Debouncing**: Software debouncing available
- **Reading**: Use `ESPMega_digitalRead(pin)` or `espmega.inputs.digitalRead(pin)`

#### Digital Outputs (O0-O15)
- **Type**: PWM-capable push-pull outputs
- **Voltage**: 12V
- **Current**: 0.6A per pin, 1.2A per group (0-3, 4-7, 8-11, 12-15)
- **PWM Resolution**: 12-bit (0-4095)
- **Control**: Use `ESPMega_digitalWrite(pin, state)` or `ESPMega_analogWrite(pin, value)`

### Two Programming Styles

#### Functional Style (Simple)

```cpp
#include <ESPMegaPRO.h>

ESPMega_begin();
ESPMega_loop();
ESPMega_digitalRead(pin);
ESPMega_digitalWrite(pin, state);
ESPMega_analogWrite(pin, value);
```

**Pros:**
- Simple and intuitive
- Arduino-like
- Great for beginners

**Cons:**
- Only supports built-in I/O
- No expansion cards
- No IoT features
- No display support

#### OOP Style (Advanced)

```cpp
#include <ESPMegaProOS.hpp>

ESPMegaPRO espmega;
espmega.begin();
espmega.loop();
espmega.inputs.digitalRead(pin);
espmega.outputs.digitalWrite(pin, state);
espmega.installCard(slot, &card);
espmega.enableIotModule();
```

**Pros:**
- Full feature access
- Expansion card support
- IoT and networking
- Display support
- Event-driven with callbacks

**Cons:**
- Slightly more complex
- Requires OOP understanding

**Recommendation:** Start with Functional style, migrate to OOP when you need advanced features.

### Memory and Storage

#### FRAM (Non-Volatile Storage)
- **Purpose**: Store settings that survive power cycles
- **Size**: Several kilobytes
- **Access**: `espmega.fram.write8()`, `espmega.fram.read8()`
- **Use cases**: Network config, MQTT settings, card states

#### RTC (Real-Time Clock)
- **Purpose**: Keep accurate time even when powered off (with battery)
- **Access**: `espmega.getTime()`, `espmega.setTime()`
- **NTP Sync**: `espmega.updateTimeFromNTP()`

---

## Next Steps

### Basic Projects

1. **Input Monitor**
   - Display all input states on Serial
   - Add debouncing
   - Count pulses on inputs

2. **Output Controller**
   - Control outputs via Serial commands
   - Create patterns (chase, fade, blink)
   - PWM motor speed control

3. **Timer Application**
   - Use RTC to trigger outputs at specific times
   - Create daily schedules
   - Countdown timers

### Intermediate Projects

4. **Network Connectivity**
   - Connect to Ethernet or WiFi
   - Get time from NTP
   - Ping test

5. **MQTT Integration**
   - Connect to MQTT broker
   - Publish input states
   - Control outputs via MQTT
   - Home Assistant integration

6. **Web Interface**
   - Enable web server
   - Monitor I/O from browser
   - Configure network settings
   - OTA updates

### Advanced Projects

7. **Expansion Cards**
   - Install Analog Card
   - Read sensors (temperature, voltage, current)
   - Control analog outputs

8. **Climate Control**
   - Install Climate Card
   - Control AC via IR
   - Monitor temperature/humidity
   - Create thermostat logic

9. **Energy Monitoring**
   - Install Current Transformer Card
   - Monitor power consumption
   - Log energy usage
   - Cost calculation

10. **Complete Automation System**
    - Multiple expansion cards
    - MQTT + Web interface
    - Display integration
    - Complex automation logic

---

## Troubleshooting

### Upload Fails

**Problem:** Can't upload to board

**Solutions:**
1. Check USB cable (try different cable)
2. Install CH340/CP210x drivers if needed
3. Press and hold BOOT button during upload
4. Check correct COM port selected
5. Try lower upload speed (115200)

### Board Not Responding

**Problem:** Program uploaded but not working

**Solutions:**
1. Check power supply (12V/24V)
2. Check Serial monitor for error messages
3. Ensure `ESPMega_begin()` called in setup()
4. Ensure `ESPMega_loop()` called in loop()
5. Press RESET button

### Outputs Not Working

**Problem:** Outputs don't turn on

**Solutions:**
1. Check power supply voltage
2. Verify correct pin numbers (0-15)
3. Check current limits (0.6A per pin)
4. Measure output with multimeter
5. Test with LED and resistor first

### Inputs Not Reading

**Problem:** Inputs always read same value

**Solutions:**
1. Check input wiring (needs +12V to activate)
2. Verify `ESPMega_loop()` is called
3. Check if using OOP, call `espmega.loop()`
4. Test with simple wire to +12V
5. Check Serial output for debugging

### Network Issues

**Problem:** Can't connect to network

**Solutions:**
1. Check Ethernet cable
2. Verify network config in FRAM
3. Check DHCP vs static IP settings
4. Monitor Serial for network errors
5. Try WiFi if Ethernet fails

---

## Additional Resources

### Documentation
- [API Reference](API_REFERENCE.md) - Complete API documentation
- [Hardware Guide](HARDWARE.md) - Detailed hardware specifications
- [MQTT Protocol](MQTT_PROTOCOL.md) - MQTT topic structure
- [Examples](../examples/) - Code examples

### Community & Support
- GitHub Issues: [Report bugs](https://github.com/SiwatINC/ESPMegaPRO3-library/issues)
- Email: siwat@siwatinc.com
- Website: [siwatinc.com](https://siwatinc.com)

### Learning Resources
- [PlatformIO Docs](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/)
- [MQTT Basics](http://mqtt.org/)

---

## Quick Reference Card

### Functional API

```cpp
// Initialization
ESPMega_begin();
ESPMega_loop();

// Digital I/O
bool state = ESPMega_digitalRead(0-15);
ESPMega_digitalWrite(0-15, HIGH/LOW);

// PWM/Analog
ESPMega_analogWrite(0-15, 0-4095);
int16_t value = ESPMega_analogRead(0-7);  // Analog Card only
ESPMega_dacWrite(0-3, 0-4095);            // Analog Card only

// RTC
rtctime_t time = ESPMega_getTime();
ESPMega_setTime(h, m, s, day, month, year);
ESPMega_updateTimeFromNTP();
```

### OOP API

```cpp
// Initialization
ESPMegaPRO espmega;
espmega.begin();
espmega.loop();

// Digital I/O
espmega.inputs.digitalRead(0-15);
espmega.outputs.digitalWrite(0-15, HIGH/LOW);
espmega.outputs.analogWrite(0-15, 0-4095);

// Cards
espmega.installCard(slot, &card);
espmega.getCard(slot);

// IoT
espmega.enableIotModule();
espmega.iot->connectNetwork();
espmega.iot->connectToMqtt();
espmega.iot->publish(topic, message);

// Display
espmega.enableInternalDisplay(&Serial);

// Web Server
espmega.enableWebServer(80);

// Storage & Time
espmega.fram.write8(addr, value);
espmega.getTime();
espmega.setTime(h, m, s, day, month, year);
```

---

**Ready to build amazing IoT automation projects! 🚀**
