/*
 * main.c
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */
///
/// @mainpage	RlcMasterFw
///
/// @details    led controlller avr chip firmware
/// @n
/// @n
///
/// @date		27. 3. 2015
/// @version
///
///
/// @see		ReadMe.txt for references
///


#define DEBUG 1

#define F_CPU 16000000UL

#include <stdlib.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <string.h>
#include <util/atomic.h>
#include <avr/eeprom.h>

#include "cppfix.h"
#include "spi.h"
#include "ili9341.h"
#include "utils.h"
#include "ClickEncoder.h"

#include "main.h"
#include "twi.h"

#include "rtc8563.h"
#include "uart.h"

#include "dbg_putchar.h"

#include "lang.h"

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define LOW_BYTE(x)        	(x & 0xff)
#define HIGH_BYTE(x)       	((x >> 8) & 0xff)

extern uint8_t Retro8x16[];
extern uint8_t Sinclair_S[];
extern uint8_t Icon[];
extern uint8_t DotMatrix_M_Num[];
extern uint8_t Icon16x16[];

const uint16_t colors[]={VGA_PURPLE,VGA_NAVY,VGA_GREEN,VGA_RED,VGA_WHITE,ILI9341_ORANGE,ILI9341_DARKCYAN};

//RIZENI
unsigned long timeStamp01 = 0;
unsigned long timeStamp02 = 0;
unsigned long timeStamp03 = 0;

unsigned long sec = 0, mls = 0;

/*
 * Metoda vraci sumu setupData. Pouziva se pro zjisteni, zdali se
 * nastaveni zmenilo.
 *
 * @return unsigned long suma.
 */
static unsigned long getSetupDataSum() {
    unsigned long result = 0UL;
    for (int i = 0, n = sizeof (setupData); i < n; i++) {
        result += (uint8_t) ((uint8_t *) &setupData) [i];
    }
    return result;
}

/*
 * Ulozi data struktury setupData do EEPROM.
 */
static void saveSetupData() {

    setupData.version = VERSION;

    unsigned long newSum = getSetupDataSum();
    if (newSum != lastSetupDataSum) {
    	eeprom_update_block (&setupData, (void *) 0, sizeof (setupData));
        lastSetupDataSum = newSum;
    }
}

/*
 * Nacte data struktury /setupData/ z EEPROM a zajisti,
 * aby byla jednotliva cisla v mezich.
 */
static void readSetupData() {
    eeprom_read_block (&setupData, (void *) 0, sizeof (setupData));
    lastSetupDataSum = getSetupDataSum();

    if (setupData.version == VERSION) {
    	setupData.menuTimeout = constrain(setupData.menuTimeout,5,127);
    	setupData.lcdTimeout = constrain(setupData.lcdTimeout,5,127);
    } else {
    	//first run or new version
    	initPwmValue();
    	//reset override value
    	for (int8_t i=0;i<CHANNELS;i++) {
    		setupData.overrideVal[i] = 0;
    	}
    	setupData.menuTimeout = TIMEOUTMENU;
    	setupData.lcdTimeout = TIMEOUTPODSVICENI;
    	setupData.tm_fmt = 0;
    }
}

/*
 *  Posun kurzoru
 */
static void moveCrosshair(int16_t x, int16_t y) {
	static int16_t oldX=0, oldY=0;

	tft_setFont(Sinclair_S);
	//smazeme stary kriz
	tft_setColor(VGA_BLACK);
	tft_printChar('+',oldX-4,oldY-4);

	//nakreslime novy
    tft_setColor(VGA_WHITE);
    tft_printChar('+',x-3,y-4);
    tft_setFont(Retro8x16);

    oldX=x; oldY=y;
}

/*
 * Inicializace pole hodnot LED
 */
static void initPwmValue() {
  for (int x=0; x<MAXPWMCOUNT;x++) {
    setupData.pwmValues[x].timeSlot=255; setupData.pwmValues[x].pwmChannel=255; setupData.pwmValues[x].pwmValue=0;
  }
}

/*
 * Pridani LED hodnoty do pole
 */
static uint8_t addPwmValue(uint8_t timeslot, uint8_t channel, int16_t pwm) {
  uint8_t tmp = 255;

  for (uint8_t i=0;i<(MAXPWMCOUNT-1);i++) {
    if (setupData.pwmValues[i].timeSlot == 255)  { //jsme na konci seznamu
      tmp = i;
      break;
    }
    if ((setupData.pwmValues[i].timeSlot == timeslot) && (setupData.pwmValues[i].pwmChannel == channel)) {
      setupData.pwmValues[i].pwmValue = pwm;
      break;
    }
  }

  if ((tmp != 255) && (tmp < (MAXPWMCOUNT-1))) { //pridame novou hodnotu
    setupData.pwmValues[tmp].timeSlot = timeslot;
    setupData.pwmValues[tmp].pwmChannel = channel;
    setupData.pwmValues[tmp].pwmValue = pwm;
    sortPwmValues(1);
  } else if (tmp >= (MAXPWMCOUNT-1)) {
	 return  1;
  }
  return 0;
}

/*
 * Trideni pole LED hodnot dle timeslotu a kanalu
 */
static void sortPwmValues(uint8_t k) {
  uint8_t i, j, x, y;
  int16_t z;

  for(i = 1; i <  (MAXPWMCOUNT) ; i++) {
    x = setupData.pwmValues[i].timeSlot;
    y = setupData.pwmValues[i].pwmChannel;
    z = setupData.pwmValues[i].pwmValue;
    j = i;
    while ((j > 0) && (k==0?(x < setupData.pwmValues[j-1].timeSlot):(y < setupData.pwmValues[j-1].pwmChannel))) {
      setupData.pwmValues[j].timeSlot = setupData.pwmValues[j-1].timeSlot;
      setupData.pwmValues[j].pwmChannel = setupData.pwmValues[j-1].pwmChannel;
      setupData.pwmValues[j].pwmValue = setupData.pwmValues[j-1].pwmValue;
      j--;
    }
    setupData.pwmValues[j].timeSlot = x;
    setupData.pwmValues[j].pwmChannel = y;
    setupData.pwmValues[j].pwmValue = z;
  }
  if (k == 1) sortPwmValues(0);
}

