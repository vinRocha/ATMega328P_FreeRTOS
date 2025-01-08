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

#define comBUFFER_LEN                  20
#define comSTACK_SIZE                  configMINIMAL_STACK_SIZE + (comBUFFER_LEN / 2) + 16

#define comRX_LED_OFFSET               0
#define comTX_LED_OFFSET               1

#define comNO_BLOCK                    (TickType_t) 0
#define com500_MS_DELAY               (TickType_t) (500 / portTICK_PERIOD_MS)

/* The transmit task */
static portTASK_FUNCTION_PROTO(vComTxTask, pvParameters);

/* The LED that should be toggled by the Rx and Tx tasks.  The Rx task will
 * toggle LED ( uxBaseLED + comRX_LED_OFFSET).  The Tx task will toggle LED
 * ( uxBaseLED + comTX_LED_OFFSET ). */
//static UBaseType_t uxBaseLED = 0;

void prvStartComTask(UBaseType_t uxPriority, unsigned long ulBaudRate, QueueHandle_t dataQueue) {
    xSerialPortInitMinimal(ulBaudRate, comBUFFER_LEN);
    xTaskCreate(vComTxTask, "COMTx", comSTACK_SIZE, (void*) dataQueue, uxPriority, (TaskHandle_t *) NULL);
}
/*-----------------------------------------------------------*/

static portTASK_FUNCTION(vComTxTask, pvParameters) {
    //distance: 00.000
    char msg[comBUFFER_LEN];
 
    float distance;
    QueueHandle_t dataQueue = (QueueHandle_t)pvParameters;

    for (;;) {
        if (xQueueReceive(dataQueue, &distance, com500_MS_DELAY)) {
            snprintf(msg, comBUFFER_LEN, "distance: %.3f\r\n", distance);
            for (int i = 0; msg[i]; i++) {
                xSerialPutChar(NULL, msg[i], comNO_BLOCK);
            }
        }
    }
}
/*-----------------------------------------------------------*/
