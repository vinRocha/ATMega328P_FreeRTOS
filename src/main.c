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
#include "FreeRTOSConfig.h"
#include "projdefs.h"
#include "task.h"
#include "queue.h"
#include "com_task.h"
#include "sensor_task.h"
#include "transport_esp8266.h"
#include "drivers/digital_io.h"

/* Tasks' priority definitions */
#define m8266RX_PRIORITY           (tskIDLE_PRIORITY + 1)
#define mSENSOR_PRIORITY           (tskIDLE_PRIORITY + 2)
#define mCOM_PRIORITY              (tskIDLE_PRIORITY + 3)

/* Tasks' StackSize definitions */
#define m8266RX_STACK_SIZE         112
#define mSENSOR_STACK_SIZE         120
#define mCOM_STACK_SIZE            180

void vApplicationIdleHook(void);

short main(void) {

    /* Initialize Digital IO ports */
    digitalIOInitialise();

    /* Initialize esp8266AT transport interface */
    esp8266Initialise(m8266RX_STACK_SIZE, m8266RX_PRIORITY);

    /* Create Queue for Sensor/COM tasks */
    QueueHandle_t mQueue = xQueueCreate(1, (UBaseType_t) sizeof(sensor_distance));
    if(!mQueue) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {}
    }

   /*  Create SENSOR task */
   if (xTaskCreate(sensorTask, "sensorT", mSENSOR_STACK_SIZE, (void*) mQueue, mSENSOR_PRIORITY, NULL) != pdPASS) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {
            //for(ul = 0; ul < 0xfffff; ul++ ) {}
            //digitalIOToggle(mERROR_LED);
        }
    }

    /* Create COM task */
    if (xTaskCreate(comTask, "comT", mCOM_STACK_SIZE, (void*) mQueue, mCOM_PRIORITY, NULL) != pdPASS) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {}
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
