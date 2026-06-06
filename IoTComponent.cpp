#include <IoTComponent.hpp>

void IoTComponent::setMqttClient(PubSubClient *mqtt) {
    this->mqtt = mqtt;
}

void IoTComponent::publishRelative(const char *topic, const char *payload) {
    // Not static: shared across components / not reentrant across concurrent callbacks
    char absolute_topic[100];
    snprintf(absolute_topic, sizeof(absolute_topic), "%s/%02d/%s", base_topic, card_id, topic);
    ESP_LOGD("IoTComponent", "Publishing to %s : %s", absolute_topic, payload);
    mqtt->publish(absolute_topic, payload);
    mqtt->loop();
}

void IoTComponent::subscribeRelative(const char *topic) {
    char absolute_topic[100];
    snprintf(absolute_topic, sizeof(absolute_topic), "%s/%02d/%s", base_topic, card_id, topic);
    ESP_LOGD("IoTComponent", "Subscribing to %s", absolute_topic);
    mqtt->subscribe(absolute_topic);
    mqtt->loop();
}

void IoTComponent::loop() {
    // Placeholder, Do nothing
}