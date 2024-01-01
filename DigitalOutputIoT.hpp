#pragma once
#include <IoTComponent.hpp>
#include <ExpansionCard.hpp>
#include <DigitalOutputCard.hpp>

/**
 * @brief The DigitalOutputIoT class is a class interfacing with the Digital Output Card through MQTT.
 * 
 * @warning You should not instantiate this class directly, instead use ESPMegaIO's registerCard function.
 */
class DigitalOutputIoT : public IoTComponent {
    public:
        DigitalOutputIoT();
        ~DigitalOutputIoT();
        bool begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic);
        void handleMqttMessage(char *topic, char *payload);
        void publishDigitalOutputs();
        void publishDigitalOutput(uint8_t pin);
        void publishDigitalOutputState(uint8_t pin);
        void publishDigitalOutputValue(uint8_t pin);
        void setDigitalOutputsPublishEnabled(bool enabled);
        void handleValueChange(uint8_t pin, bool state, uint16_t value);
        void publishReport();
        void subscribe();
        void loop();
        uint8_t getType();
    private:
        bool digital_outputs_publish_enabled = false;
        bool processSetStateMessage(char *topic, char *payload, uint8_t topic_length);
        bool processSetValueMessage(char *topic, char *payload, uint8_t topic_length);
        bool processRequestStateMessage(char *topic, char *payload, uint8_t topic_length);
        DigitalOutputCard *card;
        char *state_report_topic;
        char *value_report_topic;
        uint8_t set_state_length;
        uint8_t set_value_length;
        uint8_t state_length;
        uint8_t value_length;
        uint8_t request_state_length;
        uint8_t publish_enable_length;
};