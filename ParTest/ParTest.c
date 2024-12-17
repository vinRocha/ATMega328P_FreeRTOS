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
#include "partest.h"

/*-----------------------------------------------------------
 * Simple parallel port IO routines.
 *-----------------------------------------------------------*/

#define partstALL_BITS_OUTPUT			( ( unsigned char ) 0xff )
#define partstALL_OUTPUTS_OFF			( ( unsigned char ) 0x00 ) //Activate in high.
#define partstMAX_OUTPUT_LED			( ( unsigned char ) 7 )


/*-----------------------------------------------------------*/

void vParTestInitialise( void )
{
	/* PORTD0..1 are used as RX/TX for serial comunication.
	 * PORTD2..7 and PORTB4..5 will be used for LED output.
	 * These corresponds to Digital 2..7 - 12..13 in Arduino UNO R3 board. */
	/* Bitwise 'or' to set specific bits and Bitwise 'and not' to reset */
	
	DDRB |= 0x30;
	PORTB &= ~0x30;

	DDRD |= 0xfc;
	PORTD &= ~0xfc;
}
/*-----------------------------------------------------------*/

void vParTestSetLED( unsigned portBASE_TYPE uxLED, signed portBASE_TYPE xValue )
{
unsigned char ucBit = ( unsigned char ) 1;

	if( uxLED <= partstMAX_OUTPUT_LED )
	{
		/* LEDs 0..5 need to have index increased by 2, as they correspond to PORTD register */
		/* LEDs 6..7 need to have index decreased by 2, as they correspond to PORTD register */
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

void vParTestToggleLED( unsigned portBASE_TYPE uxLED )
{
unsigned char ucBit = ( unsigned char ) 1;

	if( uxLED <= partstMAX_OUTPUT_LED )
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
