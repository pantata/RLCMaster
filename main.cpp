/*
 * main.c
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */


// FIXME: oprava vypoctu PWM hodnoty dle casu
// TODO: komunikace s I2C ledkama
// TODO: implementace mesice, mraku

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

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define LOW_BYTE(x)        	(x & 0xff)
#define HIGH_BYTE(x)       	((x >> 8) & 0xff)

extern uint8_t Retro8x16[];
extern uint8_t Sinclair_S[];
extern uint8_t Icon[];
extern uint8_t DotMatrix_M_Num[];
extern uint8_t Icon16x16[];

const uint16_t colors[]={VGA_PURPLE,VGA_NAVY,VGA_GREEN,VGA_RED,VGA_WHITE,ILI9341_ORANGE,ILI9341_DARKCYAN};

unsigned long sec = 0, mls = 0;

/*
 * Metoda vraci sumu setupData. Pouziva se pro zjisteni, zdali se
 * nastaveni zmenilo.
 *
 * @return unsigned long suma.
 */
unsigned long getSetupDataSum() {
    unsigned long result = 0UL;
    for (int i = 0, n = sizeof (setupData); i < n; i++) {
        result += (uint8_t) ((uint8_t *) &setupData) [i];
    }
    return result;
}

/*
 * Ulozi data struktury setupData do EEPROM.
 */
