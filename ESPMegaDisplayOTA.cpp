#include <ESPMegaDisplayOTA.hpp>

ESPMegaDisplayOTA::ESPMegaDisplayOTA() {

}

void ESPMegaDisplayOTA::begin(const char* base_path, ESPMegaDisplay *display, ESPMegaWebServer *webServer) {
    this->display = display;
    this->webServer = webServer;
    this->server = webServer->getServer();
    this->base_path = base_path;
    char ota_begin_path[100];
    char ota_write_path[100];
    char ota_end_path[100];
    char ota_page_path[100];
    sprintf(ota_begin_path, "%s/ota/begin", base_path);
    sprintf(ota_write_path, "%s/ota/write", base_path);
    sprintf(ota_end_path, "%s/ota/end", base_path);
    sprintf(ota_page_path, "%s/index.html", base_path);
    this->otaUpdateBeginWebHandler = new AsyncCallbackJsonWebHandler(ota_begin_path, std::bind(&ESPMegaDisplayOTA::otaUpdateBeginHandler, this, std::placeholders::_1, std::placeholders::_2));
    this->otaUpdateWriteWebHandler = new AsyncCallbackJsonWebHandler(ota_write_path, std::bind(&ESPMegaDisplayOTA::otaUpdateWriteHandler, this, std::placeholders::_1, std::placeholders::_2), 8192U);
    this->otaUpdateEndWebHandler = new AsyncCallbackJsonWebHandler(ota_end_path, std::bind(&ESPMegaDisplayOTA::otaUpdateEndHandler, this, std::placeholders::_1, std::placeholders::_2));
    this->server->addHandler(this->otaUpdateBeginWebHandler);
    this->server->addHandler(this->otaUpdateWriteWebHandler);
    this->server->addHandler(this->otaUpdateEndWebHandler);
    this->server->on(ota_page_path, HTTP_GET, std::bind(&ESPMegaDisplayOTA::displayWebPageHandler, this, std::placeholders::_1));
}

void ESPMegaDisplayOTA::otaUpdateBeginHandler(AsyncWebServerRequest *request, JsonVariant &json) {
    this->webServer->checkAuthentication(request);
    // The content type of the request is application/json
    // The body of the request is a JSON object with the following field:
    // - size: the size of the update

    // Parse the JSON object
    JsonObject content = json.as<JsonObject>();
    // Check if the size field is present
    if(!content.containsKey("size"))
        return;
    // Get the size field
    this->updateSize = content["size"].as<size_t>();
    // Begin the update
    if(!this->display->beginUpdate(this->updateSize)) {
        // If the update cannot be started, return an error
        request->send(500, "application/json", "{\"status\": \"error\"}");
    } else {
        // If the update can be started, return a success
        request->send(200, "application/json", "{\"status\": \"success\"}");
    }

}
void ESPMegaDisplayOTA::otaUpdateWriteHandler(AsyncWebServerRequest *request, JsonVariant &json) {
    this->webServer->checkAuthentication(request);
    // The content type of the request is application/json
    // The body of the request is a JSON object with the following field:
    // - size: the size of the update in bytes
    // - data: the data to write
    
    //Parse the JSON object
    JsonObject content = json.as<JsonObject>();
    // // Check if the size and data fields are present
    // Serial.println("Checking if size and data fields are present");
    // if(!content.containsKey("size") || !content.containsKey("data"))
    //     Serial.println("Size or data field is missing");
    //     request->send(500, "application/json", "{\"status\": \"error\", \"message\": \"The size or data field is missing\"}");
    //     return;
    // Serial.println("Size and data fields are present, getting size");
    // Get the size field
    size_t size = content["size"].as<size_t>();
    if(size>4096) {
        // If the size is greater than 4096 bytes, return an error
        request->send(500, "application/json", "{\"status\": \"error\", \"message\": \"The size of the update is too big\"}");
        return;
    }
    // Get the data field
    JsonArray data = content["data"].as<JsonArray>();
    if(this->updateProgress+size>this->updateSize) {
        // If the update is too big, return an error
        request->send(500, "application/json", "{\"status\": \"error\", \"message\": \"The update chunk is too big\"}");
        return;
    }
    // Convert JsonArray to uint8_t*
    uint8_t data_array[4096];
    for(size_t i=0; i<size; i++) {
        data_array[i] = data[i].as<uint8_t>();
    }
    // Write the data to the display
    display->writeUpdate(data_array, size);
    request->send(200, "application/json", "{\"status\": \"success\",\"bytes_written\": "+String(this->display->getUpdateBytesWritten())+"}");
}
void ESPMegaDisplayOTA::otaUpdateEndHandler(AsyncWebServerRequest *request, JsonVariant &json) {
    this->webServer->checkAuthentication(request);
    display->endUpdate();
    request->send(200, "application/json", "{\"status\": \"success\"}");
    esp_restart();
}

void ESPMegaDisplayOTA::displayWebPageHandler(AsyncWebServerRequest *request) {
    this->webServer->checkAuthentication(request);
    request->send_P(200, "text/html", display_html);
}