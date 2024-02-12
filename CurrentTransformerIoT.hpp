#include <CurrentTransformerCard.hpp>
#include <IoTComponent.hpp>
#include <ExpansionCard.hpp>

#define CT_REQUESTSTATE_TOPIC "requeststate"
#define CT_SET_ENERGY_TOPIC "energy/set"
#define CT_RESET_ENERGY_TOPIC "energy/reset"
#define CT_ENERGY_TOPIC "energy"
#define CT_POWER_TOPIC "power"
#define CT_CURRENT_TOPIC "current"

class CurrentTransformerIoT : public IoTComponent
{
public:
    CurrentTransformerIoT();
    ~CurrentTransformerIoT();
    bool begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic);
    void handleMqttMessage(char *topic, char *payload);
    void subscribe();
    void loop();
    void publishReport();
    uint8_t getType();
private:
    CurrentTransformerCard *currentTransformerCard;
    bool processSetEnergyMessage(char* topic, char* payload, uint8_t topic_length);
    void handleCTCallback(float current, double energy);
};