/**
 * @file ESPMegaWebServer.cpp
 * @brief Implementation file for the ESPMegaWebServer class.
 * 
 * This file contains the implementation of the ESPMegaWebServer class, which is responsible for handling web server functionality for the ESPMegaPRO firmware.
 * The ESPMegaWebServer class provides methods for starting the web server, handling HTTP requests, and managing credentials and configurations.
 */
#include <ESPMegaWebServer.hpp>

/**
 * @brief Construct a new ESPMegaWebServer::ESPMegaWebServer objecy
 * 
 * @note Although you can instantiate this class directly, it is recommended to use the ESPMegaPRO.webServer object instead.
 * 
 * @param port The TCP port to listen on
 * @param iot A pointer to the ESPMegaIoT object
 */
ESPMegaWebServer::ESPMegaWebServer(uint16_t port, ESPMegaIoT *iot)
{
    this->port = port;
    this->iot = iot;
    this->server = new AsyncWebServer(port);
    this->saveConfigHandler = new AsyncCallbackJsonWebHandler("/save_config", std::bind(&ESPMegaWebServer::saveConfigJSONHandler, this, std::placeholders::_1, std::placeholders::_2));
}

/**
 * @brief Destroy the ESPMegaWebServer::ESPMegaWebServer object
 */
ESPMegaWebServer::~ESPMegaWebServer()
{
    delete this->server;
}

/**
 * @brief Start the web server
 * 
 * This method starts the web server and registers the necessary handlers.
 * 
 * @note This method should be called after the ESPMegaIoT object has been initialized.
 * @note This method is automatically called if you use ESPMegaPRO::enableWebServer()
 */
void ESPMegaWebServer::begin()
{
    this->loadCredentialsFromFRAM();
    this->server->begin();
    auto bindedDashboardHandler = std::bind(&ESPMegaWebServer::dashboardHandler, this, std::placeholders::_1);
    this->server->on("/", HTTP_GET, bindedDashboardHandler);
    auto bindedConfigHandler = std::bind(&ESPMegaWebServer::configHandler, this, std::placeholders::_1);
    this->server->on("/config", HTTP_GET, bindedConfigHandler);
    this->server->addHandler(saveConfigHandler);
    auto bindedOtaRequestHandler = std::bind(&ESPMegaWebServer::otaRequestHandler, this, std::placeholders::_1);
    auto bindedOtaUploadHandler = std::bind(&ESPMegaWebServer::otaUploadHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    this->server->on("/ota_update", HTTP_POST, bindedOtaRequestHandler, bindedOtaUploadHandler);
    this->server->on("/reboot", HTTP_GET, std::bind(&ESPMegaWebServer::rebootHandler, this, std::placeholders::_1));
    auto bindedGetConfigHandler = std::bind(&ESPMegaWebServer::getConfigHandler, this, std::placeholders::_1);  
    this->server->on("/get_config", HTTP_GET, bindedGetConfigHandler);
    auto bindedGetDeviceInfoHandler = std::bind(&ESPMegaWebServer::getDeviceInfoHandler, this, std::placeholders::_1);
    this->server->on("/get_device_info", HTTP_GET, bindedGetDeviceInfoHandler);
}

/**
 * @brief The loop function for the web server
 * 
 * @note This method is not used by the ESPMegaWebServer class as of now.
 */
void ESPMegaWebServer::loop()
{
    // AsyncWebServer doesn't have a loop function
}

/**
 * @brief Bind the FRAM object to the web server
 * 
 * This method binds the FRAM object to the web server so that the web server can read and write credentials to the FRAM.
 * 
 * @note The FRAM object must be bound to the web server before calling ESPMegaWebServer::begin()
 * @note This class takes 64 bytes of FRAM starting from address 301, however address 301-400 is reserved for it.
 * 
 * @param fram A pointer to the FRAM object
 */
void ESPMegaWebServer::bindFRAM(FRAM *fram)
{
    this->fram = fram;
}

/**
 * @brief Load web username and password from FRAM
 * 
 * This method loads the web server credentials from the FRAM.
 * 
 * @note This method is automatically called by ESPMegaWebServer::begin()
 */
void ESPMegaWebServer::loadCredentialsFromFRAM()
{
    this->fram->read(301, (uint8_t *)this->webUsername, 32);
    this->fram->read(333, (uint8_t *)this->webPassword, 32);
    // Verify if credentials are valid
    // A valid username and password is null terminated
    // Scan for null terminator
    bool validUsername = false;
    bool validPassword = false;
    for (int i = 0; i < 32; i++)
    {
        if (this->webUsername[i] == '\0')
        {
            validUsername = true;
            break;
        }
    }
    for (int i = 0; i < 32; i++)
    {
        if (this->webPassword[i] == '\0')
        {
            validPassword = true;
            break;
        }
    }
    if (!validUsername || !validPassword)
    {
        this->resetCredentials();
        return;
    }
    // A valid username and password is at least 1 character long
    if (strlen(this->webUsername) == 0 || strlen(this->webPassword) == 0)
    {
        this->resetCredentials();
        return;
    }
}

/**
 * @brief Save web username and password to FRAM
 * 
 * This method saves the web server credentials to the FRAM.
 */
void ESPMegaWebServer::saveCredentialsToFRAM()
{
    this->fram->write(301, (uint8_t *)this->webUsername, 32);
    this->fram->write(333, (uint8_t *)this->webPassword, 32);
}

/**
 * @brief Reset web username and password to default
 * 
 * This method resets the web server credentials to the default username and password.
 * 
 * @note The default username and password is both "admin"
 */
void ESPMegaWebServer::resetCredentials()
{
    // The default username and password is "admin"
    strcpy(this->webUsername, "admin");
    strcpy(this->webPassword, "admin");
    this->saveCredentialsToFRAM();
}

/**
 * @brief Get the web username
 * 
 * @warning The returned pointer should not be freed or modified.
 * 
 * @return The web username
 */
char *ESPMegaWebServer::getWebUsername()
{
    return this->webUsername;
}

/**
 * @brief Get the web password
 * 
 * @warning The returned pointer should not be freed or modified.
 * 
 * @return The web password
 */
char *ESPMegaWebServer::getWebPassword()
{
    return this->webPassword;
}

/**
 * @brief Set the web username
 * 
 * @param username The new web username
 */
void ESPMegaWebServer::setWebUsername(const char *username)
{
    strcpy(this->webUsername, username);
}

/**
 * @brief Set the web password
 * 
 * @param password The new web password
 */
void ESPMegaWebServer::setWebPassword(const char *password)
{
    strcpy(this->webPassword, password);
}

/**
 * @brief Handle HTTP requests to the dashboard (/) page
 * 
 * @param request The AsyncWebServerRequest object
 */
void ESPMegaWebServer::dashboardHandler(AsyncWebServerRequest *request)
{
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        return request->requestAuthentication();
    }
    request->send_P(200, "text/html", ota_html);
}

