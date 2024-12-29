    ,------.            ,--.,--.            ,--.              ,--.     
    |  .--. ' ,--,--. ,-|  |`--' ,---.      |  | ,--,--. ,---.|  |,-.  
    |  '--'.'' ,-.  |' .-. |,--.| .-. |,--. |  |' ,-.  || .--'|     /  
    |  |\  \ \ '-'  |\ `-' ||  |' '-' '|  '-'  /\ '-'  |\ `--.|  \  \  
    `--' '--' `--`--' `---' `--' `---'  `-----'  `--`--' `---'`--'`--' 
                                                            


## Install Instructions
First follow the instructions for the Arduino environment at https://github.com/Xinyuan-LilyGO/T-Dongle-S3/tree/main . 

Note: Install ESP32 version 2.0.14 or earlier. Anything after 2.0.14 is not supported by the T-Dongle at time of writing (check the GitHub above for most recent notes)

Once the provided example firmware is compiling and flashing, open the RadioJack Arduino project. Compile and flash. 

### Optional Meshtastic support
If you want to be able to use the meshtastic bridge then make sure that https://github.com/meshtastic/Meshtastic-arduino is also in your arduini library folder 
and MESH_BRIDGE_ENABLED macro is defined. Further customization (Channel index, payload length, etc) is controlled via macors in MestasticWifiBridgeClient.h )

## Usage Instructions
Connect to the WIFI SSD shown on the screen. It should be unique per device. The SSID password is defined in the code. The password is "thisisthepassword" by default and can be changed.
Telnet to the IP shown on the screen. Follow the intuitive text-based interface!

### Troubleshooting notes

If the skull animation stops laughing it means the board has frozen. Power cycle it.

If you want to reflash the board, then hold the button while you plug it into your USB port and let go AFTER it has been fully plugged in. 

## Disclaimer
This code is provided for educational purposes only.  It is intended for authorized auditing and security analysis purposes only where permitted. Users are solely responsible for compliance with all laws of their locality. Authors of this repository claim no responsibility for unauthorized or unlawful use.