/*
 * Odstraneni hodnoty LED z pole nastaveni
 */
static void delPwmValue(uint8_t timeslot, uint8_t channel, int16_t pwm) {
  for (uint8_t i=0;i<(MAXPWMCOUNT-1);i++) {
    if ((setupData.pwmValues[i].timeSlot == timeslot) && (setupData.pwmValues[i].pwmChannel == channel)) {
      setupData.pwmValues[i].timeSlot = 255;
      setupData.pwmValues[i].pwmChannel = 255;
      setupData.pwmValues[i].pwmValue = 0;
      break;
    }
  }
  sortPwmValues(1);
}

/*
 * Vrati hodnotu LED pro timeslot a kanal
 */
static int16_t getPwmValue(uint8_t timeslot,uint8_t channel,int16_t _selPwm) {
	int16_t ret=_selPwm;

	  for (uint8_t i=0;i<(MAXPWMCOUNT-1);i++) {
	    if ((setupData.pwmValues[i].timeSlot == timeslot) && (setupData.pwmValues[i].pwmChannel == channel)) {
	    	ret = setupData.pwmValues[i].pwmValue;
	      break;
	    }
	  }

	return ret;
}

/*
 * Pro prislusny kanal spocita aktualni hodnotu dle akt. casu
 */
static int16_t pwmValue(uint8_t ch) {

    int16_t ret = 0;
	int32_t currentTime =  t.hour()*3600L+t.minute()*60L+t.second();
	uint8_t startTime = 255;
	uint8_t endTime = 255;
	int16_t startval = (PWMSTEPS-1);
	int16_t endval = 0;

	if (manualOFF) return 0;

	//prohledame seznam
	 for (uint8_t i=0;i<(MAXPWMCOUNT-1);i++) {
		if ((setupData.pwmValues[i].pwmChannel == ch) && ((setupData.pwmValues[i].timeSlot*900L) < currentTime) && (setupData.pwmValues[i].timeSlot != 255) ) {
			startTime = setupData.pwmValues[i].timeSlot;
			startval = setupData.pwmValues[i].pwmValue;
		}
		if ((setupData.pwmValues[i].pwmChannel == ch) && ((setupData.pwmValues[i].timeSlot*900L) > currentTime) && (setupData.pwmValues[i].timeSlot != 255) ) {
			endTime = setupData.pwmValues[i].timeSlot;
			endval = setupData.pwmValues[i].pwmValue;
			break;
		}
	 }

	 if ((startTime == 255) || (endTime == 255)) {
		 return 0;
	 } else {
		 ret =(int16_t)(map((long)currentTime, (long)startTime*900L , (long)endTime*900L, (long)startval,(long)endval));
	 }

	 ret = constrain(ret,0,(PWMSTEPS-1));
	 return ret;
}


static void printDateTime() {
	char tempStr [9];
	tft_setFont(DotMatrix_M_Num);
	tft_setColor(VGA_TEAL);

	sprintf (tempStr,"%.2d:%.2d",setupData.tm_fmt?t.hour()>12?t.hour()-12:t.hour():t.hour(),t.minute());
	tft_print(tempStr,0,0);
	switch (setupData.dt_fmt) {  //  0 = DD.MM.YY,   1 = dd-mm-yy ,   2 = mm/dd/yy,  3 = yy-mm-dd
		case 1:
			sprintf (tempStr,"%.2d;%.2d;%2d",t.day(),t.month(),t.year()-2000);
			break;
		case 2:
			sprintf (tempStr,"%.2d/%.2d/%2d",t.month(),t.day(),t.year()-2000);
			break;
		case 3:
			sprintf (tempStr,"%.2d;%.2d;%2d",t.year()-2000,t.month(),t.day());
			break;
		default:
			sprintf (tempStr,"%.2d.%.2d.%2d",t.day(),t.month(),t.year()-2000);
			break;
	}
	tft_print(tempStr,319-(8*tft_getFontXsize()),0);

	tft_setColor(VGA_RED);
	tft_setFont(Icon16x16);
	tft_print(P(BATTERY),84,0);

	tft_setFont(Sinclair_S);
	tft_setColor(VGA_WHITE);

}

/*
 * Vykresli osy grafu
 */
static void showAxis() {
	tft_setColor(VGA_GRAY);
    tft_setFont(Sinclair_S);
    tft_print("0",  18, 210);
    tft_print("50", 10, 120);
    tft_print("100",3,  40);
    tft_drawVLine(26, 40, 180);
    for (uint8_t i=0; i<5; i++) {
        tft_drawHLine(23, 40+(i*(180/4)), 4);
    }
    tft_drawHLine(26, 220, 220);
}


/*
 * Vykresleni grafu z pole LED hodnot (kanal, timeslot, hodnota)
 */
static void drawGraph(bool del) {
	uint8_t ts1, ts2, x;
	int16_t vl1, vl2;

	for (uint8_t i=0; i<7; i++ ) {
		ts1=0; ts2=0;vl1=0; vl2=0; x=0;
		for (uint8_t k=0; k<MAXPWMCOUNT; k++) {
			if (setupData.pwmValues[k].pwmChannel == i) {
				if (setupData.pwmValues[k].timeSlot == 255) break;
				x++;
				ts1=ts2;
				vl1=vl2;
				ts2=setupData.pwmValues[k].timeSlot;
				vl2=map(setupData.pwmValues[k].pwmValue,0,(PWMSTEPS-1),0,255);
				if (x > 1) {	//kreslime
					tft_setColor(del==true?VGA_BLACK:colors[i]);
					tft_drawLine(32+ts1*2,72+(128-(vl1/2)),32+ts2*2,72+(128-(vl2/2)));
				}
			}
		}
	}
}

/*
 *  Rotary encoder interrupt  TIMER2
 */

ClickEncoder *encoder;
int16_t last, value;

/*
 * Timer 2 pro rotary enkoder
 */
