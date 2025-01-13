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

#include <string.h>
#include "transport_esp8266.h"
#include "serial.h"

//Below includes will change in FreeRTOS implementation
#include <cstdlib> //exit()
#include <cstdio> //perror()
#include <errno.h> //errno
#include <unistd.h> //usleep()
#include <mqueue.h>
#include <pthread.h>
#define SLEEP usleep(200000)

/* As networking data and control data all comes from
 * same UART interface, rxThread will be responsible to
 * collect them all and populate in two different queues
 * accordingly. dataQueue and controlQueue. The transport
 * program shall consume data from these buffers.
 */
static pthread_t thread_id; //in FreeRTOS this will be an high priority task.
static void *rxThread(void *args);
static mqd_t controlQTx, controlQRx, dataQTx, dataQRx;
static const char *control_mq_name = "/esp8266_control";
static const char *data_mq_name = "/esp8266_data";

//constants
const int BUFFER_LEN = 128; //rx is double buffered;
const unsigned long BAUD_RATE = 115200;
const TickType_t RX_BLOCK = 0xff;
const TickType_t TX_BLOCK = 0x00;
const TickType_t NO_BLOCK = 0x00;
const int AT_REPLY_LEN = 7;

enum transportStatus {
    AT_UNINITIALIZED = 0,
    MQUEUE_UNINITIALIZED,
    RX_THREAD_UNINITIALIZED,
    AT_READY,
    CONNECTED,
    ERROR
};

static char esp8266_status = AT_UNINITIALIZED;

static void check_AT(void);
static void start_TCP(const char *pHostName, const char *port);
static void send_to_controlQ(int n, const char *c);

esp8266TransportStatus_t esp8266AT_Connect(const char *pHostName, const char *port) {

    struct mq_attr mqstat;
    memset(&mqstat, 0, sizeof(struct mq_attr));
    mqstat.mq_maxmsg = BUFFER_LEN / 2;
    mqstat.mq_msgsize = 1;

    if (esp8266_status == CONNECTED) {
        return ESP8266_TRANSPORT_SUCCESS;
    }

    if (esp8266_status == AT_UNINITIALIZED) {
        xSerialPortInitMinimal(BAUD_RATE, BUFFER_LEN);
        esp8266_status = MQUEUE_UNINITIALIZED;
    }

    if (esp8266_status == MQUEUE_UNINITIALIZED) {
        controlQTx = mq_open(control_mq_name, O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR, &mqstat);
        controlQRx = mq_open(control_mq_name, O_RDONLY | O_NONBLOCK);
        mqstat.mq_maxmsg = BUFFER_LEN;
        dataQTx = mq_open(data_mq_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR, &mqstat);
        dataQRx = mq_open(data_mq_name, O_RDONLY | O_NONBLOCK);
        if ((controlQTx == (mqd_t) -1) || (controlQRx == (mqd_t) -1) || 
            (dataQTx == (mqd_t) -1)    || (dataQRx == (mqd_t) -1)) {
            perror("Error creating mqueues.");
            exit(errno);
        }
        esp8266_status = RX_THREAD_UNINITIALIZED;
    }

    if (esp8266_status == RX_THREAD_UNINITIALIZED) {
        if (pthread_create(&thread_id, NULL, &rxThread, NULL)) {
            return ESP8266_TRANSPORT_CONNECT_FAILURE;
        }
        esp8266_status = AT_READY;
    }

    if (esp8266_status > RX_THREAD_UNINITIALIZED) {
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

    return ESP8266_TRANSPORT_CONNECT_FAILURE;
}

esp8266TransportStatus_t esp8266AT_Disconnect(void) {
    esp8266_status = AT_UNINITIALIZED;
    pthread_join(thread_id, NULL);
    mq_close(controlQTx);
    mq_close(controlQRx);
    mq_close(dataQTx);
    mq_close(dataQRx);
    vSerialClose(NULL);
    mq_unlink(control_mq_name);
    mq_unlink(data_mq_name);
    return ESP8266_TRANSPORT_SUCCESS;
}

int32_t esp8266AT_recv(NetworkContext_t *pNetworkContext, void *pBuffer, size_t bytesToRecv) {
    int32_t bytes_read = 0;
    char byte;

    while(bytes_read < (int32_t) bytesToRecv) {
        if ((mq_receive(dataQRx, &byte, 1, NULL) > 0)) {
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
        //Should check for errors here, but for now, just send the data.
        SLEEP; //so ESP8266 can process the AT COMMAND;
        for (int i = 0; i < 2048; i++) {
            while(!xSerialPutChar(NULL, *((signed char*) pBuffer + bytes_sent), TX_BLOCK));
            bytes_sent++;
            bytesToSend--;
        }
        SLEEP;
        //Should check for errors here... but for now, only clear control buffer.
        while (mq_receive(controlQRx, &c, 1, NULL) > 0);
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

    SLEEP;
    //Should check for errors here... but for now, only clear control buffer.
    while (mq_receive(controlQRx, &c, 1, NULL) > 0);

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

    SLEEP; //so serial interface has enough time to receive echo.
    //Clear control buffer, if anything is there
    while (mq_receive(controlQRx, at_cmd_response, 1, NULL) > 0);

    //Complete the command
    xSerialPutChar(NULL, '\n', TX_BLOCK);

    SLEEP; //delay to receive response on serial interface
    for (int i = 0; i < AT_REPLY_LEN - 1;) {
        if (mq_receive(controlQRx, &at_cmd_response[i], 1, NULL) > 0) {
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
    SLEEP;
    //Clear rx control buffer
    while (mq_receive(controlQRx, &c, 1, NULL) > 0);

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

    SLEEP; //so esp8266 has enough time to reply us.
    mq_receive(controlQRx, &c, 1, NULL); //C, if success

    if (c != 'C') {
        esp8266_status = ERROR;
    }
    else {
        esp8266_status = CONNECTED;
    }
    //Clear rx control buffer
    while (mq_receive(controlQRx, &c, 1, NULL) > 0);
    return;
}

void *rxThread(void *args) {

    char c[10]; //convert up to 9 decimal digits to int32_t;
    int32_t data_lenght; //in bytes.... that can count a lot of data....
    unsigned char index;

    //Block until we update esp8266_status in the main therad to AT_READY;
    while(esp8266_status == RX_THREAD_UNINITIALIZED);

    //Very ugly code, but it works...
    //Keep running till esp8266AT_Disconnect() is called;
    while(esp8266_status > RX_THREAD_UNINITIALIZED) {
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
                                    mq_send(dataQTx, c, 1, 0);
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
                mq_send(controlQTx, c, 1, 0);
            }
        }
    }

    return NULL;
}

void send_to_controlQ(int n, const char *c) {
    for(int i = 0; i < n; i++) {
        mq_send(controlQTx, c + i, 1, 0);
    }
    return;
}
