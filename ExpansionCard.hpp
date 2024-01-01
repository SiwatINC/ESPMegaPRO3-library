#pragma once
#include <Arduino.h>

/**
 * @brief The base class for all expansion cards
 * 
 * In order to create a new expansion card, you should create a new class that inherits from this class.
 * Your class should implement the following functions:
 * - begin() : Initialize the card
 * - loop() : A function that is called in the main loop
 * - getType() : Get the type of the card, The type should be a unique number between 0 and 255
 * 
 * @warning This class is abstract and should not be instantiated directly.
 */
class ExpansionCard {
    public:
        // Instantiate the card with the specified address
        ExpansionCard() {}
        virtual bool begin();
        // Preform a loop to refresh the input buffers
        virtual void loop();
        // Get the card type
        virtual uint8_t getType();
};