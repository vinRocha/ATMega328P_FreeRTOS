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
#include "portmacro.h"
#include "task.h"
#include "queue.h"
#include "drivers/serial.h"
#include "drivers/digital_io.h"

#define DELAY_MS(x)                     (TickType_t) (x / portTICK_PERIOD_MS)
#define SLEEP                           vTaskDelay(DELAY_MS(200))
#define BLOCK_MS(x)                     DELAY_MS(x)
#define NO_BLOCK                        0x00
#define TX_BLOCK                        0x00
#define RX_BLOCK                        BLOCK_MS(10)

//constants
#define mSTACK_SIZE                     112
const unsigned long BAUD_RATE =         115200;
const int BUFFER_LEN =                  128;
const int AT_REPLY_LEN =                7;

enum transportStatus {
    AT_UNINITIALIZED = 0,
    QUEUES_INITIALIZED,
    RX_THREAD_INITIALIZED,
    AT_READY,
    CONNECTED,
    ERROR
};

/* As networking data and control data all comes from same UART interface,
 * COMRx will be responsible to collect them all and populate in two 
 * different queues accordingly, dataQueue and controlQueue. The transport
 * program shall consume data from these buffers.
 */
static QueueHandle_t controlQ;
static QueueHandle_t dataQ;
static char esp8266_status = AT_UNINITIALIZED;
static char mLED;

static void rxThread(void *args);
static void check_AT();
static void start_TCP(const char *pHostName, const char *port);
static void stop_TCP();
static void send_to_controlQ(int n, const char *c);

esp8266TransportStatus_t prvCreateTransportTasks(UBaseType_t uxPriority, char taskLED) {

    mLED = taskLED;
    xSerialPortInitMinimal(BAUD_RATE, BUFFER_LEN);
    controlQ = xQueueCreate(BUFFER_LEN/2, (UBaseType_t) sizeof(char));
    if (!controlQ)
        return ESP8266_TRANSPORT_CONNECT_FAILURE;
    dataQ = xQueueCreate(BUFFER_LEN, (UBaseType_t) sizeof(char));
    if (!dataQ)
        return ESP8266_TRANSPORT_CONNECT_FAILURE;
    if (xTaskCreate(rxThread, "COMRx", mSTACK_SIZE, NULL, uxPriority, NULL) != pdPASS)
        return ESP8266_TRANSPORT_CONNECT_FAILURE;
    esp8266_status = RX_THREAD_INITIALIZED;
    return ESP8266_TRANSPORT_SUCCESS;
}

esp8266TransportStatus_t esp8266AT_Connect(const char *pHostName, const char *port) {

    if (esp8266_status == CONNECTED) {
        return ESP8266_TRANSPORT_SUCCESS;
    }

    if (esp8266_status != RX_THREAD_INITIALIZED) {
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
        if (xQueueReceive(dataQ, &byte, BLOCK_MS(1))) {
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
        while (xQueueReceive(controlQ, &c, BLOCK_MS(200)) > 0);
    }

    snprintf(&command[11], 5, "%d", (int) bytesToSend);
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
    while (xQueueReceive(controlQ, &c, BLOCK_MS(200)) > 0);

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
    while (xQueueReceive(controlQ, at_cmd_response, BLOCK_MS(200)) > 0);

    //Complete the command
    xSerialPutChar(NULL, '\n', TX_BLOCK);

    for (int i = 0; i < AT_REPLY_LEN - 1;) {
        if (xQueueReceive(controlQ, &at_cmd_response[i], NO_BLOCK) > 0) {
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

    xQueueReceive(controlQ, &c, BLOCK_MS(200)); //C, if success

    if (c != 'C') {
        esp8266_status = ERROR;
    }
    else {
      esp8266_status = CONNECTED;
    }
    //Clear rx control buffer
    while (xQueueReceive(controlQ, &c, NO_BLOCK) > 0);
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
    while (xQueueReceive(controlQ, &c, BLOCK_MS(200)) > 0);
    esp8266_status = RX_THREAD_INITIALIZED;
}

void rxThread(void *args) {

    char c[10]; //convert up to 9 decimal digits to int32_t;
    int32_t data_lenght; //in bytes.... that can count a lot of data....
    unsigned char index;

    //Very ugly code, but it works...
    //Keep running forever!!! Tasks cannot return!!!
    for(;;) {
        if (xSerialGetChar(NULL, (signed char*) &c[0], RX_BLOCK)) {
            if (c[0] == '+') {
                while(!xSerialGetChar(NULL, (signed char*) &c[1], RX_BLOCK));
                if (c[1] == 'I') {
                    while(!xSerialGetChar(NULL, (signed char*) &c[2], RX_BLOCK));
                    if (c[2] == 'P') {
                        while(!xSerialGetChar(NULL, (signed char*) &c[3], RX_BLOCK));
                        if (c[3] == 'D') {
                            while(!xSerialGetChar(NULL, (signed char*) &c[4], RX_BLOCK));
                            if (c[4] == ',') { //Hit Magic header!! We got data!!
                                c[9] = 0;
                                index = 0;
                                while(index < 9) {
                                    while(!xSerialGetChar(NULL, (signed char*) &c[9], RX_BLOCK));
                                    if (c[9] == ':') {
                                        c[++index] = 0;
                                        break;
                                    }
                                    c[index++] = c[9];
                                }
                                data_lenght = atoi(c);
                                for (; data_lenght > 0; data_lenght--) {
                                    while(!xSerialGetChar(NULL, (signed char*) c, RX_BLOCK));
                                    xQueueSend(dataQ, c, BLOCK_MS(200));
                                }
                            }
                            else {
                                send_to_controlQ(5, c);
                            }
                        }
                        else {
                            send_to_controlQ(4, c);
                        }
                    }
                    else {
                        send_to_controlQ(3, c);
                    }
                }
                else {
                    send_to_controlQ(2, c);
                }
            }
            else {
                xQueueSend(controlQ, c, BLOCK_MS(200));
            }
        }
    }
}

void send_to_controlQ(int n, const char *c) {
    for(int i = 0; i < n; i++) {
        xQueueSend(controlQ, c + i, BLOCK_MS(200));
    }
    return;
}
