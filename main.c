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

#include <avr/eeprom.h>
#include "FreeRTOS.h"
#include "task.h"
#include "digital_io/digital_io.h"
#include "comtask.h"

/* Priority definitions for most of the tasks in the demo application.  Some
tasks just use the idle priority. */
#define mainQUEUE_POLL_PRIORITY			(tskIDLE_PRIORITY + 1)
#define mainCHECK_TASK_PRIORITY			(tskIDLE_PRIORITY + 2)
#define mainLED_TASK_PRIORITY			(tskIDLE_PRIORITY + 2)
#define mainCOM_TASK_PRIORITY			(tskIDLE_PRIORITY + 3)

/* Baud rate used by the serial port tasks. */
#define mainCOM_BAUD_RATE			(unsigned long) 115200

/* LED that is toggled by the check task.  The check task periodically checks
that all the other tasks are operating without error.  If no errors are found
the LED is toggled.  If an error is found at any time the LED is never toggles
again. */
#define mainCHECK_TASK_LED				0
#define mainARDUINO_BUILTIN_LED			7

/* The period between executions of the check task. */
#define main500_MS_DELAY				((TickType_t) 500 / portTICK_PERIOD_MS)
#define main3000_MS_DELAY				((TickType_t) 3000 / portTICK_PERIOD_MS)

void vApplicationIdleHook(void);

short main(void) {
	/* Initialize Digital IO ports */
    vParTestInitialise();

	/* Create comtask*/
	prvStartComTask(mainCOM_TASK_PRIORITY, mainCOM_BAUD_RATE);

	/* Start Tasks*/
	vTaskStartScheduler();

    //should never reach here.
	return 0;
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
	volatile unsigned long ul; /* volatile so it is not optimized away. */

	vParTestToggleLED(mainARDUINO_BUILTIN_LED);
	for( ul = 0; ul < 0xfffff; ul++ ) {} //some delay

	/*This function must return;*/
	return;
}
