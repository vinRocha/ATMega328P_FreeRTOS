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

#ifndef TRANSPORT_ESP8266_H
#define TRANSPORT_ESP8266_H

#ifdef __cplusplus
extern "C" {
#endif

#include "transport_interface.h"
#include "FreeRTOS.h"

typedef enum esp8266TransportStatus {
    ESP8266_TRANSPORT_SUCCESS = 1,           /**< Function successfully completed. */
    ESP8266_TRANSPORT_INVALID_PARAMETER = 2, /**< At least one parameter was invalid. */
    ESP8266_TRANSPORT_CONNECT_FAILURE = 3    /**< Initial connection to the server failed. */
} esp8266TransportStatus_t;

void prvCreateTransportTasks(UBaseType_t uxPriority, char taskLED);

//pHostName must be the target ipv4 address, port is the TCP target port number.
esp8266TransportStatus_t esp8266AT_Connect(const char *pHostName, const char *port);
esp8266TransportStatus_t esp8266AT_Disconnect(void);

int32_t esp8266AT_recv(NetworkContext_t *pNetworkContext,
                        void *pBuffer,
                        size_t bytesToRecv);

int32_t esp8266AT_send(NetworkContext_t *pNetworkContext,
                        const void *pBuffer,
                        size_t bytesToSend);


#ifdef __cplusplus
}
#endif

#endif //TRANSPORT_ESP8266_H
