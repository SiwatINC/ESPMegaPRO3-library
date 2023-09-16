#ifndef ESPMEGA
#define ESPMEGA
#define ANALOG_CARD_ENABLE
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <PCF8574.h>
#include <I2C_eeprom.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <time.h>
#ifdef ANALOG_CARD_ENABLE
#include <Adafruit_ADS1X15.h>
#include <MCP4725.h>
#endif

#define INPUT_BANK_A_ADDRESS 0x21
#define INPUT_BANK_B_ADDRESS 0x22
#define PWM_BANK_ADDRESS 0x5F
#define RTC_ADDRESS 0x68
#define ANALOG_INPUT_BANK_A_ADDRESS 0x48
#define ANALOG_INPUT_BANK_B_ADDRESS 0x49
#define DAC0_ADDRESS 0x60
#define DAC1_ADDRESS 0x61
#define DAC2_ADDRESS 0x62
#define DAC3_ADDRESS 0x63
#define EEPROM_ADDRESS 0x5A

//#define USE_INTERRUPT
#define INPUT_BANK_A_INTERRUPT 36
#define INPUT_BANK_B_INTERRUPT 39
extern I2C_eeprom ESPMega_EEPROM;
#define ESPMega_configNTP configTime
struct rtctime_t {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

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

/**
 * Get time from the onboard RTC as a struct
 * 
 * @return Time Element Struct
*/
rtctime_t ESPMega_getTime();
/**
 * Set the onboard RTC's time
 * 
 * @param hours
 * @param minutes
 * @param seconds
 * @param day Day of the month
 * @param month Month in numerical form
 * @param year Years in AD
*/
void ESPMega_setTime(int hours,int minutes, int seconds, int day, int month, int year);

/**
 * Update the onboard RTC's time
 * by using time from the NTP server
 * configured with ESPMega_configNTP();
 * @return true when updated successfully.
*/
bool ESPMega_updateTimeFromNTP();

#ifdef ANALOG_CARD_ENABLE
/**
 * Read one of the ESPMega Analog Card's Analog Input pins (A0-A7)
 * 
 * @param id The number of the pin to be read
 * @return The value of the pin (0-4095)
*/
int16_t ESPMega_analogRead(int id);
/**
 * Write a True Analog Signal to one of the ESPMega Analog Card's
 * Analog Output pins (AO0-AO3)
 * 
 * @param id The number of the pin to write to
 * @param value the analog value of the pin (0-4095)
*/
void ESPMega_dacWrite(int id, int value);
#endif

#endif