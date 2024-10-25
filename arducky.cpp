// modified from arducky -- https://github.com/Creased/arducky/blob/master/arducky.ino

// SD card read/write
#include "Arduino.h"
#include "lv_driver.h"
#include "pin_config.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "FS.h"

/**
 * Variables
 **/
#define KEY_MENU          0xED
#define KEY_PAUSE         0xD0
#define KEY_NUMLOCK       0xDB
#define KEY_PRINTSCREEN   0xCE
#define KEY_SCROLLLOCK    0xCF
#define KEY_SPACE         0xB4
#define KEY_BACKSPACE     0xB2

char *duckyfile_path = (char*)"/ducky.txt";      // File containing payload placed on SD Card

void processCommand(USBHIDKeyboard keyboard, String command) {
    /*
     * Process commands by pressing corresponding key
     * (see https://www.arduino.cc/en/Reference/KeyboardModifiers or
     *      http://www.usb.org/developers/hidpage/Hut1_12v2.pdf#page=53)
     */

    if (command.length() == 1) {     // Process key (used for example for WIN L command)
        char c = (char) command[0];  // Convert string (1-char length) to char
        keyboard.press(c);           // Press the key on keyboard
    } else if (command == "ENTER") {
        keyboard.press(KEY_RETURN);
    } else if (command == "MENU" || command == "APP") {
        keyboard.press(KEY_MENU);
    } else if (command == "DOWNARROW" || command == "DOWN") {
        keyboard.press(KEY_DOWN_ARROW);
    } else if (command == "LEFTARROW" || command == "LEFT") {
        keyboard.press(KEY_LEFT_ARROW);
    } else if (command == "RIGHTARROW" || command == "RIGHT") {
        keyboard.press(KEY_RIGHT_ARROW);
    } else if (command == "UPARROW" || command == "UP") {
        keyboard.press(KEY_UP_ARROW);
    } else if (command == "BREAK" || command == "PAUSE") {
        keyboard.press(KEY_PAUSE);
    } else if (command == "CAPSLOCK") {
        keyboard.press(KEY_CAPS_LOCK);
    } else if (command == "DELETE" || command == "DEL") {
        keyboard.press(KEY_DELETE);
    } else if (command == "END") {
        keyboard.press(KEY_END);
    } else if (command == "ESC" || command == "ESCAPE") {
        keyboard.press(KEY_ESC);
    } else if (command == "HOME") {
        keyboard.press(KEY_HOME);
    } else if (command == "INSERT") {
        keyboard.press(KEY_INSERT);
    } else if (command == "NUMLOCK") {
        keyboard.press(KEY_NUMLOCK);
    } else if (command == "PAGEUP") {
        keyboard.press(KEY_PAGE_UP);
    } else if (command == "PAGEDOWN") {
        keyboard.press(KEY_PAGE_DOWN);
    } else if (command == "PRINTSCREEN") {
        keyboard.press(KEY_PRINTSCREEN);
    } else if (command == "SCROLLLOCK") {
        keyboard.press(KEY_SCROLLLOCK);
    } else if (command == "SPACE") {
        keyboard.press(KEY_SPACE);
    } else if (command == "BACKSPACE") {
        keyboard.press(KEY_BACKSPACE);
    } else if (command == "TAB") {
        keyboard.press(KEY_TAB);
    } else if (command == "GUI" || command == "WINDOWS") {
        keyboard.press(KEY_LEFT_GUI);
    } else if (command == "SHIFT") {
        keyboard.press(KEY_RIGHT_SHIFT);
    } else if (command == "ALT") {
        keyboard.press(KEY_LEFT_ALT);
    } else if (command == "CTRL" || command == "CONTROL") {
        keyboard.press(KEY_LEFT_CTRL);
    } else if (command == "F1" || command == "FUNCTION1") {
        keyboard.press(KEY_F1);
    } else if (command == "F2" || command == "FUNCTION2") {
        keyboard.press(KEY_F2);
    } else if (command == "F3" || command == "FUNCTION3") {
        keyboard.press(KEY_F3);
    } else if (command == "F4" || command == "FUNCTION4") {
        keyboard.press(KEY_F4);
    } else if (command == "F5" || command == "FUNCTION5") {
        keyboard.press(KEY_F5);
    } else if (command == "F6" || command == "FUNCTION6") {
        keyboard.press(KEY_F6);
    } else if (command == "F7" || command == "FUNCTION7") {
        keyboard.press(KEY_F7);
    } else if (command == "F8" || command == "FUNCTION8") {
        keyboard.press(KEY_F8);
    } else if (command == "F9" || command == "FUNCTION9") {
        keyboard.press(KEY_F9);
    } else if (command == "F10" || command == "FUNCTION10") {
        keyboard.press(KEY_F10);
    } else if (command == "F11" || command == "FUNCTION11") {
        keyboard.press(KEY_F11);
    } else if (command == "F12" || command == "FUNCTION12") {
        keyboard.press(KEY_F12);
    }
}

