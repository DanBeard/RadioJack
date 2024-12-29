#ifndef MESHTASTIC_WIFI_BRIDGE_CLIENT
#define MESHTASTIC_WIFI_BRIDGE_CLIENT

#include <WiFiClient.h>
// maximum payload we want to send over meshtastic
#define MAX_MESH_PAYLOAD_LEN 200
// default broadcast address. Change this if you want to try DMs instead of private channels
#define MESH_DEST_ADDR 0xFFFFFFFF
// Meshtastic channel to use by index (maybe choose a private one and not long_fast so not to make it totally public right?)
#define MESH_CHANNEL_IDX 2


class MeshtasticWifiBridgeClient : public WiFiClient {
    size_t write(const uint8_t *buf, size_t size) override;
    String readString() override;
    void debug(char * msg);

    public:
      void (*write_to_screen)(const char *label1_text) = 0;
      void requestConfigInfo();
   
};

#endif

