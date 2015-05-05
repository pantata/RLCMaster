/*
 * ClickEncoder.cpp
 *
 *  Created on: 29. 3. 2015
 *      Author: ludek
 */

#include  <avr/io.h>
#include "utils.h"
#include "ClickEncoder.h"

// Button configuration (values for 1ms timer service calls)
//
#define ENC_BUTTONINTERVAL    20 // check button every x milliseconds, also debouce time
#define ENC_DOUBLECLICKTIME  600  // second click within 600ms
#define ENC_HOLDTIME        1200  // report held button after 1.2s

// ----------------------------------------------------------------------------
// Acceleration configuration (for 1000Hz calls to ::service())
//
#define ENC_ACCEL_TOP      3072   // max. acceleration: *12 (val >> 8)
#define ENC_ACCEL_INC        25
#define ENC_ACCEL_DEC         2

#define pinB (PINC & (1<<PC0))
#define pinA (PINC & (1<<PC2))
#define pinBut (PIND & (1<<PD2))

ClickEncoder::ClickEncoder(uint8_t stepsPerNotch, bool active):
	pinsActive(active), doubleClickEnabled(true), accelerationEnabled(true),
	steps(stepsPerNotch), acceleration(0), delta(0), last(0),  button(Open)
{

	//PINY NA INPUT
	DDRC = (1 << PC0);
	DDRC = (1 << PC2);
	DDRD = (1 << PD2);
	//PULLUP ZAPNOUT
	PORTC |= (1 << PC0); PORTC |= (1 << PC2); PORTD |= (1 << PD2);

	if (pinA == 0) {
		last = 3;
	}
	if (pinB == 0) {
	    last ^=1;
	}
}


// ----------------------------------------------------------------------------
// call this every 1 millisecond via timer ISR
//
void ClickEncoder::service(void) {
  bool moved = false;
  unsigned long now = millis();

  if (accelerationEnabled) { // decelerate every tick
    acceleration -= ENC_ACCEL_DEC;
    if (acceleration & 0x8000) { // handle overflow of MSB is set
      acceleration = 0;
    }
  }

  int8_t curr = 0;

   if (pinA == 0) {
     curr = 3;
   }

   if (pinB == 0) {
     curr ^= 1;
   }

   int8_t diff = last - curr;

   if (diff & 1) {            // bit 0 = step
       last = curr;
       delta += (diff & 2) - 1; // bit 1 = direction (+/-)
       moved = true;
    }

   if (accelerationEnabled && moved) {
     // increment accelerator if encoder has been moved
     if (acceleration <= (ENC_ACCEL_TOP - ENC_ACCEL_INC)) {
       acceleration += ENC_ACCEL_INC;
     }
   }

   static uint16_t keyDownTicks = 0;
   static uint8_t doubleClickTicks = 0;
   static unsigned long lastButtonCheck = 0;

   if ((now - lastButtonCheck) >= ENC_BUTTONINTERVAL) // debounce checking button is sufficient every 10-30ms
     {
       lastButtonCheck = now;

       if (pinBut == 0) { // key is down
         keyDownTicks++;
         if (keyDownTicks > (ENC_HOLDTIME / ENC_BUTTONINTERVAL)) {
           button = Held;
         }
       }

       if (pinBut != 0) { // key is now up
         if (keyDownTicks /*> ENC_BUTTONINTERVAL*/) {
           if (button == Held) {
             button = Released;
             doubleClickTicks = 0;
           }
           else {
             #define ENC_SINGLECLICKONLY 1
             if (doubleClickTicks > ENC_SINGLECLICKONLY) {   // prevent trigger in single click mode
               if (doubleClickTicks < (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL)) {
                 button = DoubleClicked;
                 doubleClickTicks = 0;
               }
             }
             else {
               doubleClickTicks = (doubleClickEnabled) ? (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL) : ENC_SINGLECLICKONLY;
             }
           }
         }

         keyDownTicks = 0;
       }

       if (doubleClickTicks > 0) {
         doubleClickTicks--;
         if (--doubleClickTicks == 0) {
           button = Clicked;
         }
       }
     }
}

int16_t ClickEncoder::getValue(void)
{
  int16_t val;

  cli();
  val = delta;

  if (steps == 2) delta = val & 1;
  else if (steps == 4) delta = val & 3;
  else delta = 0; // default to 1 step per notch

  sei();

  if (steps == 4) val >>= 2;
  if (steps == 2) val >>= 1;

  int16_t r = 0;
  int16_t accel = ((accelerationEnabled) ? (acceleration >> 8) : 0);

  if (val < 0) {
    r -= 1 + accel;
  }
  else if (val > 0) {
    r += 1 + accel;
  }

  return r;
}

ClickEncoder::Button ClickEncoder::getButton(void)
{
  ClickEncoder::Button ret = button;
  if (button != ClickEncoder::Held) {
    button = ClickEncoder::Open; // reset
  }
  return ret;
}