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
#include "drivers/digital_io.h"

/*-----------------------------------------------------------
 * Simple parallel port IO routines.
 *-----------------------------------------------------------*/
#define digitalIO_MAX_OUTPUT			( ( unsigned char ) 7 )


/*-----------------------------------------------------------*/

void digitalIOInitialise( void )
{
	/* PORTD0..1 are used as RX/TX for serial comunication.
	 * PORTD2..7 and PORTB4..5 will be used for LED output.
	 * These corresponds to Digital 2..7 - 12..13 in Arduino UNO R3 board. */
	/* Bitwise 'or' to set specific bits and Bitwise 'and not' to reset */

    /* Set PORTB 4..5 as output and to low logic level */
    /* Set PORTB 0..3 as input without pull-up resistor */
	PORTB &= ~0x3f;
	DDRB |= 0x30;
    DDRB &= 0xf0;

    /* Set PORTD 2..7 as output and to low logic level */
	DDRD |= 0xfc;
	PORTD &= ~0xfc;
}
/*-----------------------------------------------------------*/

void digitalIOSet( unsigned portBASE_TYPE uxLED, signed portBASE_TYPE xValue )
{
unsigned char ucBit = ( unsigned char ) 1;

	if( uxLED <= digitalIO_MAX_OUTPUT )
	{
		/* DIOs 0..5 need to have index increased by 2, as they correspond to PORTD register */
		/* DIOs 6..7 need to have index decreased by 2, as they correspond to PORTD register */
		if (uxLED < 6) ucBit <<= (uxLED + 2);
		else ucBit <<= (uxLED - 2);

		vTaskSuspendAll();
		{
			if( xValue == pdTRUE )
			{
				if (uxLED < 6) PORTD |= (ucBit);
		        else PORTB |= (ucBit);
			}
			else
			{
				if (uxLED < 6) PORTD &= ~(ucBit);
				else PORTB &= ~(ucBit);

			}
		}
		xTaskResumeAll();
	}
}
/*-----------------------------------------------------------*/

void digitalIOToggle( unsigned portBASE_TYPE uxLED )
{
unsigned char ucBit = ( unsigned char ) 1;

	if( uxLED <= digitalIO_MAX_OUTPUT )
	{
		if (uxLED < 6) {
			ucBit <<= (uxLED + 2);
	        vTaskSuspendAll();
		    {
		    	if( (PORTD & ucBit) )
		    	{
		    		PORTD &= ~(ucBit);
		    	}
		    	else
		    	{
		    		PORTD |= (ucBit);
		    	}
		    }
		    xTaskResumeAll();
		} 
		else 
		{
		     ucBit <<= (uxLED - 2);
		     vTaskSuspendAll();
		     {
		     	if( (PORTB & ucBit) )
		     	{
		     		PORTB &= ~(ucBit);
		     	}
		     	else
		     	{
		     		PORTB |= (ucBit);
		     	}
		     }
		     xTaskResumeAll();
		}
	}
}