static void timer2Init() {
	//disable interrupt
	TIMSK2 &= ~(1<<TOIE2);

	  // Set the Timer Mode to CTC
	    TCCR2A |= (1 << WGM21);

	    // Set the value that you want to count to
	    OCR2A = 0xF9;

	    TIMSK2 |= (1 << OCIE2A);    //Set the ISR COMPA vect

	    // set prescaler to 64 and start the timer
	    TCCR2B |= (1 << CS22);

	 TIMSK2 |= (1<<TOIE2);
}

/*
 * Interrupt pro timer 2
 */

ISR (TIMER2_COMPA_vect)
{
	encoder->service();
}


/*
 *  Draw default menu
 */
static void drawMenu(uint8_t i, uint8_t type) {

	tft_setFont(Icon);

	switch(type) {
		case MENUDEFAULT:
			tft_setColor(i==1?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[0],267,34);
			tft_setColor(i==2?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[1],267,34+48);
			tft_setColor(i==3?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[2],267,34+2*48);
			tft_setColor(i==4?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[3],267,34+3*48);
			break;
		case MENUEDIT:
			tft_setColor(i==1?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[4],267,34);
			tft_setColor(i==2?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[5],267,34+48);
			break;
		case MENUHOME:
			tft_setColor(i==1?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[4],267,34);
			tft_setColor(i==2?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[5],267,34+48);
			tft_setColor(i==3?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[6],267,34+2*48);
			break;
		}

}

/*
 * Vymaze oblast menu
 */
static void clearMenu() {
	tft_setColor(VGA_BLACK); tft_fillRect(267, 28, 319, 239);
}

/*
 * Vykresli zakladni obrazovku
 */
static void tftBackg() {
    tft_clrScr();
    tft_setColor(0x8410);
    tft_drawHLine(0, 26, 319);
    tft_drawVLine(257, 26, 239-26);
    drawMenu(0,MENUDEFAULT);
}

/*
 * vymazani okna
 */

static void tftClearWin(const char *title) {
    tft_setColor(VGA_BLACK); tft_fillRect(0, 28, 256, 239);
    tft_setFont(Sinclair_S);tft_setColor(VGA_GREEN);tft_print(title, 256/2-(16*8/2), 30);
}

/*
 *  Vykresli sloupcovy graf intenzity LED
 *
 */

static void drawLightChart(const int16_t* arr) {

	showAxis();

	tft_setFont(Sinclair_S);
	tft_setColor(VGA_GREEN);

	tft_print(override?P(MANUAL):manualOFF?P(OFF):P(AUTO), 256/2-(16*8/2), 30);

    uint8_t vyska = 0;
    vyska = (uint8_t)map(arr[0],0,(PWMSTEPS-1),0,180);
    if (vyska > 0) {tft_setColor(colors[0]); tft_fillRect(30+(32*0), 218-vyska, 60 +(32*0), 218); }
    tft_setColor(VGA_BLACK);tft_fillRect(30+(32*0), 38, 60 +(32*0), 38+180-vyska-1);

    vyska = map(arr[1],0,(PWMSTEPS-1),0,180);
    if (vyska > 0) {tft_setColor(colors[1]); tft_fillRect(30+(32*1), 218-vyska, 60 +(32*1), 218); }
    tft_setColor(VGA_BLACK);tft_fillRect(30+(32*1), 38, 60 +(32*1), 38+180-vyska-1);

    vyska = map(arr[2],0,(PWMSTEPS-1),0,180);
    if (vyska > 0) {tft_setColor(colors[2]); tft_fillRect(30+(32*2), 218-vyska, 60 +(32*2), 218); }
    tft_setColor(VGA_BLACK);tft_fillRect(30+(32*2), 38, 60 +(32*2), 38+180-vyska-1);

    vyska = map(arr[3],0,(PWMSTEPS-1),0,180);
    if (vyska > 0) {tft_setColor(colors[3]); tft_fillRect(30+(32*3), 218-vyska, 60 +(32*3), 218); }
    tft_setColor(VGA_BLACK);tft_fillRect(30+(32*3), 38, 60 +(32*3), 38+180-vyska-1);

    vyska = map(arr[4],0,(PWMSTEPS-1),0,180);
    if (vyska > 0) {tft_setColor(colors[4]); tft_fillRect(30+(32*4), 218-vyska, 60 +(32*4), 218); }
    tft_setColor(VGA_BLACK);tft_fillRect(30+(32*4), 38, 60 +(32*4), 38+180-vyska-1);

    vyska = map(arr[5],0,(PWMSTEPS-1),0,180);
    if (vyska > 0) {tft_setColor(colors[5]); tft_fillRect(30+(32*5), 218-vyska, 60 +(32*5), 218); }
    tft_setColor(VGA_BLACK);tft_fillRect(30+(32*5), 38, 60 +(32*5), 38+180-vyska-1);

    vyska = map(arr[6],0,(PWMSTEPS-1),0,180);
    if (vyska > 0) {tft_setColor(colors[6]); tft_fillRect(30+(32*6), 218-vyska, 60 +(32*6), 218); }
    tft_setColor(VGA_BLACK);tft_fillRect(30+(32*6), 38, 60 +(32*6), 38+180-vyska-1);



    //draw values
    if (selMenu == MENU_OFF) {
		char tempStr[5];
		tft_setFont(Retro8x16);
		tft_setColor(VGA_GRAY);

		sprintf(tempStr,"%4d",arr[0]);
		tft_print(tempStr,26+((32*0)+2),222);

		sprintf(tempStr,"%4d",arr[1]);
		tft_print(tempStr,26+((32*1)+2),222);

		sprintf(tempStr,"%4d",arr[2]);
		tft_print(tempStr,26+((32*2)+2),222);

		sprintf(tempStr,"%4d",arr[3]);
		tft_print(tempStr,26+((32*3)+2),222);

		sprintf(tempStr,"%4d",arr[4]);
		tft_print(tempStr,26+((32*4)+2),222);

		sprintf(tempStr,"%4d",arr[5]);
		tft_print(tempStr,26+((32*5)+2),222);

		sprintf(tempStr,"%4d",arr[6]);
		tft_print(tempStr,26+((32*6)+2),222);
    }

}

