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
#include "hcsr04_task.h"
#include "drivers/digital_io.h"

#define mLED                            mLED_2 //Debugging LED, PORTD bit 4;
#define TRIG_PIN                        6      //PORTB bit 4;
#define ECHO_PIN                        0x01   //PORTB bit 0;

static void delayMicrosecond(char x);

void hcsr04Task(void *pvParameters) {

    hcsr04_t interval;
    unsigned int timeout;

    for (;;) {
        interval = 0;
        timeout = 0;
        digitalIOToggle(mLED);
        digitalIOSet(TRIG_PIN, pdTRUE);
        delayMicrosecond(8);
        digitalIOSet(TRIG_PIN, pdFALSE);
        while(!(PINB & ECHO_PIN) && (timeout < 0xffff)) {
            timeout++;
        }
        while((PINB & ECHO_PIN) && (interval < 0xffff)) {
            interval++;
        }
        //Correction factor for while conditional. Consumes 8 CPU clock every interation.
        //16 CPU clocks eqs 1us
        interval = (interval / 2);
        ((hcsr04_data_t*) pvParameters)->data = interval;
        xTaskNotifyGive(((hcsr04_data_t*) pvParameters)->task);
        vTaskDelay(pdMS_TO_TICKS(3000));
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
