#pragma once

#include <IoTComponent.hpp>
#include <DigitalInputCard.hpp>
#include <FRAM.h>

// MQTT Topics
#define PUBLISH_ENABLE_TOPIC "publish_enable"
#define INPUT_REQUEST_STATE_TOPIC "requeststate"

/**
 * @brief The DigitalInputIoT class is a class for connecting the Digital Input Card to the IoT module.
 * 
 * This function allows you to control the Digital Input Card using MQTT.
 * 
 * @warning You should not instantiate this class directly, instead use ESPMegaIoT's registerCard function.
 */
class DigitalInputIoT : public IoTComponent {
    public:
        bool begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic);
        void handleMqttMessage(char *topic, char *payload);
        void publishDigitalInputs();
        void publishDigitalInput(uint8_t pin);
        void setDigitalInputsPublishEnabled(bool enabled);
        void handleValueChange(uint8_t pin, uint8_t value);
        void publishReport();
        void subscribe();
        uint8_t getType();
    private:
        bool digital_inputs_publish_enabled = false;
        DigitalInputCard *card;
};