/**
 * @brief Handle HTTP requests to the config (/config) page
 * 
 * @param request The AsyncWebServerRequest object
 */
void ESPMegaWebServer::configHandler(AsyncWebServerRequest *request)
{
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        return request->requestAuthentication();
    }
    request->send_P(200, "text/html", config_html);
}

/**
 * @brief Handle HTTP requests to the OTA update (/ota_update) page
 * 
 * @param request The AsyncWebServerRequest object
 */
void ESPMegaWebServer::otaRequestHandler(AsyncWebServerRequest *request)
{
    // Prepare to receive firmware
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    request->send(response);
    // Restart ESPMega
    ESP.restart();
}

/**
 * @brief Handle HTTP upload session to the OTA update (/ota_update) page
 * 
 * @param request The AsyncWebServerRequest object
 * @param filename The filename of the firmware
 * @param index The index of the firmware
 * @param data The firmware data
 * @param len The length of the firmware data
 * @param final Whether this is the final chunk of firmware
 */
void ESPMegaWebServer::otaUploadHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    // Receive firmware
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        return request->requestAuthentication();
    }
    if (index == 0)
    {
        ESP_LOGI("ESPMegaWebServer", "OTA Update Start");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        { // start with max available size
            ESP_LOGE("ESPMegaWebServer", "OTA Update Start Error");
            Update.printError(Serial);
        }
    }
    if (Update.write(data, len) != len)
    {
        ESP_LOGE("ESPMegaWebServer", "OTA Update Write Error");
        Update.printError(Serial);
    } else {
        ESP_LOGI("ESPMegaWebServer", "OTA Update Write Success: %uB", index + len);
    }
    if (final)
    {
        if (Update.end(true))
        { // true to set the size to the current progress
            ESP_LOGI("ESPMegaWebServer", "OTA Update Success: %uB", index + len);
        }
        else
        {
            ESP_LOGE("ESPMegaWebServer", "OTA Update End Error");
            Update.printError(Serial);
        }
    }
}

