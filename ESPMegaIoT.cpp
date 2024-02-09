#include <ESPMegaIoT.hpp>
#include <ETH.h>

/**
 * @brief Create a new ESPMegaIoT object
 * 
 * @note You shold not create this object directly, Instead, you should use the ESPMegaPRO::iot object
 */
ESPMegaIoT::ESPMegaIoT() : mqtt(tcpClient)
{
    tcpClient.setTimeout(TCP_TIMEOUT_SEC);
    // Initialize the components array
    for (int i = 0; i < 255; i++)
    {
        components[i] = NULL;
    }
    active = false;
    mqtt_connected = false;
}

/**
 * @brief Destroy the ESPMegaIoT object
 */
ESPMegaIoT::~ESPMegaIoT()
{
}

/**
 * @brief The mqtt callback function, This function is called when a message is received on a subscribed topic
 * 
 *  This function is called when a message is received on a subscribed topic
 *  The payload is copied to a buffer and a null terminator is added
 *  The payload is then passed to the respective card's mqtt callback
 * 
 * @param topic The topic of the message
 * @param payload The payload of the message in byte form
 * @param length The length of the payload
 */
void ESPMegaIoT::mqttCallback(char *topic, byte *payload, unsigned int length)
{
    // Create a null terminated string from the payload
    memcpy(payload_buffer, payload, length);
    payload_buffer[length] = '\0';
    // Remove the base topic from the topic
    char *topic_without_base = topic + strlen(this->mqtt_config.base_topic) + 1;
    for (const auto &callback : mqtt_relative_callbacks)
    {
        callback.second(topic_without_base, payload_buffer);
    }
    for (const auto &callback : mqtt_callbacks)
    {
        callback.second(topic, payload_buffer);
    }
    // Call the respective card's mqtt callback
    // Note that after the base topic, there should be the card id
    // /base_topic/card_id/...
    // First, get the card id in integer form
    char *card_id_str = strtok(topic_without_base, "/");
    uint8_t card_id = atoi(card_id_str);
    // Check if the card is registered
    if (components[card_id] == NULL)
    {
        return;
    }
    components[card_id]->handleMqttMessage(topic_without_base + 3, payload_buffer);
}

/**
 * @brief Set the base topic for the IoT
 * 
 * @param base_topic The base topic
 */
void ESPMegaIoT::setBaseTopic(char *base_topic)
{
    strcpy(this->mqtt_config.base_topic, base_topic);
    base_topic_length = strlen(this->mqtt_config.base_topic);
}

/**
 * @brief Begin the ESPMegaIoT object
 * 
 * @param cards The array of ExpansionCard objects
 */
void ESPMegaIoT::intr_begin(ExpansionCard *cards[])
{
    this->cards = cards;
    active = true;
}

/**
 * @brief The main loop for the ESPMegaIoT object
 * 
 * @note Normally you should not call this function, Instead, you should call ESPMegaPRO::loop()
 */
void ESPMegaIoT::loop()
{
    if (!active)
        return;
    // Call each component's loop function
    for (int i = 0; i < 255; i++)
    {
        if (components[i] != NULL)
        {
            components[i]->loop();
        }
    }
    mqtt.loop();
    sessionKeepAlive();
}

/**
 * @brief Register an existing card for use with IoT
 * 
 * This function registers an existing card for use with IoT
 * The card should be installed using ESPMegaPRO::installCard() before calling this function
 * 
 * @param card_id The id of the card
 */
