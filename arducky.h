// modified from arducky -- https://github.com/Creased/arducky/blob/master/arducky.ino

#include "Arduino.h"
#include "lv_driver.h"
#include "pin_config.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "FS.h"
#include <WiFiClient.h>

short executeDucky(fs::FS &fs, const char* ducky_file_path, USBHIDKeyboard& keyboard, char* errmsg, int maxErrMsg,  WiFiClient *client);