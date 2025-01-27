This application is based on FreeRTOS [MQTT Plain Text demo](https://freertos.org/Documentation/03-Libraries/03-FreeRTOS-core/02-coreMQTT/02-Demos/01-coreMQTT-demo). It utilizes FreeRTOS coreMQTT library to connect to a MQTT broker and send/receive data. It then perform some actions based on the data received.

This application is target to Arduino UNO R3 HW and requires an WiFi module that supports AT commands (such as ESP01) for networking.

This app does not perform client/server authenticaiton and exchanges informaiton in plain text. Thus, it is recommended for educational purposes only.

## Getting started

1. Clone the FreeRTOS project using the 202411.00 tag:
        `$ git clone -b 202411.00 https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules\`

2. In `FreeRTOS/FreeRTOS` directory clone this repo:
        `$ cd FreeRTOS/FreeRTOS; git clone https://github.com/vinRocha/ATMega328P_FreeRTOS.git AVR_ATMega328P_GCC`

3. Install [AVR GNU GCC compiler](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers) in your environment.

4.  In `FreeRTOS/FreeRTOS/AVR_ATMega328P_GCC` run `make` or `make all`.

5. Flash rtosdemo.hex in your ATMega328P.

For details, see https://sobremaquinas.wordpress.com/2025/01/02/starting-with-freertos/