void ESPMegaIoT::registerCard(uint8_t card_id)
{
    // Check if the card is already registered
    if (components[card_id] != NULL)
    {
        return;
    }
    // Get the card type
    uint8_t card_type = cards[card_id]->getType();
    // Create the respective IoT component
    switch (card_type)
    {
    case CARD_TYPE_ANALOG:
        components[card_id] = new AnalogIoT();
        components[card_id]->begin(card_id, cards[card_id], &mqtt, this->mqtt_config.base_topic);
        if (mqtt_connected)
        {
            components[card_id]->subscribe();
            components[card_id]->publishReport();
        }
        break;
    case CARD_TYPE_DIGITAL_INPUT:
        components[card_id] = new DigitalInputIoT();
        components[card_id]->begin(card_id, cards[card_id], &mqtt, this->mqtt_config.base_topic);
        if (mqtt_connected)
        {
            components[card_id]->subscribe();
            components[card_id]->publishReport();
        }
        break;
    case CARD_TYPE_DIGITAL_OUTPUT:
        components[card_id] = new DigitalOutputIoT();
        components[card_id]->begin(card_id, cards[card_id], &mqtt, this->mqtt_config.base_topic);
        if (mqtt_connected)
        {
            components[card_id]->subscribe();
            components[card_id]->publishReport();
        }
        break;
    case CARD_TYPE_CLIMATE:
        components[card_id] = new ClimateIoT();
        components[card_id]->begin(card_id, cards[card_id], &mqtt, this->mqtt_config.base_topic);
        if (mqtt_connected)
        {
            components[card_id]->subscribe();
            components[card_id]->publishReport();
        }
        break;
    default:
        ESP_LOGE("ESPMegaIoT", "Registering card %d failed: Unknown card", card_id);
        return;
    }
}

/**
 * @brief Unregister a card
 * 
 * @param card_id The id of the card
 */
void ESPMegaIoT::unregisterCard(uint8_t card_id)
{
    // Check if the card is registered
    if (components[card_id] == NULL)
    {
        return;
    }
    // Delete the IoT component
    delete components[card_id];
    components[card_id] = NULL;
}

/**
 * @brief Publish all cards's reports
 */
void ESPMegaIoT::publishCard(uint8_t card_id)
{
    // Check if the card is registered
    if (components[card_id] == NULL)
    {
        return;
    }
    // Publish the card
    components[card_id]->publishReport();
}

/**
 * @brief Subscribe to a topic
 * 
 * @param topic The topic to subscribe to
 */
void ESPMegaIoT::subscribe(const char *topic)
{
    mqtt.subscribe(topic);
}

/**
 * @brief Unsubscribe from a topic
 * 
 * @param topic The topic to unsubscribe from
 */
void ESPMegaIoT::unsubscribeFromTopic(const char *topic)
{
    mqtt.unsubscribe(topic);
}

/**
 * @brief Connect to a wifi network
 * 
 * @param ssid The SSID of the wifi network
 * @param password The password of the wifi network
 */
void ESPMegaIoT::connectToWifi(const char *ssid, const char *password)
{
    WiFi.begin(ssid, password);
}

/**
 * @brief Connect to a unsecured wifi network
 * 
 * @param ssid The SSID of the wifi network
 */
void ESPMegaIoT::connectToWifi(const char *ssid)
{
    WiFi.begin(ssid);
}

/**
 * @brief Disconnect from the wifi network
 */
void ESPMegaIoT::disconnectFromWifi()
{
    WiFi.disconnect();
}

/**
 * @brief Check if the wifi is connected
 * 
 * @return True if the wifi is connected, false otherwise
 */
bool ESPMegaIoT::wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Connect to a MQTT broker with authentication
 * 
 * @param client_id The client id to use
 * @param mqtt_server The MQTT server to connect to
 * @param mqtt_port The MQTT port to connect to
 * @param mqtt_user The MQTT username to use
 * @param mqtt_password The MQTT password to use
 * @return True if the connection is successful, false otherwise
 */
