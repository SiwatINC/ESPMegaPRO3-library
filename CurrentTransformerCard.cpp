#include <CurrentTransformerCard.hpp>

CurrentTransformerCard::CurrentTransformerCard(AnalogCard* analogCard, uint8_t pin, float *voltage, std::function<float(uint16_t)> adcToCurrent, uint32_t conversionInterval)
{
    this->analogCard = analogCard;
    this->pin = pin;
    this->voltage = voltage;
    this->adcToCurrent = adcToCurrent;
}

void CurrentTransformerCard::bindFRAM(FRAM *fram, uint32_t framAddress)
{
    this->fram = fram;
    this->framAddress = framAddress;
}

bool CurrentTransformerCard::begin()
{
    // Is analogCard a nullptr?
    if (this->analogCard == nullptr) {
        return false;
    }
    this->beginConversion();
    return true;
}

void CurrentTransformerCard::loop()
{
    if (this->lastConversionTime == 0) {
        this->lastConversionTime = millis();
    }
    static uint32_t lastConversionLoopTime = 0;
    if (millis() - lastConversionLoopTime > this->conversionInterval) {
        this->beginConversion();
        lastConversionLoopTime = millis();
    }
}

void CurrentTransformerCard::beginConversion()
{
    uint16_t adcValue = this->analogCard->analogRead(this->pin);
    this->current = this->adcToCurrent(adcValue);
    this->setEnergy(this->energy + this->current * *this->voltage * (millis() - this->lastConversionTime) / 3600000); // in Wh
    this->lastConversionTime = millis();
}

void CurrentTransformerCard::setEnergy(float energy)
{
    this->energy = energy;
    if (this->autoSave) {
        this->saveEnergy();
    }
    for (auto const& callback : this->callbacks) {
        callback.second(this->current, this->energy);
    }
}

void CurrentTransformerCard::resetEnergy()
{
    this->setEnergy(0);
}

float CurrentTransformerCard::getCurrent()
{
    return this->current;
}

double CurrentTransformerCard::getEnergy()
{
    return this->energy;
}

uint8_t CurrentTransformerCard::registerCallback(std::function<void(float, double)> callback) {
    this->callbacks[this->handler_count] = callback;
    return this->handler_count++;
}
void CurrentTransformerCard::unregisterCallback(uint8_t handler) {
    this->callbacks.erase(handler);
}

void CurrentTransformerCard::saveEnergy(){
    this->fram->write(this->framAddress, (uint8_t*)&this->energy, sizeof(this->energy));
}
void CurrentTransformerCard::loadEnergy(){
    this->fram->read(this->framAddress, (uint8_t*)&this->energy, sizeof(this->energy));
}

void CurrentTransformerCard::setEnergyAutoSave(bool autoSave){
    this->autoSave = autoSave;
}

float CurrentTransformerCard::getVoltage(){
    return *this->voltage;
}

float CurrentTransformerCard::getPower(){
    return this->current * *this->voltage;
}

uint8_t CurrentTransformerCard::getType() {
    return CARD_TYPE_CT;
}