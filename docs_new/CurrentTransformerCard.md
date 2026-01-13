# CurrentTransformerCard - Current and Energy Monitoring

The CurrentTransformerCard class provides comprehensive current and energy monitoring capabilities using current transformers (CTs) connected to the ESPMegaPRO's Analog Card. It supports real-time current measurement, energy accumulation, FRAM persistence, and MQTT integration for IoT applications.

## Table of Contents

1. [Overview](#overview)
2. [How Current Transformers Work](#how-current-transformers-work)
3. [Hardware Setup](#hardware-setup)
4. [Class Reference](#class-reference)
5. [ADC to Current Conversion](#adc-to-current-conversion)
6. [Energy Calculation](#energy-calculation)
7. [FRAM Persistence](#fram-persistence)
8. [Callback System](#callback-system)
9. [IoT Integration](#iot-integration)
10. [Common CT Models](#common-ct-models)
11. [Complete Examples](#complete-examples)

---

## Overview

### Key Features

- **Real-time current measurement** from current transformers
- **Automatic energy accumulation** in Watt-hours (Wh)
- **Non-volatile storage** using FRAM for energy data persistence
- **Auto-save functionality** to prevent energy data loss
- **Callback system** for event-driven monitoring
- **MQTT integration** for remote monitoring and control
- **Flexible ADC conversion** supporting various CT models
- **Configurable sampling intervals**

### What You'll Need

- ESPMegaPRO R3 board with Analog Card
- Current transformer (CT) sensor (e.g., SCT-013)
- Burden resistor (if using current-output CT)
- 3.5mm audio jack (for SCT-013 series)
- Known AC voltage reference

---

## How Current Transformers Work

### Basic Principle

A current transformer is a type of instrument transformer that produces an alternating current in its secondary coil which is proportional to the current flowing in its primary conductor.

**Key Characteristics:**
- **Non-invasive**: Clamps around the conductor without breaking the circuit
- **Isolation**: Provides electrical isolation between measured circuit and monitoring circuit
- **Step-down**: Converts high current to measurable low current
- **AC only**: Only works with alternating current

### Types of Current Transformers

#### 1. Current Output CT (e.g., SCT-013-000)

These CTs output a current proportional to the measured current.

```
Primary Current (Line) → CT → Secondary Current (typically 0-50mA)
```

**Requires:**
- Burden resistor to convert current to voltage
- Biasing circuit to shift voltage to ADC range (0-3.3V)

**Typical Configuration:**
- Burden resistor: 33Ω - 100Ω
- Bias voltage: 1.65V (VCC/2)

#### 2. Voltage Output CT (e.g., SCT-013-030)

These CTs have an internal burden resistor and output voltage directly.

```
Primary Current (Line) → CT → Output Voltage (0-1V or 0-3.3V)
```

**Requires:**
- Biasing circuit to shift voltage to ADC range
- May need voltage divider if output exceeds ADC range

### Signal Conditioning

The CT output (AC signal) must be conditioned for the ADC:

```
CT Output → Bias (DC offset) → Optional Amplifier → Low-pass Filter → ADC
```

**Basic Circuit:**
```
CT Hot ----+---- 10kΩ ----+---- ADC Input
           |              |
          33Ω          10µF capacitor
           |              |
CT Gnd ----+              +---- GND

       VCC/2 (1.65V)
```

This circuit:
1. Converts current to voltage (burden resistor)
2. Biases signal to VCC/2 for bipolar AC measurement
3. Filters high-frequency noise (RC filter)

---

## Hardware Setup

### Connecting SCT-013 to ESPMegaPRO

#### Materials Needed
- SCT-013 current transformer
- 3.5mm stereo jack
- 33Ω burden resistor (1/4W)
- 10kΩ resistor (2x)
- 10µF capacitor
- Breadboard and wires

#### Wiring Diagram

```
SCT-013 (3.5mm jack)
  Tip    = CT Signal
  Ring   = Not connected
  Sleeve = CT Ground

Circuit:
                          +3.3V
                            |
                          10kΩ
                            |
CT Tip -----+---[ 33Ω ]----+---- Analog Input (A0-A7)
            |              |
           GND           10µF
            |              |
CT Sleeve --+              +---- GND
```

#### Step-by-Step

1. **Create voltage divider for bias:**
   - Connect 3.3V to first 10kΩ resistor
   - Connect other end to second 10kΩ resistor
   - Connect second resistor to GND
   - Junction point = 1.65V bias

2. **Connect CT:**
   - CT tip (hot) to burden resistor (33Ω)
   - Other end of burden to bias point
   - CT sleeve to GND

3. **Add filter capacitor:**
   - 10µF capacitor from bias point to GND

4. **Connect to Analog Card:**
   - Bias point to one of A0-A7 pins
   - GND to GND

### Installing the CT on Power Line

**IMPORTANT SAFETY NOTES:**
- Only clip around ONE conductor (hot OR neutral, not both)
- Never open CT while energized
- Ensure CT is fully closed with audible click
- Observe CT's maximum current rating
- For mains voltage: Hire a qualified electrician if unsure

**Installation:**
1. Turn off power (if possible)
2. Open CT clamp
3. Clip around hot (black) or live conductor only
4. Ensure CT arrow points toward load
5. Close CT securely
6. Turn power back on

---

## Class Reference

### CurrentTransformerCard

```cpp
class CurrentTransformerCard : public ExpansionCard
```

#### Constructor

```cpp
CurrentTransformerCard(
    AnalogCard* analogCard,          // Pointer to AnalogCard instance
    uint8_t pin,                     // Analog pin (0-7)
    float *voltage,                  // Pointer to AC voltage reference
    std::function<float(uint16_t)> adcToCurrent,  // Conversion function
    uint32_t conversionInterval      // Sampling interval in milliseconds
);
```

**Parameters:**
- `analogCard`: Pointer to initialized AnalogCard instance
- `pin`: Analog input pin number (0-7) where CT is connected
- `voltage`: Pointer to float variable containing AC line voltage (e.g., 230.0 or 120.0)
- `adcToCurrent`: Lambda or function that converts ADC value to current in Amps
- `conversionInterval`: How often to sample current in milliseconds (e.g., 1000 for 1 second)

**Example:**
```cpp
AnalogCard analogCard;
float lineVoltage = 230.0;  // 230V AC
CurrentTransformerCard ct(
    &analogCard,
    0,                      // Use A0
    &lineVoltage,
    [](uint16_t adc) { return (adc - 2048) * 0.0146; },
    1000                    // Sample every second
);
```

---

### Methods

#### begin()

```cpp
bool begin();
```

Initializes the CurrentTransformerCard and starts monitoring.

**Returns:**
- `true` if initialization successful
- `false` if AnalogCard is nullptr

**Example:**
```cpp
if (!ct.begin()) {
    Serial.println("CT initialization failed!");
}
```

---

#### loop()

```cpp
void loop();
```

Must be called regularly (in main loop) to perform periodic current measurements and energy accumulation.

**Example:**
```cpp
void loop() {
    ct.loop();  // Call every loop iteration
}
```

---

#### getCurrent()

```cpp
float getCurrent();
```

Returns the most recent current measurement in Amperes.

**Returns:** Current in Amps (float)

**Example:**
```cpp
float current = ct.getCurrent();
Serial.printf("Current: %.2f A\n", current);
```

---

#### getPower()

```cpp
float getPower();
```

Returns the calculated power consumption in Watts.

**Formula:** `Power = Current × Voltage`

**Returns:** Power in Watts (float)

**Example:**
```cpp
float power = ct.getPower();
Serial.printf("Power: %.2f W\n", power);
```

---

#### getEnergy()

```cpp
double getEnergy();
```

Returns the accumulated energy consumption in Watt-hours (Wh).

**Returns:** Energy in Watt-hours (double)

**Example:**
```cpp
double energy = ct.getEnergy();
Serial.printf("Energy: %.3f Wh (%.3f kWh)\n", energy, energy/1000.0);
```

---

#### setEnergy()

```cpp
void setEnergy(float energy);
```

Manually sets the accumulated energy value. Useful for:
- Initializing from previous stored value
- Resetting to a specific value
- Synchronizing with external meter

**Parameters:**
- `energy`: New energy value in Watt-hours

**Side Effects:**
- Triggers all registered callbacks
- Saves to FRAM if auto-save is enabled

**Example:**
```cpp
ct.setEnergy(1234.5);  // Set energy to 1234.5 Wh
```

---

#### resetEnergy()

```cpp
void resetEnergy();
```

Resets accumulated energy to zero. Calls `setEnergy(0)`.

**Example:**
```cpp
ct.resetEnergy();  // Reset energy counter
Serial.println("Energy counter reset");
```

---

#### getVoltage()

```cpp
float getVoltage();
```

Returns the current voltage reference value.

**Returns:** Voltage in Volts (float)

**Example:**
```cpp
float voltage = ct.getVoltage();
Serial.printf("Line Voltage: %.1f V\n", voltage);
```

---

#### bindFRAM()

```cpp
void bindFRAM(FRAM *fram, uint32_t framAddress);
```

Binds a FRAM instance for non-volatile energy storage.

**Parameters:**
- `fram`: Pointer to initialized FRAM instance
- `framAddress`: Starting address in FRAM (uses 8 bytes for double)

**Note:** Each CurrentTransformerCard needs 8 bytes of FRAM

**Example:**
```cpp
extern FRAM ESPMega_FRAM;
ct.bindFRAM(&ESPMega_FRAM, 1000);  // Use FRAM address 1000-1007
```

---

#### saveEnergy()

```cpp
void saveEnergy();
```

Manually saves current energy value to FRAM. Requires `bindFRAM()` to be called first.

**Example:**
```cpp
ct.saveEnergy();  // Persist energy to FRAM
Serial.println("Energy saved to FRAM");
```

---

#### loadEnergy()

```cpp
void loadEnergy();
```

Loads energy value from FRAM. Requires `bindFRAM()` to be called first.

**Safety:**
- Validates loaded value (must be >= 0 and not NaN)
- Sets to 0 if invalid data detected

**Example:**
```cpp
ct.loadEnergy();  // Restore energy from FRAM
Serial.printf("Loaded energy: %.2f Wh\n", ct.getEnergy());
```

---

#### setEnergyAutoSave()

```cpp
void setEnergyAutoSave(bool autoSave);
```

Enables or disables automatic saving to FRAM on every energy update.

**Parameters:**
- `autoSave`: `true` to enable auto-save, `false` to disable

**Trade-offs:**
- **Enabled**: Energy data always current, but more FRAM writes (wear)
- **Disabled**: Fewer FRAM writes, but manual saves needed

**Example:**
```cpp
ct.setEnergyAutoSave(true);   // Auto-save every update
ct.setEnergyAutoSave(false);  // Manual save only
```

---

#### registerCallback()

```cpp
uint8_t registerCallback(std::function<void(float, double)> callback);
```

Registers a callback function that's called whenever energy is updated.

**Parameters:**
- `callback`: Function with signature `void callback(float current, double energy)`

**Returns:** Handler ID for later unregistering

**Callback Parameters:**
- `current`: Current measurement in Amps
- `energy`: Accumulated energy in Watt-hours

**Example:**
```cpp
uint8_t handler = ct.registerCallback([](float current, double energy) {
    Serial.printf("Update: %.2f A, %.2f Wh\n", current, energy);
});
```

---

#### unregisterCallback()

```cpp
void unregisterCallback(uint8_t handler);
```

Removes a previously registered callback.

**Parameters:**
- `handler`: Handler ID returned by `registerCallback()`

**Example:**
```cpp
ct.unregisterCallback(handler);  // Remove callback
```

---

#### getType()

```cpp
uint8_t getType();
```

Returns the card type identifier.

**Returns:** `CARD_TYPE_CT` (4)

**Example:**
```cpp
if (ct.getType() == CARD_TYPE_CT) {
    Serial.println("Current Transformer Card detected");
}
```

---

## ADC to Current Conversion

### Understanding the Conversion Function

The ADC to current conversion function is the most critical part of CT setup. It converts the raw ADC reading to actual current in Amperes.

### Generic Conversion Formula

```cpp
Current (A) = (ADC_Value - ADC_Zero) × Scale_Factor
```

Where:
- **ADC_Value**: Raw reading from ADC (0-4095 for 12-bit)
- **ADC_Zero**: ADC value at zero current (typically 2048 for 3.3V/2 bias)
- **Scale_Factor**: Conversion factor from ADC units to Amperes

### Calculating Scale Factor

#### Method 1: From CT Specifications

Given:
- CT ratio (e.g., 100A:50mA = 2000:1)
- Burden resistor (e.g., 33Ω)
- ADC reference voltage (3.3V)
- ADC resolution (12-bit = 4096 steps)

**Calculate:**
```
Max Secondary Current = 50mA = 0.05A
Burden Voltage = 0.05A × 33Ω = 1.65V peak
ADC steps from zero = 1.65V × (4096/3.3V) = 2048 steps
Scale Factor = 100A / 2048 steps = 0.0488 A/step
```

**Conversion Function:**
```cpp
auto adcToCurrent = [](uint16_t adc) {
    return (adc - 2048) * 0.0488;
};
```

#### Method 2: Calibration with Known Load

More accurate method using real measurement:

**Steps:**
1. Connect a known load (e.g., 1000W heater)
2. Calculate expected current: `I = P/V = 1000W/230V = 4.35A`
3. Read ADC value with CT installed
4. Calculate scale factor:

```cpp
// Known: 1000W load on 230V = 4.35A
// Measured: ADC reads 2937
int adcReading = 2937;
float knownCurrent = 4.35;
float scaleFactor = knownCurrent / (adcReading - 2048);
// scaleFactor = 4.35 / 889 = 0.00489
```

**Conversion Function:**
```cpp
auto adcToCurrent = [](uint16_t adc) {
    return (adc - 2048) * 0.00489;
};
```

### Creating Conversion Functions for Different CTs

#### Example 1: SCT-013-000 (100A:50mA) with 33Ω Burden

```cpp
// CT Ratio: 100A:50mA (2000:1)
// Burden: 33Ω
// Max Voltage: 50mA × 33Ω = 1.65V
// ADC range: 0-3.3V, biased at 1.65V
auto sct013_100A = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;        // 3.3V/2 in 12-bit ADC
    const float SCALE = 100.0 / 2048.0;   // 100A max over half ADC range
    return (adc - ADC_ZERO) * SCALE;
};
```

#### Example 2: SCT-013-030 (30A:1V output)

```cpp
// CT with internal burden, outputs 0-1V for 0-30A
// With voltage divider to use full 3.3V ADC range:
//   3.3kΩ and 1kΩ divider makes 1V → 3.3V
auto sct013_30A = [](uint16_t adc) {
    const float ADC_ZERO = 2048.0;
    const float SCALE = 30.0 / 2048.0;    // 30A max over half ADC range
    return (adc - ADC_ZERO) * SCALE;
};
```

#### Example 3: Generic Calibrated Function

```cpp
// Use calibration constants from real measurement
auto calibratedCT = [](uint16_t adc) {
    const float ZERO_OFFSET = 2048.0;     // Measured at no load
    const float CALIBRATION = 0.0488;     // From known load test
    return abs((adc - ZERO_OFFSET) * CALIBRATION);
};
```

#### Example 4: With RMS Calculation (Advanced)

For more accurate AC measurement, you can sample multiple times and calculate RMS:

```cpp
class CTConverter {
    AnalogCard* adc;
    uint8_t pin;
    const float SCALE = 0.0488;

public:
    CTConverter(AnalogCard* adc, uint8_t pin) : adc(adc), pin(pin) {}

    float getRMS(int samples = 100) {
        float sum = 0;
        for (int i = 0; i < samples; i++) {
            uint16_t reading = adc->analogRead(pin);
            float current = (reading - 2048.0) * SCALE;
            sum += current * current;
            delayMicroseconds(200);  // ~50Hz AC period sampling
        }
        return sqrt(sum / samples);
    }
};

// Use with CurrentTransformerCard
CTConverter converter(&analogCard, 0);
auto adcToCurrent = [&converter](uint16_t adc) {
    return converter.getRMS();  // Ignores single ADC value, samples instead
};
```

### Validating Your Conversion Function

Test your conversion function:

```cpp
void validateConversion() {
    Serial.println("CT Conversion Validation:");
    Serial.println("ADC Value | Current (A)");
    Serial.println("----------|------------");

    for (uint16_t adc = 0; adc <= 4095; adc += 512) {
        float current = adcToCurrent(adc);
        Serial.printf("%4d      | %6.2f\n", adc, current);
    }
}
```

Expected output should show:
- Zero current near ADC = 2048
- Reasonable current range for your CT
- Linear relationship

---

## Energy Calculation

### How Energy Accumulation Works

The CurrentTransformerCard automatically accumulates energy based on power consumption over time.

**Formula:**
```
Energy (Wh) = Power (W) × Time (hours)
Energy (Wh) = Current (A) × Voltage (V) × Time (hours)
```

**Implementation:**
```cpp
void CurrentTransformerCard::beginConversion() {
    // Read current from CT
    uint16_t adcValue = this->analogCard->analogRead(this->pin);
    this->current = this->adcToCurrent(adcValue);

    // Calculate time since last reading
    uint32_t timeDelta = millis() - this->lastConversionTime;

    // Calculate energy increment
    float power = this->current * (*this->voltage);
    float energyIncrement = power * (timeDelta / 3600000.0);  // Convert ms to hours

    // Accumulate energy
    this->setEnergy(this->energy + energyIncrement);

    this->lastConversionTime = millis();
}
```

### Sampling Interval Considerations

The `conversionInterval` parameter affects accuracy and update frequency:

**Fast Sampling (100-500ms):**
- Pros: Catches quick load changes, more accurate for variable loads
- Cons: More CPU usage, more FRAM writes if auto-save enabled

**Medium Sampling (1000ms - 1 second):**
- Pros: Good balance of accuracy and efficiency
- Cons: May miss very brief loads
- **Recommended for most applications**

**Slow Sampling (5000ms - 5 seconds):**
- Pros: Lower CPU usage, fewer FRAM writes
- Cons: Less accurate for variable loads, delayed updates

**Example:**
```cpp
// For refrigerator (cycling load) - use faster sampling
CurrentTransformerCard ctFridge(&analogCard, 0, &voltage, conversion, 500);

// For always-on server - slower sampling OK
CurrentTransformerCard ctServer(&analogCard, 1, &voltage, conversion, 5000);
```

### Energy Units Conversion

```cpp
double energyWh = ct.getEnergy();           // Watt-hours
double energyKWh = energyWh / 1000.0;       // Kilowatt-hours
double energyMWh = energyWh / 1000000.0;    // Megawatt-hours
double energyJ = energyWh * 3600.0;         // Joules

// Cost calculation (example: $0.12 per kWh)
double costPerKWh = 0.12;
double totalCost = energyKWh * costPerKWh;
```

### Energy Calculation Accuracy

Factors affecting accuracy:

1. **CT Accuracy**: Typically ±1-3%
2. **ADC Resolution**: 12-bit provides ±0.024% quantization
3. **Voltage Stability**: Mains voltage varies ±5-10%
4. **Power Factor**: Assumes unity power factor (resistive loads)
5. **Sampling Rate**: Nyquist theorem requires sampling > 2× signal frequency

**Improving Accuracy:**
- Calibrate with known loads
- Update voltage reference dynamically if available
- Use shorter sampling intervals
- Apply power factor correction for inductive/capacitive loads

---

## FRAM Persistence

### Why Use FRAM for Energy Storage?

FRAM (Ferroelectric RAM) advantages:
- **Non-volatile**: Retains data without power
- **Fast writes**: Nanosecond write times
- **High endurance**: 10 trillion write cycles
- **No wear leveling needed**: Unlike EEPROM/Flash

Perfect for frequently updated energy counters!

### Memory Layout

Each CurrentTransformerCard uses 8 bytes of FRAM:

```
Address Range: [framAddress, framAddress+7]
Data Type: double (8 bytes, IEEE 754 double precision)
Stores: Energy in Watt-hours
```

**Planning FRAM Addresses:**
```cpp
// Example FRAM memory map
// Reserve addresses for multiple CTs
#define CT_KITCHEN_ADDR  1000  // 1000-1007
#define CT_BEDROOM_ADDR  1008  // 1008-1015
#define CT_HVAC_ADDR     1016  // 1016-1023
#define CT_TOTAL_ADDR    1024  // 1024-1031
```

### Basic FRAM Usage

```cpp
extern FRAM ESPMega_FRAM;

// Setup
ct.bindFRAM(&ESPMega_FRAM, CT_KITCHEN_ADDR);

// Load previous energy on startup
ct.loadEnergy();
Serial.printf("Restored energy: %.2f Wh\n", ct.getEnergy());

// Save manually when needed
ct.saveEnergy();

// Or enable auto-save
ct.setEnergyAutoSave(true);
```

### Auto-Save Strategies

#### Strategy 1: Always Auto-Save (Simple, but more wear)

```cpp
void setup() {
    ct.bindFRAM(&ESPMega_FRAM, 1000);
    ct.loadEnergy();
    ct.setEnergyAutoSave(true);  // Save on every update
}
```

**Pros:** Never lose data
**Cons:** Maximum FRAM writes (every conversion interval)

#### Strategy 2: Periodic Manual Save (Balanced)

```cpp
unsigned long lastSave = 0;
const unsigned long SAVE_INTERVAL = 60000;  // Save every minute

void setup() {
    ct.bindFRAM(&ESPMega_FRAM, 1000);
    ct.loadEnergy();
    ct.setEnergyAutoSave(false);
}

void loop() {
    ct.loop();

    if (millis() - lastSave >= SAVE_INTERVAL) {
        ct.saveEnergy();
        lastSave = millis();
    }
}
```

**Pros:** Reduced FRAM writes
**Cons:** Up to 1 minute of data loss on power failure

#### Strategy 3: Threshold-Based Save (Intelligent)

```cpp
double lastSavedEnergy = 0;
const double SAVE_THRESHOLD = 10.0;  // Save every 10 Wh

void setup() {
    ct.bindFRAM(&ESPMega_FRAM, 1000);
    ct.loadEnergy();
    ct.setEnergyAutoSave(false);
    lastSavedEnergy = ct.getEnergy();
}

void loop() {
    ct.loop();

    double currentEnergy = ct.getEnergy();
    if (currentEnergy - lastSavedEnergy >= SAVE_THRESHOLD) {
        ct.saveEnergy();
        lastSavedEnergy = currentEnergy;
        Serial.println("Energy milestone saved");
    }
}
```

**Pros:** Minimal writes, no data loss for significant amounts
**Cons:** Complex logic

#### Strategy 4: Hybrid Approach (Recommended)

Combine periodic and threshold-based:

```cpp
unsigned long lastSave = 0;
double lastSavedEnergy = 0;

void loop() {
    ct.loop();

    double energy = ct.getEnergy();
    unsigned long now = millis();

    bool timeTrigger = (now - lastSave >= 300000);  // 5 minutes
    bool thresholdTrigger = (energy - lastSavedEnergy >= 5.0);  // 5 Wh

    if (timeTrigger || thresholdTrigger) {
        ct.saveEnergy();
        lastSave = now;
        lastSavedEnergy = energy;
    }
}
```

### FRAM Error Handling

```cpp
void setup() {
    // Initialize FRAM
    if (!ESPMega_FRAM.begin()) {
        Serial.println("FRAM initialization failed!");
        // Fallback: operate without persistence
        return;
    }

    ct.bindFRAM(&ESPMega_FRAM, 1000);
    ct.loadEnergy();

    // Validate loaded value
    if (ct.getEnergy() < 0 || isnan(ct.getEnergy())) {
        Serial.println("Invalid FRAM data, resetting energy");
        ct.resetEnergy();
    }
}
```

---

## Callback System

### Understanding Callbacks

Callbacks allow you to execute code automatically whenever energy is updated. Useful for:
- Logging to SD card
- Updating displays
- Triggering alarms
- Publishing to MQTT

### Callback Function Signature

```cpp
void callback(float current, double energy) {
    // current: Latest current measurement in Amps
    // energy: Accumulated energy in Watt-hours
}
```

### Basic Callback Usage

```cpp
// Define callback function
void onEnergyUpdate(float current, double energy) {
    Serial.printf("Energy Update: %.2fA, %.2fWh\n", current, energy);
}

void setup() {
    // Register callback
    uint8_t handler = ct.registerCallback(onEnergyUpdate);
}
```

### Lambda Callbacks

```cpp
void setup() {
    ct.registerCallback([](float current, double energy) {
        Serial.printf("Current: %.2f A, Energy: %.2f Wh\n", current, energy);
    });
}
```

### Multiple Callbacks

You can register multiple callbacks for different purposes:

```cpp
uint8_t logHandler;
uint8_t displayHandler;
uint8_t alarmHandler;

void setup() {
    // Logging callback
    logHandler = ct.registerCallback([](float current, double energy) {
        logToSD(current, energy);
    });

    // Display callback
    displayHandler = ct.registerCallback([](float current, double energy) {
        updateDisplay(current, energy);
    });

    // Alarm callback
    alarmHandler = ct.registerCallback([](float current, double energy) {
        if (current > 20.0) {
            triggerOverloadAlarm();
        }
    });
}
```

### Removing Callbacks

```cpp
void disableLogging() {
    ct.unregisterCallback(logHandler);
    Serial.println("Logging disabled");
}
```

### Advanced Callback Examples

#### Example 1: Moving Average Filter

```cpp
class CurrentFilter {
    static const int SAMPLES = 10;
    float readings[SAMPLES] = {0};
    int index = 0;

public:
    void update(float current, double energy) {
        readings[index] = current;
        index = (index + 1) % SAMPLES;

        float average = 0;
        for (int i = 0; i < SAMPLES; i++) {
            average += readings[i];
        }
        average /= SAMPLES;

        Serial.printf("Filtered Current: %.2f A\n", average);
    }
};

CurrentFilter filter;

void setup() {
    ct.registerCallback([](float current, double energy) {
        filter.update(current, energy);
    });
}
```

#### Example 2: Peak Detection

```cpp
class PeakDetector {
    float peakCurrent = 0;
    double peakEnergy = 0;

public:
    void update(float current, double energy) {
        if (current > peakCurrent) {
            peakCurrent = current;
            peakEnergy = energy;
            Serial.printf("New peak: %.2f A at %.2f Wh\n", peakCurrent, peakEnergy);
        }
    }

    void reset() {
        peakCurrent = 0;
        peakEnergy = 0;
    }
};

PeakDetector peak;

void setup() {
    ct.registerCallback([](float current, double energy) {
        peak.update(current, energy);
    });
}
```

#### Example 3: Threshold Alerts

```cpp
void setup() {
    ct.registerCallback([](float current, double energy) {
        // Overload detection
        if (current > 25.0) {
            Serial.println("WARNING: Current overload!");
        }

        // Energy milestone
        if ((int)energy % 1000 == 0 && energy > 0) {
            Serial.printf("Milestone: %d kWh consumed\n", (int)(energy/1000));
        }

        // Low load detection
        if (current < 0.1) {
            Serial.println("INFO: Load disconnected or idle");
        }
    });
}
```

### Callback Performance Considerations

Callbacks are executed synchronously in `setEnergy()`:
- Keep callbacks fast and non-blocking
- Avoid delays or long computations
- Be careful with nested calls

```cpp
// BAD: Blocking callback
ct.registerCallback([](float current, double energy) {
    delay(1000);  // Blocks entire system!
    sendHTTPRequest();
});

// GOOD: Set flag for later processing
volatile bool needsPublish = false;
ct.registerCallback([](float current, double energy) {
    needsPublish = true;  // Quick flag set
});

void loop() {
    ct.loop();
    if (needsPublish) {
        sendHTTPRequest();  // Process in main loop
        needsPublish = false;
    }
}
```

---

## IoT Integration

### CurrentTransformerIoT Class

The `CurrentTransformerIoT` class provides MQTT integration for remote monitoring and control.

```cpp
class CurrentTransformerIoT : public IoTComponent
```

### MQTT Topics

All topics are relative to the base topic configured in ESPMegaIoT.

**Published Topics (Status):**
- `power` - Current power consumption in Watts
- `current` - Current measurement in Amps
- `energy` - Accumulated energy in Watt-hours

**Subscribed Topics (Commands):**
- `requeststate` - Request immediate status update
- `energy/set` - Set energy value (payload: number)
- `energy/reset` - Reset energy to zero

### Topic Structure

```
{base_topic}/ct/{card_id}/{topic}

Example with base_topic = "home/espmega" and card_id = 0:
  home/espmega/ct/0/power     → "1250.50"
  home/espmega/ct/0/current   → "5.43"
  home/espmega/ct/0/energy    → "12345.67"
```

### Basic IoT Setup

```cpp
#include <ESPMegaIoT.hpp>
#include <CurrentTransformerCard.hpp>
#include <CurrentTransformerIoT.hpp>

ESPMegaIoT iot;
AnalogCard analogCard;
float voltage = 230.0;

CurrentTransformerCard ct(&analogCard, 0, &voltage, adcToCurrent, 1000);
CurrentTransformerIoT ctIot;

void setup() {
    // Initialize analog card
    analogCard.begin();

    // Initialize CT
    ct.begin();
    ct.bindFRAM(&ESPMega_FRAM, 1000);
    ct.loadEnergy();

    // Initialize IoT
    iot.intr_begin(cards);
    iot.registerCard(0);  // Register CT as card 0

    // Initialize CT IoT component
    ctIot.begin(0, &ct, iot.getMqttClient(), iot.getBaseTopic());
    ctIot.subscribe();
}

void loop() {
    ct.loop();
    iot.loop();
}
```

### MQTT Message Formats

#### Published Messages

**Power Update:**
```
Topic: home/espmega/ct/0/power
Payload: "1250.50"
Format: Float with 2 decimal places
Units: Watts
```

**Current Update:**
```
Topic: home/espmega/ct/0/current
Payload: "5.43"
Format: Float with 2 decimal places
Units: Amperes
```

**Energy Update:**
```
Topic: home/espmega/ct/0/energy
Payload: "12345.67"
Format: Float with 2 decimal places
Units: Watt-hours
```

#### Command Messages

**Request State:**
```
Topic: home/espmega/ct/0/requeststate
Payload: (any)
Effect: Publishes current, power, and energy immediately
```

**Set Energy:**
```
Topic: home/espmega/ct/0/energy/set
Payload: "5000.0"
Effect: Sets energy counter to 5000.0 Wh
```

**Reset Energy:**
```
Topic: home/espmega/ct/0/energy/reset
Payload: (any)
Effect: Resets energy counter to 0
```

### Auto-Publishing

The CurrentTransformerIoT automatically publishes updates via callback:

```cpp
void CurrentTransformerIoT::handleCTCallback(float current, double energy) {
    this->publishReport();  // Auto-publish on every update
}
```

This means MQTT messages are sent at the `conversionInterval` rate.

### Home Assistant Integration

#### Configuration YAML

```yaml
# configuration.yaml
mqtt:
  sensor:
    # Current Sensor
    - name: "Kitchen Current"
      state_topic: "home/espmega/ct/0/current"
      unit_of_measurement: "A"
      device_class: current
      state_class: measurement

    # Power Sensor
    - name: "Kitchen Power"
      state_topic: "home/espmega/ct/0/power"
      unit_of_measurement: "W"
      device_class: power
      state_class: measurement

    # Energy Sensor
    - name: "Kitchen Energy"
      state_topic: "home/espmega/ct/0/energy"
      unit_of_measurement: "Wh"
      device_class: energy
      state_class: total_increasing

  # Energy Reset Button
  button:
    - name: "Reset Kitchen Energy"
      command_topic: "home/espmega/ct/0/energy/reset"
```

#### Utility Meter for Daily/Monthly Tracking

```yaml
# configuration.yaml
utility_meter:
  kitchen_energy_daily:
    source: sensor.kitchen_energy
    cycle: daily

  kitchen_energy_monthly:
    source: sensor.kitchen_energy
    cycle: monthly
```

### Node-RED Integration

#### Example Flow: Log to InfluxDB

```json
[
    {
        "id": "mqtt-in",
        "type": "mqtt in",
        "topic": "home/espmega/ct/+/power",
        "broker": "mqtt-broker"
    },
    {
        "id": "parse",
        "type": "function",
        "func": "return {\n    payload: {\n        power: parseFloat(msg.payload),\n        location: msg.topic.split('/')[3]\n    }\n};"
    },
    {
        "id": "influx",
        "type": "influxdb out",
        "database": "home_energy"
    }
]
```

### MQTT Rate Limiting

For high-frequency sampling with slower MQTT publishing:

```cpp
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000;  // Publish every 5 seconds

ct.registerCallback([](float current, double energy) {
    if (millis() - lastPublish >= PUBLISH_INTERVAL) {
        // Manually publish instead of auto-publish
        // (Disable auto-publish in CurrentTransformerIoT)
        lastPublish = millis();
    }
});
```

### Monitoring Multiple CTs via MQTT

```cpp
// Setup multiple CTs
CurrentTransformerCard ct0(&analogCard, 0, &voltage, conv, 1000);
CurrentTransformerCard ct1(&analogCard, 1, &voltage, conv, 1000);
CurrentTransformerCard ct2(&analogCard, 2, &voltage, conv, 1000);

CurrentTransformerIoT ctIot0, ctIot1, ctIot2;

void setup() {
    ct0.begin();
    ct1.begin();
    ct2.begin();

    // Register with IoT system
    ctIot0.begin(0, &ct0, mqtt, baseTopic);
    ctIot1.begin(1, &ct1, mqtt, baseTopic);
    ctIot2.begin(2, &ct2, mqtt, baseTopic);

    ctIot0.subscribe();
    ctIot1.subscribe();
    ctIot2.subscribe();
}

// MQTT topics:
// home/espmega/ct/0/power
// home/espmega/ct/1/power
// home/espmega/ct/2/power
```

---

## Common CT Models

### SCT-013 Series (YHDC)

Popular, affordable, widely available current transformers.

#### SCT-013-000 (100A:50mA)

**Specifications:**
- Input: 0-100A AC
- Output: 0-50mA AC
- Ratio: 2000:1
- Accuracy: ±1%
- Opening: 13mm

**Required Components:**
- Burden resistor: 33Ω (for 1.65V peak output)
- Bias resistors: 2× 10kΩ
- Filter capacitor: 10µF

**Conversion Function:**
```cpp
auto sct013_000 = [](uint16_t adc) {
    // 100A over 2048 ADC steps (half range)
    return (adc - 2048.0) * (100.0 / 2048.0);
};
```

**Wiring:**
```
3.5mm Jack:
  Tip    → 33Ω → 1.65V bias → ADC input
  Sleeve → GND
```

#### SCT-013-030 (30A:1V)

**Specifications:**
- Input: 0-30A AC
- Output: 0-1V AC (internal burden)
- Accuracy: ±1%
- Opening: 13mm

**Required Components:**
- Bias resistors: 2× 10kΩ
- Filter capacitor: 10µF
- Optional: 2.2kΩ + 1kΩ divider for full ADC range

**Conversion Function:**
```cpp
auto sct013_030 = [](uint16_t adc) {
    // 30A over 2048 ADC steps (half range)
    return (adc - 2048.0) * (30.0 / 2048.0);
};
```

#### SCT-013-060 (60A:1V)

Similar to SCT-013-030 but higher range:

**Conversion Function:**
```cpp
auto sct013_060 = [](uint16_t adc) {
    return (adc - 2048.0) * (60.0 / 2048.0);
};
```

### TA12-100 (5A:5mA)

**Specifications:**
- Input: 0-5A AC
- Output: 0-5mA AC
- Ratio: 1000:1
- Opening: 12mm

**Required Components:**
- Burden resistor: 330Ω (for 1.65V peak)
- Bias resistors: 2× 10kΩ
- Filter capacitor: 10µF

**Conversion Function:**
```cpp
auto ta12_100 = [](uint16_t adc) {
    return (adc - 2048.0) * (5.0 / 2048.0);
};
```

### ZMCT103C (5A:5mA)

**Specifications:**
- Input: 0-5A AC
- Output: 0-5mA AC
- Ratio: 1000:1
- PCB mount

**Same as TA12-100:**
```cpp
auto zmct103c = [](uint16_t adc) {
    return (adc - 2048.0) * (5.0 / 2048.0);
};
```

### Selecting the Right CT

| Application | Recommended CT | Reason |
|-------------|---------------|---------|
| Residential whole-house (US 120V) | SCT-013-060 | 60A covers typical 50A service |
| Residential whole-house (EU 230V) | SCT-013-030 | 30A sufficient for most homes |
| Individual appliance | SCT-013-000 | Wide range, adjustable with burden |
| Industrial 3-phase | Multiple SCT-013-000 | One per phase |
| Low-power monitoring (<5A) | TA12-100 | Better resolution at low current |

### CT Installation Tips

1. **Correct Orientation**: Arrow on CT points toward load
2. **Single Conductor**: Only clip around one wire (hot or neutral, not both)
3. **Fully Close**: Ensure CT clicks fully closed
4. **Avoid Magnetic Materials**: Keep away from steel panels
5. **Calibration**: Test with known load before deployment

---

## Complete Examples

### Example 1: Basic Current Monitoring

Monitor current on one circuit with serial output.

```cpp
#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>

AnalogCard analogCard;
float lineVoltage = 230.0;  // Set your AC voltage

// Conversion function for SCT-013-000 (100A) with 33Ω burden
auto adcToCurrent = [](uint16_t adc) {
    return (adc - 2048.0) * 0.0488;  // 100A / 2048 steps
};

CurrentTransformerCard ct(
    &analogCard,
    0,              // Use analog input A0
    &lineVoltage,
    adcToCurrent,
    1000            // Sample every 1 second
);

void setup() {
    Serial.begin(115200);
    Serial.println("Current Transformer Monitor");

    if (!analogCard.begin()) {
        Serial.println("ERROR: Analog Card initialization failed!");
        while(1);
    }

    if (!ct.begin()) {
        Serial.println("ERROR: CT initialization failed!");
        while(1);
    }

    Serial.println("Monitoring started...");
    Serial.println("Current (A) | Power (W)");
    Serial.println("------------|----------");
}

void loop() {
    ct.loop();

    float current = ct.getCurrent();
    float power = ct.getPower();

    Serial.printf("%8.2f    | %8.2f\n", current, power);

    delay(1000);
}
```

### Example 2: Energy Logging with FRAM

Log energy consumption with persistence across power cycles.

```cpp
#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>
#include <FRAM.h>

extern FRAM ESPMega_FRAM;
AnalogCard analogCard;
float lineVoltage = 230.0;

auto adcToCurrent = [](uint16_t adc) {
    return (adc - 2048.0) * 0.0488;
};

CurrentTransformerCard ct(&analogCard, 0, &lineVoltage, adcToCurrent, 1000);

void setup() {
    Serial.begin(115200);
    Serial.println("Energy Logger with FRAM");

    // Initialize FRAM
    if (!ESPMega_FRAM.begin()) {
        Serial.println("ERROR: FRAM initialization failed!");
        while(1);
    }

    analogCard.begin();
    ct.begin();

    // Bind FRAM and load previous energy
    ct.bindFRAM(&ESPMega_FRAM, 1000);
    ct.loadEnergy();

    Serial.printf("Restored energy: %.2f Wh\n", ct.getEnergy());

    // Save every 5 Wh to reduce FRAM wear
    ct.setEnergyAutoSave(false);
}

double lastSavedEnergy = 0;

void loop() {
    ct.loop();

    double energy = ct.getEnergy();

    // Save every 5 Wh
    if (energy - lastSavedEnergy >= 5.0) {
        ct.saveEnergy();
        lastSavedEnergy = energy;
        Serial.printf("Energy saved: %.2f Wh\n", energy);
    }

    // Print status every 10 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 10000) {
        Serial.printf("Current: %.2f A | Power: %.2f W | Energy: %.2f Wh (%.3f kWh)\n",
            ct.getCurrent(),
            ct.getPower(),
            energy,
            energy / 1000.0
        );
        lastPrint = millis();
    }
}
```

### Example 3: Power Monitor with Alerts

Monitor power consumption with overload alerts and callbacks.

```cpp
#include <AnalogCard.hpp>
#include <CurrentTransformerCard.hpp>

AnalogCard analogCard;
float lineVoltage = 230.0;
const float MAX_CURRENT = 20.0;  // Overload threshold
const float MIN_CURRENT = 0.5;   // Idle threshold

auto adcToCurrent = [](uint16_t adc) {
    return (adc - 2048.0) * 0.0488;
};

CurrentTransformerCard ct(&analogCard, 0, &lineVoltage, adcToCurrent, 500);

bool overloadAlarmActive = false;

void onEnergyUpdate(float current, double energy) {
    // Overload detection
    if (current > MAX_CURRENT && !overloadAlarmActive) {
        overloadAlarmActive = true;
        Serial.println("!!! OVERLOAD ALERT !!!");
        Serial.printf("Current: %.2f A exceeds limit of %.2f A\n", current, MAX_CURRENT);
        // Add alarm output here (buzzer, relay, etc.)
    } else if (current <= MAX_CURRENT) {
        overloadAlarmActive = false;
    }

    // Idle detection
    if (current < MIN_CURRENT) {
        Serial.println("INFO: Circuit idle");
    }

    // Energy milestones
    int energyKWh = (int)(energy / 1000.0);
    static int lastMilestone = 0;
    if (energyKWh > lastMilestone) {
        Serial.printf(">>> Energy Milestone: %d kWh consumed\n", energyKWh);
        lastMilestone = energyKWh;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Power Monitor with Alerts");

    analogCard.begin();
    ct.begin();

    // Register callback for monitoring
    ct.registerCallback(onEnergyUpdate);

    Serial.printf("Overload threshold: %.1f A (%.0f W)\n", MAX_CURRENT, MAX_CURRENT * lineVoltage);
    Serial.printf("Idle threshold: %.1f A\n", MIN_CURRENT);
    Serial.println("Monitoring started...");
}

void loop() {
    ct.loop();

    // Main loop can do other things
    // Monitoring happens in callback
    delay(100);
}
```

For more comprehensive examples including MQTT integration and multiple CT monitoring, see the `/home/user/ESPMegaPRO3-library/examples/EnergyMonitoring/` directory.

---

## Troubleshooting

### Problem: Always Reads Zero Current

**Possible Causes:**
1. CT not installed on conductor
2. CT reversed (arrow pointing wrong way)
3. CT clipped around both hot and neutral (cancels out)
4. Burden resistor not connected
5. Wrong conversion function

**Solutions:**
- Verify CT is installed on ONE conductor only
- Check CT arrow points toward load
- Verify burden resistor value and connections
- Test with known load
- Check ADC readings are changing

### Problem: Negative Current Readings

**Possible Causes:**
1. CT installed backwards
2. Wrong zero offset in conversion function

**Solutions:**
- Reverse CT installation
- Add `abs()` to conversion function: `return abs((adc - 2048.0) * scale);`
- Calibrate zero offset

### Problem: Inaccurate Power Readings

**Possible Causes:**
1. Incorrect voltage reference
2. Power factor not unity
3. Poor CT calibration
4. ADC noise

**Solutions:**
- Measure actual line voltage
- Calibrate with known load
- Add filtering/averaging
- Check grounding

### Problem: Energy Not Persisting

**Possible Causes:**
1. FRAM not initialized
2. Auto-save disabled and no manual saves
3. FRAM address conflict

**Solutions:**
- Check `ESPMega_FRAM.begin()` returns true
- Enable auto-save or add manual saves
- Verify unique FRAM addresses for each CT

### Problem: Erratic Current Readings

**Possible Causes:**
1. Poor ground connections
2. EMI/RFI interference
3. ADC noise
4. Loose CT installation

**Solutions:**
- Improve grounding
- Add shielded cable
- Increase filter capacitor (try 100µF)
- Ensure CT fully closed
- Add software filtering (moving average)

---

## Additional Resources

### Datasheets
- [SCT-013 Series](https://www.poweruc.pl/collections/split-core-current-transformers)
- [ADS1115 ADC](https://www.ti.com/lit/ds/symlink/ads1115.pdf)

### Tutorials
- [OpenEnergyMonitor CT Guide](https://learn.openenergymonitor.org/electricity-monitoring/ct-sensors/how-to-build-an-arduino-energy-monitor)
- [AC Current Measurement](https://www.seeedstudio.com/blog/2020/02/15/what-is-alternating-current-ac-and-how-to-measure-it-m/)

### Related Documentation
- [AnalogCard Reference](./AnalogCard.md)
- [FRAM Usage](./FRAM.md)
- [ESPMegaIoT Guide](./ESPMegaIoT.md)

---

## License

This documentation is part of the ESPMegaPRO R3 library.
Copyright (c) 2024 SIWAT INC.
