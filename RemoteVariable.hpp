#pragma once
#include <ESPMegaIoT.hpp>
#include <map>
/**
 * @brief A class that create a variable that exists on other devices and can be accessed remotely
 * 
 * This class is used to create a variable that exists on other devices and can be accessed remotely.
 * Supports setting and getting values from the variable.
 * Also support value request. 
*/
class RemoteVariable
{
    public:
        RemoteVariable();
        ~RemoteVariable();
        void begin(size_t size, const char* topic, ESPMegaIoT* iot);
        void begin(size_t size, const char* topic, ESPMegaIoT* iot, bool useValueRequest, const char* valueRequestTopic);
        void setValue(const char* value);
        void enableSetValue(const char* setValueTopic);
        void subscribe();
        void requestValue();
        char* getValue();
        int getValueAsInt();
        long getValueAsLong();
        double getValueAsDouble();
        void setIntValue(int value);
        void setLongValue(long value);
        void setDoubleValue(double value);
        uint8_t registerCallback(std::function<void(char*)>);
        void unregisterCallback(uint8_t handler);
        
    private:
        void mqtt_callback(char* topic, char* payload);
        ESPMegaIoT* iot;
        const char* topic;
        char* value;
        size_t size;
        bool useValueRequest;
        const char* valueRequestTopic;
        bool useSetValue;
        const char* setValueTopic;
        uint8_t valueChangeCallbackCount;
        std::map<uint8_t,std::function<void(char*)>> valueChangeCallback;
};