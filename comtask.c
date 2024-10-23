/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "comtask.h"
#include "serial/serial.h"
#include "digital_io/digital_io.h"

#define comBUFFER_LEN                  80
#define comSTACK_SIZE                  configMINIMAL_STACK_SIZE + (comBUFFER_LEN)

#define comTX_LED_OFFSET               1
#define comRX_LED_OFFSET               0

#define comNO_BLOCK                    (TickType_t) 0
#define com2000_MS_DELAY               (TickType_t) (2000 / portTICK_PERIOD_MS)

/* We only support one UART */
static xComPortHandle xPort = NULL;

/* The transmit task */
static portTASK_FUNCTION_PROTO(vComTxTask, pvParameters);

/* The LED that should be toggled by the Rx and Tx tasks.  The Rx task will
 * toggle LED ( uxBaseLED + comRX_LED_OFFSET).  The Tx task will toggle LED
 * ( uxBaseLED + comTX_LED_OFFSET ). */
static UBaseType_t uxBaseLED = 0;

void prvStartComTask(UBaseType_t uxPriority, unsigned long ulBaudRate) {
    xSerialPortInitMinimal(ulBaudRate, comBUFFER_LEN);
    xTaskCreate(vComTxTask, "COMTx", comSTACK_SIZE, NULL, uxPriority, (TaskHandle_t *) NULL);
}
/*-----------------------------------------------------------*/

static portTASK_FUNCTION(vComTxTask, pvParameters) {
    size_t heapFree;
    char line[comBUFFER_LEN] = {0};

    /* Just to stop compiler warnings. */
    (void) pvParameters;

    for (;;) {
        heapFree = xPortGetFreeHeapSize();
        snprintf(line, comBUFFER_LEN, "FreeHeapSize: %d\r\n", heapFree);

        for(int i = 0; line[i]; i++) {
            xSerialPutChar(xPort, line[i], comNO_BLOCK);
        }

        vTaskDelay(com2000_MS_DELAY);
    }
}
/*-----------------------------------------------------------*/