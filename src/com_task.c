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
#define mSTACK_SIZE                     configMINIMAL_STACK_SIZE + mBUFFER_LEN * 2 + 36

#define mNO_BLOCK                       (TickType_t) 0
#define mDELAY_MS(x)                    (TickType_t) (x / portTICK_PERIOD_MS)

static void vComTxTask(void *pvParameters);
static char mLED;

BaseType_t createComTask(StackType_t stackSize, UBaseType_t priority, unsigned long ulBaudRate,
        QueueHandle_t dataQueue, char taskLED) {

    mLED = taskLED;
    xSerialPortInitMinimal(ulBaudRate, serialBUFFER_LEN);
    if (xTaskCreate(vComTxTask, "COMTx", stackSize, (void*) dataQueue, priority, NULL) != pdPASS)
        return pdFAIL;
    return pdPASS;
}
/*-----------------------------------------------------------*/

void vComTxTask(void *pvParameters) {

    float distance;
    //"distance: 000.000\r\n" - size: 20
    char msg[mBUFFER_LEN];
    QueueHandle_t dataQueue = (QueueHandle_t)pvParameters;

    for (;;) {
        digitalIOToggle(mLED);
        if (xQueueReceive(dataQueue, &distance, mDELAY_MS(200))) {
            snprintf(msg, mBUFFER_LEN, "distance: %7.3f\r\n", distance);
            for (int i = 0; msg[i]; i++) {
                xSerialPutChar(NULL, msg[i], mNO_BLOCK);
            }
        }
    }
}
/*-----------------------------------------------------------*/
