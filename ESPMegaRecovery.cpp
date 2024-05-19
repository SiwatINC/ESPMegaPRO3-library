#include <ESPMegaRecovery.hpp>
ESPMegaRecovery::ESPMegaRecovery()
{
    // Initialize all the pointers to null
    this->fram = nullptr;
    this->fram_address = 0;
    this->bootloop_counter = 0;
    this->recovery_mode = false;
    this->web_server = nullptr;
    this->iot = nullptr;
}
void ESPMegaRecovery::begin() {
    // Retrieve the bootloop counter from the FRAM
    if(this->fram != nullptr) {
        this->bootloop_counter = this->fram->read8(this->fram_address);
    }
    // Inclement the bootloop counter
    this->inclementBootloopCounter();
    ESP_LOGV("ESPMegaRecovery", "Bootloop counter: %d", this->getBootloopCounter());
    // If the bootloop counter is greater than 5, enter recovery mode
    if(this->getBootloopCounter() > 5) {
        ESP_LOGE("ESPMegaRecovery", "Bootloop detected");
        // Reset the bootloop counter to prevent re-entering recovery mode
        // The device might unintentionally restart multiple times
        // By resetting the counter, the user can press reset once in recovery mode to exit
        ESP_LOGD("ESPMegaRecovery", "Resetting bootloop counter");
        this->resetBootloopCounter();
        ESP_LOGW("ESPMegaRecovery", "Entering recovery mode");
        this->enterRecoveryMode();
        this->loop();
    }
}
void ESPMegaRecovery::loop() {
    // If the device is in recovery mode, block all other tasks
    if(this->isRecoveryMode()) {
        int i = 0;
        while(true) {
            if (i%10 == 0) {
                ESP_LOGV("ESPMegaRecovery", "System is in recovery mode, no tasks will be executed");
                ESP_LOGV("ESPMegaRecovery", "Please upload a new firmware to exit recovery mode");
            }
            // This code will become the new loop
            delay(1000);
            i++;
        }
    }
    // Watchdog timer
    static bool booted = false;
    static uint32_t boot_time = millis();
    if(!booted) {
        if(millis() - boot_time > RECOVERY_WATCHDOG_TIMEOUT * 1000) {
            ESP_LOGI("ESPMegaRecovery", "System booted successfully, resetting bootloop counter");
            this->resetBootloopCounter();
            booted = true;
        }
    }
}
void ESPMegaRecovery::enterRecoveryMode() {
    // Set the recovery mode flag
    this->recovery_mode = true;
    // Enabling the IoT module
    ESP_LOGI("ESPMegaRecovery", "Enabling the IoT module");
    this->iot = new ESPMegaIoT();
    this->iot->bindFRAM(this->fram);
    ETH.begin();
    this->iot->bindEthernetInterface(&ETH);
    ESP_LOGI("ESPMegaRecovery", "Loading network configuration");
    this->iot->loadNetworkConfig();
    ESP_LOGI("ESPMegaRecovery", "Attempting to connect to the network");
    this->iot->connectNetwork();
    // Start the web server
    ESP_LOGI("ESPMegaRecovery", "Starting the web server");
    this->web_server = new ESPMegaWebServer(80, this->iot);
    this->web_server->bindFRAM(this->fram);
    this->web_server->loadCredentialsFromFRAM();
    ESP_LOGI("ESPMegaRecovery", "Aquiring the web server instance");
    AsyncWebServer *server = this->web_server->getServer();
    server->begin();
    // Add OTA update and restart endpoint
    ESP_LOGI("ESPMegaRecovery", "Adding OTA update and reboot endpoints");
    auto bindedDashboardHandler = std::bind(&ESPMegaWebServer::dashboardHandler, this->web_server, std::placeholders::_1);
    auto bindedOtaRequestHandler = std::bind(&ESPMegaWebServer::otaRequestHandler, this->web_server, std::placeholders::_1);
    auto bindedOtaUploadHandler = std::bind(&ESPMegaWebServer::otaUploadHandler, this->web_server, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    auto bindedDeviceInfoHandler = std::bind(&ESPMegaRecovery::getDeviceInfoHandler, this, std::placeholders::_1);
    auto bindedConfigHandler = std::bind(&ESPMegaRecovery::configHandler, this, std::placeholders::_1);
    server->on("/", HTTP_GET, bindedDashboardHandler);
    server->on("/ota_update", HTTP_POST, bindedOtaRequestHandler, bindedOtaUploadHandler);
    server->on("/reboot", HTTP_GET, std::bind(&ESPMegaWebServer::rebootHandler, this->web_server, std::placeholders::_1));
    server->on("/get_device_info", HTTP_GET, bindedDeviceInfoHandler);
    server->on("/config", HTTP_GET, bindedConfigHandler);
}
void ESPMegaRecovery::bindFRAM(FRAM *fram, uint32_t address) {
    this->fram = fram;
    this->fram_address = address;
}
uint8_t ESPMegaRecovery::getBootloopCounter() { 
    return this->bootloop_counter;
}
void ESPMegaRecovery::inclementBootloopCounter() {
    this->bootloop_counter++;
    if(this->fram != nullptr) {
        this->fram->write8(this->fram_address, this->bootloop_counter);
    }
}
void ESPMegaRecovery::resetBootloopCounter() {
    this->bootloop_counter = 0;
    if(this->fram != nullptr) {
        this->fram->write8(this->fram_address, this->bootloop_counter);
    }
}
bool ESPMegaRecovery::isRecoveryMode() {
    return this->recovery_mode;
}

void ESPMegaRecovery::getDeviceInfoHandler(AsyncWebServerRequest *request) {
    if (!request->authenticate(this->web_server->getWebUsername(), this->web_server->getWebPassword()))
    {
        return request->requestAuthentication();
    }
    StaticJsonDocument<512> doc;
    doc["hostname"] = this->iot->getNetworkConfig()->hostname;
    doc["ip_address"] = this->iot->getIp().toString();
    doc["mac_address"] = this->iot->getMac();
    doc["model"] = BOARD_MODEL;
    doc["mqtt_server"] = "Recovery";
    doc["mqtt_port"] = "Mode";
    doc["base_topic"] = "Recovery Mode";
    doc["mqtt_connected"] = "Recovery Mode";
    doc["software_version"] = "EMG-SAFE-1.0.0";
    doc["sdk_version"] = SDK_VESRION;
    doc["idf_version"] = IDF_VER;
    char buffer[512];
    serializeJson(doc, buffer);
    request->send(200, "application/json", buffer);
}

void ESPMegaRecovery::configHandler(AsyncWebServerRequest *request){
    // Say Not Available in Recovery Mode
    // Wait 3s then redirect to /
    request->send(500, "text/html", "<h1>RECOVERY MODE</h1><p>Configuration is not available in recovery mode</p><script>setTimeout(function(){window.location.href = '/';}, 1500);</script>");
}