/*
 *  obsluha menu nastaveni data a casu
 *  TODO: doplnit timeout pro menu a LCD
 */
static void setTime(int16_t value, ClickEncoder::Button b) {

	static int8_t menuPos = -1;
    static bool edit = false;

    static uint16_t color = VGA_WHITE;

    static int8_t _datetime[] = {0,0,0,0,0,0};

    //reset menu
    if (resetMenu) {
		//staticke promenne na inicializacni hodnotu
    	menuPos = -1;
    	edit = false;
		color = VGA_WHITE;

		//vypneme priznak, ze jsme v menu
		selMenu = MENU_OFF;

		//vycistime okno
		tftClearWin(P(NONE));

		//prekreslime menu
		clearMenu();
		drawMenu(MENU_OFF,MENUDEFAULT);
		resetMenu = false;
		return;
    }

    //prvni vstup,
    if (menuPos < 0) {
    	//kopie casu pro editaci
        _datetime[2] = t.hour();
        _datetime[1] = t.minute();
        _datetime[0] = t.second();
        _datetime[3] = t.day();
        _datetime[4] = t.month();
        _datetime[5] = t.year()-2000;

    	menuPos = 0;
    	clearMenu();
    	drawMenu(0,MENUEDIT);
    }

    if (!edit && value != 0) {
    	menuPos = menuPos + value;
    	if (menuPos < 1) menuPos = 10;
    	if (menuPos > 10) menuPos = 1;
    } else if (edit && value != 0) {
    	switch (menuPos) {
    	case 1:
    		setupData.dt_fmt += value;
    		if (setupData.dt_fmt > 3) setupData.dt_fmt = 0;
    		if (setupData.dt_fmt < 0) setupData.dt_fmt = 3;
    				break;
    	case 2:
    		setupData.tm_fmt = !setupData.tm_fmt;
    				break;
    	case 3: //rok
    		_datetime[5] += value;
    		if (_datetime[5] < 15) _datetime[5] = 30;
    		if (_datetime[5] > 30) _datetime[5] = 15;
    	    		break;
    	case 4: //mesic
    		_datetime[4] += value;
    		if (_datetime[4] > 12 ) _datetime[4] = 1;
    		if (_datetime[4] < 1 ) _datetime[4] = 12;
    	    		break;
    	case 5: //den TODO: osetrit meze
    		_datetime[3] += value;
    		if (_datetime[3] > days[_datetime[4] - 1]) _datetime[3] = 1;
    		if (_datetime[3] < 1) _datetime[3] = days[_datetime[4] - 1];
    				break;
    	case 6: //hodiny
    		_datetime[2] += value;
    		if (_datetime[2] > 23) _datetime[2] = 0;
    		if (_datetime[2] < 0) _datetime[2] = 23;
    	    		break;
    	case 7: //minuty
    		_datetime[1] += value;
    		if (_datetime[1] > 59) _datetime[2] = 0;
    		if (_datetime[1] < 0) _datetime[2] = 59;
    	    		break;
    	case 8: //sekundy
    		_datetime[0] +=  value;
    		if (_datetime[0] > 59) _datetime[2] = 0;
    		if (_datetime[0] < 0) _datetime[2] = 59;
    	    		break;
    	}
    }

	if (b==ClickEncoder::DoubleClicked ) {
		//save and exit
	    RTC.adjust(_datetime[5],_datetime[4], _datetime[3],
	    		_datetime[2], _datetime[1], _datetime[0]);
		resetMenu = true;
	} else if (b==ClickEncoder::Held) {
		// cancel
		resetMenu = true;
	} else if (b==ClickEncoder::Clicked) {
		// zahajeni nebo ukonceni editace, nebo save nebo cancel
		if (menuPos == 9) {
			//cancel
			resetMenu = true;
		} else if (menuPos == 10) {
			//save and exit
		    RTC.adjust(_datetime[5],_datetime[4], _datetime[3],
		    		_datetime[2], _datetime[1], _datetime[0]);
			resetMenu = true;
		} else {
			edit = !edit; if (edit) color = VGA_YELLOW; else color = VGA_WHITE;
		}
	}

	char tempStr[9];
		//zobrazeni
		tft_setFont(Retro8x16);
		tft_setColor(VGA_GRAY);tft_print(P(DTFORMAT),10,52);
		switch (setupData.dt_fmt) {   //0 = DD.MM.YY,   1 = dd-mm-yy ,   2 = mm/dd/yy,  3 = yy-mm-dd
			case 1:
				sprintf(tempStr,"%s",P(DDMMYY1));
				break;
			case 2:
				sprintf(tempStr,"%s",P(MMDDYY1));
				break;
			case 3:
				sprintf(tempStr,"%s",P(YYMMDD1));
				break;
			default:
				sprintf(tempStr,"%s",P(DDMMYY2));
				break;
		}
		tft_setColor(menuPos==1?color:VGA_GRAY); tft_print(tempStr,74,52);
		sprintf(tempStr,"%s",setupData.tm_fmt?P(H12):P(H24));tft_setColor(menuPos==2?color:VGA_GRAY); tft_print(tempStr,160,52);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(YEAR));tft_print(tempStr,10,72);
		sprintf(tempStr,"%02d",_datetime[5]);tft_setColor(menuPos==3?color:VGA_GRAY); tft_print(tempStr,74,72);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(MONTH));tft_print(tempStr,10,92);
		sprintf(tempStr,"%02d",_datetime[4]); tft_setColor(menuPos==4?color:VGA_GRAY);tft_print(tempStr,74,92);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(DAY));tft_print(tempStr,10,112);
		sprintf(tempStr,"%02d",_datetime[3]);tft_setColor(menuPos==5?color:VGA_GRAY); tft_print(tempStr,74,112);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(HOUR));tft_print(tempStr,10,132);
		sprintf(tempStr,"%02d %s",setupData.tm_fmt==0?_datetime[2]:_datetime[2]>12?_datetime[2]-12:_datetime[2],setupData.tm_fmt==0?"   ":_datetime[2]>12?P(PM):P(AM));tft_setColor(menuPos==6?color:VGA_GRAY); tft_print(tempStr,74,132);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(MINUTE));tft_print(tempStr,10,152);
		sprintf(tempStr,"%02d",_datetime[1]); tft_setColor(menuPos==7?color:VGA_GRAY);tft_print(tempStr,74,152);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(SECOND));tft_print(tempStr,10,172);
		sprintf(tempStr,"%02d",_datetime[0]); tft_setColor(menuPos==8?color:VGA_GRAY);tft_print(tempStr,74,172);
		drawMenu(menuPos==9?1:menuPos==10?2:0,MENUEDIT);

}