void processLine(String line, USBHIDKeyboard keyboard) {
    /*
     * Process Ducky Script according to the official documentation (see https://github.com/hak5darren/USB-Rubber-Ducky/wiki/Duckyscript).
     *
     * (1) Commands without payload:
     *  - ENTER
     *  - MENU <=> APP
     *  - DOWNARROW <=> DOWN
     *  - LEFTARROW <=> LEFT
     *  - RIGHTARROW <=> RIGHT
     *  - UPARROW <=> UP
     *  - BREAK <=> PAUSE
     *  - CAPSLOCK
     *  - DELETE
     *  - END
     *  - ESC <=> ESCAPE
     *  - HOME
     *  - INSERT
     *  - NUMLOCK
     *  - PAGEUP
     *  - PAGEDOWN
     *  - PRINTSCREEN
     *  - SCROLLLOCK
     *  - SPACE
     *  - TAB
     *  - REPLAY (global commands aren't implemented)
     *
     * (2) Commands with payload:
     *  - DEFAULT_DELAY <=> DEFAULTDELAY (global commands aren't implemented.)
     *  - DELAY (+int)
     *  - STRING (+string)
     *  - GUI <=> WINDOWS (+char)
     *  - SHIFT (+char or key)
     *  - ALT (+char or key)
     *  - CTRL <=> CONTROL (+char or key)
     *  - REM (+string)
     *
     */

    int space = line.indexOf(' ');  // Find the first 'space' that'll be used to separate the payload from the command
    String command = "";
    String payload = "";

    if (space == -1) {  // There is no space -> (1)
        if (
            line == "ENTER" ||
            line == "MENU" || line == "APP" |
            line == "DOWNARROW" || line == "DOWN" ||
            line == "LEFTARROW" || line == "LEFT" ||
            line == "RIGHTARROW" || line == "RIGHT" ||
            line == "UPARROW" || line == "UP" ||
            line == "BREAK" || line == "PAUSE" ||
            line == "CAPSLOCK" ||
            line == "DELETE" ||
            line == "END" ||
            line == "ESC" || line == "ESCAPE" ||
            line == "HOME" ||
            line == "INSERT" ||
            line == "NUMLOCK" ||
            line == "PAGEUP" ||
            line == "PAGEDOWN" ||
            line == "PRINTSCREEN" ||
            line == "SCROLLLOCK" ||
            line == "SPACE" ||
            line == "TAB"
        ) {
            command = line;
        }
    } else {  // Has a space -> (2)
        command = line.substring(0, space);   // Get chars in line from start to space position
        payload = line.substring(space + 1);  // Get chars in line from after space position to EOL

        if (
            command == "DELAY" ||
            command == "STRING" ||
            command == "GUI" || command == "WINDOWS" ||
            command == "SHIFT" ||
            command == "ALT" ||
            command == "CTRL" || command == "CONTROL" ||
            command == "REM"
         ) { } else {
            // Invalid command
            command = "";
            payload = "";
         }
    }

    if (payload == "" && command != "") {                       // Command from (1)
        processCommand(keyboard, command);                                // Process command
    } else if (command == "DELAY") {                            // Delay before the next commande
        delay((int) payload.toInt());                           // Convert payload to integer and make pause for 'payload' time
    } else if (command == "STRING") {                           // String processing
        keyboard.print(payload);                                // Type-in the payload
    } else if (command == "REM") {                              // Comment
    } else if (command != "") {                                 // Command from (2)
        String remaining = line;                                // Prepare commands to run
        while (remaining.length() > 0) {                        // For command in remaining commands
            int space = remaining.indexOf(' ');                 // Find the first 'space' that'll be used to separate commands
            if (space != -1) {                                  // If this isn't the last command
                processCommand(keyboard, remaining.substring(0, space));  // Process command
                remaining = remaining.substring(space + 1);     // Pop command from remaining commands
            } else {                                            // If this is the last command
                processCommand(keyboard, remaining);                      // Pop command from remaining commands
                remaining = "";                                 // Clear commands (end of loop)
            }
        }
    } else {
        // Invalid command
    }

    keyboard.releaseAll();
}

short executeDucky(fs::FS &fs, USBHIDKeyboard& keyboard, char* errmsg, int maxErrMsg) {
      
      File file = fs.open(duckyfile_path);
      if (!file) {
        strncpy(errmsg, "Failed to open file for reading", maxErrMsg);
        return 3;
      }

      // read from the file and execute line by line
      String line = "";
      while (file.available()) {  // For each char in buffer
                // Read char from buffer
                char c = file.read();
    
                // Process char
                if ((int) c == 0x0a){                 // Line ending (LF) reached
                    processLine(line, keyboard);      // Process script line by reading command and payload
                    line = "";                        // Clean the line to process next
                } else if((int) c != 0x0d) {          // If char isn't a carriage return (CR)
                    line += c;                        // Put char into line
                }
            }

      return 0;

}
