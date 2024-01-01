#pragma once
#include <ExpansionCard.hpp>
#include <PubSubClient.h>
#include <esp_log.h>

/**
 * @brief The IoTComponent class is a base class that is used to interface with an expansion card through MQTT.
 * 
 * In order to create a new IoTComponent, you should create a new class that inherits from this class.
 * Your class should implement the following functions:
 * - begin() : Initialize the component, record the card id, ExpansionCard object, the PubSubClient object and the base topic
 * - handleMqttMessage() : Handle the MQTT messages for the component
 * - publishReport() : Publish all the reports for the component
 * - getType() : Get the type of the component, This should return the underlying ExpansionCard type
 * - subscribe() : Subscribe to the MQTT topics used by the component
 * - loop() : A function that is called in the main loop
 * 
 * Additionally, the inherited class will have access to these helper functions:
 * - publishRelative() : Publish a message to a topic relative to the base topic and the card id
 * - subscribeRelative() : Subscribe to a topic relative to the base topic and the card id
 * 
 * @warning This class is abstract and should not be instantiated directly.
 */
class IoTComponent {
    public:
        virtual bool begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic);
        virtual void handleMqttMessage(char *topic, char *payload);
        void setMqttClient(PubSubClient *mqtt);
        virtual void publishReport();
        virtual uint8_t getType();
        virtual void subscribe();
        void loop();
    protected:
        char *base_topic;
        void publishRelative(const char *topic, const char *payload);
        void subscribeRelative(const char *topic);
        PubSubClient *mqtt;
        uint8_t card_id;
};
