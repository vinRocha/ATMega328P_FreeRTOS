/*
 * Copyright (C) 2024 silva.viniciusr@gmail.com,  all rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "transport_esp8266.h"
#include "FreeRTOS.h"
#include "task.h"
#include "drivers/serial.h"
#include "drivers/digital_io.h"

#define SLEEP                           vTaskDelay(pdMS_TO_TICKS(200))
#define BLOCK_MS(x)                     pdMS_TO_TICKS(x)
#define NO_BLOCK                        0x00
#define TX_BLOCK                        0x00
#define RX_BLOCK                        BLOCK_MS(10)

#define mLED                            mLED_2

#define BAUD_RATE                       115200
#define BUFFER_LEN                      128
#define AT_REPLY_LEN                    7

enum transportStatus {
    COM_UNINITIALIZED = 0,
    COM_INITIALIZED,
    AT_READY,
    CONNECTED,
    ERROR
};

static char esp8266_status = COM_UNINITIALIZED;

static void check_AT(void);
static void start_TCP(const char *pHostName, const char *port);
static void stop_TCP(void);
static UBaseType_t rxByte(char *byte, TickType_t block, UBaseType_t control);

void esp8266Initialise(void) {

    xSerialPortInitMinimal(BAUD_RATE, BUFFER_LEN);
    esp8266_status = COM_INITIALIZED;
}

esp8266TransportStatus_t esp8266AT_Connect(const char *pHostName, const char *port) {

    if (esp8266_status == CONNECTED) {
        return ESP8266_TRANSPORT_SUCCESS;
    }

    if (esp8266_status != COM_INITIALIZED) {
        return ESP8266_TRANSPORT_CONNECT_FAILURE;
    }

    check_AT();
    if (esp8266_status == ERROR) {
        return ESP8266_TRANSPORT_CONNECT_FAILURE;
    }

    start_TCP(pHostName, port);
    if (esp8266_status == ERROR) {
        return ESP8266_TRANSPORT_CONNECT_FAILURE;
    }

    return ESP8266_TRANSPORT_SUCCESS;
}

esp8266TransportStatus_t esp8266AT_Disconnect(void) {
    if (esp8266_status != CONNECTED) {
        return ESP8266_TRANSPORT_CONNECT_FAILURE;
    }
    stop_TCP();
    return ESP8266_TRANSPORT_SUCCESS;
}

int32_t esp8266AT_recv(NetworkContext_t *pNetworkContext, void *pBuffer, size_t bytesToRecv) {
    int32_t bytes_read = 0;
    char byte;

    while(bytes_read < (int32_t) bytesToRecv) {
        if (rxByte(&byte, RX_BLOCK, pdFALSE)) {
            *((char*) pBuffer + bytes_read) = byte;
            bytes_read++;
        }
        else {
            break;
        }
    }

    return bytes_read;
}

int32_t esp8266AT_send(NetworkContext_t *pNetworkContext, const void *pBuffer, size_t bytesToSend) {

    //In a single ATSEND command, we can send up to 2048 bytes at a time;
    int32_t bytes_sent = 0;
    char command[] = "AT+CIPSEND=2048";
    char c;

    while (bytesToSend / 2048) {
        //Send AT command
        for(int i = 0; command[i]; i++) {
            xSerialPutChar(NULL, command[i], TX_BLOCK);
        }
        xSerialPutChar(NULL, '\r', TX_BLOCK);
        xSerialPutChar(NULL, '\n', TX_BLOCK);
        SLEEP; //so ESP8266 can process the AT COMMAND;
        //Should check for errors here, but for now, just send the data.
        for (int i = 0; i < 2048; i++) {
            while(!xSerialPutChar(NULL, *((signed char*) pBuffer + bytes_sent), TX_BLOCK));
            bytes_sent++;
            bytesToSend--;
        }
        //Should check for errors here... but for now, only clear control buffer.
        while (rxByte(&c, BLOCK_MS(200), pdTRUE) > 0);
    }

    snprintf(&command[11], 5, "%u", bytesToSend);
    //Send AT command
    for(int i = 0; command[i]; i++) {
        xSerialPutChar(NULL, command[i], TX_BLOCK);
    }
    xSerialPutChar(NULL, '\r', TX_BLOCK);
    xSerialPutChar(NULL, '\n', TX_BLOCK);
    SLEEP; //so ESP8266 can process the AT COMMAND;
    //Should check for errors here, but for now, just send the data.
    for (; bytesToSend > 0; bytesToSend--) {
        while(!xSerialPutChar(NULL, *((signed char*) pBuffer + bytes_sent), TX_BLOCK));
        bytes_sent++;
    }
    //Should check for errors here... but for now, only clear control buffer.
    while (rxByte(&c, BLOCK_MS(200), pdTRUE) > 0);

    return bytes_sent;
}

void check_AT(void) {

    char at_cmd_response[AT_REPLY_LEN] = {0};

    //Send AT command
    xSerialPutChar(NULL, 'A', TX_BLOCK);
    xSerialPutChar(NULL, 'T', TX_BLOCK);
    xSerialPutChar(NULL, 'E', TX_BLOCK); //
    xSerialPutChar(NULL, '0', TX_BLOCK); // Disable echo
    xSerialPutChar(NULL, '\r', TX_BLOCK);
    //Clear control buffer, if anything is there
    while (rxByte(at_cmd_response, BLOCK_MS(200), pdTRUE) > 0);

    //Complete the command
    xSerialPutChar(NULL, '\n', TX_BLOCK);

    for (int i = 0; i < AT_REPLY_LEN - 1;) {
        if (rxByte(&at_cmd_response[i], RX_BLOCK, pdTRUE) > 0) {
            i++;
        }
    }

    if(strcmp(at_cmd_response, "\r\nOK\r\n")) {
        esp8266_status = ERROR;
    }
    else {
       esp8266_status = AT_READY;
    }
    return;
}

void start_TCP(const char *pHostName, const char *port) {

    char c;

    //Close existing TCP connection, if any
    stop_TCP();

    //AT header to start TCP connection
    xSerialPutChar(NULL, 'A', TX_BLOCK);
    xSerialPutChar(NULL, 'T', TX_BLOCK);
    xSerialPutChar(NULL, '+', TX_BLOCK);
    xSerialPutChar(NULL, 'C', TX_BLOCK);
    xSerialPutChar(NULL, 'I', TX_BLOCK);
    xSerialPutChar(NULL, 'P', TX_BLOCK);
    xSerialPutChar(NULL, 'S', TX_BLOCK);
    xSerialPutChar(NULL, 'T', TX_BLOCK);
    xSerialPutChar(NULL, 'A', TX_BLOCK);
    xSerialPutChar(NULL, 'R', TX_BLOCK);
    xSerialPutChar(NULL, 'T', TX_BLOCK);
    xSerialPutChar(NULL, '=', TX_BLOCK);
    xSerialPutChar(NULL, '"', TX_BLOCK);
    xSerialPutChar(NULL, 'T', TX_BLOCK);
    xSerialPutChar(NULL, 'C', TX_BLOCK);
    xSerialPutChar(NULL, 'P', TX_BLOCK);
    xSerialPutChar(NULL, '"', TX_BLOCK);
    xSerialPutChar(NULL, ',', TX_BLOCK);
    xSerialPutChar(NULL, '"', TX_BLOCK);

    //Target IP
    for (int i = 0; *(pHostName + i); i++) {
        xSerialPutChar(NULL, *(pHostName + i), TX_BLOCK);
    }

    xSerialPutChar(NULL, '"', TX_BLOCK);
    xSerialPutChar(NULL, ',', TX_BLOCK);

    //Target TCP Port
    for (int i = 0; *(port + i); i++) {
        xSerialPutChar(NULL, *(port + i), TX_BLOCK);
    }
    xSerialPutChar(NULL, '\r', TX_BLOCK);
    xSerialPutChar(NULL, '\n', TX_BLOCK);

    rxByte(&c, BLOCK_MS(200), pdTRUE); //C, if success

    if (c != 'C') {
        esp8266_status = ERROR;
    }
    else {
      esp8266_status = CONNECTED;
    }
    //Clear rx control buffer
    while (rxByte(&c, RX_BLOCK, pdTRUE) > 0);
}

void stop_TCP() {

    char c;
    //Close existing TCP connection, if any
    xSerialPutChar(NULL, 'A', TX_BLOCK);
    xSerialPutChar(NULL, 'T', TX_BLOCK);
    xSerialPutChar(NULL, '+', TX_BLOCK);
    xSerialPutChar(NULL, 'C', TX_BLOCK);
    xSerialPutChar(NULL, 'I', TX_BLOCK);
    xSerialPutChar(NULL, 'P', TX_BLOCK);
    xSerialPutChar(NULL, 'C', TX_BLOCK);
    xSerialPutChar(NULL, 'L', TX_BLOCK);
    xSerialPutChar(NULL, 'O', TX_BLOCK);
    xSerialPutChar(NULL, 'S', TX_BLOCK);
    xSerialPutChar(NULL, 'E', TX_BLOCK);
    xSerialPutChar(NULL, '\r', TX_BLOCK);
    xSerialPutChar(NULL, '\n', TX_BLOCK);
    //Clear rx control buffer
    while (rxByte(&c, BLOCK_MS(200), pdTRUE) > 0);
    esp8266_status = COM_INITIALIZED;
}

UBaseType_t rxByte(char *byte, TickType_t block, UBaseType_t control) {

    char c[6]; //convert up to 5 decimal digits to int16_t;
    static uint16_t data_length = 0; //in bytes.... that can count a lot of data....
    unsigned char index;

    digitalIOToggle(mLED);

    if (control) {
        return xSerialGetChar(NULL, (signed char*) byte, block);
    }

    if (data_length) {
        data_length--;
        return xSerialGetChar(NULL, (signed char*) byte, block);
    }

    //Very ugly code, but it works...
    xSerialGetChar(NULL, (signed char*) &c[0], block);
    if (c[0] == '+') {
        while(!xSerialGetChar(NULL, (signed char*) &c[1], RX_BLOCK));
        if (c[1] == 'I') {
            while(!xSerialGetChar(NULL, (signed char*) &c[2], RX_BLOCK));
            if (c[2] == 'P') {
                while(!xSerialGetChar(NULL, (signed char*) &c[3], RX_BLOCK));
                if (c[3] == 'D') {
                    while(!xSerialGetChar(NULL, (signed char*) &c[4], RX_BLOCK));
                    if (c[4] == ',') { //Hit Magic header!! We got data!!
                        c[5] = 0;
                        index = 0;
                        while(index < 6) {
                            while(!xSerialGetChar(NULL, (signed char*) &c[5], RX_BLOCK));
                            if (c[5] == ':') {
                                c[++index] = 0;
                                break;
                            }
                            c[index++] = c[5];
                        }
                        data_length = atoi(c);
                        data_length--;
                        return xSerialGetChar(NULL, (signed char*) byte, RX_BLOCK);
                    }
                }
            }
        }
    }
    return pdFAIL;
}
