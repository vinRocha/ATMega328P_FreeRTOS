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
#include "drivers/digital_io.h"
#include "transport_esp8266.h"
#include "mqtt_task.h"

/* Tasks' priority definitions */
#define mCOM_PRIORITY              (tskIDLE_PRIORITY + 4)
#define mMQTT_PRIORITY             (tskIDLE_PRIORITY + 3)

/* Tasks' StackSize definitions */
#define mCOM_STACK_SIZE                 112
#define mMQTT_STACK_SIZE                181

/* Tasks' debugging LED */
#define mCOM_LED                        0
#define mMQTT_LED                       1

/* The period between executions of the check task. */
#define DELAY_MS(x)                     (TickType_t) (x / portTICK_PERIOD_MS)

void vApplicationIdleHook(void);

short main(void) {

    /* Initialize Digital IO ports */
    digitalIOInitialise();

    /* Create COM task */
    if (createTransportTasks(mCOM_STACK_SIZE, mCOM_PRIORITY, mCOM_LED) != pdPASS) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {};
    }

    /* Create Echo task */
   if (createMQTTtask(mMQTT_STACK_SIZE, mMQTT_PRIORITY, mMQTT_LED) != pdPASS) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {};
    }

    /* Start Tasks*/
    vTaskStartScheduler();

    //should never reach here.
    return 0;
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
    //digitalIOToggle(mARDUINO_BUILTIN_LED);
    /*This function must return;*/
}
