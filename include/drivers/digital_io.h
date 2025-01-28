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

#ifndef DIGITAL_IO_H
#define DIGITAL_IO_H

#ifdef __cplusplus
extern "C" {
#endif

/* LEDs */
#define mARDUINO_BUILTIN_LED            7 //PORTB bit 5;
#define mLED_6                          6 //PORTB bit 4;
#define mLED_5                          5 //PORTD bit 7;
#define mLED_4                          4 //PORTD bit 6;
#define mLED_3                          3 //PORTD bit 5;
#define mLED_2                          2 //PORTD bit 4;
#define mLED_1                          1 //PORTD bit 3;
#define mLED_0                          0 //PORTD bit 2;

#define mERROR_LED                      mARDUINO_BUILTIN_LED

/* To enable debugging LEDs, define DEBUG_LED macro below */
/* #define DEBUG_LED */
#define mLED_8266RX                     mLED_1
#define mLED_HCSR04                     mLED_2
#define mLED_MQTT                       mLED_3

void digitalIOInitialise( void );
void digitalIOSet( UBaseType_t uxLED,
                     BaseType_t xValue );
void digitalIOToggle( UBaseType_t uxLED );

#ifdef __cplusplus
}
#endif

#endif