void saveSetupData() {

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
void readSetupData() {
    eeprom_read_block (&setupData, (void *) 0, sizeof (setupData));
    lastSetupDataSum = getSetupDataSum();

    if (setupData.version == VERSION) {
    	menuTimeout = constrain(setupData.menuTimeout,5,255);
    	lcdTimeout = constrain(setupData.lcdTimeout,5,255);
    } else {
    	//first run or new version
    	initPwmValue();
    	//reset override value
    	for (int8_t i=0;i<CHANNELS;i++) {
    		setupData.overrideVal[i] = 0;
    	}
    	menuTimeout = TIMEOUTMENU;
    	setupData.menuTimeout = TIMEOUTMENU;
    	lcdTimeout = TIMEOUTPODSVICENI;
    	setupData.lcdTimeout = TIMEOUTPODSVICENI;
    }
}

/*
 *  Posun kurzoru
 */
void moveCrosshair(int16_t x, int16_t y) {
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
void initPwmValue() {
  for (int x=0; x<MAXPWMCOUNT;x++) {
    setupData.pwmValues[x].timeSlot=255; setupData.pwmValues[x].pwmChannel=255; setupData.pwmValues[x].pwmValue=0;
  }
}

/*
 * Pridani LED hodnoty do pole
 */
uint8_t addPwmValue(uint8_t timeslot, uint8_t channel, int16_t pwm) {
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
void sortPwmValues(uint8_t k) {
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
void delPwmValue(uint8_t timeslot, uint8_t channel, int16_t pwm) {
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
int16_t getPwmValue(uint8_t timeslot,uint8_t channel,int16_t _selPwm) {
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
int16_t pwmValue(uint8_t ch) {

	uint8_t ret = 0;
	uint32_t currentTime =  t.hour()*3600L+t.minute()*60L+t.second();
	uint8_t startTime = 255;
	uint8_t endTime = 255;
	int16_t startval = 1023;
	int8_t endval = 0;

	if (manualOFF) return 0;

	//prohledame seznam
	 for (uint8_t i=0;i<(MAXPWMCOUNT-1);i++) {
		if ((setupData.pwmValues[i].pwmChannel == ch) && ((setupData.pwmValues[i].timeSlot*900L) < currentTime) && setupData.pwmValues[i].timeSlot != 255 ) {
			startTime = setupData.pwmValues[i].timeSlot;
			startval = setupData.pwmValues[i].pwmValue;
		}
		if ((setupData.pwmValues[i].pwmChannel == ch) && ((setupData.pwmValues[i].timeSlot*900L) > currentTime) && setupData.pwmValues[i].timeSlot != 255 ) {
			endTime = setupData.pwmValues[i].timeSlot;
			endval = setupData.pwmValues[i].pwmValue;
		}
	 }

	 if ((startTime == 255) || (endTime == 255)) {
		 return 0;
	 } else {
		 ret = map(currentTime, startTime*900L , endTime*900L, startval ,endval);
	 }

	 ret = constrain(ret,0,1023);
	 return (int16_t)ret;
}

/*
 * Zobrazeni casu na LCD
 */
void printTime() {
    char tempStr [9];

    sprintf (tempStr,"%.2d:%.2d:%.2d",t.hour(),t.minute(),t.second());
	tft_setFont(DotMatrix_M_Num);
	tft_setColor(VGA_TEAL);
	tft_print(tempStr,0,0);
	tft_setColor(VGA_WHITE);
	tft_setFont(Sinclair_S);

}

/*
 * Zobrayeni data na LCD
 */
void printDate() {
    char tempStr [9];

    sprintf (tempStr,"%.2d.%.2d.%2d",t.day(),t.month(),t.year());
	tft_setFont(DotMatrix_M_Num);
	tft_setColor(VGA_TEAL);
	tft_print(tempStr,319-(8*tft_getFontXsize()),0);
	tft_setColor(VGA_WHITE);
	tft_setFont(Sinclair_S);

}

/*
 * Vykresli osy grafu
 */
void showAxis() {
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
 * Vymayani oblasti grafu
 */
void clearGraph() {
	tft_setColor(VGA_BLACK);
	tft_fillRect(2,0,96*3,199);
}

/*
 * Vykresleni grafu z pole LED hodnot (kanal, timeslot, hodnota)
 */
void drawGraph(bool del) {
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
				vl2=map(setupData.pwmValues[k].pwmValue,0,1023,0,255);
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
void timer2Init() {
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
void drawMenu(uint8_t i, uint8_t type) {

	tft_setFont(Icon);

    for (uint8_t y=0; y<type; y++) {
    	tft_setColor(i==y+1?VGA_YELLOW:VGA_GRAY);tft_print(menuIcon[type==MENUDEFAULT?y:y+4],267,34+y*48);
    }
}

/*
 * Vymaze oblast menu
 */
void clearMenu() {
	tft_setColor(VGA_BLACK); tft_fillRect(267, 28, 319, 239);
}

/*
 * Vykresli zakladni obrazovku
 */
void tftBackg() {
    tft_clrScr();
    tft_setColor(0x8410);
    tft_drawHLine(0, 26, 319);
    tft_drawVLine(257, 26, 239-26);
    drawMenu(0,MENUDEFAULT);
}

/*
 * vymazani okna
 */

void tftClearWin(char *title) {
    tft_setColor(VGA_BLACK); tft_fillRect(0, 28, 256, 239);
    tft_setFont(Sinclair_S);tft_setColor(VGA_GREEN);tft_print(title, 256/2-(16*8/2), 30);
}

/*
 *  Vykresli sloupcovy graf intenzity LED
 *
 */

static void drawLightChart(int16_t* arr) {

	showAxis();

	tft_setFont(Sinclair_S);
	tft_setColor(VGA_GREEN);

	tft_print(override?MANUAL:manualOFF?OFF:AUTO, 256/2-(16*8/2), 30);

    for (uint8_t i=0; i<CHANNELS; i++) {
        //spocitame vysku dle PWM hodnoty
    	//uint16_t vyska = manualOFF?0:map(arr[i],0,1023,0,180);
    	uint16_t vyska = map(arr[i],0,1023,0,180);

    	//vykreslime barvu
    	tft_setColor(colors[i]);
    	if (vyska > 0) tft_fillRect(30+(32*i), 218-vyska, 60 +(32*i), 218);

    	//vykreslime cernou
    	tft_setColor(VGA_BLACK);
    	tft_fillRect(30+(32*i), 38, 60 +(32*i), 38+180-vyska-1);

    }

    //draw values
    if (selMenu == MENU_OFF) {
		char tempStr[4];
		tft_setFont(Retro8x16);
		for (uint8_t i=0; i<CHANNELS; i++) {
			sprintf(tempStr,"%4d",arr[i]);
			tft_setColor(VGA_GRAY); tft_print(tempStr,26+((32*i)+2),222);
		}
    }

}

/*
 *  obsluha menu nastaveni data a casu
 *  TODO: doplnit timeout pro menu a LCD
 *  TODO: doplnit 12/24 hod
 *  TODO: doplnit format data
 */
void setTime(int16_t value, ClickEncoder::Button b) {

	static int8_t menuPos = -1;
    static bool edit = false;

    static uint16_t color = VGA_WHITE;

    char tempStr [3];

    static uint8_t _datetime[] = {0,0,0,0,0,0};

    //reset menu
    if (resetMenu) {
		//staticke promenne na inicializacni hodnotu
    	menuPos = -1;
    	edit = false;
		color = VGA_WHITE;

		//vypneme priznak, ze jsme v menu
		selMenu = MENU_OFF;

		//vycistime okno
		tftClearWin(NONE);

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
    	if (menuPos < 1) menuPos = 8;
    	if (menuPos > 8) menuPos = 1;
    } else if (edit && value != 0) {
    	switch (menuPos) {
    	case 1: //den TODO: osetrit meze
    		_datetime[3] += value;
    				break;
    	case 2: //mesic
    		_datetime[4] += value;
    	    		break;
    	case 3: //rok
    		_datetime[5] += value;
    	    		break;
    	case 4: //hodiny
    		_datetime[2] += value;
    	    		break;
    	case 5: //minuty
    		_datetime[1] += value;
    	    		break;
    	case 6: //sekundy
    		_datetime[0] +=  value;
    	    		break;
    	}
    }

	if (b==ClickEncoder::DoubleClicked ) {
		//save and exit
	    RTC.adjust(DateTime (2000+_datetime[5],_datetime[4], _datetime[3],
	    		_datetime[0], _datetime[1], _datetime[2]));
		resetMenu = true;
	} else if (b==ClickEncoder::Held) {
		// cancel
		resetMenu = true;
	} else if (b==ClickEncoder::Clicked) {
		// zahajeni nebo ukonceni editace, nebo save nebo cancel
		if (menuPos == 7) {
			//cancel
			resetMenu = true;
		} else if (menuPos == 8) {
			//save and exit
		    RTC.adjust(DateTime (2000+_datetime[5],_datetime[4], _datetime[3],
		    		_datetime[0], _datetime[1], _datetime[2]));
			resetMenu = true;
		} else {
			edit = !edit; if (edit) color = VGA_YELLOW; else color = VGA_WHITE;
		}
	}

		//zobrazeni
		tft_setFont(DotMatrix_M_Num);

		sprintf(tempStr,"%02d",_datetime[3]);tft_setColor(menuPos==1?color:VGA_GRAY); tft_print(tempStr,64,82);tft_setColor(VGA_GRAY);
		tft_print((char*)".",96,82);
		sprintf(tempStr,"%02d",_datetime[4]); tft_setColor(menuPos==2?color:VGA_GRAY);tft_print(tempStr,112,82);tft_setColor(VGA_GRAY);
		tft_print((char*)".",144,82);
		sprintf(tempStr,"%02d",_datetime[5]);tft_setColor(menuPos==3?color:VGA_GRAY); tft_print(tempStr,160,82);

		sprintf(tempStr,"%02d",_datetime[2]);tft_setColor(menuPos==4?color:VGA_GRAY); tft_print(tempStr,64,115);tft_setColor(VGA_GRAY);
		tft_print((char*)":",96,115);
		sprintf(tempStr,"%02d",_datetime[1]); tft_setColor(menuPos==5?color:VGA_GRAY);tft_print(tempStr,112,115);tft_setColor(VGA_GRAY);
		tft_print((char*)":",144,115);
		sprintf(tempStr,"%02d",_datetime[0]); tft_setColor(menuPos==6?color:VGA_GRAY);tft_print(tempStr,160,115);
		drawMenu(menuPos==7?1:menuPos==8?2:0,MENUEDIT);

}

/*
 * Obsluha nastaveni LED hodnot
 */
void settings(int16_t value, ClickEncoder::Button b  ) {

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
		tftClearWin(NONE);

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
			if (_selPwm > 1023) _selPwm = 0;
			if (_selPwm < 0) _selPwm = 1023;
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

	//TODO: uprava na 10bit (1023)
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
void setManual(int16_t value, ClickEncoder::Button b ) {

	static int8_t menuPos = -1;
	static bool edit = false;

	static uint16_t color = VGA_WHITE;

	char tempStr [4];


	//reset menu
	if (resetMenu) {
		//staticke promenne na inicializacni hodnotu
		menuPos = -1;
		edit = false;
		color = VGA_WHITE;

		//vypneme priznak, ze jsme v menu
		selMenu = MENU_OFF;

		//vycistime okno
		tftClearWin(NONE);

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
    		if (setupData.overrideVal[menuPos-1] > 1023) setupData.overrideVal[menuPos-1] = 0;
    		if (setupData.overrideVal[menuPos-1] < 0) setupData.overrideVal[menuPos-1] = 1023;
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

uint8_t  searchSlave() {
	  uint8_t error, address, idx=0;
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
	    }


	  }
	  modulesCount = idx;
	  return modulesCount;

}

void sendValue(uint8_t address, uint8_t channel, int16_t value) {
	twi_begin_transmission(address);
	twi_send_byte(channel);
	//HIGH BYTE
	twi_send_byte(HIGH_BYTE(value));
	//Pak LOW BYTE
	twi_send_byte(LOW_BYTE(value));
	twi_end_transmission();
}

uint8_t readTemperature(uint8_t address) {
	uint8_t temp=0xff;

	twi_begin_transmission(address);
	twi_send_byte(10);
	twi_end_transmission();

	twi_request_from(address, 1);
	temp = twi_receive();
	twi_end_transmission();

	return temp;
}

void printTemp() {
	static uint8_t idx = 0;
	uint8_t temp = 0;
	char tmpStr[4];

	if (modulesCount > 0) {
		temp = 	readTemperature(slaveAddr[idx]);
		idx ++;
		if (idx > (modulesCount-1)) idx = 0;
	}

	sprintf(tmpStr,"%3d~",temp);
	tft_setColor(VGA_YELLOW);tft_setFont(Sinclair_S);
	tft_print(tmpStr,144,18);
}

void displayAccelerationStatus() {
  tft_print("Acceleration ",0,1*16);
  tft_print(encoder->getAccelerationEnabled() ? "on " : "off",100,1*16);
}

/*
 * Hlavni smycka
 */
int main(void) {

	/*
	 *  Setup
	 */

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
	}

	readSetupData();
	uint8_t slaveCount = searchSlave();

	tftBackg();
    tftClearWin(AUTO);
    drawLightChart(channelVal);
	tft_setColor(VGA_YELLOW);tft_setFont(Sinclair_S);
    tft_printNumI(slaveCount,160,0);

    /*
     * Hlavni smycka
     */
	while (1) {

		if ((millis() - timeStamp03) > LEDINTERVAL) {
			timeStamp03 = millis();
			for(uint8_t i=0;i<CHANNELS;i++) {
				int16_t delta = (override?setupData.overrideVal[i]:channelVal[i]) - lastChannelVal[i];
				if (delta != 0) {
					lastChannelVal[i] = lastChannelVal[i] + (abs(delta)<LEDSTEP?delta:delta<0?-LEDSTEP:LEDSTEP);
					//FIXME: odeslat led hodnoty
					//TODO: samostane nastaveni dle adresy modulu
					for (uint16_t addr = 0; addr < modulesCount; addr++) {
						sendValue(slaveAddr[addr], i, lastChannelVal[i]);
					}

				}
			}
		}

		if ((millis() - timeStamp01) > SECINTERVAL) {
		    t = RTC.now();
			if (lcdTimeout > 0 ) {
				printTime();
				lcdTimeout--;
				printTemp();
			}

			if (menuTimeout > 0 ) { menuTimeout--; }

		    if (t.day() != _dateTimeStamp) {
		    	_dateTimeStamp = t.day();
		    }

			if (lcdTimeout > 0 ) { printDate(); }

			//spocitame hodnotu
			for(uint8_t i=0;i<CHANNELS;i++) {
				//lastChannelVal[i] = channelVal[i];
				channelVal[i] = pwmValue(i);
			}
			timeStamp01 = millis(); // take a new timestamp
		}

		if ((millis() - timeStamp02) > LCDREFRESH) {
			if (selMenu == 0 ) drawLightChart(override?setupData.overrideVal:channelVal);
			timeStamp02 = millis();
		}


		/*
		 *  Obsluha UART komunikace
		 */

		if (uart_available()) { //data cekaji ve fromte

			uint8_t uart_data[8];
			uint8_t *p = uart_getPckt();

			memcpy((void*)uart_data,(void*)p,8);

			//tft_setFont(Icon16x16);tft_setColor(VGA_BLUE);tft_print("6",BTPOSX,BTPOSY);

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
			//tft_setFont(Icon16x16);tft_setColor(VGA_BLACK);tft_print("0",BTPOSX,BTPOSY);
		} //konec obsluhy UART

		ClickEncoder::Button b = encoder->getButton();
		value += encoder->getValue();

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
				tftClearWin(NONE);
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
					tftClearWin(SETTINGS);
					break;
				case MENU_TIME:
					tftClearWin(SETTIME);
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
					menuItem++;
					if (menuItem > MENUDEFAULT) menuItem = 1;
				} else {
					menuItem--;
					if (menuItem < 1) menuItem = MENUDEFAULT;
				}
				drawMenu(menuItem,MENUDEFAULT);
		 }

		last = value;
	}
}