/**
 * @brief Handle JSON POST requests to the save_config (/save_config) page
 * 
 * @param request The AsyncWebServerRequest object
 * @param json The JSON object representing the request body
 */
void ESPMegaWebServer::saveConfigJSONHandler(AsyncWebServerRequest *request, JsonVariant &json)
{
    /**
     * Request POST body should be a JSON object
     * containing the following fields:
     * ip_address: String, the IP address of the device
     * netmask: String, the netmask of the device
     * gateway: String, the gateway of the device
     * dns: String, the DNS of the device
     * hostname: String, the hostname of the device
     * bms_ip: String, the IP address of the MQTT broker
     * bms_port: int, the port of the MQTT broker
     * bms_useauth: Boolean, true if the MQTT broker requires authentication
     * bms_username: String, the username of the MQTT broker
     * bms_password: String, the password of the MQTT broker
     * bms_endpoint: String, the base topic of the MQTT broker
     * web_username: String, the username of the web server
     * web_password: String, the password of the web server
     */
    ESP_LOGD("ESPMegaWebServer", "Saving config");
    JsonObject root = json.as<JsonObject>();
    // Network Config
    NetworkConfig networkConfig;
    networkConfig.useStaticIp = true;
    networkConfig.useWifi = false;
    IPAddress ip;
    ESP_LOGD("ESPMegaWebServer", "Checking IP Address");
    if (!ip.fromString(root["ip_address"].as<String>()))
    {
        ESP_LOGE("ESPMegaWebServer", "Invalid Config IP Address");
        request->send(400, "text/plain", "Invalid IP Address");
        return;
    }
    networkConfig.ip = ip;
    ESP_LOGD("ESPMegaWebServer", "Checking Netmask");
    if (!ip.fromString(root["netmask"].as<String>()))
    {
        ESP_LOGE("ESPMegaWebServer", "Invalid Config Netmask");
        request->send(400, "text/plain", "Invalid Netmask");
        return;
    }
    networkConfig.subnet = ip;
    ESP_LOGD("ESPMegaWebServer", "Checking Gateway");
    if (!ip.fromString(root["gateway"].as<String>()))
    {
        ESP_LOGE("ESPMegaWebServer", "Invalid Config Gateway");
        request->send(400, "text/plain", "Invalid Gateway");
        return;
    }
    networkConfig.gateway = ip;
    ESP_LOGD("ESPMegaWebServer", "Checking DNS");
    if (!ip.fromString(root["dns"].as<String>()))
    {
        ESP_LOGE("ESPMegaWebServer", "Invalid Config DNS");
        request->send(400, "text/plain", "Invalid DNS");
        return;
    }
    networkConfig.dns1 = ip;
    ESP_LOGD("ESPMegaWebServer", "Setting Hostname");
    strcpy(networkConfig.hostname, root["hostname"].as<String>().c_str());
    // MQTT Config
    MqttConfig mqttConfig;
    ESP_LOGD("ESPMegaWebServer", "Setting MQTT Server");
    strcpy(mqttConfig.mqtt_server, root["bms_ip"].as<String>().c_str());
    ESP_LOGD("ESPMegaWebServer", "Checking MQTT Port");
    uint16_t mqttPort = root["bms_port"].as<int>();
    if (mqttConfig.mqtt_port <= 0 || mqttConfig.mqtt_port > 65535)
    {
        ESP_LOGE("ESPMegaWebServer", "Invalid Config MQTT Port");
        request->send(400, "text/plain", "Invalid MQTT Port");
        return;
    }
    mqttConfig.mqtt_port = mqttPort;
    ESP_LOGD("ESPMegaWebServer", "Checking MQTT Use Auth");
    mqttConfig.mqtt_useauth = root["bms_useauth"].as<bool>();
    ESP_LOGD("ESPMegaWebServer", "Setting MQTT Username");
    strcpy(mqttConfig.mqtt_user, root["bms_username"].as<String>().c_str());
    ESP_LOGD("ESPMegaWebServer", "Setting MQTT Password");
    strcpy(mqttConfig.mqtt_password, root["bms_password"].as<String>().c_str());
    ESP_LOGD("ESPMegaWebServer", "Setting MQTT Base Topic");
    strcpy(mqttConfig.base_topic, root["bms_endpoint"].as<String>().c_str());
    // Web Server Config
    ESP_LOGD("ESPMegaWebServer", "Setting Web Username");
    strcpy(this->webUsername, root["web_username"].as<String>().c_str());
    ESP_LOGD("ESPMegaWebServer", "Setting Web Password");
    strcpy(this->webPassword, root["web_password"].as<String>().c_str());
    // Commit changes to FRAM
    ESP_LOGD("ESPMegaWebServer", "Committing Network Config to FRAM");
    this->iot->setNetworkConfig(networkConfig);
    this->iot->saveNetworkConfig();
    ESP_LOGD("ESPMegaWebServer", "Committing MQTT Config to FRAM");
    this->iot->setMqttConfig(mqttConfig);
    this->iot->saveMqttConfig();
    ESP_LOGD("ESPMegaWebServer", "Committing Web Server Config to FRAM");
    this->saveCredentialsToFRAM();
    ESP_LOGD("ESPMegaWebServer", "Config saved");
    // Send response
    request->send(200, "text/plain", "OK");
    ESP.restart();
}

