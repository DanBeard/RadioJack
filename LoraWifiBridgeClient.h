#ifndef LORA_WIFI_BRIDGE_CLIENT
#define LORA_WIFI_BRIDGE_CLIENT

#include <WiFiClient.h>
// maximum payload we want to send over meshtastic
#define MAX_MESH_PAYLOAD_LEN 200


class LoraWifiBridgeClient : public WiFiClient {
    size_t write(const uint8_t *buf, size_t size) override;
    String readString() override;
    void debug(char * msg);

    public:
      void (*write_to_screen)(const char *label1_text) = 0;
      void requestConfigInfo();
   
};

#endif

