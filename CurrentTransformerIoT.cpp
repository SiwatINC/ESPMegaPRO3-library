#include <CurrentTransformerIoT.hpp>

CurrentTransformerIoT::CurrentTransformerIoT() {
    
}

CurrentTransformerIoT::~CurrentTransformerIoT() {
    
}

bool CurrentTransformerIoT::begin(uint8_t card_id, ExpansionCard *card, PubSubClient *mqtt, char *base_topic) {
    ESP_LOGD("CurrentTransformerIoT", "Beginning CurrentTransformerIoT");
    this->card_id = card_id;
    this->currentTransformerCard = (CurrentTransformerCard*) card;
    this->mqtt = mqtt;
    this->base_topic = base_topic;
    auto bindedCTCallback = std::bind(&CurrentTransformerIoT::handleCTCallback, this, std::placeholders::_1, std::placeholders::_2);
    this->currentTransformerCard->registerCallback(bindedCTCallback);
    return true;
}

void CurrentTransformerIoT::handleMqttMessage(char *topic, char *payload) {
    uint8_t payload_length = strlen(payload);
    if(this->processSetEnergyMessage(topic, payload, payload_length)) return;
    if (!strcmp(topic, CT_RESET_ENERGY_TOPIC)) {
        this->currentTransformerCard->resetEnergy();
        return;
    } else if (!strcmp(topic, CT_REQUESTSTATE_TOPIC)) {
        this->publishReport();
        return;
    }
}

void CurrentTransformerIoT::subscribe() {
    this->subscribeRelative(CT_SET_ENERGY_TOPIC);
    this->subscribeRelative(CT_RESET_ENERGY_TOPIC);
    this->subscribeRelative(CT_REQUESTSTATE_TOPIC);
}

void CurrentTransformerIoT::loop() {
    // Not used, still need this to meet polymorphism requirements
}

void CurrentTransformerIoT::publishReport() {
    char outputBuffer[256];
    snprintf(outputBuffer, sizeof(outputBuffer), "%.2f", this->currentTransformerCard->getPower());
    this->publishRelative(CT_POWER_TOPIC, outputBuffer);
    snprintf(outputBuffer, sizeof(outputBuffer), "%.2f", this->currentTransformerCard->getEnergy());
    this->publishRelative(CT_ENERGY_TOPIC, outputBuffer);
    snprintf(outputBuffer, sizeof(outputBuffer), "%.2f", this->currentTransformerCard->getCurrent());
    this->publishRelative(CT_CURRENT_TOPIC, outputBuffer);
}

uint8_t CurrentTransformerIoT::getType() {
    return this->currentTransformerCard->getType();
}

bool CurrentTransformerIoT::processSetEnergyMessage(char* topic, char* payload, uint8_t topic_length) {
    if(strcmp(topic, CT_SET_ENERGY_TOPIC)) return false;
    this->currentTransformerCard->setEnergy(atof(payload));
    return true;
}

void CurrentTransformerIoT::handleCTCallback(float current, double energy) {
    this->publishReport();
}