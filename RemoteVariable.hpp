#pragma once
#include <ESPMegaIoT.hpp>
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
};