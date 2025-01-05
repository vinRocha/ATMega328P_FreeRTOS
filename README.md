This branch holds the port of 202411.00 FreeRTOS AVR_ATMega323_WinAVR demo app for ATMega328P_GCC.

## Getting started

1. Clone the FreeRTOS project using the 202411.00 tag:
        `$ git clone -b 202411.00 https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules\`

2. In `FreeRTOS/FreeRTOS` directory clone this repo:
        `$ cd FreeRTOS/FreeRTOS; git clone -b portFrom_atmega323 https://github.com/vinRocha/ATMega328P_FreeRTOS.git AVR_ATMega328P_GCC`

3. Install [AVR GNU GCC compiler](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers) in your environment.

4.  In `FreeRTOS/FreeRTOS/AVR_ATMega328P_GCC` run `make` or `make all`.

5. Flash rtosdemo.hex in your ATMega328P.

For details, see https://sobremaquinas.wordpress.com/2025/01/02/starting-with-freertos/
