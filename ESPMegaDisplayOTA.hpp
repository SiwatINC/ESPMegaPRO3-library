#pragma once
#include <ESPMegaDisplay.hpp>
#include <ESPMegaWebServer.hpp>

class ESPMegaDisplayOTA {
    public:
        ESPMegaDisplayOTA();
        void begin(const char* base_path, ESPMegaDisplay *display, ESPMegaWebServer *webServer);
    private:
        AsyncCallbackJsonWebHandler *otaUpdateBeginWebHandler;
        AsyncCallbackJsonWebHandler *otaUpdateWriteWebHandler;
        AsyncCallbackJsonWebHandler *otaUpdateEndWebHandler;   
        void otaUpdateBeginHandler(AsyncWebServerRequest *request, JsonVariant &json);
        void otaUpdateWriteHandler(AsyncWebServerRequest *request, JsonVariant &json);
        void otaUpdateEndHandler(AsyncWebServerRequest *request, JsonVariant &json);
        void displayWebPageHandler(AsyncWebServerRequest *request);
        const char *base_path;
        AsyncWebServer *server;
        ESPMegaDisplay *display;
        ESPMegaWebServer *webServer;
        size_t updateSize;
        size_t updateProgress;
};