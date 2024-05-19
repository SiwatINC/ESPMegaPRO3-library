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
};