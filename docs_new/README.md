# ESPMegaPRO R3 Documentation

Complete documentation for the ESPMegaPRO R3 Industrial IoT Controller Library.

## 📚 Getting Started

- **[Getting Started Guide](GETTING_STARTED.md)** - Complete beginner's guide with hardware setup, installation, and first programs

## 📖 Core Documentation

### System Components
- **[ESPMegaIoT](ESPMegaIoT.md)** - Network connectivity, WiFi, Ethernet, and MQTT integration
- **[ESPMegaWebServer](ESPMegaWebServer.md)** - Web interface, REST API, and OTA updates
- **[InternalDisplay](InternalDisplay.md)** - Built-in touchscreen display interface

### Expansion Cards

#### I/O Cards
- **[DigitalInputCard](DigitalInputCard.md)** - 16-channel digital input with debouncing and callbacks
- **[DigitalOutputCard](DigitalOutputCard.md)** - 16-channel PWM output with FRAM persistence
- **[AnalogCard](AnalogCard.md)** - 8 ADC inputs + 4 DAC outputs

#### Specialty Cards
- **[ClimateCard](ClimateCard.md)** - IR air conditioner control with temperature/humidity sensors
- **[CurrentTransformerCard](CurrentTransformerCard.md)** - Energy monitoring and power measurement

## 💻 Code Examples

All examples are located in the `examples/` directory and are organized by component:

### Digital I/O Examples
- `examples/DigitalInput/` - Input reading, debouncing, callbacks, MQTT
- `examples/DigitalOutput/` - Output control, PWM, patterns, FRAM persistence

### Analog Examples
- `examples/Analog/` - ADC reading, DAC output, sensor interfacing

### Climate Control Examples
- `examples/Climate/` - AC control, temperature monitoring, thermostat logic, IR capture

### Energy Monitoring Examples
- `examples/EnergyMonitoring/` - Current measurement, energy logging, multiple CTs

### Networking Examples
- `examples/Networking/` - Ethernet, WiFi, MQTT, Home Assistant, Node-RED integration

### Web Server Examples
- `examples/WebServer/` - Web interface, custom endpoints, REST API, OTA updates

### Display Examples
- `examples/Display/` - Display integration with I/O cards and climate control

### Legacy Examples
- `examples/Functional_PinOperation/` - Simple functional-style examples
- `examples/OOP_Firmware/` - Complete OOP firmware example

## 🔍 Quick Reference

### Programming Styles

The library supports two programming styles:

#### Functional Style (Simple)
```cpp
#include <ESPMegaPRO.h>
ESPMega_begin();
ESPMega_digitalRead(pin);
ESPMega_digitalWrite(pin, state);
```

**Best for:** Simple projects, beginners, quick prototypes
**Limitations:** Built-in I/O only, no expansion cards, no IoT features

#### Object-Oriented Style (Advanced)
```cpp
#include <ESPMegaProOS.hpp>
ESPMegaPRO espmega;
espmega.inputs.digitalRead(pin);
espmega.outputs.digitalWrite(pin, state);
espmega.installCard(slot, &card);
```

**Best for:** Complex projects, multiple cards, IoT integration
**Features:** Full hardware access, networking, web server, display

## 📊 Documentation Statistics

| Component | Documentation | Examples | Total Lines |
|-----------|--------------|----------|-------------|
| DigitalInputCard | ✅ 25KB | 4 examples | ~40,000 |
| DigitalOutputCard | ✅ 24KB | 5 examples | ~45,000 |
| AnalogCard | ✅ 26KB | 5 examples | ~38,000 |
| ClimateCard | ✅ 37KB | 5 examples | ~52,000 |
| CurrentTransformerCard | ✅ 41KB | 5 examples | ~48,000 |
| ESPMegaIoT | ✅ 58KB | 6 examples | ~70,000 |
| ESPMegaWebServer | ✅ 33KB | 4 examples | ~42,000 |
| InternalDisplay | ✅ 37KB | 3 examples | ~38,000 |
| **Total** | **294KB** | **37 examples** | **~373,000** |

## 🎯 Learning Path

### Beginner (Week 1-2)
1. Read [Getting Started Guide](GETTING_STARTED.md)
2. Try `examples/DigitalInput/basic_reading/`
3. Try `examples/DigitalOutput/basic_control/`
4. Experiment with PWM dimming

### Intermediate (Week 3-4)
5. Read [DigitalInputCard](DigitalInputCard.md) and [DigitalOutputCard](DigitalOutputCard.md)
6. Implement callbacks and debouncing
7. Add FRAM persistence
8. Try analog I/O examples

### Advanced (Week 5-6)
9. Read [ESPMegaIoT](ESPMegaIoT.md)
10. Set up network connectivity
11. Implement MQTT integration
12. Try Home Assistant integration

### Expert (Week 7+)
13. Install expansion cards (Climate, Energy Monitoring)
14. Build complete automation system
15. Create custom web interface
16. Implement advanced control logic

## 🔗 External Resources

### Hardware
- [ESPMegaPRO Product Page](https://siwatinc.com)
- [Purchase Options](https://siwatinc.com)

### Software
- [GitHub Repository](https://github.com/SiwatINC/ESPMegaPRO3-library)
- [PlatformIO Library](https://platformio.org)

### Support
- [GitHub Issues](https://github.com/SiwatINC/ESPMegaPRO3-library/issues)
- Email: siwat@siwatinc.com

### Community
- [Arduino Forum](https://forum.arduino.cc/)
- [PlatformIO Community](https://community.platformio.org/)
- [ESP32 Forum](https://esp32.com/)

## 📝 Contributing to Documentation

Found an error or want to improve the documentation? Contributions are welcome!

1. Fork the repository
2. Make your changes
3. Submit a pull request

## 📄 License

This documentation is part of the ESPMegaPRO R3 library and is licensed under the MIT License.

---

**Last Updated:** November 2024
**Library Version:** 2.10.0
**Documentation Version:** 1.0.0
