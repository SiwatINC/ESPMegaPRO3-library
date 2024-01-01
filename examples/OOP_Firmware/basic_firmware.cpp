#include <ESPMegaProOS.hpp>
#include <InternalDisplay.hpp>
#include <ETH.h>
#include <ClimateCard.hpp>

// #define FRAM_DEBUG
// #define MQTT_DEBUG
// #define WRITE_DEFAULT_NETCONF
#define CLIMATE_CARD_ENABLE
#define MQTT_CARD_REGISTER
//#define DISPLAY_ENABLE
#define WEB_SERVER_ENABLE

// Demo PLC firmware using the ESPMegaPRO OOP library

ESPMegaPRO espmega = ESPMegaPRO();

#ifdef CLIMATE_CARD_ENABLE
// Climate Card
const uint16_t irCode[15][4][4][1] = {0};
const char *mode_names[] = {"off", "fan_only", "cool"};
const char *fan_speed_names[] = {"auto", "low", "medium", "high"};

size_t getInfraredCode(uint8_t mode, uint8_t fan_speed, uint8_t temperature, const uint16_t **codePtr)
{
    // Change the code pointer to point to the IR timing array
    *codePtr = &(irCode[mode][fan_speed][temperature][0]);
    return sizeof(irCode[mode][fan_speed][temperature]) / sizeof(uint16_t);
}

AirConditioner ac = {
    .max_temperature = 30,
    .min_temperature = 16,
    .modes = 4,
    .mode_names = mode_names,
    .fan_speeds = 4,
    .fan_speed_names = fan_speed_names,
    .getInfraredCode = &getInfraredCode};

ClimateCard climateCard = ClimateCard(14, ac);
#endif

void input_change_callback(uint8_t pin, uint8_t value)
{
    Serial.print("Input change callback: ");
    Serial.print(pin);
    Serial.print(" ");
    Serial.println(value);
}

void mqtt_callback(char *topic, char* payload)
{
    Serial.print("MQTT Callback: ");
    Serial.print(topic);
    Serial.print(" ");
    Serial.println(payload);
}

#ifdef WRITE_DEFAULT_NETCONF
void setNetworkConfig()
{
    NetworkConfig config = {
        .ip = {192, 168, 0, 10},
        .gateway = {192, 168, 0, 1},
        .subnet = {255, 255, 255, 0},
        .dns1 = {8, 8, 8, 8},
        .dns2 = {8, 8, 4, 4},
        .useStaticIp = true,
        .useWifi = false,
        .wifiUseAuth = false,
    };
    strcpy(config.ssid, "your_ssid");
    strcpy(config.password, "your_password");
    strcpy(config.hostname, "your_hostname");
    Serial.println("Setting network config");
    espmega.iot->setNetworkConfig(config);
    espmega.iot->saveNetworkConfig();
}

void setMqttConfig()
{
    MqttConfig config = {
        .mqtt_port = 1883,
        .mqtt_useauth = false};
    strcpy(config.mqtt_server, "<mqtt_server_ip>");
    strcpy(config.base_topic, "<base_topic>");
    espmega.iot->setMqttConfig(config);
    espmega.iot->saveMqttConfig();
}
#endif

void setup()
{
    ESP_LOGI("Initializer", "Starting ESPMegaPRO OOP demo");
    espmega.begin();
    ESP_LOGI("Initializer", "Enabling IOT module");
    espmega.enableIotModule();
    ESP_LOGI("Initializer", "Enabling Ethernet");
    ETH.begin();
    ESP_LOGI("Initializer", "Binding Ethernet to IOT module");
    espmega.iot->bindEthernetInterface(&ETH);
    #ifdef WRITE_DEFAULT_NETCONF
    setNetworkConfig();
    #else
    ESP_LOGI("Initializer", "Loading network config");
    espmega.iot->loadNetworkConfig();
    #endif
    ESP_LOGI("Initializer", "Connecting to network");
    espmega.iot->connectNetwork();
    #ifdef WRITE_DEFAULT_NETCONF
    setMqttConfig();
    #else
    ESP_LOGI("Initializer", "Loading MQTT config");
    espmega.iot->loadMqttConfig();
    #endif
    ESP_LOGI("Initializer", "Connecting to MQTT");
    espmega.iot->connectToMqtt();
    espmega.iot->registerMqttCallback(mqtt_callback);
    #ifdef MQTT_CARD_REGISTER
    ESP_LOGI("Initializer", "Registering cards 0");
    espmega.iot->registerCard(0);
    ESP_LOGI("Initializer", "Registering cards 1");
    espmega.iot->registerCard(1);
    #endif
    ESP_LOGI("Initializer", "Registering Input change callback");
    espmega.inputs.registerCallback(input_change_callback);
    #ifdef CLIMATE_CARD_ENABLE
    ESP_LOGI("Initializer", "Installing climate card");
    espmega.installCard(2, &climateCard);
    ESP_LOGI("Initializer", "Binding climate card to FRAM");
    climateCard.bindFRAM(&espmega.fram, 1001);
    ESP_LOGI("Initializer", "Loading climate card state from FRAM");
    climateCard.loadStateFromFRAM();
    ESP_LOGI("Initializer", "Enabling climate card FRAM autosave");
    climateCard.setFRAMAutoSave(true);
    ESP_LOGI("Initializer", "Registering cards 2");
    espmega.iot->registerCard(2);
    #endif
    #ifdef DISPLAY_ENABLE
    ESP_LOGI("Initializer", "Enabling internal display");
    espmega.enableInternalDisplay(&Serial);
    ESP_LOGI("Initializer", "Binding climate card to internal display");
    espmega.display->bindClimateCard(&climateCard);
    #endif
    #ifdef WEB_SERVER_ENABLE
    ESP_LOGI("Initializer", "Enabling web server");
    espmega.enableWebServer(80);
    // This will set the web server username and password to "admin"
    // If this line is not commented out,
    // the web server credentials will be reset to "admin" every time the device boots
    espmega.webServer->setWebUsername("admin");
    espmega.webServer->setWebPassword("admin");
    espmega.webServer->saveCredentialsToFRAM();
    #endif
}

void loop()
{
    espmega.loop();
#ifdef FRAM_DEBUG
    // Every 20 seconds, dump FRAM 0-500 to serial
    static uint32_t last_fram_dump = 0;
    if (millis() - last_fram_dump >= 20000)
    {
        last_fram_dump = millis();
        Serial.println("Dumping FRAM");
        espmega.dumpFRAMtoSerial(0, 500);
        Serial.println("Dumping FRAM ASCII");
        espmega.dumpFRAMtoSerialASCII(0, 500);
    }
#endif

// Every 5 seconds, publish "I'm alive" to MQTT
#ifdef MQTT_DEBUG
    static uint32_t last_mqtt_publish = 0;
    if (millis() - last_mqtt_publish >= 5000)
    {
        last_mqtt_publish = millis();
        espmega.iot->publish("/espmegai/alive", "true");
    }
    static uint32_t last_mqtt_status = 0;
    if (millis() - last_mqtt_status >= 1000)
    {
        last_mqtt_status = millis();
        Serial.print("MQTT Status: ");
        Serial.println(espmega.iot->mqttConnected() ? "Connected" : "Disconnected");
    }
#endif
}