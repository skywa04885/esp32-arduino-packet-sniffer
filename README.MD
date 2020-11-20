# WiFi LoRa Collector / Forwarder

This repository contains the source code for the Cybox WiFi & LoRa MAC address
collection system. In the current repository we use C++, but mostly use the C
style since the original code before LoRa was written in it.

## How to use

1. Install VSCode
2. Go to extensions, and install PlatformIO IDE
3. Clone the current git repository
4. Open the git repository in VSCode
5. Modify the default.h file to meet requirements, see 'Help: default.h'
6. Once done, go to the bottom bar, and press the arrow which says 'Platformio upload'

## Help: default.h

The default.h file contains all options which are required before building and flashing,
here I give an description on what definitions are used, and what they mean.

To disable USART debug mode, comment out: '#define GLOBAL_DEBUG'

Pre-compile config:
1. 'GLOBAL_MEASUREMENT_BUFFER_SIZE': the number of measurements kept until transmission
1. 'GLOBAL_CHANNEL_SWITCH_DELAY': The delay in ms between channel switching
1. 'GLOBAL_RECEIVER_BUFFER_SIZE': the size of the LoRa receive buffer
1. 'GLOBAL_USART_BAUD': The serial baud rate
1. 'BAND': The LoRa frequency band

Device information:
1. 'DEVICE_MAC': the unique address of the device
1. 'GATEWAY_MAC': the mac of the gateway ( another ESP32 )
1. 'GLOBAL_WIFI_SSID': the SSID of the wifi network ( required in gateway only )
1. 'GLOBAL_WIFI_PASS': The password of the wifi network ( required in gateway only )
1. 'GLOBAL_API_KEY': The API key for the MAC collectors ( required in transmitter )
1. 'GLOBAL_SERVER_IP': The server which performs the parsing of packets ( required in transmitter )
1. 'GLOBAL_SERVER_PORT': The port of the packet parsing server ( required in transmitter )

