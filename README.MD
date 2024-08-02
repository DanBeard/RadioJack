                                                                   
,------.            ,--.,--.            ,--.              ,--.     
|  .--. ' ,--,--. ,-|  |`--' ,---.      |  | ,--,--. ,---.|  |,-.  
|  '--'.'' ,-.  |' .-. |,--.| .-. |,--. |  |' ,-.  || .--'|     /  
|  |\  \ \ '-'  |\ `-' ||  |' '-' '|  '-'  /\ '-'  |\ `--.|  \  \  
`--' '--' `--`--' `---' `--' `---'  `-----'  `--`--' `---'`--'`--' 
                                                                   


## Install Instructions
First follow the instructions for the Arduino environment at https://github.com/Xinyuan-LilyGO/T-Dongle-S3/tree/main . Note: Install ESP32 version 2.0.3 ONLY -- other versions fail for unknown reason *shrug*

Once the provided example firmware is compiling and flashing, open the RadioJack Arduino project. Compile and flash. 

## Usage Instructions
Connect to the WIFI SSD shown on the screen. It should be unique per device. The SSID password is defined in the code and can be changed.
Telnet to the IP shown on the screen. Follow the intuitive text-based interface!

### Troubleshooting notes

If the skull animation stops laughing it means the board has frozen. Power cycle it.

If you want to reflash the board, then hold the button while you plug it into your USB port and let go AFTER it has been fully plugged in. 

## Disclaimer
This code is provided for educational purposes only.  It is intended for authorized auditing and security analysis purposes only where permitted. Users are solely responsible for compliance with all laws of their locality. Authors of this repository claim no responsibility for unauthorized or unlawful use.




