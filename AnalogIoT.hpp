#pragma once
#include <IoTComponent.hpp>
#include <AnalogCard.hpp>
#include <map>

// MQTT Topics
#define DAC_SET_STATE_TOPIC "/set/state"
#define DAC_SET_VALUE_TOPIC "/set/value"
#define DAC_STATE_TOPIC "/dac/00/state"
#define DAC_VALUE_TOPIC "/dac/00/value"
#define DAC_PUBLISH_ENABLE_TOPIC "/publish_enable"
#define REQUEST_STATE_TOPIC "requeststate"

/**
 * @brief The AnalogIoT class is a class for connecting the Analog Card to the IoT module.
 * 
 * This function allows you to control the Analog Card using MQTT.
 * 
 * @warning You should not instantiate this class directly, instead use ESPMegaIoT's registerCard function.
 */
class AnalogIoT : public IoTComponent {
    public:
        AnalogIoT();
        ~AnalogIoT();
        bool begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic);
        void handleMqttMessage(char *topic, char *payload);
        void handleDACChange(uint8_t pin, uint16_t value);
        void publishADCs();
        void publishADC(uint8_t pin);
        void publishDACs();
        void publishDAC(uint8_t pin);
        void publishDACState(uint8_t pin);
        void publishDACValue(uint8_t pin);
        void setADCsPublishInterval(uint32_t interval);
        void setADCsPublishEnabled(bool enabled);
        uint8_t registerADCConversionCallback(std::function<void(uint8_t, uint16_t)> callback);
        void unregisterADCConversionCallback(uint8_t handler);
        void setADCConversionInterval(uint8_t pin, uint16_t interval);
        void setADCConversionEnabled(uint8_t pin, bool enabled);
        bool processADCSetConversionIntervalMessage(char *topic, char *payload, uint8_t topic_length);
        bool processADCSetConversionEnabledMessage(char *topic, char *payload, uint8_t topic_length);
        bool processDACSetStateMessage(char *topic, char *payload, uint8_t topic_length);
        bool processDACSetValueMessage(char *topic, char *payload, uint8_t topic_length);
        bool processRequestStateMessage(char *topic, char *payload, uint8_t topic_length);
        void publishReport();
        void subscribe();
        void loop();
        uint8_t getType();
    private:
        // The index of the next callback to be registered
        uint8_t adc_conversion_callback_index = 0;
        // We keep track of the length of the topics so we don't have to calculate it every time
        uint8_t dac_set_state_length;
        uint8_t dac_set_value_length;
        uint8_t dac_state_length;
        uint8_t dac_value_length;
        uint8_t request_state_length;
        uint8_t dac_publish_enable_length;
        uint32_t last_adc_publish = 0;
        AnalogCard *card;
        bool adc_publish_enabled[8];
        uint16_t adc_conversion_interval[8];
        uint32_t last_adc_conversion[8];
        std::map<uint8_t, std::function<void(uint8_t, uint16_t)>> adc_conversion_callbacks;
};