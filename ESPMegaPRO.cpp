#include <ESPMegaPRO.h>

uint8_t inputBufferA;
uint8_t inputBufferB;

PCF8574 inputBankA(INPUT_BANK_A_ADDRESS);
PCF8574 inputBankB(INPUT_BANK_B_ADDRESS);
Adafruit_PWMServoDriver pwmBank = Adafruit_PWMServoDriver(0x5F);

void ESPMega_begin()
{
    Wire.begin(14, 33);

    inputBankA.begin();
    inputBankB.begin();
    pwmBank.begin();

    #ifdef USE_INTERRUPT
    attachInterrupt(digitalPinToInterrupt(INPUT_BANK_A_INTERRUPT),refreshInputBankA,FALLING);
    attachInterrupt(digitalPinToInterrupt(INPUT_BANK_B_INTERRUPT),refreshInputBankB,FALLING);
    #endif
}

void ESPMega_loop() {
    
}

bool ESPMega_digitalRead(int id)
{
    if (id >= 0 && id <= 7)
    {
        #ifndef USE_INTERRUPT
        refreshInputBankA(); //Only poll if interrupt is not enabled
        #endif

        return ((inputBufferA>>id)&1); //Extract bit from buffer
    }
    if (id >= 8 && id <= 15)
    {
        id-=8;

        #ifndef USE_INTERRUPT
        refreshInputBankB(); //Only poll if interrupt is not enabled
        #endif

        return ((inputBufferB>>id)&1); //Extract bit from buffer
    }
    return false;
}

void ESPMega_analogWrite(int id, int value)
{
    pwmBank.setPin(id, value);
}

void ESPMega_digitalWrite(int id, bool value)
{
    if(value)
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