/*
 * MIT License
 * Copyright (c) 2024 Vinicius Silva.  All Rights Reserved.
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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "com_task.h"
#include "transport_esp8266.h"
#include "drivers/digital_io.h"
#include "hcsr04.h"

#define mLED                 mLED_1 //Debugging LED, PORTD bit 3

void comTask(void *pvParameters) {

    hcsr04_t interval;

    while (esp8266AT_Connect("192.168.0.235", "1883") != ESP8266_TRANSPORT_SUCCESS) {
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    for (;;) {
        digitalIOToggle(mLED);
        if (xQueueReceive((QueueHandle_t) pvParameters, &interval, pdMS_TO_TICKS(5000))) {
            interval = uxTaskGetStackHighWaterMark(NULL);
            esp8266AT_send(NULL, "+", 1);
            esp8266AT_send(NULL, &interval, sizeof(interval));
            //consume received data....
            while(esp8266AT_recv(NULL, &interval, sizeof(interval)));
        }
    }
}
/*-----------------------------------------------------------*/
