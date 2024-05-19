#pragma once
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <ESPMegaIoT.hpp>
#include <Update.h>
#include <FRAM.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <all_html.h>
#include <ESPMegaCommon.hpp>

/**
 * @brief Provides a web server for ESPMegaPRO
 * 
 * This class provides a web server for ESPMegaPRO. It is used to configure the device and to update the firmware.
 * This class also allows to save the credentials to access the web server in the FRAM memory.
 * User can also add custom endpoints to the web server.
 * 
 * This class use FRAM address 301-400
 */
class ESPMegaWebServer
{
    public:
        ESPMegaWebServer(uint16_t port, ESPMegaIoT *iot);
        ~ESPMegaWebServer();
        void begin();
        void loop();
        void resetCredentials();
        char* getWebUsername();
        char* getWebPassword();
        void setWebUsername(const char* username);
        void setWebPassword(const char* password);
        void bindFRAM(FRAM *fram);
        void loadCredentialsFromFRAM();
        void saveCredentialsToFRAM();
        AsyncWebServer* getServer();
        bool checkAuthentication(AsyncWebServerRequest *request);
        // Endpoints Handlers
        void dashboardHandler(AsyncWebServerRequest *request);
        void configHandler(AsyncWebServerRequest *request);
        AsyncCallbackJsonWebHandler *saveConfigHandler;
        void saveConfigJSONHandler(AsyncWebServerRequest *request, JsonVariant &json);
        void getConfigHandler(AsyncWebServerRequest *request);
        void getDeviceInfoHandler(AsyncWebServerRequest *request);
        void otaRequestHandler(AsyncWebServerRequest *request);
        void otaUploadHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
        void restAPIHandler(AsyncWebServerRequest *request);
        void rebootHandler(AsyncWebServerRequest *request);
    private:
        // FRAM
        FRAM *fram;
        // Credentials
        char webUsername[32];
        char webPassword[32];
        // Web Server
        AsyncWebServer *server;
        uint16_t port;
        // ESPMegaIoT
        ESPMegaIoT *iot;
};