/*
 * Obsluha nastaveni LED hodnot
 */
static void settings(int16_t value, ClickEncoder::Button b  ) {

	static int8_t menuPos = -1;
    static bool edit = false;

    static int8_t _selChannel = 0;
    static int8_t _selTimeslot = 0;
    static int16_t _selPwm = 0;

    static uint16_t color = VGA_WHITE;

    char tempStr[12];

    //reset menu
    if (resetMenu) {
		//staticke promenne na inicializacni hodnotu

		_selChannel = 0;
		_selTimeslot = 0;
		_selPwm = 0;

		menuPos = -1;
		edit = false;
		color = VGA_WHITE;

		//vypneme priznak, ze jsme v menu
		selMenu = MENU_OFF;

		//vycistime okno
		tftClearWin(P(NONE));

		//prekreslime menu
		clearMenu();
		drawMenu(MENU_OFF,MENUDEFAULT);
		resetMenu = false;
		return;
    }

//prvni vstup,
	if (menuPos < 0) {
		menuPos = 1;
		clearMenu();
		drawMenu(0,MENUHOME);
		showAxis();
		drawGraph(false);
	}

	if (!edit && value != 0) {
		menuPos += value;
		if (menuPos < 1) menuPos = 6;
		if (menuPos > 6) menuPos = 1;
	} else if (edit && value != 0) {
		switch (menuPos) {
		case 1:
			_selTimeslot += value;
			if (_selTimeslot > 95) _selTimeslot = 0;
			if (_selTimeslot < 0) _selTimeslot = 95;
			break;
		case 2:
			_selChannel +=  value;
			if (_selChannel > 6) _selChannel = 0;
			if (_selChannel < 0) _selChannel = 6;
			color = colors[_selChannel];
			break;
		case 3:
			_selPwm += value;
			if (_selPwm > (PWMSTEPS-1)) _selPwm = 0;
			if (_selPwm < 0) _selPwm = (PWMSTEPS-1);
			break;
		}
	}

	if ( (b==ClickEncoder::DoubleClicked ) && (edit) ) {
		//add point
		drawGraph(true);
		addPwmValue(_selTimeslot, _selChannel, _selPwm);

		color = VGA_WHITE;
		edit = !edit;
	} else if ( (b==ClickEncoder::Held ) && (edit) ) {
		//delete point
		drawGraph(true);
		delPwmValue(_selTimeslot, _selChannel, _selPwm);
		color = VGA_WHITE;
		edit = !edit;
	} else if (b==ClickEncoder::Clicked) {
		// zahajeni nebo ukonceni editace, nebo save nebo cancel
		if (menuPos == 4) {
			drawGraph(true);
			delPwmValue(_selTimeslot, _selChannel, _selPwm);
			color = VGA_WHITE;
		}else if (menuPos == 5) {
			drawGraph(true);
			addPwmValue(_selTimeslot, _selChannel, _selPwm);
			color = VGA_WHITE;
		} else if (menuPos == 6) {
			//cancel
			saveSetupData();
			resetMenu = true;
		} else
		{
			edit = !edit; if (edit) color = VGA_YELLOW; else color = VGA_WHITE;
		}
	}

	//zobrazeni
	drawGraph(false);

	moveCrosshair(32+_selTimeslot*2,200-(getPwmValue(_selTimeslot, _selChannel,_selPwm)/8));

	tft_setFont(Retro8x16);
	sprintf(tempStr,"TIME:%02d:%02d",_selTimeslot/4,(_selTimeslot-((_selTimeslot/4)*4))*15);
	tft_setColor(menuPos==1?color:VGA_GRAY);tft_print(tempStr,30,222);tft_setColor(VGA_GRAY);
	sprintf(tempStr,"CH:%01d",_selChannel);
	tft_setColor(menuPos==2?color:VGA_GRAY);tft_print(tempStr,30+(12*8),222);tft_setColor(VGA_GRAY);
	sprintf(tempStr,"PMW:%04d",getPwmValue(_selTimeslot, _selChannel,_selPwm));
	tft_setColor(menuPos==3?color:VGA_GRAY);tft_print(tempStr,30+(18*8),222);tft_setColor(VGA_GRAY);

	drawMenu(menuPos==4?1:menuPos==5?2:menuPos==6?3:0,MENUHOME);
}

/*
 * Obsluha nastaveni LED hodnot pro manualni ovladani
 */
