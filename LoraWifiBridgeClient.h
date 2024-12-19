#ifndef LORA_WIFI_BRIDGE_CLIENT
#define LORA_WIFI_BRIDGE_CLIENT

#include <WiFiClient.h>

class LoraWifiBridgeClient : public WiFiClient {
    size_t write(const uint8_t *buf, size_t size) override;
    String readString() override;
   
};

#endif

