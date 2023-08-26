#ifndef ESPMEGA
#define ESPMEGA

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <PCF8574.h>

#define INPUT_BANK_A_ADDRESS 0x21
#define INPUT_BANK_B_ADDRESS 0x22
#define PWM_BANK_ADDRESS 0x5F
#define OUTPUT_BANK_ADDRESS 0x21
#define EEPROM_ADDRESS 0x22

//#define USE_INTERRUPT
#define INPUT_BANK_A_INTERRUPT 36
#define INPUT_BANK_B_INTERRUPT 39

/**
 * Initiate ESPMega PRO Internal Components
 * 
 * This function will initiate the PWM bank, Input banks, and the EEPROM.
*/
void ESPMega_begin();

/**
 * Run ESPMega PRO Internal Routines
 * 
 * This function must be called regularly for the correct
 * operation of the ESPMega!
*/
void ESPMega_loop();

/**
 * Read one of the ESPMega's Digital Input pins (I0-I15)
 * 
 * @param id The number of the pin to be read
 * @return The state of the pin (HIGH/LOW)
*/
bool ESPMega_digitalRead(int id);

/**
 * Write a pulse wave modulated signal to one of the ESPMega's
 * PWM pins (P0-P15)
 * 
 * @param id The number of the pin to write to
 * @param value the "HIGH" duty cycle of the PWM wave (0-4095)
*/
void ESPMega_analogWrite(int id, int value);

/**
 * Write a digital signal to one of the ESPMega's
 * PWM pins (P0-P15)
 * 
 * @param id The number of the pin to write to
 * @param value the new state for the pin (HIGH/LOW)
*/
void ESPMega_digitalWrite(int id, bool value);
void IRAM_ATTR refreshInputBankA();
void IRAM_ATTR refreshInputBankB();

#endif