static void setManual(int16_t value, ClickEncoder::Button b ) {

	static int8_t menuPos = -1;
	static bool edit = false;

	static uint16_t color = VGA_WHITE;

	char tempStr [5];


	//reset menu
	if (resetMenu) {
		//staticke promenne na inicializacni hodnotu
		menuPos = -1;
		edit = false;
		color = VGA_WHITE;

		//vypneme priznak, ze jsme v menu
		selMenu = MENU_OFF;

		//vycistime okno
		tftClearWin(P(NONE));

		//prekreslime menu
		clearMenu();
		drawMenu(MENU_OFF,MENUDEFAULT);
		resetMenu = false;
		return;
	}

	    //prvni vstup,
	if (menuPos < 0) {
		menuPos = 1;
		clearMenu();
		drawMenu(MENU_OFF,MENUEDIT);
		drawLightChart(setupData.overrideVal);
		override = true;
	}

    if (!edit && value != 0) {
    	menuPos = menuPos + value;
    	if (menuPos < 1) menuPos = 9;
    	if (menuPos > 9) menuPos = 1;
    } else if (edit && value != 0) {
    	if ((menuPos > 0) && (menuPos < 8)) {
    		setupData.overrideVal[menuPos-1] += value;
    		if (setupData.overrideVal[menuPos-1] > (PWMSTEPS-1)) setupData.overrideVal[menuPos-1] = 0;
    		if (setupData.overrideVal[menuPos-1] < 0) setupData.overrideVal[menuPos-1] = (PWMSTEPS-1);
    	}
    }

	if (b==ClickEncoder::DoubleClicked ) {
		//save and exit
		override = true;
		resetMenu = true;
		saveSetupData();
	} else if (b==ClickEncoder::Held) {
		// cancel
		override = false;
		resetMenu = true;
	} else if (b==ClickEncoder::Clicked) {
		// zahajeni nebo ukonceni editace, nebo save nebo cancel
		if (menuPos == 8) {
			//cancel
			override = false;
			resetMenu = true;
		} else if (menuPos == 9) {
			//save and exit
			override = true;
			resetMenu = true;
			saveSetupData();
		} else {
			edit = !edit; if (edit) color = VGA_YELLOW; else color = VGA_WHITE;
		}
	}

	//zobrazeni
	if ( value != 0) drawLightChart(setupData.overrideVal);

	tft_setFont(Retro8x16);
	for (uint8_t i=0; i<CHANNELS; i++) {
		sprintf(tempStr,"%4d",setupData.overrideVal[i]);
		tft_setColor(menuPos==i+1?color:ILI9341_DARKGREY); tft_print(tempStr,26+(32*i+2),222);
	}
	drawMenu(menuPos==8?1:menuPos==9?2:0,MENUEDIT);
}

static void  searchSlave() {
	  uint8_t error, address, idx=0;

#ifdef DEBUG
	  char tempStr [20];
#endif

	  memset(slaveAddr,255,16);
	  for(address = 32; address < 64; address = address + 2)
	  {
		twi_begin_transmission(address);
		twi_send_byte(16);
		twi_send_byte(0xff);
	    error = twi_end_transmission();

	    if (error == 0) {
	    	slaveAddr[idx] = address;
	    	idx ++;
#ifdef DEBUG
	    	sprintf(tempStr,"%d: %03xh\r\n",idx-1, slaveAddr[idx-1]);
	    	dbg_puts(tempStr);
#endif
	    }

      modulesCount = idx;

	  }
	  return;
}

static void sendValue(uint8_t address, int16_t *value) {

	uint16_t crc = 0xffff;
	for (uint8_t i = 0; i < 7; i++) {
		crc = crc16_update(crc, value[i]);
	}

	twi_begin_transmission(address);
	twi_send_byte(16);twi_send_byte(0xFF);
	twi_end_transmission();
/*
	twi_begin_transmission(address);
	twi_send_byte(0);

	twi_send_byte(HIGH_BYTE(value[0])); twi_send_byte(LOW_BYTE(value[0]));

	twi_send_byte(HIGH_BYTE(value[1])); twi_send_byte(LOW_BYTE(value[1]));

	twi_send_byte(HIGH_BYTE(value[2])); twi_send_byte(LOW_BYTE(value[2]));

	twi_send_byte(HIGH_BYTE(value[3])); twi_send_byte(LOW_BYTE(value[3]));

	twi_send_byte(HIGH_BYTE(value[4])); twi_send_byte(LOW_BYTE(value[4]));

	twi_send_byte(HIGH_BYTE(value[5])); twi_send_byte(LOW_BYTE(value[5]));

	twi_send_byte(HIGH_BYTE(value[6])); twi_send_byte(LOW_BYTE(value[6]));

	twi_send_byte(HIGH_BYTE(crc)); twi_send_byte(LOW_BYTE(crc));

	twi_end_transmission();
*/
#ifdef DEBUG
	char tempStr [20];
	twi_begin_transmission(address);
	twi_send_byte(16);
	twi_end_transmission();

	twi_request_from(address, 1);
	sprintf(tempStr,"16: 0x%02xh\r\n",twi_receive());
	dbg_puts(tempStr);
	twi_end_transmission();

/*
	twi_begin_transmission(address);
	twi_send_byte(0);

	twi_request_from(address, 14);
	sprintf(tempStr,"A: 0x%02xh\r\n",address);dbg_puts(tempStr);
	sprintf(tempStr,"0: 0x%02xh 1: 0x%02xh\r\n",twi_receive(),twi_receive());dbg_puts(tempStr);
	sprintf(tempStr,"2: 0x%02xh 3: 0x%02xh\r\n",twi_receive(),twi_receive());dbg_puts(tempStr);
	sprintf(tempStr,"4: 0x%02xh 5: 0x%02xh\r\n",twi_receive(),twi_receive());dbg_puts(tempStr);
	sprintf(tempStr,"6: 0x%02xh 7: 0x%02xh\r\n",twi_receive(),twi_receive());dbg_puts(tempStr);
	sprintf(tempStr,"8: 0x%02xh 9: 0x%02xh\r\n",twi_receive(),twi_receive());dbg_puts(tempStr);
	sprintf(tempStr,"10: 0x%02xh 11: 0x%02xh\r\n",twi_receive(),twi_receive());dbg_puts(tempStr);
	sprintf(tempStr,"12: 0x%02xh 13: 0x%02xh\r\n",twi_receive(),twi_receive());dbg_puts(tempStr);

	twi_end_transmission();
*/
#endif

}

static uint8_t readTemperature(uint8_t address) {
	uint8_t temp=0xff;

	twi_begin_transmission(address);
	twi_send_byte(18);
	twi_end_transmission();

	twi_request_from(address, 1);
	temp = twi_receive();
	twi_end_transmission();

	return temp;
}