bool ESPMegaIoT::connectToMqtt(char *client_id, char *mqtt_server, uint16_t mqtt_port, char *mqtt_user, char *mqtt_password)
{
    mqtt.setServer(mqtt_server, mqtt_port);
    auto boundCallback = std::bind(&ESPMegaIoT::mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    mqtt.setCallback(boundCallback);
    if (mqtt_user == nullptr || mqtt_password == nullptr || strlen(mqtt_user) == 0 || strlen(mqtt_password) == 0)
    {
        mqtt_connected = false;
        ESP_LOGE("ESPMegaIoT", "MQTT Connection failed: Username or password not set but MQTT use_auth is true");
        return false;
    }
    if (mqtt.connect(client_id, mqtt_user, mqtt_password))
    {
        sessionKeepAlive();
        mqttSubscribe();
        // Publish all cards
        for (int i = 0; i < 255; i++)
        {
            if (components[i] != NULL)
            {
                components[i]->publishReport();
            }
        }
        mqtt_connected = true;
        return true;
    }
    mqtt_connected = false;
    return false;
}

/**
 * @brief Connect to a MQTT broker without authentication
 * 
 * @param client_id The client id to use
 * @param mqtt_server The MQTT server to connect to
 * @param mqtt_port The MQTT port to connect to
 * @return True if the connection is successful, false otherwise
 */
bool ESPMegaIoT::connectToMqtt(char *client_id, char *mqtt_server, uint16_t mqtt_port)
{
    ESP_LOGD("ESPMegaIoT", "Setting MQTT server to %s:%d", mqtt_server, mqtt_port);
    mqtt.setServer(mqtt_server, mqtt_port);
    auto boundCallback = std::bind(&ESPMegaIoT::mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    ESP_LOGD("ESPMegaIoT", "Binding MQTT callback");
    mqtt.setCallback(boundCallback);
    if (mqtt.connect(client_id))
    {
        ESP_LOGD("ESPMegaIoT", "MQTT Connected, Calling session keep alive");
        sessionKeepAlive();
        ESP_LOGD("ESPMegaIoT", "Subscribing to topics");
        mqttSubscribe();
        ESP_LOGD("ESPMegaIoT", "Publishing reports");
        // Publish all cards
        for (int i = 0; i < 255; i++)
        {
            if (components[i] != NULL)
            {
                components[i]->publishReport();
            }
        }
        ESP_LOGI("ESPMegaIoT", "MQTT Connected OK.");
        mqtt_connected = true;
        return true;
    }
    ESP_LOGW("ESPMegaIoT", "MQTT Connection failed: %d", mqtt.state());
    mqtt_connected = false;
    return false;
}

/**
 * @brief Disconnect from the MQTT broker
 */
void ESPMegaIoT::disconnectFromMqtt()
{
    mqtt.disconnect();
}

/**
 * @brief Publish a message to a topic
 * 
 * @param topic The topic to publish to
 * @param payload The payload to publish
 */
void ESPMegaIoT::publish(const char *topic, const char *payload)
{
    mqtt.publish(topic, payload);
}

/**
 * @brief Register a callback for MQTT messages
 * 
 * @param callback The callback function
 * @return The handler for the callback
 */
uint8_t ESPMegaIoT::registerMqttCallback(std::function<void(char *, char *)> callback)
{
    mqtt_callbacks[mqtt_callbacks_handler_index] = callback;
    return mqtt_callbacks_handler_index++;
}

/**
 * @brief Unregister a callback
 * 
 * @param handler The handler of the callback
 */
void ESPMegaIoT::unregisterMqttCallback(uint8_t handler)
{
    mqtt_callbacks.erase(handler);
}

/**
 * @brief Subscribe to all components's topics
 */
void ESPMegaIoT::mqttSubscribe()
{
    ESP_LOGD("ESPMegaIoT", "Begin MQTT Subscription");
    for (const auto &callback : subscribe_callbacks)
    {
        callback.second();
        mqtt.loop();
    }
    // Subscribe to all topics
    for (int i = 0; i < 255; i++)
    {
        if (components[i] != NULL)
        {
            ESP_LOGD("ESPMegaIoT","Subscribing component %d", i);
            components[i]->subscribe();
            mqtt.loop();
        }
    }
}

/**
 * @brief Publish relative to the base topic
 */
void ESPMegaIoT::publishRelative(uint8_t card_id, char *topic, char *payload)
{
    char absolute_topic[100];
    sprintf(absolute_topic, "%s/%d/%s", this->mqtt_config.base_topic, card_id, topic);
    mqtt.publish(absolute_topic, payload);
}

/**
 * @brief Subscribe relative to the base topic
 */
bool ESPMegaIoT::mqttReconnect()
{
    if (this->mqtt_config.mqtt_useauth)
    {
        return this->connectToMqtt(this->network_config.hostname, this->mqtt_config.mqtt_server, this->mqtt_config.mqtt_port, this->mqtt_config.mqtt_user, this->mqtt_config.mqtt_password);
    }
    else
    {
        return this->connectToMqtt(this->network_config.hostname, this->mqtt_config.mqtt_server, this->mqtt_config.mqtt_port);
    }
}

/**
 * @brief Keep the MQTT session alive
 * @note This function is called automatically by the ESPMegaIoT object, You should not call this function directly
 */
void ESPMegaIoT::sessionKeepAlive()
{
    // This reconnect the MQTT if it disconnect.
    // If a disconnect happens, this will reconnect the MQTT within 1 second.
    // A connection attempt will be made at most once every MQTT_RECONNECT_INTERVAL
    // This have the effect of reconnecting to the server immediately if the connection is lost
    // and the connection was previously stable for at least MQTT_RECONNECT_INTERVAL
    // But will not reconnect if the connection was unstable and the connection was lost
    static unsigned long lastSessionKeepAlive = 0;
    static unsigned long lastConnectionAttempt = 0;
    if (millis() - lastSessionKeepAlive > 1000)
    {
        lastSessionKeepAlive = millis();
        // Check if mqtt is connected
        if (!mqtt.connected())
        {
            // Try to reconnect if lastConnectionAttempt exceed MQTT_RECONNECT_INTERVAL
            if (millis() - lastConnectionAttempt > MQTT_RECONNECT_INTERVAL)
            {
                lastConnectionAttempt = millis();
                mqtt_connected = mqttReconnect();
            }
        }
    }
}

/**
 * @brief Register a callback for MQTT messages relative to the base topic
 * 
 * The message's base topic will be removed before calling the callback
 * 
 * @param callback The callback function
 * @return The handler for the callback
 */
uint8_t ESPMegaIoT::registerRelativeMqttCallback(std::function<void(char *, char *)> callback)
{
    mqtt_relative_callbacks[mqtt_relative_callbacks_handler_index] = callback;
    return mqtt_relative_callbacks_handler_index++;
}

/**
 * @brief Unregister a relative MQTT callback
 * 
 * @param handler The handler of the callback
 */
void ESPMegaIoT::unregisterRelativeMqttCallback(uint8_t handler)
{
    mqtt_relative_callbacks.erase(handler);
}

/**
 * @brief Publish a message relative to the base topic
 * 
 * @param topic The topic to publish to
 * @param payload The payload to publish
 */
void ESPMegaIoT::publishRelative(const char *topic, const char *payload)
{
    char absolute_topic[100];
    sprintf(absolute_topic, "%s/%s", this->mqtt_config.base_topic, topic);
    mqtt.publish(absolute_topic, payload);
    mqtt.loop();
}

/**
 * @brief Subscribe to a topic relative to the base topic
 * 
 * @param topic The topic to subscribe to
 */
void ESPMegaIoT::subscribeRelative(const char *topic)
{
    char absolute_topic[100];
    sprintf(absolute_topic, "%s/%s", this->mqtt_config.base_topic, topic);
    mqtt.subscribe(absolute_topic);
    mqtt.loop();
}

/**
 * @brief Register a function to be called when the ESPMegaIoT object is subscribing to topics
 * 
 * @param callback The callback function
 * @return The handler for the callback
 */
uint8_t ESPMegaIoT::registerSubscribeCallback(std::function<void(void)> callback)
{
    subscribe_callbacks[subscribe_callbacks_handler_index] = callback;
    return subscribe_callbacks_handler_index++;
}

/**
 * @brief Unregister a subscribe callback
 * 
 * @param handler The handler of the callback
 */
void ESPMegaIoT::unregisterSubscribeCallback(uint8_t handler)
{
    subscribe_callbacks.erase(handler);
}

/**
 * @brief Set the network config
 * 
 * @param network_config The network config struct
 */
void ESPMegaIoT::setNetworkConfig(NetworkConfig network_config)
{
    this->network_config = network_config;
}

/**
 * @brief Load the network config from FRAM
 */
void ESPMegaIoT::loadNetworkConfig()
{
    // Load the network config from FRAM
    network_config.ip = fram->read32(IOT_FRAM_ADDRESS);
    network_config.gateway = fram->read32(IOT_FRAM_ADDRESS + 4);
    network_config.subnet = fram->read32(IOT_FRAM_ADDRESS + 8);
    network_config.dns1 = fram->read32(IOT_FRAM_ADDRESS + 12);
    network_config.dns2 = fram->read32(IOT_FRAM_ADDRESS + 16);
    fram->read(IOT_FRAM_ADDRESS + 20, (uint8_t *)network_config.hostname, 32);
    network_config.useStaticIp = fram->read8(IOT_FRAM_ADDRESS + 52);
    network_config.useWifi = fram->read8(IOT_FRAM_ADDRESS + 53);
    network_config.wifiUseAuth = fram->read8(IOT_FRAM_ADDRESS + 54);
    fram->read(IOT_FRAM_ADDRESS + 55, (uint8_t *)network_config.ssid, 32);
    fram->read(IOT_FRAM_ADDRESS + 87, (uint8_t *)network_config.password, 32);
}

/**
 * @brief Save the network config to FRAM
 */
void ESPMegaIoT::saveNetworkConfig()
{
    // Save the network config to FRAM
    fram->write32(IOT_FRAM_ADDRESS, network_config.ip);
    fram->write32(IOT_FRAM_ADDRESS + 4, network_config.gateway);
    fram->write32(IOT_FRAM_ADDRESS + 8, network_config.subnet);
    fram->write32(IOT_FRAM_ADDRESS + 12, network_config.dns1);
    fram->write32(IOT_FRAM_ADDRESS + 16, network_config.dns2);
    fram->write(IOT_FRAM_ADDRESS + 20, (uint8_t *)network_config.hostname, 32);
    fram->write8(IOT_FRAM_ADDRESS + 52, network_config.useStaticIp);
    fram->write8(IOT_FRAM_ADDRESS + 53, network_config.useWifi);
    fram->write8(IOT_FRAM_ADDRESS + 54, network_config.wifiUseAuth);
    fram->write(IOT_FRAM_ADDRESS + 55, (uint8_t *)network_config.ssid, 32);
    fram->write(IOT_FRAM_ADDRESS + 87, (uint8_t *)network_config.password, 32);
}

/**
 * @brief Begin the ethernet interface
 */
void ESPMegaIoT::ethernetBegin()
{
    ethernetIface->setHostname(network_config.hostname);
}

/**
 * @brief Load the MQTT config from FRAM
 */
void ESPMegaIoT::loadMqttConfig()
{
    // Load the mqtt config from FRAM
    // We skip bytes 119-127 because they are reserved for the network config
    mqtt_config.mqtt_port = fram->read16(IOT_FRAM_ADDRESS + 128);
    fram->read(IOT_FRAM_ADDRESS + 130, (uint8_t *)mqtt_config.mqtt_server, 32);
    fram->read(IOT_FRAM_ADDRESS + 162, (uint8_t *)mqtt_config.mqtt_user, 32);
    fram->read(IOT_FRAM_ADDRESS + 194, (uint8_t *)mqtt_config.mqtt_password, 32);
    mqtt_config.mqtt_useauth = fram->read8(IOT_FRAM_ADDRESS + 226);
    fram->read(IOT_FRAM_ADDRESS + 227, (uint8_t *)mqtt_config.base_topic, 32);
    this->base_topic_length = strlen(mqtt_config.base_topic);
}

/**
 * @brief Save the MQTT config to FRAM
 */
void ESPMegaIoT::saveMqttConfig()
{
    fram->write16(IOT_FRAM_ADDRESS + 128, mqtt_config.mqtt_port);
    fram->write(IOT_FRAM_ADDRESS + 130, (uint8_t *)mqtt_config.mqtt_server, 32);
    fram->write(IOT_FRAM_ADDRESS + 162, (uint8_t *)mqtt_config.mqtt_user, 32);
    fram->write(IOT_FRAM_ADDRESS + 194, (uint8_t *)mqtt_config.mqtt_password, 32);
    fram->write8(IOT_FRAM_ADDRESS + 226, mqtt_config.mqtt_useauth);
    fram->write(IOT_FRAM_ADDRESS + 227, (uint8_t *)mqtt_config.base_topic, 32);
}

/**
 * @brief Connect to MQTT with the current config
 */
void ESPMegaIoT::connectToMqtt()
{
    if (mqtt_config.mqtt_useauth)
    {
        ESP_LOGD("ESPMegaIoT", "Connecting to MQTT with auth");
        this->connectToMqtt(network_config.hostname, mqtt_config.mqtt_server, mqtt_config.mqtt_port, mqtt_config.mqtt_user, mqtt_config.mqtt_password);
    }
    else
    {
        ESP_LOGD("ESPMegaIoT", "Connecting to MQTT without auth");
        this->connectToMqtt(network_config.hostname, mqtt_config.mqtt_server, mqtt_config.mqtt_port);
    }
}

/**
 * @brief Connect to the network using the current config
 */
void ESPMegaIoT::connectNetwork()
{
    if (network_config.useWifi)
    {
        if (network_config.wifiUseAuth)
            this->connectToWifi(network_config.ssid, network_config.password);
        else
            this->connectToWifi(network_config.ssid);
        if (network_config.useStaticIp)
            WiFi.config(network_config.ip, network_config.gateway, network_config.subnet);
        else
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }
    else
    {
        this->ethernetBegin();
        if (network_config.useStaticIp)
            ethernetIface->config(network_config.ip, network_config.gateway, network_config.subnet, network_config.dns1, network_config.dns2);
    }
}

/**
 * @brief Set the MQTT config
 * 
 * @param mqtt_config The MQTT config struct
 */
void ESPMegaIoT::setMqttConfig(MqttConfig mqtt_config)
{
    this->mqtt_config = mqtt_config;
    this->base_topic_length = strlen(mqtt_config.base_topic);
}

/**
 * @brief Bind an ethernet interface to the ESPMegaIoT object
 * 
 * @param ethernetIface The ethernet interface to bind (ETH for ESPMegaPRO R3)
 */
void ESPMegaIoT::bindEthernetInterface(ETHClass *ethernetIface)
{
    this->ethernetIface = ethernetIface;
}

/**
 * @brief Get the IoTComponent object for a card
 * 
 * @param card_id The id of the card
 * @return The IoTComponent object for the card
 */
IoTComponent *ESPMegaIoT::getComponent(uint8_t card_id)
{
    return components[card_id];
}

/**
 * @brief Get the network config
 * 
 * @warning You should not modify the returned struct directly
 * 
 * @return The network config struct
 */
NetworkConfig *ESPMegaIoT::getNetworkConfig()
{
    return &network_config;
}

/**
 * @brief Get the MQTT config
 * 
 * @warning You should not modify the returned struct directly
 * 
 * @return The MQTT config struct
 */
MqttConfig *ESPMegaIoT::getMqttConfig()
{
    return &mqtt_config;
}

/**
 * @brief Check if the MQTT is connected
 * 
 * @return True if the MQTT is connected, false otherwise
 */
bool ESPMegaIoT::mqttConnected()
{
    //return mqtt_connected;
    return mqtt.connected();
}

/**
 * @brief Check if the network is connected
 * 
 * @return True if the network is connected, false otherwise
 */
bool ESPMegaIoT::networkConnected()
{
    if (network_config.useWifi)
        return WiFi.status() == WL_CONNECTED;
    else
        return ethernetIface->linkUp();
}

/**
 * @brief Bind a FRAM object to the ESPMegaIoT object
 * @note This class is hardcode to use the FRAM address 34-300
 * 
 * @param fram The FRAM object to bind
 */
void ESPMegaIoT::bindFRAM(FRAM *fram)
{
    this->fram = fram;
}

/**
 * @brief Get the Wifi IP address
 *
 * @return The Wifi IP address
 */
IPAddress ESPMegaIoT::getWifiIp() {
    return WiFi.localIP();
}

/**
 * @brief Get the Ethernet IP Address
 * 
 * @return The Ethernet IP Address
 */
IPAddress ESPMegaIoT::getETHIp() {
    return ETH.localIP();
}

/**
 * @brief Get the IP address of the currently active network interface
 * 
 * @return The IP address of the currently active network interface
 */
IPAddress ESPMegaIoT::getIp() {
    if (network_config.useWifi)
        return this->getWifiIp();
    else
        return this->getETHIp();
}

/**
 * @brief Get the MAC Address of the Ethernet interface
 * 
 * @return The MAC Address of the Ethernet interface
 */
String ESPMegaIoT::getETHMac() {
    return ETH.macAddress();
}

/**
 * @brief Get the MAC Address of the Wifi interface
 * 
 * @return The MAC Address of the Wifi interface
 */
String ESPMegaIoT::getWifiMac() {
    return WiFi.macAddress();
}

/**
 * @brief Get the MAC Address of the currently active network interface
 * 
 * @return The MAC Address of the currently active network interface
 */
String ESPMegaIoT::getMac() {
    if (network_config.useWifi)
        return this->getWifiMac();
    else
        return this->getETHMac();
}
