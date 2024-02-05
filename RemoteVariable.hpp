#pragma once
#include <ESPMegaIoT.hpp>
class RemoteVariable
{
    public:
        RemoteVariable();
        ~RemoteVariable();
        void begin(size_t size, char* topic, ESPMegaIoT* iot);
        void begin(size_t size, char* topic, ESPMegaIoT* iot, bool useValueRequest, char* valueRequestTopic);
        void setValue(char* value);
        void enableSetValue(char* setValueTopic);
        void subscribe();
        void requestValue();
        char* getValue();
    private:
        void mqtt_callback(char* topic, char* payload);
        ESPMegaIoT* iot;
        char* topic;
        char* value;
        size_t size;
        bool useValueRequest;
        char* valueRequestTopic;
        bool useSetValue;
        char* setValueTopic;
};