static void printTemp() {
	static uint8_t idx = 0;
	uint8_t temp = 0;
	char tmpStr[8];

	if (modulesCount > 0) {
		temp = 	readTemperature(slaveAddr[idx]);
		idx ++;
		if (idx > (modulesCount-1)) idx = 0;
	}

	sprintf(tmpStr,"%02d:%02d~",idx,temp);
	tft_setColor(VGA_YELLOW);tft_setFont(Sinclair_S);
	tft_print(tmpStr,CENTER,18);
}

/*
 * Hlavni smycka
 */
int main(void) {

	/*
	 *  Setup
	 */
#ifdef DEBUG
	dbg_tx_init();
#endif

	timer2Init();

	millis_init();
	twi_init_master();
	uart_init();

	RTC.begin();

	encoder = new ClickEncoder(4);

	sei();
	spi_init();

	tft_init();
	tft_setRotation(3);

	for(uint8_t i=0;i<CHANNELS;i++) {
		lastChannelVal[i] = 0;
		channelVal[i] = 0;
	}

	readSetupData();

	int8_t lcdTimeout = setupData.lcdTimeout;
	int8_t menuTimeout = setupData.menuTimeout;

	searchSlave();

	tftBackg();
    tftClearWin(P(AUTO));
    drawLightChart(channelVal);

    /*
     * Hlavni smycka
     */
	while (1) {

		unsigned long currentTime = millis();

		ClickEncoder::Button b = encoder->getButton();
		value += encoder->getValue();

		if ((currentTime - timeStamp03) >= LEDINTERVAL) {
			timeStamp03 = millis();
			int16_t delta = 0;

			delta = (override?setupData.overrideVal[0]:channelVal[0]) - lastChannelVal[0];
			if (delta != 0)  lastChannelVal[0] = lastChannelVal[0] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);

			delta = (override?setupData.overrideVal[1]:channelVal[1]) - lastChannelVal[1];
			if (delta != 0)  lastChannelVal[1] = lastChannelVal[1] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);

			delta = (override?setupData.overrideVal[2]:channelVal[2]) - lastChannelVal[2];
			if (delta != 0)  lastChannelVal[2] = lastChannelVal[2] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);

			delta = (override?setupData.overrideVal[3]:channelVal[3]) - lastChannelVal[3];
			if (delta != 0)  lastChannelVal[3] = lastChannelVal[3] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);

			delta = (override?setupData.overrideVal[4]:channelVal[4]) - lastChannelVal[4];
			if (delta != 0)  lastChannelVal[4] = lastChannelVal[4] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);

			delta = (override?setupData.overrideVal[5]:channelVal[5]) - lastChannelVal[5];
			if (delta != 0)  lastChannelVal[5] = lastChannelVal[5] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);

			delta = (override?setupData.overrideVal[6]:channelVal[6]) - lastChannelVal[6];
			if (delta != 0)  lastChannelVal[6] = lastChannelVal[6] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);


			//TODO: samostane nastaveni dle adresy modulu
			for (uint16_t addr = 0; addr < modulesCount; addr++) {
				sendValue(slaveAddr[addr], lastChannelVal);
			}

		}

		if ((currentTime - timeStamp01) >= SECINTERVAL) {
 	 	    t = RTC.now();

			if (lcdTimeout > 0 ) {
				printDateTime();
				lcdTimeout--;
				printTemp();
			}
			if (menuTimeout > 0 ) { menuTimeout--; }

			//spocitame hodnotu
			for(uint8_t i=0;i<CHANNELS;i++) {
				channelVal[i] = pwmValue(i);
			}

			timeStamp01 = millis(); // take a new timestamp
		}

		if ((currentTime - timeStamp02) >= LCDREFRESH) {
			if (selMenu == 0 ) drawLightChart(override?setupData.overrideVal:channelVal);
			timeStamp02 = millis();
		}


		/*
		 *  Obsluha UART komunikace s wifi
		 */

		if (uart_available()) { //data cekaji ve fromte

			uint8_t uart_data[8];
			uint8_t *p = uart_getPckt();

			memcpy((void*)uart_data,(void*)p,8);

			//precteme 8 byte z bufferu
			if (!isUartData) {

				uint16_t crc = 0xffff;
				for (uint8_t i = 0; i < 8; i++) {
					//uart_putB(uart_data[i]);
					//a soucasne kontrolujeme crc
					crc = crc16_update(crc, uart_data[i]);
				}

				if (crc == 0 ) { //crc je ok, zjistime, o jaky jde prikaz
					uart_cmd = (uart_data[0]);
					//uart_write((char*)"CMD OK\n");
					switch (uart_cmd) {
						case 0x81:  //get led
							//uart_write((char*)"GET LED\n");
							for (uint8_t i=0;i<CHANNELS;i++) {
								uart_putB(setupData.overrideVal[i]);
							}
							uart_putB(UART_OK >> 8);
							uart_putB(UART_OK & 0xFF);
							break;
						case 0x83:  //set auto / manual
							//uart_write("SET AUTO/MANUAL\n");
							override = !override;
							uart_cmd = 0;
							uart_putB(UART_OK >> 8);
							uart_putB(UART_OK & 0xFF);
							break;
						case 0x85:  //get eeprom
							//uart_write((char*)"GET EEPROM\n");
							for (int x=0; x<MAXPWMCOUNT;x++) {
								if (setupData.pwmValues[x].timeSlot == 255) break;
								uart_putB(setupData.pwmValues[x].timeSlot);
								uart_putB(setupData.pwmValues[x].pwmChannel);
								uart_putB(setupData.pwmValues[x].pwmValue);
							 }
							uart_putB(UART_OK >> 8);
							uart_putB(UART_OK & 0xFF);
							uart_cmd = 0;
							break;
						case 0x82:  //set led
							//uart_write((char*)"SET LED\n");
							isUartData = true;
							uDataLength = (uart_data[3] << 8) | uart_data[2];
							_address = uart_data[1];
							uart_DataLength = uDataLength;
							uart_putB(UART_OK >> 8);
							uart_putB(UART_OK & 0xFF);
							break;
						case 0x84:  //set time
							isUartData = true;
							uDataLength = (uart_data[3] << 8) | uart_data[2];
							uart_DataLength = uDataLength;
							uart_putB(UART_OK >> 8);
							uart_putB(UART_OK & 0xFF);
							break;
						case 0x86:  //set eeprom
							isUartData = true;
							uDataLength = (uart_data[3] << 8) | uart_data[2];
							uart_DataLength = uDataLength;
							uart_putB(UART_OK >> 8);
							uart_putB(UART_OK & 0xFF);
							break;
						default:
							uart_cmd = 0;
							isUartData = false;
							isUartData = true;
							uart_putB(UART_BADCMD >> 8);
							uart_putB(UART_BADCMD & 0xFF);
							break;
					  }

				} else {
					//crc error
					uart_putB(UART_CRCERROR >> 8);
					uart_putB(UART_CRCERROR & 0xFF);
				}
			} else {
				if (uDataLength > 0) {
						uint16_t crc = 0xffff;
						for (uint8_t i = 0; i < 8; i++) { //kontrolujeme crc
							crc = crc16_update(crc, uart_data[i]);
						}
						if (crc == 0) {
							//uart_write((char*)"DATA OK\n");
							if (uDataLength > 6) {
								uDataLength = uDataLength - 6;
								for (uint8_t i = 0; i < 6; i++) {
									_data[i+(_bytes*6)] = uart_data[i];
								}
								_bytes++;
							} else {
								//zbytek dat
								for (uint8_t i = 0; i < uDataLength; i++) {
									_data[i+(_bytes*6)] = uart_data[i];
								}
								uDataLength = 0;
							}
							uart_putB(UART_OK >> 8);
							uart_putB(UART_OK & 0xFF);
						} else {
							uart_putB(UART_CRCERROR >> 8);
							uart_putB(UART_CRCERROR & 0xFF);
						}
				} //uDataLength > 0

				if (uDataLength == 0) {
					switch (uart_cmd) {
						case 0x82:  //set led
							if (uart_DataLength == 1) {
								override = true;
								if (_address == 0) {
									//nastav vsechny led
									for(uint8_t x = 0; x< CHANNELS; x++) {
										setupData.overrideVal[x] = _data[0];
									}
								} else {
									setupData.overrideVal[_address - 1] = _data[0];
								}
							}
							break;
						case 0x84:  //set time
							if (uart_DataLength == 6) {
								/*
								t->year = _data[0];
								t->mon =  _data[1];
								t->mday = _data[2];
								t->hour = _data[3];
								t->min =  _data[4];
								t->sec =  _data[5];
								rtc_set_time(t);
								*/
							}
							break;
						case 0x86:  //set eeprom
							initPwmValue();
							for (uint16_t i=0;i<(uart_DataLength);i=i+3) {
								setupData.pwmValues[i/3].pwmChannel = _data[i];
								setupData.pwmValues[i/3].timeSlot = _data[i+1];
								setupData.pwmValues[i/3].pwmValue = _data[i+2];
							}
							sortPwmValues(1);
							saveSetupData();
							break;
					}
					isUartData = false;
					_bytes = 0;
					_address = 0;
					uart_DataLength = 0;
					//free memory
					memset(_data,0,400);
				}
			}
		} //konec obsluhy UART

		//pri stisku ovl. prvku rozsvitime display a resetujeme timeout
		if (b != ClickEncoder::Open || value != last) {
			if (lcdTimeout == -1) tft_on();
			lcdTimeout = TIMEOUTPODSVICENI;
			menuTimeout = TIMEOUTMENU;
		}

		//pokud je timeout na display, pak vypneme
		if (lcdTimeout == 0) {
			//reset menu
			menuTimeout = 0;
			//vypnuti lcd
			tft_off();
			lcdTimeout = -1;
		}

		//pokud je timeout na menu, pak reset do hlavni obrazovky
		if ((menuTimeout == 0) && ((isMenu) || (selMenu !=0) )) {
			if (isMenu) {
				//jsme=li v hlavnim menu, pak provedeme reset a prekresleni
				isMenu = false;
				clearMenu();
				drawMenu(MENU_OFF,MENUDEFAULT);
				tftClearWin(P(NONE));
				menuTimeout = -1;
			} else {
				// jsme=li v podmenu, pak nastavime priznak ukonceni
				// pro funkci obsluhujici menu
				resetMenu = true;
			}
		}

		//obluha menu - vyber menu
		if (b == ClickEncoder::Clicked && !isMenu && selMenu == MENU_OFF) {
			isMenu = true;
			drawMenu(MENU_MANUAL,MENUDEFAULT);
			menuItem=MENU_MANUAL;
		} else  if (b == ClickEncoder::Clicked && isMenu ) {
			//vykresleni obrazovky
			switch (menuItem) {
				case MENU_PREF:
					tftClearWin(P(SETTINGS));
					break;
				case MENU_TIME:
					tftClearWin(P(SETTIME));
					break;
				case MENU_MANUAL:
					tftClearWin(MANUAL);
					break;
				case MENU_LEDOFF:
					tftClearWin(OFF);
					break;
				}
				isMenu = false;
				selMenu = menuItem;
		} else if (selMenu != MENU_OFF) {
			//jsme v podmenu, obsluhuj jednotl. funkce
			switch (selMenu) {
				case MENU_PREF:
					settings(value-last, b);
					break;
				case MENU_TIME:
					setTime(value-last, b);
					break;
				case MENU_MANUAL:
					setManual(value-last, b);
					break;
				case MENU_LEDOFF:
					manualOFF=!manualOFF;
					selMenu = MENU_OFF;
					drawMenu(selMenu,MENUDEFAULT);
					break;
				}
		}

		//obsluha otoceni knofliku v hlavnim menu
		if (isMenu && (selMenu == MENU_OFF) && (value != last) ) {
				if (value>last) {
					menuItem = menuItem == MENUDEFAULT?1:menuItem+1;
				} else {
					menuItem = menuItem == 1?4:menuItem-1;
				}
				drawMenu(menuItem,MENUDEFAULT);
		 }

		last = value;
	}
}

