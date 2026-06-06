#pragma once
#include <ExpansionCard.hpp>
#include <DigitalInputCard.hpp>
#include <DigitalOutputCard.hpp>
#include <ClimateCard.hpp>
#include <AnalogCard.hpp>
#include <ESPMegaIoT.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <FRAM.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <time.h>
#include <TimeStructure.hpp>
#include <ESPMegaDisplay.hpp>
#include <InternalDisplay.hpp>
#include <ESPMegaWebServer.hpp>
#include <ESPMegaRecovery.hpp>

// ESPMega Pro R3 Board Address
#define FRAM_ADDRESS 0x56
#define INPUT_BANK_A_ADDRESS 0x21
#define INPUT_BANK_B_ADDRESS 0x22
#define PWM_BANK_ADDRESS 0x5F
#define RTC_ADDRESS 0x68

// I2C bus pins (ESPMegaPRO R3)
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 33
// I2C bus robustness
#define I2C_CLOCK_HZ 100000  // 100kHz for noise immunity / long-cable tolerance
#define I2C_TIMEOUT_MS 50     // bound every transaction so a wedged slave returns instead of hanging
// Loop task watchdog: turns a frozen loop() into an auto-reboot instead of a permanent hang
#define LOOP_WDT_TIMEOUT_S 10

// Constants
#define NTP_TIMEOUT_MS 5000
#define NTP_UPDATE_INTERVAL_MS 60000
#define NTP_INITIAL_SYNC_DELAY_MS 15000

/**
 * @brief The ESPMegaPRO class is the main class for the ESPMegaPRO library.
 * 
 * This class provides functions for managing the ESPMegaPRO board, such as installing expansion cards, managing the internal RTC, and managing the internal FRAM.
 * This class also provides functions for managing the ESPMegaIoT module and the internal display.
 * 
 * This class provide a Object Oriented Programming (OOP) interface for the ESPMegaPRO board.
 * If you are looking for a more simple and a more procedural interface, please use the ESPMegaPRO class in ESPMegaPRO.hpp.
 * But note that the ESPMegaPRO class only interfaces with the built-in Digital Input and Digital Output cards and other onboard components.
 * It does not provide an interface for expansion cards, the ESPMegaIoT module, and the internal display.
 * 
 * @warning Only one ESPMegaPRO object can be created, creating more than one will result in undefined behavior.
 */
class ESPMegaPRO {
    public:
        ESPMegaPRO();
        bool begin();
        void loop();  
        bool installCard(uint8_t slot, ExpansionCard* card);
        bool updateTimeFromNTP();
        void enableIotModule();
        void enableInternalDisplay(HardwareSerial *serial);
        void enableWebServer(uint16_t port);
        void setTimezone(const char* offset);
        rtctime_t getTime();
        void dumpFRAMtoSerial(uint16_t start, uint16_t end);
        void dumpFRAMtoSerialASCII(uint16_t start, uint16_t end);
        void setTime(int hours, int minutes, int seconds, int day, int month, int year);
        ExpansionCard* getCard(uint8_t slot);
        FRAM fram;
        /**
         * @brief The Digital Input Card Built-in to the ESPMegaPRO board.
         * @typedef DigitalInputCard
         * @note This card is installed by default at slot 0 on the ESPMegaPRO R3 board.
         */
        DigitalInputCard inputs = DigitalInputCard(INPUT_BANK_A_ADDRESS, INPUT_BANK_B_ADDRESS);
        /**
         * @brief The Digital Output Card Built-in to the ESPMegaPRO board.
         * @typedef DigitalOutputCard
         * @note This card is installed by default at slot 1 on the ESPMegaPRO R3 board.
         */
        DigitalOutputCard outputs = DigitalOutputCard(PWM_BANK_ADDRESS);
        /**
         * @brief The Display Built-in to the ESPMegaPRO board.
         * @typedef InternalDisplay
         * @note SKU EMG-PRO-R3-XXX-(F)-(12/24)V does not have a built-in display.
         */
        InternalDisplay *display;
        /**
         * @brief This component is used to connect the ESPMegaPRO board to the internet and communicate with it through MQTT.
         * @typedef ESPMegaIoT
         * @note You must call enableIotModule() before using this component.
         */
        ESPMegaIoT *iot;
        /**
         * @brief This component is used to create a web server on the ESPMegaPRO board.
         * @typedef ESPMegaWebServer
         * @note You must call enableWebServer() before using this component.
         */
        ESPMegaWebServer *webServer;
        /**
         * @brief This component is used to enter recovery mode when the ESPMegaPRO board is in a bootloop.
         * @typedef ESPMegaRecovery
         */
        ESPMegaRecovery recovery;
    private:
        bool iotEnabled = false;
        bool internalDisplayEnabled = false;
        bool webServerEnabled = false;
        ExpansionCard* cards[255];
        bool cardInstalled[255];
        uint8_t cardCount = 0;
        /**
         * @brief Recover a wedged I2C bus by clocking out a slave that is holding SDA low.
         *
         * A slave reset mid-transfer can latch SDA low, which no Wire timeout can clear because
         * the master never regains the bus. This bit-bangs up to 9 SCL pulses to clock the stuck
         * slave through its byte until it releases SDA, then issues a STOP. Called once at the
         * start of begin() before Wire.begin(), so a bus that came up wedged from a brownout/reset
         * is cleared without a manual power cycle.
         */
        void recoverI2CBus();
};