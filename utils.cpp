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

#define BAUD 9600
#include <util/setbaud.h>


#define CLOCKSEL (_BV(CS01)|_BV(CS00))
#define PRESCALER 64

#define REG_TCCRA		TCCR0A
#define REG_TCCRB		TCCR0B
#define REG_TIMSK		TIMSK0
#define REG_OCR			OCR0A
#define BIT_WGM			WGM01
#define BIT_OCIE		OCIE0A
#define ISR_VECT		TIMER0_COMPA_vect
#define pwr_enable()	power_timer0_enable()
#define pwr_disable()	power_timer0_disable()

#define SET_TCCRA()	(REG_TCCRA |= _BV(BIT_WGM))
#define SET_TCCRB()	(REG_TCCRB |= CLOCKSEL)

static volatile millis_t milliseconds;

// Initialise library
void millis_init()
{
	// Timer settings
	SET_TCCRA();
	SET_TCCRB();
	REG_TIMSK |= _BV(BIT_OCIE);
	REG_OCR = ((F_CPU / PRESCALER) / 1000);
}

// Get current milliseconds
millis_t millis_get()
{
	millis_t ms;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ms = milliseconds;
	}
	return ms;
}

// Turn on timer and resume time keeping
void millis_resume()
{
	pwr_enable();
	REG_TIMSK |= _BV(BIT_OCIE);
}

// Pause time keeping and turn off timer to save power
void millis_pause()
{
	REG_TIMSK &= ~_BV(BIT_OCIE);
	pwr_disable();
}

// Reset milliseconds count to 0
void millis_reset()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		milliseconds = 0;
	}
}

// Add time
void millis_add(millis_t ms)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		milliseconds += ms;
	}
}

// Subtract time
void millis_subtract(millis_t ms)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		milliseconds -= ms;
	}
}

ISR(ISR_VECT)
{
	++milliseconds;

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




