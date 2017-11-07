/*
 * encoder.h
 *
 *  Created on: 22. 2. 2017
 *      Author: slouf
 *
 *      in setup call enc_init();
 *      then call enc_service() every 1ms (eg. in timer interrupt)
 *
 *      in main:
 *       enc_value() return last value;
 *       enc_button() return last button status;
 *       enc_doubleClick([dbl_enable]|[dbl_disable]) set double click status
 *       enc_doubleClick()  return double click status,
 *
 */

#ifndef ENCODER_H_
#define ENCODER_H_

// ******************************************************
// change when need
// ******************************************************

#define INTERNAL_PULLUP //comment, when not use internal pullup resistor

// Button configuration (values for 1ms timer service calls)
// if no button, comment this
//
#define _enc_port_BUTTON PC0

#define ENC_BUTTONINTERVAL    30L // check button every x milliseconds, also debouce time
#define ENC_DOUBLECLICKTIME  600L  // second click within 600ms
#define ENC_HOLDTIME        1200L  // report held button after 1.2s

//port configuration
#define _enc_port_A PC1
#define _enc_port_B PC2

#define _enc_ddr  DDRC
#define _enc_port PORTC
#define _enc_pin  PINC


// ******************************************************
// no change
// ******************************************************
#define _enc_pinA (_enc_pin & (1<<_enc_port_B))
#define _enc_pinB (_enc_pin & (1<<_enc_port_A))

#ifdef _enc_port_BUTTON
#define _enc_pinBut (_enc_pin & (1<<_enc_port_BUTTON))

	typedef enum dbl_click_e {
		dbl_enable = 0,
		dbl_disable = 1,
		dbl_GetStatus = 2
	} dbl_click_t;

	typedef enum enc_button_e {
		enc_Open = 0,
		enc_Closed,

		enc_Pressed,
		enc_Held,
		enc_Released,

		enc_Clicked,
		enc_DoubleClicked,

		enc_GetValue
	} enc_button_t;


	enc_button_t enc_button(enc_button_t i = enc_GetValue);
	bool enc_doubleClick(dbl_click_t i = dbl_GetStatus);

#endif

void enc_init();
void enc_service();
int16_t enc_value(int8_t i=0);

#endif /* ENCODER_H_ */
