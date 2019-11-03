/*
 * encoder.cpp
 *
 *  Created on: 22. 2. 2017
 *      Author: slouf
 *
 */

#include  <avr/io.h>
#include "utils.h"
#include "encoder.h"

int8_t read_gray_code_from_encoder(void ) {

 int8_t enc_states[] =   { 0, 0, 0, 0,-1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

 static uint8_t old_AB = 0;
 uint8_t tmp = (( PINC & 0x06) >> 1);

 if ((old_AB & 0x03) == tmp) return 0;

 old_AB <<= 2;   //remember previous state
 old_AB |= tmp;  //add current state

 return enc_states[( old_AB & 0x0f )];
}

void enc_init() {

#ifdef INTERNAL_PULLUP //pulup on
	_enc_port |= (1 << _enc_pinA) | (1 << _enc_pinB);
#ifdef 	_enc_port_BUTTON
	_enc_port |= (1 << _enc_pinBut);
#endif
#endif

	//input
	_enc_ddr |= (1 << _enc_pinA) | (1 << _enc_pinB);
#ifdef 	_enc_port_BUTTON
	_enc_port |=  (1 << _enc_pinBut);
#endif
}

int16_t enc_value(int8_t i) {
	static int16_t r = 0;
	r = r + i;
	return r;
}

#ifdef 	_enc_port_BUTTON
enc_button_t enc_button(enc_button_t i) {
	static enc_button_t b = enc_Open;
	if (i != enc_GetValue) {
		b = i;
	}
	return b;
}


bool enc_doubleClick(dbl_click_t i) {
	static bool doubleClickEnabled = true;
	switch (i) {
		case dbl_enable:
			doubleClickEnabled = true;
			break;
		case dbl_disable:
			doubleClickEnabled = false;
			break;
		default:
			;
	}
	return doubleClickEnabled;
}
#endif

//call in 1ms interrupt
void enc_service() {
	unsigned long now = millis();
	//rotary
	 enc_value(read_gray_code_from_encoder());

#ifdef 	_enc_port_BUTTON //button

   static uint16_t keyDownTicks = 0;
   static uint8_t doubleClickTicks = 0;
   static unsigned long lastButtonCheck = 0;

   if ((now - lastButtonCheck) >= ENC_BUTTONINTERVAL) // debounce checking button is sufficient every 10-30ms
	 {
	   lastButtonCheck = now;

	   if (!_enc_pinBut) { // key is down
		 keyDownTicks++;
		 if (keyDownTicks > (ENC_HOLDTIME / ENC_BUTTONINTERVAL)) {
			 enc_button(enc_Held);
		 }
	   }

	   if (_enc_pinBut) { // key is now up
		 if (keyDownTicks /*> ENC_BUTTONINTERVAL*/) {
		   if (enc_button() == enc_Held) {
			   enc_button(enc_Released);
			 doubleClickTicks = 0;
		   }
		   else {
			 #define ENC_SINGLECLICKONLY 1
			 if (doubleClickTicks > ENC_SINGLECLICKONLY) {   // prevent trigger in single click mode
			   if (doubleClickTicks < (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL)) {
				 enc_button(enc_DoubleClicked);
				 doubleClickTicks = 0;
			   }
			 }
			 else {
			   doubleClickTicks = (enc_doubleClick()) ? (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL) : ENC_SINGLECLICKONLY;
			 }
		   }
		 }

		 keyDownTicks = 0;
	   }

	   if (doubleClickTicks > 0) {
		 doubleClickTicks--;
		 if (--doubleClickTicks == 0) {
			 enc_button(enc_Clicked);
		 }
	   }
	}
#endif
}
