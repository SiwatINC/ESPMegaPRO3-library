#pragma once
#include <FRAM.h>
#include <ESPMegaIoT.hpp>
#include <map>

/**
 * @brief SmartVariable is a local variable that can be accessed remotely and have FRAM support
 */

class SmartVariable {
public:
    SmartVariable();
    ~SmartVariable();
    void begin(size_t size);
    void enableIoT(ESPMegaIoT* iot, const char* topic);
    void enableValueRequest(const char* valueRequestTopic);
    void setValue(const char* value);
    char* getValue();
    void enableSetValue(const char* setValueTopic);
    void publishValue();
    void bindFRAM(FRAM *fram, uint32_t framAddress);
    void bindFRAM(FRAM *fram, uint32_t framAddress, bool loadValue);
    void loadValue();
    void saveValue();
    void setValueAutoSave(bool autoSave);
    uint16_t registerCallback(void (*callback)(char*));
    void unregisterCallback(uint16_t handlerId);
    int32_t getIntValue();
    void setIntValue(int32_t value);
    double getDoubleValue();
    void setDoubleValue(double value);
protected:
    ESPMegaIoT* iot;
    bool iotEnabled;
    const char* topic;
    char* value;
    size_t size;
    bool useValueRequest;
    const char* valueRequestTopic;
    bool setValueEnabled;
    const char* setValueTopic;
    bool autoSave;
    FRAM *fram;
    uint32_t framAddress;
    void handleMqttCallback(char* topic, char* payload);
    void subscribeMqtt();
    // Value Change Callback
    uint16_t currentHandlerId;
    std::map<uint16_t, void (*)(char*)> valueChangeCallbacks;
};