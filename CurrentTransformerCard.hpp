#pragma once
#include <AnalogCard.hpp>
#include <ExpansionCard.hpp>
#include <FRAM.h>
#include <map>

#define CARD_TYPE_CT 4

/**
 * @brief The CurrentTransformer class is a class for reading the current and energy from a current transformer that's connected to the AnalogCard.
 * Also supports storing energy to FRAM.
 */

class CurrentTransformerCard : public ExpansionCard
{
    public:
        CurrentTransformerCard(AnalogCard* analogCard, uint8_t pin, float *voltage, std::function<float(uint16_t)> adcToCurrent, uint32_t conversionInterval);
        void bindFRAM(FRAM *fram, uint32_t framAddress); // Takes 16 bytes of FRAM (long double energy)
        bool begin();
        void loop();
        void beginConversion();
        void setEnergy(float energy);
        void resetEnergy();
        float getCurrent();
        double getEnergy();
        float getPower();
        void saveEnergy();
        void loadEnergy();
        void setEnergyAutoSave(bool autoSave);
        float getVoltage();
        uint8_t registerCallback(std::function<void(float, double)> callback);
        void unregisterCallback(uint8_t handler);
        uint8_t getType();
    private:
        AnalogCard* analogCard;
        uint8_t pin;
        uint32_t framAddress;
        FRAM *fram;
        uint32_t conversionInterval;
        bool autoSave;
        float calibration;
        double energy;
        float current;
        float *voltage;
        uint32_t lastConversionTime;
        std::function<float(uint16_t)> adcToCurrent; // std::function that convert adc value to current in amps
        uint8_t handler_count = 0;
        std::map<uint8_t,std::function<void(float, double)>> callbacks;
        uint32_t lastConversionLoopTime;
};

