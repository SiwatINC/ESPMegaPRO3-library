#pragma once
#include <ExpansionCard.hpp>
#include <AnalogCard.hpp>
#include <AnalogIoT.hpp>
#include <DigitalInputCard.hpp>
#include <DigitalInputIoT.hpp>
#include <DigitalOutputCard.hpp>
#include <DigitalOutputIoT.hpp>
#include <ClimateCard.hpp>
#include <ClimateIoT.hpp>
#include <CurrentTransformerCard.hpp>
#include <CurrentTransformerIoT.hpp>
#include <IoTComponent.hpp>
#include <PubSubClient.h>
#include <ETH.h>
#include <WiFi.h>
#include <FRAM.h>
#include <map>
#include <ArduinoJson.h>
#include <ESPMegaCommon.hpp>

// MQTT Connection Parameters
#define TCP_TIMEOUT_SEC 5
#define MQTT_RECONNECT_INTERVAL 30000

// FRAM Address for ESPMegaPROIoT
// Starts from 34
// Ends at 300 (inclusive)
// Total of 267 bytes
#define IOT_FRAM_ADDRESS 34

/**
 * @brief The network configuration struct
 * @note This struct will be saved to FRAM when calling saveNetworkConfig
 */
struct NetworkConfig
{
    IPAddress ip; ///< The IP address
    IPAddress gateway; ///< The gateway address
    IPAddress subnet; ///< The subnet mask
    IPAddress dns1; ///< The primary DNS server
    IPAddress dns2; ///< The secondary DNS server
    char hostname[32]; ///< The hostname
    bool useStaticIp; ///< Whether to use a static IP, if false, DHCP will be used
    bool useWifi; ///< Whether to use WiFi or Ethernet, if false, Ethernet will be used
    bool wifiUseAuth; ///< Whether to use WiFi authentication, if false, ssid and password will be ignored
    char ssid[32]; ///< The WiFi SSID, even if wifiUseAuth is false, this should be set
    char password[32]; ///< The WiFi password, even if wifiUseAuth is false, this should be set
};

/**
 * @brief The MQTT configuration struct
 * @note This struct will be saved to FRAM when calling saveMqttConfig
 */
struct MqttConfig
{
    char mqtt_server[32]; ///< The MQTT server address
    uint16_t mqtt_port; ///< The MQTT server port
    char mqtt_user[32]; ///< The MQTT username, even if mqtt_useauth is false, this should be set
    char mqtt_password[32]; ///< The MQTT password, even if mqtt_useauth is false, this should be set
    bool mqtt_useauth; ///< Whether to use MQTT authentication, if false, mqtt_user and mqtt_password will be ignored
    char base_topic[32]; ///< The base topic for the MQTT messages
};

/**
 * @brief The ESPMegaIoT class is a class that is used to interface with the ESPMegaPRO IoT module
 * 
 * This class allows you to register IoT components and interface with them through MQTT.
 * This class also manages the network and MQTT connections for you.
 * Supports both WiFi and Ethernet.
 * Also allows you to save and load network and MQTT configurations to and from FRAM.
 * Also provides MQTT helpers for publishing and subscribing to topics.
 */
class ESPMegaIoT
{
public:
    ESPMegaIoT();
    ~ESPMegaIoT();
    void intr_begin(ExpansionCard *cards[]);
    void loop();
    void registerCard(uint8_t card_id);
    void unregisterCard(uint8_t card_id);
    void publishCard(uint8_t card_id);
    // Publish topic appended with base topic
    void publishRelative(const char *topic, const char *payload);
    void publishRelative(const char *topic, const char *payload, unsigned int length);
    // Subscribe topic appended with base topic
    void subscribeRelative(const char *topic);
    void subscribe(const char *topic);
    void unsubscribeFromTopic(const char *topic);
    void connectToWifi(const char *ssid, const char *password);
    void connectToWifi(const char *ssid);
    void disconnectFromWifi();
    bool wifiConnected();
    void ethernetBegin();
    void loadNetworkConfig();
    void saveNetworkConfig();
    NetworkConfig* getNetworkConfig();
    MqttConfig* getMqttConfig();
    void setMqttConfig(MqttConfig mqtt_config);
    void saveMqttConfig();
    void loadMqttConfig();
    void connectNetwork();
    void setNetworkConfig(NetworkConfig network_config);
    void connectToMqtt();
    bool connectToMqtt(char *client_id, char *mqtt_server, uint16_t mqtt_port, char *mqtt_user, char *mqtt_password);
    bool connectToMqtt(char *client_id, char *mqtt_server, uint16_t mqtt_port);
    bool mqttConnected();
    void disconnectFromMqtt();
    void publish(const char *topic, const char *payload);
    void publish(const char *topic, const char *payload, unsigned int length);
    uint16_t registerMqttCallback(std::function<void(char *, char *)> callback);
    void unregisterMqttCallback(uint16_t handler);
    uint16_t registerRelativeMqttCallback(std::function<void(char *, char *)> callback);
    void unregisterRelativeMqttCallback(uint16_t handler);
    uint16_t registerSubscribeCallback(std::function<void(void)> callback);
    void unregisterSubscribeCallback(uint16_t handler);
    void setBaseTopic(char *base_topic);
    void bindEthernetInterface(ETHClass *ethernetIface);
    bool networkConnected();
    void bindFRAM(FRAM *fram);
    void publishSystemSummary();

    IoTComponent* getComponent(uint8_t card_id);
    IPAddress getETHIp();
    IPAddress getWifiIp();
    IPAddress getIp();

    String getETHMac();
    String getWifiMac();
    String getMac();

private:
    FRAM *fram;
    bool useWifi;
    bool WifiUseAuth;
    char ssid[32];
    char password[32];
    WiFiClient tcpClient;
    void sessionKeepAlive();
    bool mqttReconnect();
    void wifiReconnect();
    void mqttSubscribe();
    void mqttCallback(char *topic, byte *payload, unsigned int length);
    uint16_t mqtt_callbacks_handler_index;
    uint16_t mqtt_relative_callbacks_handler_index;
    uint16_t subscribe_callbacks_handler_index;
    std::map<uint16_t, std::function<void(char*, char*)>> mqtt_callbacks;
    std::map<uint16_t, std::function<void(char*, char*)>> mqtt_relative_callbacks;
    std::map<uint16_t, std::function<void(void)>> subscribe_callbacks;
    void publishRelative(uint8_t card_id, char *topic, char *payload);
    bool active;
    PubSubClient mqtt;
    IoTComponent *components[255];
    char payload_buffer[200];
    uint8_t base_topic_length;
    ExpansionCard **cards; // Points to card array in ESPMegaPRO Core
    // MQTT Connection Parameters
    bool mqtt_connected;
    NetworkConfig network_config;
    MqttConfig mqtt_config;
    ETHClass *ethernetIface;
};