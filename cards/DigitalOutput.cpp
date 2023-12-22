#include <DigitalOutput.hpp>
DigitalOutputCard::(uint8_t address) {
    this->address = address;

}
// Instantiate the card with the specified position on the dip switch
DigitalOutputCard::(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4) {
    this->address = 0x20;
    if (bit0) this->address += 1;
    if (bit1) this->address += 2;
    if (bit2) this->address += 4;
    if (bit3) this->address += 8;
    if (bit4) this->address += 16;

}
// Initialize the card
DigitalOutputCard::void begin() {
    this->pwm = Adafruit_PWMServoDriver(this->address);
    this->pwm.begin();  
}
// Set the output to the specified state
DigitalOutputCard::void digitalWrite(uint8_t pin, bool value) {
    this->pwm.setPin(pin, value ? 4095 : 0);
}
// Set the output to the specified pwm value
DigitalOutputCard::void analogWrite(uint8_t pin, uint16_t value) {
    // If value is greater than 4095, set it to 4095
    if (value > 4095) value = 4095;
    // Set the pwm value
    this->pwm.setPWM(pin, value);
}