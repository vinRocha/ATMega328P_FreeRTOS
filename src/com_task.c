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

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "com_task.h"
#include "drivers/serial.h"
#include "drivers/digital_io.h"

#define serialBUFFER_LEN                16
#define mBUFFER_LEN                     20
#define mSTACK_SIZE                     configMINIMAL_STACK_SIZE + mBUFFER_LEN * 2

#define mNO_BLOCK                       (TickType_t) 0
#define mDELAY_MS(x)                    (TickType_t) (x / portTICK_PERIOD_MS)

static void vComTxTask(void *pvParameters);
static char mLED;

void prvStartComTask(UBaseType_t uxPriority, unsigned long ulBaudRate,
        QueueHandle_t dataQueue, char taskLED) {

    mLED = taskLED;
    xSerialPortInitMinimal(ulBaudRate, serialBUFFER_LEN);
    xTaskCreate(vComTxTask, "COMTx", mSTACK_SIZE, (void*) dataQueue, uxPriority, NULL);
}
/*-----------------------------------------------------------*/

static portTASK_FUNCTION(vComTxTask, pvParameters) {
 
    //distance: 00.000
    char msg[mBUFFER_LEN];
 
    float distance;
    QueueHandle_t dataQueue = (QueueHandle_t)pvParameters;

    for (;;) {
        digitalIOToggle(mLED);
        if (xQueueReceive(dataQueue, &distance, mDELAY_MS(200))) {
            snprintf(msg, mBUFFER_LEN, "distance: %6.3f\r\n", distance);
            for (int i = 0; msg[i]; i++) {
                xSerialPutChar(NULL, msg[i], mNO_BLOCK);
            }
        }
    }
}
/*-----------------------------------------------------------*/
