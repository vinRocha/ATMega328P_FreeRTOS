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
#include "transport_esp8266.h"
#include "drivers/digital_io.h"

/* Priority definitions for tasks */
#define mCOM_TASK_PRIORITY              (tskIDLE_PRIORITY + 4)
#define mECHO_TASK_PRIORITY             (tskIDLE_PRIORITY + 3)
#define mECHO_STACK_SIZE                 configMINIMAL_STACK_SIZE + 100

/* Debugin LED */
#define mARDUINO_BUILTIN_LED            7
#define mCOM_TASK_LED                   0

/* The period between executions of the check task. */
#define DELAY_MS(x)                    (TickType_t) (x / portTICK_PERIOD_MS)

static void prvEchoTask(void *pv);
void vApplicationIdleHook(void);

short main(void) {

    /* Initialize Digital IO ports */
    digitalIOInitialise();

    /* Create COM task */
    prvCreateTransportTasks(mCOM_TASK_PRIORITY, mCOM_TASK_LED);

    /* Create Echo task */
    xTaskCreate(prvEchoTask, "EchoTask", mECHO_STACK_SIZE, NULL, mECHO_TASK_PRIORITY, NULL);

    /* Start Tasks*/
    vTaskStartScheduler();

    //should never reach here.
    return 0;
}
/*-----------------------------------------------------------*/

void prvEchoTask(void *pv) {
    (void) pv;
    char buffer[32];

    //connect to my server:
    esp8266AT_Connect("192.168.0.235", "1883");

    for(;;) {
        //receive msg
        esp8266AT_recv(NULL, buffer, 32);

        //echo received msg
        esp8266AT_send(NULL, buffer, 32);
        vTaskDelay(DELAY_MS(200));
    }

}

void vApplicationIdleHook(void) {
    /*This function must return;*/
}
