# IR (Infrared) Library Documentation

The ESPMegaPRO3 library provides powerful infrared transmission and reception capabilities through two classes: `IRBlaster` for sending IR signals and `IRReceiver` for capturing IR codes. These classes leverage the ESP32's RMT (Remote Control) peripheral to provide accurate timing and reliable IR communication.

## Table of Contents

1. [IRBlaster Class](#irblaster-class)
2. [IRReceiver Class](#irreceiver-class)
3. [IR Code Format](#ir-code-format)
4. [Examples](#examples)
5. [Integration with ClimateCard](#integration-with-climatecard)
6. [Common IR Protocols](#common-ir-protocols)

---

## IRBlaster Class

The `IRBlaster` class handles infrared signal transmission using the ESP32's RMT (Remote Control) peripheral. It provides precise timing control and supports the standard 38kHz carrier frequency used by most consumer IR devices.

### How It Works

The `IRBlaster` uses the ESP32's RMT peripheral, which is specifically designed for generating and parsing waveforms. The RMT peripheral has 8 independent channels, allowing up to 7 `IRBlaster` objects simultaneously (or 6 if using `IRReceiver`).

**Key Features:**
- **38kHz Carrier Frequency:** Standard for most consumer IR devices (TVs, ACs, etc.)
- **Microsecond Precision:** Clock divider of 80 provides 1μs tick resolution
- **Hardware-Based:** Uses ESP32's dedicated RMT peripheral for accurate timing
- **Multiple Channels:** Support for up to 8 simultaneous IR transmitters

**RMT Configuration:**
```cpp
Clock divider: 80        // 80MHz APB / 80 = 1MHz = 1μs per tick
Carrier enabled: true
Carrier frequency: 38000 Hz
```

### API Reference

#### Constructors

##### `IRBlaster(uint8_t pin, rmt_channel_t channel)`

Creates an IRBlaster object with explicit RMT channel selection.

**Parameters:**
- `pin` - GPIO pin number for IR LED connection
- `channel` - RMT channel to use (`RMT_CHANNEL_0` through `RMT_CHANNEL_7`)

**Example:**
```cpp
#include <IRBlaster.hpp>

IRBlaster irBlaster(14, RMT_CHANNEL_0);
```

**RMT Channel Usage:**
Each `IRBlaster` instance requires a unique RMT channel. The ESP32 has 8 channels:
- `RMT_CHANNEL_0` through `RMT_CHANNEL_7`

If you use `IRReceiver`, it typically uses one channel, leaving 7 for `IRBlaster` objects.

---

##### `IRBlaster(uint8_t pin)`

Creates an IRBlaster object using the default RMT channel (RMT_CHANNEL_0).

**Parameters:**
- `pin` - GPIO pin number for IR LED connection

**Example:**
```cpp
IRBlaster irBlaster(14);  // Uses RMT_CHANNEL_0 by default
```

**Note:** This constructor is convenient when using only one IR transmitter.

---

#### Methods

##### `void send(const uint16_t* data, size_t size)`

Transmits an IR signal using the provided timing array.

**Parameters:**
- `data` - Pointer to array of timing values in microseconds
- `size` - Number of elements in the timing array

**How It Works:**

The timing array contains alternating HIGH and LOW durations in microseconds:
- Even indices (0, 2, 4...): IR LED ON duration
- Odd indices (1, 3, 5...): IR LED OFF duration

The method converts this timing array into `rmt_item32_t` structures used by the RMT peripheral. Each `rmt_item32_t` contains two transitions:
- `level0`, `duration0`: First transition (typically HIGH)
- `level1`, `duration1`: Second transition (typically LOW)

**Example:**
```cpp
// NEC protocol example: Power button
const uint16_t powerCode[] = {
    9000, 4500,  // Start: 9ms HIGH, 4.5ms LOW
    560, 560,    // Bit 0: 560μs HIGH, 560μs LOW
    560, 1690,   // Bit 1: 560μs HIGH, 1690μs LOW
    560, 560,    // Bit 0
    // ... more bits
};

irBlaster.send(powerCode, sizeof(powerCode) / sizeof(powerCode[0]));
```

**Memory Handling:**

The method dynamically allocates memory for RMT items and automatically frees it after transmission. The original timing array is not modified.

**Odd-Length Arrays:**

If the timing array has an odd number of elements, a trailing zero is automatically added to complete the final `rmt_item32_t` pair.

---

#### Destructor

##### `~IRBlaster()`

Automatically called when the IRBlaster object is destroyed. Uninstalls the RMT driver and frees associated resources.

**Example:**
```cpp
{
    IRBlaster irBlaster(14, RMT_CHANNEL_0);
    // ... use irBlaster
}  // Destructor called here, RMT driver uninstalled
```

---

### Pin Configuration

**IR LED Wiring:**

The IR LED requires a current-limiting resistor and typically a transistor for proper current drive:

```
ESP32 GPIO ---[ 220Ω ]---+
                          |
                         Base (NPN Transistor)
                          |
                      Collector --- IR LED Anode
                          |
                      Emitter --- GND

IR LED Cathode --- 3.3V or 5V (depending on LED)
```

**Recommended Components:**
- IR LED: 940nm wavelength (most common)
- Transistor: 2N2222 or similar NPN transistor
- Resistor: 220Ω for base, appropriate resistor for LED current limiting
- Power: 3.3V or 5V depending on IR LED specifications

**Pin Selection:**

Most ESP32 GPIO pins can be used for IR transmission. Commonly used pins:
- GPIO 14, 15, 16, 17 (avoid strapping pins)
- Avoid GPIO 0, 2, 12, 15 if you need reliable boot

---

## IRReceiver Class

The `IRReceiver` class captures infrared signals using GPIO interrupts. It records the timing of signal transitions to reconstruct the IR code, which can then be stored and retransmitted using `IRBlaster`.

### How It Works

The `IRReceiver` uses interrupt-driven timing to capture IR signals:

1. **Interrupt Attachment:** When `start_long_receive()` is called, an interrupt is attached to the IR receiver pin
2. **Edge Detection:** On each signal transition (rising or falling edge), the interrupt handler records the current timestamp in microseconds
3. **Buffer Storage:** Up to 1000 timestamps are stored in a circular buffer
4. **Timing Conversion:** When `end_long_receive()` is called, absolute timestamps are converted to relative timings (differences between transitions)

**Key Features:**
- **Interrupt-Driven:** No polling required, efficient CPU usage
- **Microsecond Precision:** Uses `micros()` for timestamp accuracy
- **1000-Sample Buffer:** Can capture complex IR codes
- **Static Class:** Only one instance can be used at a time

**Important Limitation:**

`IRReceiver` is implemented as a static class, meaning **only one instance can capture IR signals at a time**. This is due to the shared interrupt handler and buffer.

---

### API Reference

#### Static Methods

##### `static void begin(uint8_t pin)`

Initializes the IR receiver on the specified GPIO pin.

**Parameters:**
- `pin` - GPIO pin number connected to IR receiver module

**Example:**
```cpp
#include <IRReceiver.hpp>

void setup() {
    IRReceiver::begin(15);  // Initialize on GPIO 15
}
```

**What It Does:**
- Stores the pin number for later use
- Resets the internal buffer pointer
- Does NOT attach the interrupt yet (call `start_long_receive()` to start capturing)

---

##### `static void start_long_receive()`

Starts capturing IR signals by attaching an interrupt to the receiver pin.

**Example:**
```cpp
Serial.println("Point remote at receiver and press button...");
IRReceiver::start_long_receive();
delay(2000);  // Wait for user to press button
```

**What It Happens:**
1. Resets buffer pointer to 0
2. Attaches interrupt handler to the receiver pin
3. Triggers on both RISING and FALLING edges (CHANGE mode)
4. Each transition stores a microsecond timestamp

**Buffer Overflow Protection:**

The interrupt handler stops recording if the buffer exceeds 1000 samples to prevent memory corruption.

---

##### `static ir_data_t end_long_receive()`

Stops capturing IR signals and returns the captured timing data.

**Returns:**
- `ir_data_t` structure containing:
  - `unsigned int* data` - Array of timing values in microseconds
  - `size_t size` - Number of elements in the array

**Example:**
```cpp
ir_data_t captured = IRReceiver::end_long_receive();

if (captured.size > 0) {
    Serial.printf("Captured %d timing values\n", captured.size);

    // Print the timing array
    for (size_t i = 0; i < captured.size; i++) {
        Serial.printf("%u, ", captured.data[i]);
    }

    // Free memory when done
    free(captured.data);
} else {
    Serial.println("No data captured!");
}
```

**What It Does:**
1. Detaches the interrupt handler
2. Allocates memory for the timing array
3. Converts absolute timestamps to relative timings
4. Returns the data structure

**Memory Management:**

The returned `data` pointer is dynamically allocated using `calloc()`. **You must call `free(captured.data)` when finished** to prevent memory leaks.

**Timing Conversion:**

The raw buffer contains absolute timestamps. This method converts them to relative timings:
```cpp
// If buffer contains: [1000, 10000, 10560, 12250]
// Result will be:     [9000, 560, 1690]  (differences)
```

**Error Handling:**

If memory allocation fails, returns:
```cpp
{
    .data = nullptr,
    .size = 0
}
```

---

#### Data Structures

##### `ir_data_t`

Structure containing captured IR timing data.

```cpp
struct ir_data_t {
    unsigned int* data;  // Array of timing values (microseconds)
    size_t size;         // Number of elements in array
};
```

**Usage:**
```cpp
ir_data_t irCode = IRReceiver::end_long_receive();

// Check if capture was successful
if (irCode.data != nullptr && irCode.size > 0) {
    // Use the data
    irBlaster.send(irCode.data, irCode.size);

    // Clean up
    free(irCode.data);
}
```

---

### Pin Configuration

**IR Receiver Wiring:**

Most IR receiver modules (TSOP38238, VS1838B, etc.) have three pins:

```
IR Receiver Module
┌─────────────┐
│   [Dome]    │
│             │
│  OUT VCC GND│
└──┬───┬───┬──┘
   │   │   │
   │   │   └─── GND
   │   └─────── 3.3V or 5V
   └─────────── GPIO 15 (or any GPIO)
```

**Connection:**
- **OUT (Data):** Connect to ESP32 GPIO (e.g., GPIO 15)
- **VCC:** Connect to 3.3V or 5V (check module specifications)
- **GND:** Connect to ground

**Recommended IR Receiver Modules:**
- **TSOP38238:** 38kHz, 3.3V/5V compatible, 45m range
- **VS1838B:** 38kHz, 3.3V/5V compatible, 18m range
- **TSOP4838:** 38kHz, 2.5V-5.5V, high sensitivity

**Important Notes:**
- IR receivers are sensitive to light - keep away from bright lights
- Most modules include a built-in demodulator and filter
- The 38kHz carrier is automatically removed by the receiver
- Output is active-LOW (LOW when IR detected)

---

## IR Code Format

IR codes in this library are represented as arrays of timing values in microseconds. Understanding this format is crucial for capturing, storing, and transmitting IR signals.

### Timing Array Structure

The timing array alternates between HIGH and LOW periods:

```cpp
const uint16_t example[] = {
    9000,  // [0] HIGH: IR LED on for 9000μs (9ms)
    4500,  // [1] LOW:  IR LED off for 4500μs (4.5ms)
    560,   // [2] HIGH: IR LED on for 560μs
    560,   // [3] LOW:  IR LED off for 560μs
    560,   // [4] HIGH: IR LED on for 560μs
    1690,  // [5] LOW:  IR LED off for 1690μs
    // ... more timings
};
```

**Pattern:**
- **Even indices (0, 2, 4...):** Duration of HIGH state (IR LED emitting)
- **Odd indices (1, 3, 5...):** Duration of LOW state (IR LED off)

### Why Microseconds?

Infrared protocols use very precise timing:
- **NEC Protocol:** Uses 560μs base unit
- **RC5 Protocol:** Uses 889μs base unit
- **Sony SIRC:** Uses 600μs base unit

Microsecond precision ensures accurate reproduction of these protocols.

### Creating IR Codes Manually

You can create IR codes manually if you know the protocol specification:

```cpp
// NEC protocol for address 0x00, command 0x12
const uint16_t necCode[] = {
    // Leader code
    9000, 4500,

    // Address byte (0x00 = 00000000)
    560, 560,   // 0
    560, 560,   // 0
    560, 560,   // 0
    560, 560,   // 0
    560, 560,   // 0
    560, 560,   // 0
    560, 560,   // 0
    560, 560,   // 0

    // Inverted address (0xFF = 11111111)
    560, 1690,  // 1
    560, 1690,  // 1
    560, 1690,  // 1
    560, 1690,  // 1
    560, 1690,  // 1
    560, 1690,  // 1
    560, 1690,  // 1
    560, 1690,  // 1

    // Command byte (0x12 = 00010010)
    560, 560,   // 0
    560, 1690,  // 1
    560, 560,   // 0
    560, 560,   // 0
    560, 1690,  // 1
    560, 560,   // 0
    560, 560,   // 0
    560, 560,   // 0

    // Inverted command (0xED = 11101101)
    560, 1690,  // 1
    560, 560,   // 0
    560, 1690,  // 1
    560, 1690,  // 1
    560, 560,   // 0
    560, 1690,  // 1
    560, 1690,  // 1
    560, 1690,  // 1

    // Stop bit
    560
};
```

### Data Type Selection

**Why `uint16_t`?**

- Maximum value: 65,535 μs (≈65.5 ms)
- Most IR timings are < 10,000 μs (10 ms)
- Saves memory compared to `uint32_t`
- Compatible with RMT peripheral requirements

**When captured data uses `unsigned int`:**

The `IRReceiver` returns `unsigned int*` for broader compatibility, but you should convert to `uint16_t` for storage:

```cpp
ir_data_t captured = IRReceiver::end_long_receive();

// Convert to uint16_t for storage
uint16_t* storedCode = new uint16_t[captured.size];
for (size_t i = 0; i < captured.size; i++) {
    storedCode[i] = (uint16_t)captured.data[i];
}

// Now free original
free(captured.data);
```

---

## Common IR Protocols

Understanding common IR protocols helps when manually creating codes or troubleshooting.

### NEC Protocol

**Most Common Protocol** - Used by many TVs, DVD players, and AC units.

**Characteristics:**
- **Carrier:** 38 kHz
- **Leader:** 9ms HIGH, 4.5ms LOW
- **Bit 0:** 560μs HIGH, 560μs LOW
- **Bit 1:** 560μs HIGH, 1690μs LOW
- **Data:** 8-bit address + 8-bit inverted address + 8-bit command + 8-bit inverted command
- **Repeat:** 9ms HIGH, 2.25ms LOW, 560μs HIGH (for held buttons)

**Example:**
```cpp
// NEC: Address 0x00, Command 0xFF (typically power)
const uint16_t necPower[] = {
    9000, 4500,                          // Leader
    560, 560, 560, 560, 560, 560, 560, 560,   // Address 0x00
    560, 560, 560, 560, 560, 560, 560, 560,
    560, 1690, 560, 1690, 560, 1690, 560, 1690, // ~Address 0xFF
    560, 1690, 560, 1690, 560, 1690, 560, 1690,
    560, 1690, 560, 1690, 560, 1690, 560, 1690, // Command 0xFF
    560, 1690, 560, 1690, 560, 1690, 560, 1690,
    560, 560, 560, 560, 560, 560, 560, 560,     // ~Command 0x00
    560, 560, 560, 560, 560, 560, 560, 560,
    560                                   // Stop bit
};
```

### RC5 Protocol

**Used by:** Philips and many European devices.

**Characteristics:**
- **Carrier:** 36 kHz
- **Manchester Encoding:** Each bit is 1.778ms total
- **Bit 0:** 889μs LOW, 889μs HIGH
- **Bit 1:** 889μs HIGH, 889μs LOW
- **Data:** 2 start bits + 1 toggle + 5-bit address + 6-bit command

**Note:** RC5 requires different carrier frequency (36 kHz vs 38 kHz). The current `IRBlaster` is configured for 38 kHz.

### Sony SIRC Protocol

**Used by:** Sony devices (TVs, audio equipment).

**Characteristics:**
- **Carrier:** 40 kHz
- **Leader:** 2.4ms HIGH, 600μs LOW
- **Bit 0:** 600μs HIGH, 600μs LOW
- **Bit 1:** 1.2ms HIGH, 600μs LOW
- **Data:** 7-bit command + 5/8/13-bit device code (varies by version)

### Air Conditioner Protocols

AC units often use proprietary protocols with much longer codes:

**Characteristics:**
- **Much Longer:** 100-400 timing values (vs 67 for simple NEC)
- **State-Based:** Entire AC state encoded (mode, temp, fan, swing, etc.)
- **Manufacturer-Specific:** Different for each brand
- **Complex Structure:** May include checksums, parity bits

**Example Structure:**
```cpp
// Typical AC IR code (simplified)
const uint16_t acCoolMode24C[] = {
    // Header
    9000, 4500,

    // Mode bits (cool = 001)
    560, 560, 560, 560, 560, 1690,

    // Temperature bits (24C = 10000)
    560, 1690, 560, 560, 560, 560, 560, 560, 560, 560,

    // Fan speed bits (auto = 00)
    560, 560, 560, 560,

    // More settings (swing, turbo, sleep, etc.)
    // ... 50+ more timing pairs ...

    // Checksum
    // ... checksum bits ...

    // Trailing pulse
    560
};
```

---

## Examples

### Example 1: Simple IR Transmission

Transmit a pre-defined IR code (e.g., TV power button).

```cpp
#include <Arduino.h>
#include <IRBlaster.hpp>

// Define IR code for TV power (example NEC protocol)
const uint16_t tvPowerCode[] = {
    9000, 4500,
    560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560,
    560, 1690, 560, 1690, 560, 560, 560, 1690,
    560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 1690, 560, 1690, 560, 560,
    560, 560, 560, 560, 560, 560, 560, 1690,
    560, 560, 560, 560, 560, 560, 560, 1690,
    560, 1690, 560, 1690, 560, 1690, 560, 560,
    560
};

#define IR_LED_PIN 14
#define BUTTON_PIN 0  // Boot button on most ESP32 boards

IRBlaster irBlaster(IR_LED_PIN, RMT_CHANNEL_0);

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    Serial.println("IR Transmitter Ready");
    Serial.println("Press BOOT button to send TV power code");
}

void loop() {
    // Check if button pressed
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Sending IR code...");

        // Send the IR code
        irBlaster.send(tvPowerCode, sizeof(tvPowerCode) / sizeof(tvPowerCode[0]));

        Serial.println("IR code sent!");

        // Debounce delay
        delay(500);
    }
}
```

---

### Example 2: Capturing IR Codes

Capture IR codes from a remote control for later use.

```cpp
#include <Arduino.h>
#include <IRReceiver.hpp>

#define IR_RX_PIN 15

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== IR Code Capture ===");
    Serial.println("Initializing IR receiver on GPIO 15...");

    IRReceiver::begin(IR_RX_PIN);

    Serial.println("Ready to capture!");
    Serial.println("Commands:");
    Serial.println("  's' - Start capture");
    Serial.println("  'e' - End capture and display code");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();

        if (cmd == 's' || cmd == 'S') {
            Serial.println("\n>>> Starting capture...");
            Serial.println(">>> Point remote at receiver and press button NOW!");

            IRReceiver::start_long_receive();

            Serial.println(">>> Capturing... (press 'e' when done)");
        }
        else if (cmd == 'e' || cmd == 'E') {
            Serial.println("\n>>> Ending capture...");

            ir_data_t captured = IRReceiver::end_long_receive();

            if (captured.size == 0 || captured.data == nullptr) {
                Serial.println(">>> ERROR: No data captured!");
            } else {
                Serial.printf(">>> Captured %d timing values\n\n", captured.size);

                // Print as C array
                Serial.println("const uint16_t irCode[] = {");
                Serial.print("    ");
                for (size_t i = 0; i < captured.size; i++) {
                    Serial.print(captured.data[i]);
                    if (i < captured.size - 1) {
                        Serial.print(", ");
                        if ((i + 1) % 10 == 0) {
                            Serial.println();
                            Serial.print("    ");
                        }
                    }
                }
                Serial.println();
                Serial.println("};");
                Serial.printf("Size: %d elements (%d bytes)\n\n",
                             captured.size,
                             captured.size * sizeof(uint16_t));

                // Clean up
                free(captured.data);
            }

            Serial.println(">>> Press 's' to capture another code");
        }
    }
}
```

---

### Example 3: IR Code Repeater

Capture an IR code and retransmit it when a button is pressed.

```cpp
#include <Arduino.h>
#include <IRBlaster.hpp>
#include <IRReceiver.hpp>

#define IR_RX_PIN 15
#define IR_TX_PIN 14
#define BUTTON_PIN 0

IRBlaster irBlaster(IR_TX_PIN, RMT_CHANNEL_0);

// Storage for captured code
uint16_t* storedCode = nullptr;
size_t storedSize = 0;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    delay(2000);

    Serial.println("\n=== IR Code Repeater ===");

    // Initialize receiver
    IRReceiver::begin(IR_RX_PIN);

    // Capture code on startup
    Serial.println("\nPoint remote at receiver and press button...");
    delay(2000);

    Serial.println("Capturing in 3...");
    delay(1000);
    Serial.println("2...");
    delay(1000);
    Serial.println("1...");
    delay(1000);
    Serial.println("NOW!");

    IRReceiver::start_long_receive();
    delay(2000);  // Wait for button press

    ir_data_t captured = IRReceiver::end_long_receive();

    if (captured.size > 0 && captured.data != nullptr) {
        // Store the code
        storedSize = captured.size;
        storedCode = new uint16_t[storedSize];

        for (size_t i = 0; i < storedSize; i++) {
            storedCode[i] = (uint16_t)captured.data[i];
        }

        free(captured.data);

        Serial.printf("Code captured! (%d timings)\n", storedSize);
        Serial.println("\nPress BOOT button to retransmit");
    } else {
        Serial.println("ERROR: No code captured!");
    }
}

void loop() {
    if (storedCode != nullptr && digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Transmitting...");
        irBlaster.send(storedCode, storedSize);
        Serial.println("Sent!");
        delay(500);  // Debounce
    }
}
```

---

### Example 4: Multiple IR Codes with Selection

Store multiple IR codes and select which one to transmit.

```cpp
#include <Arduino.h>
#include <IRBlaster.hpp>

#define IR_LED_PIN 14

// TV Power (NEC)
const uint16_t tvPower[] = {
    9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 1690, 560, 1690, 560, 560, 560, 560,
    560, 560, 560, 560, 560, 1690, 560, 560, 560, 560,
    560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690,
    560, 560, 560, 1690, 560, 1690, 560, 560, 560
};

// TV Volume Up (NEC)
const uint16_t tvVolumeUp[] = {
    9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 560, 560, 560, 560, 1690, 560, 560, 560, 560,
    560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560,
    560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690,
    560, 560, 560, 560, 560, 1690, 560, 560, 560
};

// TV Volume Down (NEC)
const uint16_t tvVolumeDown[] = {
    9000, 4500, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 560, 560, 1690, 560, 560, 560, 560,
    560, 560, 560, 560, 560, 1690, 560, 560, 560, 1690,
    560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690,
    560, 560, 560, 1690, 560, 560, 560, 560, 560
};

IRBlaster irBlaster(IR_LED_PIN, RMT_CHANNEL_0);

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== IR Multi-Code Transmitter ===");
    Serial.println("Commands:");
    Serial.println("  'p' - TV Power");
    Serial.println("  '+' - Volume Up");
    Serial.println("  '-' - Volume Down");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();

        switch (cmd) {
            case 'p':
            case 'P':
                Serial.println("Sending: TV Power");
                irBlaster.send(tvPower, sizeof(tvPower) / sizeof(tvPower[0]));
                break;

            case '+':
                Serial.println("Sending: Volume Up");
                irBlaster.send(tvVolumeUp, sizeof(tvVolumeUp) / sizeof(tvVolumeUp[0]));
                break;

            case '-':
                Serial.println("Sending: Volume Down");
                irBlaster.send(tvVolumeDown, sizeof(tvVolumeDown) / sizeof(tvVolumeDown[0]));
                break;

            default:
                Serial.println("Unknown command");
                break;
        }
    }
}
```

---

### Example 5: Converting Between Formats

Convert captured IR data (unsigned int) to storage format (uint16_t) and save to EEPROM or SPIFFS.

```cpp
#include <Arduino.h>
#include <IRReceiver.hpp>
#include <EEPROM.h>

#define IR_RX_PIN 15
#define EEPROM_SIZE 512
#define EEPROM_ADDR_SIZE 0      // Address to store size
#define EEPROM_ADDR_DATA 2      // Address to store data

void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    IRReceiver::begin(IR_RX_PIN);

    delay(2000);
    Serial.println("\n=== IR Code Storage ===");
    Serial.println("Commands:");
    Serial.println("  'c' - Capture and save code");
    Serial.println("  'r' - Read saved code");
}

void captureAndSave() {
    Serial.println("\nPoint remote and press button...");
    delay(2000);

    IRReceiver::start_long_receive();
    delay(2000);
    ir_data_t captured = IRReceiver::end_long_receive();

    if (captured.size == 0 || captured.data == nullptr) {
        Serial.println("ERROR: No data captured");
        return;
    }

    Serial.printf("Captured %d timings\n", captured.size);

    // Convert unsigned int to uint16_t for storage
    uint16_t size16 = (uint16_t)captured.size;

    // Save size
    EEPROM.write(EEPROM_ADDR_SIZE, size16 & 0xFF);
    EEPROM.write(EEPROM_ADDR_SIZE + 1, (size16 >> 8) & 0xFF);

    // Save data (convert to uint16_t)
    for (size_t i = 0; i < captured.size && i < 250; i++) {
        uint16_t value = (uint16_t)captured.data[i];
        EEPROM.write(EEPROM_ADDR_DATA + i * 2, value & 0xFF);
        EEPROM.write(EEPROM_ADDR_DATA + i * 2 + 1, (value >> 8) & 0xFF);
    }

    EEPROM.commit();
    free(captured.data);

    Serial.println("Code saved to EEPROM!");
}

void readSaved() {
    // Read size
    uint16_t size = EEPROM.read(EEPROM_ADDR_SIZE) |
                   (EEPROM.read(EEPROM_ADDR_SIZE + 1) << 8);

    if (size == 0 || size == 0xFFFF) {
        Serial.println("No code saved");
        return;
    }

    Serial.printf("Saved code has %d timings\n", size);

    // Read data
    Serial.println("\nconst uint16_t savedCode[] = {");
    Serial.print("    ");
    for (uint16_t i = 0; i < size; i++) {
        uint16_t value = EEPROM.read(EEPROM_ADDR_DATA + i * 2) |
                        (EEPROM.read(EEPROM_ADDR_DATA + i * 2 + 1) << 8);
        Serial.print(value);
        if (i < size - 1) {
            Serial.print(", ");
            if ((i + 1) % 10 == 0) {
                Serial.println();
                Serial.print("    ");
            }
        }
    }
    Serial.println();
    Serial.println("};");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();

        if (cmd == 'c' || cmd == 'C') {
            captureAndSave();
        } else if (cmd == 'r' || cmd == 'R') {
            readSaved();
        }
    }
}
```

---

## Integration with ClimateCard

The `ClimateCard` class demonstrates advanced use of `IRBlaster` for controlling air conditioners. Understanding this integration helps when building your own IR-controlled devices.

### Overview

The `ClimateCard` uses `IRBlaster` to send IR commands to air conditioners based on the desired state (mode, fan speed, temperature). Air conditioners are complex IR devices because:

1. **State-Based:** The IR code encodes the entire AC state, not just one parameter
2. **No Feedback:** AC units don't confirm state changes
3. **Manufacturer-Specific:** Each AC brand uses different IR protocols

### AirConditioner Structure

The `AirConditioner` struct defines the AC's capabilities and provides IR codes:

```cpp
struct AirConditioner {
    uint8_t max_temperature;           // Maximum temperature (e.g., 30)
    uint8_t min_temperature;           // Minimum temperature (e.g., 16)
    uint8_t modes;                     // Number of modes (e.g., 4: off, cool, heat, fan)
    const char** mode_names;           // Mode names: {"Off", "Cool", "Heat", "Fan"}
    uint8_t fan_speeds;                // Number of fan speeds (e.g., 4: auto, low, med, high)
    const char** fan_speed_names;      // Fan names: {"Auto", "Low", "Medium", "High"}

    // Function to retrieve IR code for a specific state
    size_t (*getInfraredCode)(uint8_t mode, uint8_t fan_speed,
                              uint8_t temperature, const uint16_t** code);
};
```

### IR Code Organization

AC IR codes are typically organized in a multi-dimensional array:

```cpp
// Example IR code storage for an AC
// Dimensions: [modes][fan_speeds][temperatures]
const uint16_t acIrCodes[4][4][15][200] = {
    // Mode 0: Off
    {
        // Fan Auto
        {
            {/* Temperature 16C code */},
            {/* Temperature 17C code */},
            // ... up to 30C
        },
        // Fan Low
        {/* ... */},
        // Fan Medium
        {/* ... */},
        // Fan High
        {/* ... */}
    },
    // Mode 1: Cool
    {/* ... */},
    // Mode 2: Heat
    {/* ... */},
    // Mode 3: Fan
    {/* ... */}
};

// Code retrieval function
size_t getAcInfraredCode(uint8_t mode, uint8_t fan_speed,
                         uint8_t temperature, const uint16_t** code) {
    *code = acIrCodes[mode][fan_speed][temperature];
    return 200;  // Return the size of the code array
}
```

### Example: Creating an AirConditioner Definition

```cpp
#include <ClimateCard.hpp>

// Mode names
const char* modeName[] = {"Off", "Cool", "Heat", "Fan"};

// Fan speed names
const char* fanSpeedNames[] = {"Auto", "Low", "Medium", "High"};

// IR code storage (simplified - normally much larger)
const uint16_t irCode_Off_Auto_16[67] = {
    9000, 4500, 560, 560, /* ... */
};
const uint16_t irCode_Cool_Auto_24[67] = {
    9000, 4500, 560, 1690, /* ... */
};
// ... hundreds more codes ...

// Code retrieval function
size_t getMyACCode(uint8_t mode, uint8_t fan, uint8_t temp,
                   const uint16_t** code) {
    // Simplified lookup - use your own logic
    if (mode == 0) {  // Off
        *code = irCode_Off_Auto_16;
        return 67;
    } else if (mode == 1 && temp == 24) {  // Cool, 24C
        *code = irCode_Cool_Auto_24;
        return 67;
    }
    // ... more conditions ...

    *code = nullptr;
    return 0;
}

// Define the AC
AirConditioner myAC = {
    .max_temperature = 30,
    .min_temperature = 16,
    .modes = 4,
    .mode_names = modeName,
    .fan_speeds = 4,
    .fan_speed_names = fanSpeedNames,
    .getInfraredCode = getMyACCode
};

// Create ClimateCard
ClimateCard climateCard(14, myAC, RMT_CHANNEL_0);

void setup() {
    climateCard.begin();

    // Set AC to Cool mode, Auto fan, 24C
    climateCard.setState(1, 0, 24);
}

void loop() {
    climateCard.loop();
}
```

### Capturing AC IR Codes

Use the provided `ir_code_capture` example to systematically capture all AC codes:

**Complete Example Path:**
```
/home/user/ESPMegaPRO3-library/examples/Climate/ir_code_capture/ir_code_capture.cpp
```

**Workflow:**

1. **Set Up Hardware:**
   - Connect IR receiver to GPIO 15
   - Connect IR LED to GPIO 14

2. **Upload the Example:**
   ```bash
   # Using PlatformIO
   pio run -t upload -e <your_environment>
   ```

3. **Capture Systematically:**
   ```
   Commands: start, stop, list, export, test

   For each Mode:
     For each Fan Speed:
       For each Temperature:
         1. Type 'start'
         2. Press AC remote button
         3. Type 'stop'
         4. Name: "Cool_Auto_24"
   ```

4. **Export All Codes:**
   ```
   Type: export

   Output:
   const uint16_t irCode_Cool_Auto_24[67] = {
       9000, 4500, 560, 560, ...
   };
   // ... all other codes
   ```

5. **Organize into Structure:**
   ```cpp
   // Create 4D array from captured codes
   const uint16_t acCodes[modes][fans][temps][max_size];
   ```

### ClimateCard Usage Example

```cpp
#include <ClimateCard.hpp>
#include <FRAM.h>

// Assume myAC is defined as above
extern AirConditioner myAC;

ClimateCard climateCard(14, myAC, AC_SENSOR_TYPE_DHT22, 13, RMT_CHANNEL_0);
FRAM fram(0x50);

void setup() {
    Serial.begin(115200);

    // Initialize FRAM
    fram.begin();

    // Initialize ClimateCard
    climateCard.begin();

    // Bind FRAM for state persistence
    climateCard.bindFRAM(&fram, 0x0000);
    climateCard.setFRAMAutoSave(true);

    // Load previous state
    climateCard.loadStateFromFRAM();

    // Register callback for state changes
    climateCard.registerChangeCallback([](uint8_t mode, uint8_t fan, uint8_t temp) {
        Serial.printf("AC State Changed: Mode=%d, Fan=%d, Temp=%d\n",
                      mode, fan, temp);
    });

    // Register sensor callback
    climateCard.registerSensorCallback([](float temp, float humidity) {
        Serial.printf("Room: %.1fC, %.1f%%\n", temp, humidity);
    });
}

void loop() {
    climateCard.loop();

    // Example: Control via serial
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd.startsWith("temp ")) {
            int temp = cmd.substring(5).toInt();
            climateCard.setTemperature(temp);
            Serial.printf("Set temperature to %dC\n", temp);
        }
        else if (cmd.startsWith("mode ")) {
            String mode = cmd.substring(5);
            climateCard.setModeByName(mode.c_str());
            Serial.printf("Set mode to %s\n", mode.c_str());
        }
        else if (cmd.startsWith("fan ")) {
            String fan = cmd.substring(4);
            climateCard.setFanSpeedByName(fan.c_str());
            Serial.printf("Set fan to %s\n", fan.c_str());
        }
        else if (cmd == "status") {
            Serial.printf("Mode: %s\n", climateCard.getModeName());
            Serial.printf("Fan: %s\n", climateCard.getFanSpeedName());
            Serial.printf("Temp: %dC\n", climateCard.getTemperature());
            Serial.printf("Room: %.1fC, %.1f%%\n",
                         climateCard.getRoomTemperature(),
                         climateCard.getHumidity());
        }
    }
}
```

---

## Advanced Topics

### Multiple IR Transmitters

The ESP32 has 8 RMT channels, allowing multiple simultaneous IR transmitters:

```cpp
#include <IRBlaster.hpp>

// Create multiple transmitters on different channels
IRBlaster tvBlaster(14, RMT_CHANNEL_0);
IRBlaster acBlaster(15, RMT_CHANNEL_1);
IRBlaster lightBlaster(16, RMT_CHANNEL_2);

void setup() {
    // Each can transmit independently
    const uint16_t tvPower[] = {/* ... */};
    const uint16_t acCool[] = {/* ... */};
    const uint16_t lightOn[] = {/* ... */};

    tvBlaster.send(tvPower, sizeof(tvPower) / sizeof(tvPower[0]));
    delay(100);
    acBlaster.send(acCool, sizeof(acCool) / sizeof(acCool[0]));
    delay(100);
    lightBlaster.send(lightOn, sizeof(lightOn) / sizeof(lightOn[0]));
}
```

### Custom Carrier Frequencies

The default carrier frequency is 38 kHz. To change it, modify the IRBlaster constructor:

**Current Implementation (38 kHz):**
```cpp
config.tx_config.carrier_freq_hz = 38000;
```

**For 36 kHz (RC5):**
```cpp
config.tx_config.carrier_freq_hz = 36000;
```

**For 40 kHz (Sony):**
```cpp
config.tx_config.carrier_freq_hz = 40000;
```

**Note:** You'll need to modify `IRBlaster.cpp` to support custom frequencies, or create a derived class.

### Error Handling

```cpp
// Check if IRReceiver captured data
ir_data_t captured = IRReceiver::end_long_receive();

if (captured.data == nullptr || captured.size == 0) {
    Serial.println("Capture failed!");
    // Possible causes:
    // - No IR signal received
    // - Memory allocation failed
    // - Buffer overflow (>1000 samples)
} else {
    // Success - use the data
    Serial.printf("Captured %d timings\n", captured.size);

    // Process data...

    // Always free when done!
    free(captured.data);
}
```

### Debugging IR Codes

Print timing patterns to verify captured codes:

```cpp
void printTimingPattern(const uint16_t* data, size_t size) {
    Serial.println("\nTiming Pattern:");
    Serial.println("Index | Type | Duration (μs) | Description");
    Serial.println("------|------|---------------|-------------");

    for (size_t i = 0; i < size; i++) {
        char type = (i % 2 == 0) ? 'H' : 'L';  // High or Low
        const char* desc = "";

        // Identify common patterns
        if (i == 0 && data[i] > 8000 && data[i] < 10000) {
            desc = "Leader (High)";
        } else if (i == 1 && data[i] > 4000 && data[i] < 5000) {
            desc = "Leader (Low)";
        } else if (data[i] > 500 && data[i] < 700) {
            desc = "Bit timing";
        } else if (data[i] > 1500 && data[i] < 1800) {
            desc = "Bit 1 space";
        }

        Serial.printf("%5d |  %c   | %6d        | %s\n",
                     i, type, data[i], desc);
    }
}
```

---

## Troubleshooting

### IR Transmission Issues

**Problem:** IR LED doesn't seem to transmit
- **Check wiring:** Ensure transistor is connected correctly
- **Test LED:** Use a smartphone camera - IR LED will appear purple/white when transmitting
- **Verify voltage:** IR LED should have appropriate current-limiting resistor
- **Check RMT channel:** Ensure no conflicts with other RMT users

**Problem:** IR code doesn't control device
- **Carrier frequency:** Verify device uses 38 kHz (most common)
- **Code accuracy:** Recapture the code multiple times and compare
- **Distance:** Test at different distances (5cm - 5m)
- **Angle:** Point directly at device's IR receiver

### IR Reception Issues

**Problem:** No data captured
- **Check wiring:** Verify IR receiver connections (VCC, GND, OUT)
- **Pin assignment:** Ensure correct GPIO pin number
- **Remote batteries:** Try fresh batteries in remote
- **Distance:** Keep remote 5-10cm from receiver during capture
- **Light interference:** Avoid bright lights, especially sunlight

**Problem:** Inconsistent captures
- **Timing variation:** Some remotes have slight timing variations - capture multiple times
- **Buffer overflow:** If code is very long (>1000 samples), increase buffer size
- **Interrupt conflicts:** Ensure no other interrupts on the same pin

**Problem:** Captured code has too many/few values
- **Normal variation:** Different protocols have different lengths
- **NEC:** ~67 values
- **AC units:** 100-400 values
- **Multiple presses:** Ensure only one button press during capture

### Memory Issues

**Problem:** Memory leak / crash
- **Always free data:** Call `free(captured.data)` after using `ir_data_t`
- **Check nullptr:** Verify `data != nullptr` before using
- **Buffer size:** Large captures use significant RAM

**Problem:** Out of memory
- **Reduce captures:** Store only necessary codes
- **Use PROGMEM:** Store IR codes in flash memory
```cpp
const uint16_t PROGMEM tvPower[] = {9000, 4500, /* ... */};

// Read from PROGMEM when needed
uint16_t code[67];
memcpy_P(code, tvPower, sizeof(tvPower));
irBlaster.send(code, sizeof(code) / sizeof(code[0]));
```

---

## Best Practices

### Code Organization

```cpp
// Store IR codes in separate header file
// ir_codes.h
#ifndef IR_CODES_H
#define IR_CODES_H

#include <Arduino.h>

namespace TVCodes {
    extern const uint16_t power[];
    extern const size_t power_size;

    extern const uint16_t volumeUp[];
    extern const size_t volumeUp_size;
}

namespace ACCodes {
    size_t getCoolCode(uint8_t temp, const uint16_t** code);
}

#endif
```

### Power Management

IR LEDs can draw significant current. For battery-powered applications:

```cpp
// Use transistor to control IR LED power
#define IR_LED_POWER_PIN 12

void setup() {
    pinMode(IR_LED_POWER_PIN, OUTPUT);
    digitalWrite(IR_LED_POWER_PIN, LOW);  // Off by default
}

void sendIRCode(const uint16_t* code, size_t size) {
    digitalWrite(IR_LED_POWER_PIN, HIGH);  // Power on
    delay(10);  // Stabilize

    irBlaster.send(code, size);

    delay(10);
    digitalWrite(IR_LED_POWER_PIN, LOW);   // Power off
}
```

### Code Validation

Before storing captured codes, validate them:

```cpp
bool validateIRCode(const ir_data_t& captured) {
    // Check minimum size
    if (captured.size < 10) {
        Serial.println("Code too short");
        return false;
    }

    // Check maximum size
    if (captured.size > 500) {
        Serial.println("Code suspiciously long");
        return false;
    }

    // Check for reasonable timing values
    for (size_t i = 0; i < captured.size; i++) {
        if (captured.data[i] > 50000) {  // > 50ms is suspicious
            Serial.printf("Unusual timing at index %d: %u\n",
                         i, captured.data[i]);
            return false;
        }
    }

    return true;
}
```

---

## Reference Summary

### IRBlaster Quick Reference

```cpp
#include <IRBlaster.hpp>

// Create transmitter
IRBlaster ir(pin, channel);      // With channel
IRBlaster ir(pin);                // Default channel

// Send code
ir.send(codeArray, arraySize);
```

### IRReceiver Quick Reference

```cpp
#include <IRReceiver.hpp>

// Initialize
IRReceiver::begin(pin);

// Capture
IRReceiver::start_long_receive();
// ... wait for IR signal ...
ir_data_t data = IRReceiver::end_long_receive();

// Use data
if (data.data != nullptr) {
    // Process data.data[0] through data.data[data.size-1]
    free(data.data);  // Important!
}
```

### File Locations

- **IRBlaster Header:** `/home/user/ESPMegaPRO3-library/IRBlaster.hpp`
- **IRBlaster Implementation:** `/home/user/ESPMegaPRO3-library/IRBlaster.cpp`
- **IRReceiver Header:** `/home/user/ESPMegaPRO3-library/IRReceiver.hpp`
- **IRReceiver Implementation:** `/home/user/ESPMegaPRO3-library/IRReceiver.cpp`
- **IR Capture Example:** `/home/user/ESPMegaPRO3-library/examples/Climate/ir_code_capture/ir_code_capture.cpp`
- **ClimateCard Integration:** `/home/user/ESPMegaPRO3-library/ClimateCard.hpp`

---

## Conclusion

The ESPMegaPRO3 IR library provides a powerful and flexible foundation for infrared communication. Whether you're building a universal remote, controlling air conditioners, or creating custom IR devices, the `IRBlaster` and `IRReceiver` classes offer the precision and reliability needed for successful IR projects.

**Key Takeaways:**
- `IRBlaster` uses ESP32's RMT peripheral for accurate IR transmission
- `IRReceiver` captures IR codes using interrupt-driven timing
- IR codes are stored as arrays of microsecond timing values
- The ClimateCard demonstrates advanced IR control for air conditioners
- Always free memory allocated by `IRReceiver::end_long_receive()`
- Use the ir_code_capture example to systematically capture IR codes

For more information, see the ClimateCard documentation and the provided examples.
