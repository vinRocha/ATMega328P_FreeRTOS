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

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sensor_task.h"
#include "drivers/digital_io.h"

#define mSTACK_SIZE                     configMINIMAL_STACK_SIZE

#define mNO_BLOCK                       (TickType_t) 0
#define mDELAY_MS(x)                    (TickType_t) (x / portTICK_PERIOD_MS)

#define TRIG_PIN                        6    //PORTB bit 4;
#define ECHO_PIN                        0x01 //PORTB bit 0;

static void vSensorTask(void *pvParameters);
static void delayMicrosecond(char x);
static char mLED;

void prvCreateSensorTask(UBaseType_t uxPriority, QueueHandle_t dataQueue, char taskLED) {

    mLED = taskLED;
    xTaskCreate(vSensorTask, "Sensor", mSTACK_SIZE, (void*) dataQueue, uxPriority, NULL);
}
/*-----------------------------------------------------------*/

void vSensorTask(void *pvParameters) {
 
    float distance;
    unsigned int interval, timeout;
    QueueHandle_t dataQueue = (QueueHandle_t) pvParameters;

    for (;;) {
        interval = 0;
        timeout = 0;
        digitalIOToggle(mLED);
        digitalIOSet(TRIG_PIN, pdTRUE);
        delayMicrosecond(8);
        digitalIOSet(TRIG_PIN, pdFALSE);
        while(!(PINB & ECHO_PIN) && (timeout < 65535)) {
            timeout++;
        }
        while((PINB & ECHO_PIN) && (interval < 65535)) {
            delayMicrosecond(1);
            interval += 1;
        }
        interval += (interval/520 * 58); //correction factor 
        distance = ((float) interval * 0.0343) / 2;
        xQueueSend(dataQueue, &distance, mDELAY_MS(500));
        vTaskDelay(mDELAY_MS(3000));
    }
}
/*-----------------------------------------------------------*/

void delayMicrosecond(char x) {
    static volatile char a;
    a = x - 1;
    if (a > 0)
        delayMicrosecond(a);
    return;
}
