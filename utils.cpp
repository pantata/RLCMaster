/*
 * utils.cpp
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/atomic.h>

#include "utils.h"

static volatile millis_t milliseconds;
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8)

ISR (TIMER1_COMPA_vect) {
	milliseconds++;

}

unsigned long millis () {
    unsigned long millis_return;

    // Ensure this cannot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        millis_return = milliseconds;
    }

    return millis_return;
}

// Initialise library
void millis_init()
{
	// Timer settings
	TCCR1B |= (1 << WGM12) | (1 << CS11);
	 OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
	 OCR1AL = CTC_MATCH_OVERFLOW;
	 // Enable the compare match interrupt
	 TIMSK1 |= (1 << OCIE1A);
}



/*********************************
 *
 */
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t crc16_update(uint16_t crc, uint8_t a)
{
  int i;

  crc ^= a;
  for (i = 0; i < 8; ++i)
  {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }

  return crc;
}




