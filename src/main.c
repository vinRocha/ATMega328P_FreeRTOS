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
#include "app_data_types.h"
#include "transport_esp8266.h"
#include "hcsr04_task.h"
#include "mqtt_task.h"
#include "drivers/digital_io.h"

/* Tasks' priority definitions */
#define mMQTT_PRIORITY              (tskIDLE_PRIORITY + 0)
#define m8266RX_PRIORITY            (tskIDLE_PRIORITY + 1)
#define mHCSR04_PRIORITY            (tskIDLE_PRIORITY + 2)

/* Tasks' StackSize definitions: minimal size + padding */
#define mMQTT_STACK_SIZE            348 + 8
#define m8266RX_STACK_SIZE          96  + 8
#define mHCSR04_STACK_SIZE          46  + 8

static app_data_handle_t app_data;

void vApplicationIdleHook(void); //not used in this app

short main(void) {

    /* Initialize Digital IO ports */
    digitalIOInitialise();

    /* Initialize esp8266AT transport interface */
    if (esp8266Initialise(m8266RX_STACK_SIZE, NULL, m8266RX_PRIORITY) != pdPASS) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {}
    }

    /* Create HC-SR04 task */
    if (xTaskCreate(hcsr04Task, "HCSR", mHCSR04_STACK_SIZE, &app_data,
                    mHCSR04_PRIORITY, &(app_data.sensor_task)) != pdPASS) {
        digitalIOSet(mERROR_LED, pdTRUE);
        for (;;) {}
    }

    /*  Create MQTT task */
    if (xTaskCreate(MQTTtask, "MQTT", mMQTT_STACK_SIZE, &app_data,
                    mMQTT_PRIORITY, &(app_data.mqtt_task)) != pdPASS) {
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
