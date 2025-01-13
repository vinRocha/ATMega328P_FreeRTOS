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
#include "transport_esp8266.h"
#include "drivers/digital_io.h"

/* Priority definitions for tasks */
#define mCOM_TASK_PRIORITY              (tskIDLE_PRIORITY + 4)
#define mECHO_TASK_PRIORITY             (tskIDLE_PRIORITY + 3)
#define mECHO_STACK_SIZE                 configMINIMAL_STACK_SIZE + 96

/* Debuging LED */
#define mARDUINO_BUILTIN_LED            7
#define mCOM_TASK_LED                   0
#define mECHO_TASK_LED                  1
#define mERROR_LED                      2

/* The period between executions of the check task. */
#define DELAY_MS(x)                    (TickType_t) (x / portTICK_PERIOD_MS)

static void prvEchoTask(void *pv);
void vApplicationIdleHook(void);

short main(void) {

    /* Initialize Digital IO ports */
    digitalIOInitialise();

    /* Create COM task */
    if (prvCreateTransportTasks(mCOM_TASK_PRIORITY, mCOM_TASK_LED) != ESP8266_TRANSPORT_SUCCESS) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {};
    }

    /* Create Echo task */
    if (xTaskCreate(prvEchoTask, "EchoT", mECHO_STACK_SIZE, NULL, mECHO_TASK_PRIORITY, NULL) != pdPASS) {
        digitalIOSet(mERROR_LED, pdTRUE);
    }

    /* Start Tasks*/
    vTaskStartScheduler();

    //should never reach here.
    return 0;
}
/*-----------------------------------------------------------*/

void prvEchoTask(void *pv) {
    (void) pv;
    char rxB = 0;
    char buffer[32];

    //connect to my server:
    esp8266AT_Connect("192.168.0.235", "1883");

    for(;;) {
        digitalIOToggle(mECHO_TASK_LED);
        //receive msg
//        rxB = esp8266AT_recv(NULL, buffer, 32);

        //echo received msg
        if (rxB)
            esp8266AT_send(NULL, buffer, 32);
        snprintf(buffer, 32, "FreeH: %d\r\n", xPortGetFreeHeapSize());
        esp8266AT_send(NULL, buffer, 13);
        vTaskDelay(DELAY_MS(500));
    }

}

void vApplicationIdleHook(void) {
    digitalIOToggle(mARDUINO_BUILTIN_LED);
    /*This function must return;*/
}