/**
 * @brief Get the AsyncWebServer object
 * 
 * @return The AsyncWebServer object
 */
AsyncWebServer *ESPMegaWebServer::getServer() {
    return this->server;
}

/**
 * @brief Request authentication from the client
 * 
 * This method requests authentication from the client.
 * 
 * @param request The AsyncWebServerRequest object
 */
bool ESPMegaWebServer::checkAuthentication(AsyncWebServerRequest *request) {
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        request->requestAuthentication();
        return false;
    }
    return true;
}

/**
 * @brief Handle HTTP requests to the reboot (/reboot) page
 * 
 * @param request The AsyncWebServerRequest object
 */
void ESPMegaWebServer::rebootHandler(AsyncWebServerRequest *request)
{
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        return request->requestAuthentication();
    }
    request->send(200, "text/plain", "Rebooting ESPMega PRO...");
    esp_restart();
}

void ESPMegaWebServer::getConfigHandler(AsyncWebServerRequest *request) {
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        return request->requestAuthentication();
    }
    StaticJsonDocument<512> doc;
    NetworkConfig *networkConfig = this->iot->getNetworkConfig();
    MqttConfig *mqttConfig = this->iot->getMqttConfig();
    doc["ip_address"] = networkConfig->ip.toString();
    doc["netmask"] = networkConfig->subnet.toString();
    doc["gateway"] = networkConfig->gateway.toString();
    doc["dns"] = networkConfig->dns1.toString();
    doc["hostname"] = networkConfig->hostname;
    doc["bms_ip"] = mqttConfig->mqtt_server;
    doc["bms_port"] = mqttConfig->mqtt_port;
    doc["bms_useauth"] = mqttConfig->mqtt_useauth;
    doc["bms_username"] = mqttConfig->mqtt_user;
    doc["bms_password"] = mqttConfig->mqtt_password;
    doc["bms_endpoint"] = mqttConfig->base_topic;
    doc["web_username"] = this->webUsername;
    doc["web_password"] = this->webPassword;
    char buffer[512];
    serializeJson(doc, buffer);
    request->send(200, "application/json", buffer);
}

void ESPMegaWebServer::getDeviceInfoHandler(AsyncWebServerRequest *request) {
    if (!request->authenticate(this->webUsername, this->webPassword))
    {
        return request->requestAuthentication();
    }
    StaticJsonDocument<512> doc;
    doc["hostname"] = this->iot->getNetworkConfig()->hostname;
    doc["ip_address"] = this->iot->getIp().toString();
    doc["mac_address"] = this->iot->getMac();
    doc["model"] = BOARD_MODEL;
    doc["mqtt_server"] = this->iot->getMqttConfig()->mqtt_server;
    doc["mqtt_port"] = this->iot->getMqttConfig()->mqtt_port;
    doc["base_topic"] = this->iot->getMqttConfig()->base_topic;
    doc["mqtt_connected"] = this->iot->mqttConnected() ? "Connected" : "Standalone";
    doc["software_version"] = SW_VERSION;
    doc["sdk_version"] = SDK_VESRION;
    doc["idf_version"] = IDF_VER;
    char buffer[512];
    serializeJson(doc, buffer);
    request->send(200, "application/json", buffer);
}