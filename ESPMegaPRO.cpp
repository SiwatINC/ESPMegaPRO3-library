#include <ESPMegaPRO.h>

uint8_t inputBufferA;
uint8_t inputBufferB;

PCF8574 inputBankA(INPUT_BANK_A_ADDRESS);
PCF8574 inputBankB(INPUT_BANK_B_ADDRESS);
Adafruit_PWMServoDriver pwmBank = Adafruit_PWMServoDriver(PWM_BANK_ADDRESS);
#ifdef ANALOG_CARD_ENABLE
Adafruit_ADS1115 analogInputBankA;
Adafruit_ADS1115 analogInputBankB;
MCP4725 DAC0(DAC0_ADDRESS);
MCP4725 DAC1(DAC1_ADDRESS);
MCP4725 DAC2(DAC2_ADDRESS);
MCP4725 DAC3(DAC3_ADDRESS);
#endif

void ESPMega_begin()
{
    Wire.begin(14, 33);

    inputBankA.begin();
    inputBankB.begin();
    pwmBank.begin();

    // ESPMegaPRO v3 use the PWMBank to drive Half Bridge
    // Push Pull Output is required.
    pwmBank.setOutputMode(true);

#ifdef USE_INTERRUPT
    pinMode(INPUT_BANK_A_INTERRUPT, INPUT_PULLUP);
    pinMode(INPUT_BANK_B_INTERRUPT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INPUT_BANK_A_INTERRUPT), refreshInputBankA, FALLING);
    attachInterrupt(digitalPinToInterrupt(INPUT_BANK_B_INTERRUPT), refreshInputBankB, FALLING);
#endif

#ifdef ANALOG_CARD_ENABLE
    analogInputBankA.begin(ANALOG_INPUT_BANK_A_ADDRESS);
    analogInputBankB.begin(ANALOG_INPUT_BANK_B_ADDRESS);
    DAC0.begin();
    DAC1.begin();
    DAC2.begin();
    DAC3.begin();
#endif
}

void ESPMega_loop()
{
}

bool ESPMega_digitalRead(int id)
{
    if (id >= 0 && id <= 7)
    {
#ifndef USE_INTERRUPT
        refreshInputBankA(); // Only poll if interrupt is not enabled
#endif

        return ((inputBufferA >> id) & 1); // Extract bit from buffer
    }
    if (id >= 8 && id <= 15)
    {
        id -= 8;

#ifndef USE_INTERRUPT
        refreshInputBankB(); // Only poll if interrupt is not enabled
#endif

        return ((inputBufferB >> id) & 1); // Extract bit from buffer
    }
    return false;
}

void ESPMega_analogWrite(int id, int value)
{
    pwmBank.setPin(id, value);
}

void ESPMega_digitalWrite(int id, bool value)
{
    if (value)
        pwmBank.setPin(id, 4095);
    else
        pwmBank.setPin(id, 0);
}

void IRAM_ATTR refreshInputBankA()
{
    inputBufferA = inputBankA.read8();
}

void IRAM_ATTR refreshInputBankB()
{
    inputBufferB = inputBankB.read8();
}

#ifdef ANALOG_CARD_ENABLE
int16_t ESPMega_analogRead(int id)
{
    if (id >= 0 && id <= 3)
        return analogInputBankA.readADC_SingleEnded(3-id);
    else if (id >= 4 && id <= 7)
        return analogInputBankB.readADC_SingleEnded(7-id);
    return 0;
}
void ESPMega_dacWrite(int id, int value) {
    switch (id)
    {
    case 0:
        DAC0.setValue(value);
        break;
    case 1:
        DAC1.setValue(value);
        break;
    case 2:
        DAC2.setValue(value);
        break;    
    case 3:
        DAC3.setValue(value);
        break;
    default:
        break;
    }
}
#endif