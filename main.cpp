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


#define DEBUG

#define F_CPU 20000000UL

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
//#include "encoder.h"

#include "state.h"
#include "main.h"
#include "twi.h"

#include "rtc8563.h"
#include "uart.h"

#ifdef DEBUG
#include "dbg_putchar.h"
#endif

#include "lang.h"
#include "slave.h"
#include "twi_registry.h"

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define LOW_BYTE(x)        	(uint8_t)(x & 0x00ff)
#define HIGH_BYTE(x)       	(uint8_t)((x & 0xff00) >> 8)

#define BYTELOW(v)   (*(((unsigned char *) (&v))))
#define BYTEHIGH(v)  (*((unsigned char *) (&v)+1))

extern uint8_t Font8x16[];
extern uint8_t Sinclair_S[];
extern uint8_t Icon[];
extern uint8_t DotMatrix_M_Num[];
extern uint8_t Icon16x16[];
extern uint8_t nereus[];

const uint16_t colors[]={VGA_WHITE,VGA_PURPLE,VGA_NAVY,VGA_BLUE,VGA_GREEN,VGA_RED,ILI9341_ORANGE};

//RIZENI
unsigned long timeStamp01 = 0;
unsigned long timeStamp02 = 0;
unsigned long timeStamp03 = 0;

unsigned long sec = 0, mls = 0;

bool redraw = false;

#ifdef DEBUG
static void tft_println(const char *s) {

	#define MAX_LINES 25
	#define OFFSET    30

	static uint8_t line = 0;

	tft_setFont(Sinclair_S);
	tft_setColor(VGA_YELLOW);

	tft_print(s, 0, OFFSET + (line * tft_getFontYsize()));

	line++;
	if (line > MAX_LINES) {
		line = 0;
		//clear debug area
		tftClearWin(P(NONE));
	}
}

static void tft_printT(unsigned long t) {
	char b[12];
	sprintf(b,"T:%6lu",t);
	tft_setFont(Sinclair_S);
	tft_setColor(VGA_YELLOW);
	tft_print(b, 320-(tft_getFontXsize()*8), 240 - tft_getFontYsize());
}


static void printUartPacket() {
	char tmp[26];
	sprintf(tmp,"%02X %02X %02X %02X %02X %02X %02X %02X\n",uart_data[0],uart_data[1],uart_data[2],uart_data[3],uart_data[4],uart_data[5],uart_data[6],uart_data[7]);
	dbg_puts(tmp);
}

#define dprint(s) dbg_puts(s);
#else
#define tft_println(...) for(;0;)
#define tft_printT(...) for(;0;)
#define dprint(s) for(;0;)
#endif


void clearData() {
	memset(_data,0,sizeof(_data));
}

void setConfigFromWifi() {
	setupData.lcdTimeout = 127;//constrain(_data[0],5,127);
	setupData.menuTimeout = 127; //constrain(_data[1],5,127);
	lcdTimeout = setupData.lcdTimeout;
	menuTimeout = setupData.menuTimeout;
	setupData.dt_fmt =constrain(_data[2],0,4);
	setupData.tm_fmt = constrain(_data[3],0,7);
	setupData.useDST = constrain(_data[4],0,1);
	clearData() ;
}

void setNetInfo(uint16_t l) {
	uint8_t stg = 0;
	uint8_t idx = 0;
	//4 byte ip addresa,
	//wifi nazev, zakonceno \255
	//heslo zakonceno \255
	//hostname zakonceno \0
	ip[0] = _data[0];
	ip[1] = _data[1];
	ip[2] = _data[2];
	ip[3] = _data[3];

	for (uint16_t d=6;d<l;d++) {
		//ssid
		switch (stg) {
			case 0: //ssid
				hostname[idx] = _data[d];
				break;
			case 1: //pwd
				pwd[idx] = _data[d];
				break;
			case 2: //pwd
				ssid[idx] = _data[d];
				break;
			/*
			case 2: //hostname
				hostname[idx] = _data[d];
				break;
			*/
		}
		idx++;
		if (_data[d] == 0xff) { stg++; idx = 0; }
	}
}

void setLedVal(bool man, uint16_t l) {
	uint16_t c=0;
	//TODO: doplnit manual (overrideVal)

	for (uint8_t mod=0; mod < 16; mod++ ) {
		for (uint8_t led=0; led < 7; led ++) {
			if (man)
				setupData.overrideVal[mod][led] = ( _data[c] | (_data[c+1] << 8) );
			else
				channelVal[mod][led] = ( _data[c] | (_data[c+1] << 8) );
			c = c+2;
		}
	}

	redraw = true;
}


void move(){
    override_x++;
    if(override_x == 7){
        override_y++;
        override_x = 0;
        if(override_y >= 16){
            override_y = 0;
            override_x = 0;
        }
    }
    ledLowValuesOK = false;
    ledHighValuesOK = false;
}

void sendOverrideValue(int16_t value){

	uint8_t intValue;

    if(!ledValuesOK && !ledLowValuesOK){
    	intValue = LOW_BYTE(value);
        uart_writeB(&intValue,sizeof(intValue));
    }else{
        ledLowValuesOK = true;
        ledValuesOK = false;
    }
    if(ledLowValuesOK && !ledValuesOK && !ledHighValuesOK){
    	intValue = HIGH_BYTE(value);
    	uart_writeB(&intValue,sizeof(intValue));
    }else{
        ledHighValuesOK = true;
        ledValuesOK = false;
    }
    if(ledHighValuesOK && ledLowValuesOK){
       move();
    }
}

void handleWifiData() {

#ifdef DEBUG
	printUartPacket();
#endif

	if (!isUartData) {
		uart_cmd = (uart_data[0]);
		switch (wifiState) {
			case ping_s:
				if ((uart_data[1] == 'O') && (uart_data[2] == 'K') ) {
					wifiFound();
					wifiState = config;
				} else {
					wifiState = ping;
				}
				wifiClear = true;
				break;
			case config_s:
				//handle Config data
				_bytes = 0;
				isUartData = true;
				uDataIsComplete = false;
				uart_DataLength = (uart_data[2] << 8) | uart_data[1];
				uDataLength = uart_DataLength;
				break;
			case change_s:
				//any change on settings
				//tft_print("No Change",100,0);
				wifiClear = true;
				break;
			case time_s:
				setTimeFromWiFi(uart_data);
				wifiClear = true;
				wifiState = none;
				break;
			case ledTimeSlots_s:
				//nasleduji data
				_bytes = 0;
				isUartData = true;
				uDataIsComplete = false;
				uart_DataLength = (uart_data[2] << 8) | uart_data[1];
				uDataLength = uart_DataLength;
				wifiState = (wifiState_t)uart_data[0];
				break;
			case ledManual_s:
				if ((uart_data[1] == 'O') && (uart_data[2] == 'K') ) {
					wifiState = ledValues;
				}
				wifiClear = true;
				break;
			case ledOff_s:
				if ((uart_data[1] == 'O') && (uart_data[2] == 'K') ) {
					;
				}
				wifiClear = true;
				break;
			default:
				//nic jsme nezadali, web vyvolal akci
				//otestujeme, zda nejde o command bez dat
				switch (uart_cmd) {
					case none:
						break;
					case ledOff_s:
						manualOFF = true;
						wifiClear = true;
						wifiState = none;
						break;
					case time_s:
						setTimeFromWiFi(uart_data);
						wifiClear = true;
						wifiState = none;
						break;
					default:
						dprint("DEFAULT");
						wifiState = (wifiState_t)uart_cmd;
						_bytes = 0;
						isUartData = true;
						uDataIsComplete = false;
						uart_DataLength = (uart_data[2] << 8) | uart_data[1];
						uDataLength = uart_DataLength;
						break;
				}
				break;
		}
	} else {
		//v paketech jsou data
		//6 byte, konci kontrolnim souctem
		//az do celkova delka
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

		if (uDataLength == 0) uDataIsComplete = true;

		if (uDataIsComplete) { //zpracujeme prijata
			isUartData = false;
			wifiClear = true;
			switch (wifiState) {
				case config_s:
					setConfigFromWifi();
					wifiState = time;
					wifiClear = true;
					break;
				case ledTimeSlots_s: //aktualni timeslot hodnoty
					override = false;
					wifiState = none;
					wifiClear = true;
					setLedVal(false,uart_DataLength);
					break;
				case ledValues_s: //hodnoty led z manualniho nastaveni
					override = true;
					setLedVal(true,uart_DataLength);
					wifiState = none;
					wifiClear = true;
					break;
				case netInfo_s:
					setNetInfo(uart_DataLength);
					wifiState = none;
					wifiClear = true;
					break;
				default:
					break;
			}
		}
	}
}

void sendWiFiCommand() {
	char tmpbuff[9];
	memset((void*)tmpbuff, 0,9);
	uint16_t crc;
	switch (wifiState) {
		case ping:
			sprintf(tmpbuff,PING);
			crc = getCrc(tmpbuff);
			tmpbuff[6] = LOW_BYTE(crc);
			tmpbuff[7] = HIGH_BYTE(crc);
			uart_writeB((uint8_t *)tmpbuff,8);
			wifiState = ping_s;
			wifiClear = false;
			dprint("ping_s:\n");
			break;
		case time:
			sprintf(tmpbuff,GETTIME);
			crc = getCrc(tmpbuff);
			tmpbuff[6] = LOW_BYTE(crc);
			tmpbuff[7] = HIGH_BYTE(crc);
			uart_writeB((uint8_t *)tmpbuff,8);
			wifiState = time_s;
			wifiClear = false;
			dprint("time_s\n");
			break;
		case config:
			if (RTC.getStatus()) unixtime.time = 0; else  unixtime.time = RTC.now().unixtime();
			sprintf(tmpbuff,GETCONFIG,modulesCount, unixtime.btime[0],unixtime.btime[1],unixtime.btime[2],unixtime.btime[3]);
			crc = getCrc(tmpbuff);
			tmpbuff[6] = LOW_BYTE(crc);
			tmpbuff[7] = HIGH_BYTE(crc);
			uart_writeB((uint8_t *)tmpbuff,8);
			wifiState = config_s;
			wifiClear = false;
			dprint("config_s\n");
			break;
		case ledValues:
			sprintf(tmpbuff,GETLEDVALUES);
			crc = getCrc(tmpbuff);
			tmpbuff[6] = LOW_BYTE(crc);
			tmpbuff[7] = HIGH_BYTE(crc);
			uart_writeB((uint8_t *)tmpbuff,8);

			wifiState = ledTimeSlots_s;
			wifiClear = false;
			dprint("ledTimeSlots_s\n");
			break;
		case netInfo:
			sprintf(tmpbuff,GETNETVALUES);
			crc = getCrc(tmpbuff);
			tmpbuff[6] = LOW_BYTE(crc);
			tmpbuff[7] = HIGH_BYTE(crc);
			uart_writeB((uint8_t *)tmpbuff,8);
			wifiState = netInfo_s;
			wifiClear = false;
			dprint("netInfo_s\n");
			break;
		case ledManual:
			sprintf(tmpbuff,LEDMANUAL,override);
			crc = getCrc(tmpbuff);
			tmpbuff[6] = LOW_BYTE(crc);
			tmpbuff[7] = HIGH_BYTE(crc);
			uart_writeB((uint8_t *)tmpbuff,8);
			wifiState = ledManual_s;
			dprint("ledManual_s\n");
			if (override) {
				///TODO: send led values
				//uart_writeB((uint8_t*)setupData.overrideVal,sizeof(setupData.overrideVal));
			}
			wifiClear = false;
			break;
		case ledOff:
			sprintf(tmpbuff,LEDOFF);
			crc = getCrc(tmpbuff);
			tmpbuff[6] = LOW_BYTE(crc);
			tmpbuff[7] = HIGH_BYTE(crc);
			uart_writeB((uint8_t *)tmpbuff,8);
			wifiState = ledOff_s;
			wifiClear = false;
			dprint("ledOff_s\n");
			break;

		default:
			break;
	}
}

void wifiFound() {
	isWiFi = true;
}

void setTimeFromWiFi(uint8_t *data) {
	if (data[1] == 2) {
		unixtime.btime[0] = data[2];
		unixtime.btime[1] = data[3];
		unixtime.btime[2] = data[4];
		unixtime.btime[3] = data[5];
		DateTime at = DateTime(unixtime.time);
		RTC.adjust(at.year()>2000?at.year()-2000:0, at.month(), at.day(), at.hour(), at.minute(), at.second());
	}
}

/*
 *
 */
uint16_t checkCrc(uint8_t *data) {

	uint16_t crc = 0xffff;

	for (uint8_t i = 0; i < 8; i++) {
		crc = crc16_update(crc, data[i]);
	}
	return crc;
}


uint16_t getCrc(char *data) {

	uint16_t crc = 0xffff;

	for (uint8_t i = 0; i < 6; i++) {
		crc = crc16_update(crc, data[i]);
	}
	return crc;
}


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
    	setupData.tm_fmt = constrain(setupData.tm_fmt,0,7);
    	setupData.dt_fmt = constrain(setupData.dt_fmt,0,4);
    } else {
    	setupData.menuTimeout = TIMEOUTMENU;
    	setupData.lcdTimeout = TIMEOUTPODSVICENI;
    	setupData.tm_fmt = 0;
    	setupData.dt_fmt = 0;
    }
}


static void printDateTime() {
	char tempStr [13];
	tft_setFont(DotMatrix_M_Num);
	tft_setColor(VGA_TEAL);
	switch (setupData.tm_fmt) { //["HH:MI:SS P","HH.MI:SS P","HH24:MI:SS","HH24.MI:SS","HH:MI P","HH.MI P","HH24:MI","HH24.MI"];
	case 0:
		sprintf (tempStr,"%02d:%02d:%02d%s",t.hour()>12?t.hour()-12:t.hour(),t.minute(), t.second(),t.hour()>12?">":"<");
		break;
	case 1:
		sprintf (tempStr,"%02d.%02d.%02d%s",t.hour()>12?t.hour()-12:t.hour(),t.minute(), t.second(),t.hour()>12?">":"<");
		break;
	case 2:
		sprintf (tempStr,"%02d:%02d:%02d=",t.hour(),t.minute(), t.second());
		break;
	case 3:
		sprintf (tempStr,"%02d.%02d:%02d=",t.hour(),t.minute(), t.second());
		break;
	case 4:
		sprintf (tempStr,"%02d:%02d%s=====",t.hour()>12?t.hour()-12:t.hour(),t.minute(),t.hour()>12?">":"<");
		break;
	case 5:
		sprintf (tempStr,"%02d.%02d%s=====",t.hour()>12?t.hour()-12:t.hour(),t.minute(),t.hour()>12?">":"<");
		break;
    case 6:
    	sprintf (tempStr,"%02d:%02d======",t.hour(),t.minute());
		break;
	case 7:
		sprintf (tempStr,"%02d.%02d======",t.hour(),t.minute());
		break;
	default:
		sprintf (tempStr,"%02d.%02d======",t.hour(),t.minute());
	}
	tft_print(tempStr,0,0);
	memset(tempStr,'\0',13);
	switch (setupData.dt_fmt) {  //"DD.MM.YY","DD/MM/YY","DD-MM-YY","YY/MM/DD","YY-MM-DD"
		case 0:
			sprintf (tempStr,"%02d.%02d.%02d",t.day(),t.month(),t.year()-2000);
			break;
		case 1:
			sprintf (tempStr,"%02d/%02d/%02d",t.day(), t.month(),t.year()-2000);
			break;
		case 2:
			sprintf (tempStr,"%02d;%02d;%02d",t.day(), t.month(),t.year()-2000);
			break;
		case 3:
			sprintf (tempStr,"%02d/%02d/%02d",t.year()-2000,t.month(),t.day());
			break;
		case 4:
			sprintf (tempStr,"%02d;%02d;%02d",t.year()-2000,t.month(),t.day());
			break;
		default:
			sprintf (tempStr,"%02d.%02d.%02d",t.day(),t.month(),t.year()-2000);
	}
	tft_print(tempStr,319-(8*tft_getFontXsize()),0);

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
	    //OCR2A = 0xF9;
	    OCR2A = 78;

	    TIMSK2 |= (1 << OCIE2A);    //Set the ISR COMPA vect

	    // set prescaler to 64 and start the timer
	    TCCR2B |= (1 << CS22) | (1 << CS21);

	 TIMSK2 |= (1<<TOIE2);
}

/*
 * Interrupt pro timer 2
 */

ISR (TIMER2_COMPA_vect)
{
	encoder->service();
	//enc_service();
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
    tft_setFont(Font8x16);tft_setColor(VGA_GREEN);tft_print(title, 256-(16*8), 30);
}

/*
 * vykresli status bar
 * levy sloupec o sirce 4*16 bodu
 *  vyska 240 - 30 bodu
 *  TODO: pridat parametry obsahu status baru
 */
static void drawStatusBar() {
	char tmpStr[5];

	if (isWiFi) {
		tft_setFont(Icon16x16);
		tft_setColor(VGA_YELLOW);
		tft_print(wifi[2],0,30);
	}

	tft_setColor(VGA_WHITE);
	tft_setFont(Font8x16);;
	sprintf(tmpStr,"\\ % 2d",modulesCount);
	tft_print(tmpStr,20,30);
	printTemperature();
	tft_setColor(VGA_WHITE); tft_drawVLine(4*8+4,30,15);
}

/*
 *  Vykresli sloupcovy graf intenzity LED
 *
 */

static void drawLightChart() {

	tft_setFont(Font8x16);
	tft_setColor(VGA_GREEN);
	//modules info
	char s[2];
	s[0]=modulesCount>0?64+selMod:'X';s[1]='\0';
	tft_print(s,20+12*8,30);

	if (override) {
		tft_print(P(MANUAL),256-(16*8), 30);
	} else if (manualOFF) {
		tft_print(P(OFF),256-(16*8), 30);
	} else {
		tft_print(P(AUTO),256-(16*8), 30);
	}
	//tft_print(override?P(MANUAL):manualOFF?P(OFF):P(AUTO), 256-(16*8), 30);

	if (modulesCount == 0) return;

	uint8_t m = selMod>0?selMod-1:0;
	uint8_t vyska = 0;


	for (uint8_t x=0; x<7;x++) {
		char tempStr[5];
		vyska = (uint8_t)map(override?setupData.overrideVal[m][x]:channelVal[m][x],0,PWMSTEPS,0,158);
		int start = 8+(x*32)+(x*3);
		tft_setColor(colors[x]); tft_fillRect(start+8,208-vyska,start+24,208);
		tft_setColor(VGA_BLACK);tft_fillRect(start+9,51,start+23,208-vyska);
		tft_setColor(colors[x]); tft_drawRect(start+8,50,start+24,208);
		if (selMenu == MENU_OFF) {
			tft_setColor(VGA_GRAY);
			sprintf(tempStr,"%4d",override?setupData.overrideVal[m][x]:channelVal[m][x]);
			tft_print(tempStr,start,222);
		};
	}
	tft_setColor(VGA_GRAY);
	tft_drawHLine(8,209,246);
}



/*
 * Zobrazi nastaveni wifi
 * ip adresu , nazev site a heslo
 *
 */
static void settings(int16_t value, ClickEncoder::Button b) {
	static int8_t menuPos = -1;
    static bool edit = false;

    //reset menu
    if (resetMenu) {
		//staticke promenne na inicializacni hodnotu
    	menuPos = -1;
    	edit = false;
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

    if (menuPos < 0) {
    	menuPos = 0;
    	clearMenu();
    	drawMenu(0,MENUEDIT);
    }

    if (!edit && value != 0) {
    	menuPos = menuPos + value;
    	if (menuPos < 1) menuPos = 1;
    	if (menuPos > 1) menuPos = 1;
    }

	if (b==ClickEncoder::DoubleClicked ) {
		resetMenu = true;
	} else if (b==ClickEncoder::Held) {
		resetMenu = true;
	} else if (b==ClickEncoder::Clicked) {
		resetMenu = true;
	}

	char tempStr[33]; //(max delka ssid je 32 char)
	//zobrazeni
	tft_setFont(Font8x16);
	tft_setColor(VGA_GRAY);
	//TODO zobrazeni: WiFi network, password, ip adresa, url
	sprintf(tempStr,"%3d.%3d.%3d.%3d",ip[0],ip[1],ip[2],ip[3]);
	tft_print(tempStr,0,60);

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
    		if (setupData.dt_fmt > 4) setupData.dt_fmt = 0;
    		if (setupData.dt_fmt < 0) setupData.dt_fmt = 4;
    				break;
    	case 2:
    		setupData.tm_fmt += value;
    		if (setupData.tm_fmt > 7) setupData.tm_fmt = 0;
    		if (setupData.tm_fmt < 0) setupData.tm_fmt = 7;
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
		    //save Setup
		    saveSetupData();
		    wifiState = config;
			resetMenu = true;
		} else {
			edit = !edit; if (edit) color = VGA_YELLOW; else color = VGA_WHITE;
		}
	}

	char tempStr[9];
		//zobrazeni
		tft_setFont(Font8x16);
		tft_setColor(VGA_GRAY);tft_print(P(DTFORMAT),10,52);
		switch (setupData.dt_fmt) {
			case 0:
				sprintf(tempStr,"%s",P(DDMMYY0));
				break;
			case 1:
				sprintf(tempStr,"%s",P(DDMMYY1));
				break;
			case 2:
				sprintf(tempStr,"%s",P(DDMMYY2));
				break;
			case 3:
				sprintf(tempStr,"%s",P(DDMMYY3));
				break;
			case 4:
				sprintf(tempStr,"%s",P(DDMMYY4));
				break;
			default:
				sprintf(tempStr,"%s",P(DDMMYY0));
				break;
		}
		tft_setColor(menuPos==1?color:VGA_GRAY); tft_print(tempStr,74,52);
		switch (setupData.tm_fmt) {
			case 0:
				sprintf(tempStr,"%s",P(HH0));
				break;
			case 1:
				sprintf(tempStr,"%s",P(HH1));
				break;
			case 2:
				sprintf(tempStr,"%s",P(HH2));
				break;
			case 3:
				sprintf(tempStr,"%s",P(HH3));
				break;
			case 4:
				sprintf(tempStr,"%s",P(HH4));
				break;
			case 5:
				sprintf(tempStr,"%s",P(HH5));
				break;
			case 6:
				sprintf(tempStr,"%s",P(HH6));
				break;
			case 7:
				sprintf(tempStr,"%s",P(HH7));
				break;
			default:
				sprintf(tempStr,"%s",P(HH7));
				break;
		}
		tft_setColor(menuPos==2?color:VGA_GRAY); tft_print(tempStr,160,52);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(YEAR));tft_print(tempStr,10,72);
		sprintf(tempStr,"%02d",_datetime[5]);tft_setColor(menuPos==3?color:VGA_GRAY); tft_print(tempStr,74,72);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(MONTH));tft_print(tempStr,10,92);
		sprintf(tempStr,"%02d",_datetime[4]); tft_setColor(menuPos==4?color:VGA_GRAY);tft_print(tempStr,74,92);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(DAY));tft_print(tempStr,10,112);
		sprintf(tempStr,"%02d",_datetime[3]);tft_setColor(menuPos==5?color:VGA_GRAY); tft_print(tempStr,74,112);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(HOUR));tft_print(tempStr,10,132);
		sprintf(tempStr,"%02d %s",(setupData.tm_fmt & 3)>1?_datetime[2]:_datetime[2]>12?_datetime[2]-12:_datetime[2],(setupData.tm_fmt & 3)>1?"   ":_datetime[2]>12?P(PM):P(AM));tft_setColor(menuPos==6?color:VGA_GRAY); tft_print(tempStr,74,132);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(MINUTE));tft_print(tempStr,10,152);
		sprintf(tempStr,"%02d",_datetime[1]); tft_setColor(menuPos==7?color:VGA_GRAY);tft_print(tempStr,74,152);

		tft_setColor(VGA_GRAY);sprintf(tempStr,"%s",P(SECOND));tft_print(tempStr,10,172);
		sprintf(tempStr,"%02d",_datetime[0]); tft_setColor(menuPos==8?color:VGA_GRAY);tft_print(tempStr,74,172);
		drawMenu(menuPos==9?1:menuPos==10?2:0,MENUEDIT);

}

/*
 * Obsluha nastaveni LED hodnot pro manualni ovladani
 */
static void setManual(int16_t value, ClickEncoder::Button b ) {

	static int8_t menuPos = -1;
	static bool edit = false;

	static uint16_t color = VGA_WHITE;

	char tempStr [5];

	int8_t m = selMod>0?selMod-1:0;

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
		override = true;
		menuPos = 1;
		clearMenu();
		drawMenu(MENU_OFF,MENUEDIT);
		drawLightChart();
	}

    if (!edit && value != 0) {
    	menuPos = menuPos + value;
    	if (menuPos < 1) menuPos = 10;
    	if (menuPos > 10) menuPos = 1;
    } else if (edit && value != 0) {
    	switch (menuPos) {
    		case 1:
				 if (modulesCount > 0) {
					selMod = selMod + value;
					if (selMod > modulesCount) selMod = 0;
					if (selMod < 0) selMod = modulesCount;
					m = selMod>0?selMod-1:0;
				 }
    			break;
    		case 2 ... 8:
				if (selMod == 0) {
					for (uint8_t x=0; x< CHANNELS; x++) {
						setupData.overrideVal[x][menuPos-2] += value;
						if (setupData.overrideVal[x][menuPos-2] > (PWMSTEPS-1)) setupData.overrideVal[x][menuPos-2] = 0;
						if (setupData.overrideVal[x][menuPos-2] < 0) setupData.overrideVal[x][menuPos-2] = (PWMSTEPS-1);
					}
				} else {
					setupData.overrideVal[m][menuPos-2] += value;
					if (setupData.overrideVal[m][menuPos-2] > (PWMSTEPS-1)) setupData.overrideVal[m][menuPos-2] = 0;
					if (setupData.overrideVal[m][menuPos-2] < 0) setupData.overrideVal[m][menuPos-2] = (PWMSTEPS-1);
				}
				break;
    	}
    }

	if (b==ClickEncoder::DoubleClicked ) {
		//save and exit
		if (selMod == 0) selMod = 1;
		override = true;
		resetMenu = true;
		wifiState = ledManual;
		saveSetupData();
	} else if (b==ClickEncoder::Held) {
		// cancel
		if (selMod == 0) selMod = 1;
		override = false;
		resetMenu = true;
		wifiState = ledManual;
	} else if (b==ClickEncoder::Clicked) {
		if (selMod == 0) selMod = 1;
		// zahajeni nebo ukonceni editace, nebo save nebo cancel
		if (menuPos == 9) {
			//cancel
			override = false;
			resetMenu = true;
			wifiState = ledManual;
		} else if (menuPos == 10) {
			//save and exit
			override = true;
			resetMenu = true;
			wifiState = ledManual;
		} else {
			edit = !edit; if (edit) color = VGA_YELLOW; else color = VGA_WHITE;
		}
	}

	if ( value != 0) drawLightChart();

	tft_setFont(Font8x16);
	for (uint8_t i=0; i<CHANNELS; i++) {
		sprintf(tempStr,"%4d",setupData.overrideVal[m][i]);
		tft_setColor(menuPos==i+2?color:ILI9341_DARKGREY); tft_print(tempStr,8+(i*32)+(i*3),222);
	}
	drawMenu(menuPos==9?1:menuPos==10?2:0,MENUEDIT);
}

static void  searchSlave() {
	  uint8_t error, address, idx=0;

#ifdef DEBUG
	  char tempStr [20];
#endif

	  memset(slaveAddr,255,16);
	  for(address = 0x10; address <= 0x40; address++) {
		twi_begin_transmission(address);
			twi_send_byte(reg_MASTER); //register
			twi_send_byte(0xff); //value 0xFF = controller presence
	    error = twi_end_transmission();

	    if (error == 0) {
	    	slaveAddr[idx] = address;
	    	idx ++;
#ifdef DEBUG
	    	sprintf(tempStr,"%d: %03xh",idx-1, slaveAddr[idx-1]);
	    	tft_println(tempStr);
#endif
	    }

      modulesCount = idx;

	  }
	  return;
}

static void sendValue(uint8_t address, int8_t m) {

	uint8_t error = 0;

	//remap led position
	#define c_white  0
	#define c_uv     1
	#define c_rb     2
	#define c_blue   3
	#define c_green  4
	#define c_red    5
	#define c_yellow 6

	//uint8_t led_colors[CHANNELS] = {c_rb,c_white,c_green,c_red,c_green,c_red,c_uv};

	uint16_t crc = 0xffff;
//#ifndef DEBUG

//#define CRC(x)  crc = crc16_update(crc,LOW_BYTE(override?setupData.overrideVal[m][x]:channelVal[m][x]));crc = crc16_update(crc,HIGH_BYTE(override?setupData.overrideVal[m][x]:channelVal[m][x]))
//#define SEND(color)  twi_send_byte(HIGH_BYTE(override?setupData.overrideVal[m][color]:channelVal[m][color])); twi_send_byte(LOW_BYTE(override?setupData.overrideVal[m][color]:channelVal[m][color]))

/*#else

#define CRC(x)  crc = crc16_update(crc,1);crc = crc16_update(crc,0)
#define SEND(color)  twi_send_byte(0); twi_send_byte(1)

#endif*/

#ifdef DEBUG
	char tempStr [30];
    tft_clrScr();
#endif

	twi_begin_transmission(address);
	twi_send_byte(reg_LED_START); //register address

	for (uint8_t x=0; x < CHANNELS; x++) {


		uint16_t c_val = override?setupData.overrideVal[m][x]:channelVal[m][x];
		uint8_t lb = LOW_BYTE(c_val);
		uint8_t hb = HIGH_BYTE(c_val);

		crc = crc16_update(crc,lb);
		crc = crc16_update(crc,hb);

#ifdef DEBUG
		sprintf(tempStr,"%d: %d - %d - %d",setupData.overrideVal[m][x],hb,lb);
		tft_println(tempStr);
#endif
		twi_send_byte(hb);
		twi_send_byte(lb);


		twi_send_byte(HIGH_BYTE(override?setupData.overrideVal[m][x]:channelVal[m][x]));
		twi_send_byte(LOW_BYTE(override?setupData.overrideVal[m][x]:channelVal[m][x]));

	}


/*
	CRC(led_colors[0]);
	CRC(led_colors[1]);
	CRC(led_colors[2]);
	CRC(led_colors[3]);
	CRC(led_colors[4]);
	CRC(led_colors[5]);
	CRC(led_colors[6]);

	//register 0 , 7x 2B channel values
	twi_begin_transmission(address);
	twi_send_byte(reg_LED_START); //register address
	//send channel values


	SEND(led_colors[0]);
	SEND(led_colors[1]);
	SEND(led_colors[2]);
	SEND(led_colors[3]);
	SEND(led_colors[4]);
	SEND(led_colors[5]);
	SEND(led_colors[6]);
*/

	//crc
	twi_send_byte(HIGH_BYTE(crc)); twi_send_byte(LOW_BYTE(crc));
	error = twi_end_transmission();

	twi_begin_transmission(address);
	twi_send_byte(reg_DATA_OK); //register address
	twi_send_byte(error);
	error = twi_end_transmission();


#ifdef DEBUG
	/*
	_delay_ms(50);
	char tempStr [30];
	if (error == 0) tft_println("SUCCESS");
	uint16_t temp;

	uint8_t temp1=0x00;
	uint8_t temp2=0x00;

	twi_begin_transmission(address);
	twi_send_byte(reg_LED_0);
	error = twi_end_transmission();

	uint8_t cnt = twi_request_from(address, 14);

	uint8_t x=0;
    if (cnt > 13) {
    	while (twi_available) {
    		BYTEHIGH(temp) = twi_receive();
    		BYTELOW(temp) = twi_receive();
			sprintf(tempStr,"%d: %04d-%04d-0x%04x",x,override?setupData.overrideVal[m][led_colors[x]]:channelVal[m][led_colors[x]],temp,temp);
			tft_println(tempStr);
			x++;
    	}
	}

    twi_begin_transmission(address);
	twi_send_byte(reg_CRC);
	error = twi_end_transmission();

	cnt = twi_request_from(address, 2);

    if (cnt > 1) {
		temp1 = twi_receive();
		temp2 = twi_receive();
	}

	sprintf(tempStr,"0x%02x:0x%02x-0x%02x:0x%02x",BYTEHIGH(crc),BYTELOW(crc),temp1, temp2);
	tft_println(tempStr);
*/
#endif

}

static uint8_t readTemperature(uint8_t address) {
	uint8_t ret = 0;
	uint8_t temp=0xff;
	uint8_t temp_status=0xee;

	twi_begin_transmission(address);
	twi_send_byte(reg_THERM_STATUS);
	twi_end_transmission();

	uint8_t cnt = twi_request_from(address, 2);
	if (cnt > 1) {
		temp_status = twi_receive();
		temp = twi_receive();
	}

	if (temp_status) {
		ret  = temp;
	}

	return ret;
}

void printTemperature() {

	uint8_t temp = 0;
	char tmpStr[8];

	tft_setFont(Font8x16);

	if (modulesCount > 0) {
		temp = 	readTemperature(slaveAddr[selMod-1]);
	}

#ifdef DEBUG
    //temp = 36;
#endif

	if (temp < 30) tft_setColor(VGA_GREEN);
		else if (temp < 40)
			tft_setColor(VGA_YELLOW);
				else tft_setColor(VGA_RED);
	if (temp == 0) {
		tft_print("^ - ~",20+5*8,30);
	} else {
		sprintf(tmpStr,"^ %2d~",temp);
		tft_print(tmpStr,20+5*8,30);
	}
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
	cli();
	//encoder
	timer2Init();

	//millis
	millis_init();
	//twi
	twi_init_master();
	//uart
	uart_init();

	encoder = new ClickEncoder(4);

	sei();

	//spi
	spi_init();

	//rtc
	RTC.begin();

	//tft
	tft_init();
	tft_setRotation(3);
    tft_clrScr();


    tft_clrScr();
    tft_setColor(VGA_WHITE);
    tft_setFont(Sinclair_S);

    tft_print("Init...",0,0);

	//Search slave modules
    tft_println("Search ...");
	searchSlave();

#ifdef DEBUG
	if (modulesCount < 1) modulesCount = 1;
#endif

	//wait for wifi/*
    tft_setColor(VGA_PURPLE);
    tft_setFont(nereus);
    tft_print("012134",64,64);

    for (uint8_t i=0; i<7; i++) {
    	tft_setColor(colors[i]);
    	tft_fillRect(64,130+(i*3),64+186,130+(i*3)+3);
    	_delay_ms(3000);
    }

	readSetupData();


	//get initial values
	lcdTimeout = setupData.lcdTimeout;
	menuTimeout = setupData.menuTimeout;

	//first screen
	tftBackg();
    tftClearWin(P(AUTO));
    drawLightChart();

    //zjistime wifi a nacteme cas, konfig atd ...
    wifiState = ping;

    /*
     * Hlavni smycka
     */

	while (1) {
		//Encoder
		ClickEncoder::Button b = encoder->getButton();
		//enc_button_t b = enc_button();
		value += encoder->getValue();
		//value += enc_value();

		currentTime = millis();

		/*
		 *  Obsluha UART komunikace s wifi
		 */
		//posleme data
		if ( (wifiClear) && (wifiState != none) ) {
			sendWiFiCommand();
			uartTimeout = millis();
		}

		//zpracujeme prichozi data
		if (uart_available()) { //paket ceka na zpracovani
			memcpy((void*)uart_data,(void*)uart_getPckt(),8); //precteme 8 byte z bufferu
			uint16_t crc = checkCrc(uart_data); //kontrola CRC

			if (crc == 0 ) { //crc je ok,
				handleWifiData();  //zpracujeme dat
				uart_putB(0x14);uart_putB(0x10); // a odpovime OK
			} else {
				#ifdef DEBUG
					printUartPacket();
				#endif
				uart_putB(0x10);uart_putB(0x10); // vratime chybu
				dprint("crc error\n"); //tft_println(P(CRC_ERROR));
				clearData();
			}
		} //konec obsluhy UART

		//UART TIMEOUT
		if ( (!wifiClear) && ((millis() - uartTimeout) > UARTTIMEOUT)) {
			wifiClear = true;
			wifiState = none;
			char tmp[30];
			sprintf(tmp,"tout: %lu\n",(millis() - uartTimeout));
			dprint(tmp);
			clearData();
			uartTimeout = millis();
		}

		//process ledvalues
		if ((millis() - timeStamp03) >= LEDINTERVAL) {
			if (wifiClear && !override) wifiState = ledValues;

			timeStamp03 = millis();

			for (uint8_t m = 0; m < modulesCount; m++) {
				sendValue(slaveAddr[m],m);
			}
		}

		//display time
		if ((millis() - timeStamp01) >= SECINTERVAL) {

 	 	    t = RTC.now();
 	 	    if (lcdTimeout > 0 ) {
				printDateTime();
				lcdTimeout--;
			}

			if (menuTimeout > 0 ) { menuTimeout--; }
			timeStamp01 = millis(); // take a new timestamp
		}

		//display values
		if (((millis() - timeStamp02) >= LCDREFRESH) || (redraw) ){


		if (selMenu == 0 ) drawLightChart();

			drawStatusBar();
			redraw = false;
			timeStamp02 = millis();
		}

		//pri stisku ovl. prvku rozsvitime display a resetujeme timeout
		if (b != ClickEncoder::Open || value != last) {
			if (lcdTimeout == -1) tft_on();
			lcdTimeout = setupData.lcdTimeout;
			menuTimeout = setupData.menuTimeout;
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
					wifiState = netInfo;
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
		} else  if (selMenu != MENU_OFF) {
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
					manualOFF = !manualOFF;
					selMenu = MENU_OFF;
					drawMenu(selMenu,MENUDEFAULT);
					if (manualOFF) {
						if (modulesCount > 0) {
							for (uint8_t x=0; x<CHANNELS;x++) {
								channelVal[selMod-1][x] = 0;
							}
						}
					}

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
		 } else if ((lcdTimeout > 0) && (selMenu == MENU_OFF) && (value != last)) {
				 if (modulesCount > 0) {
					 if (value>last) {
						 selMod = selMod >= modulesCount? 1: selMod+1;
					 } else {
						 selMod = selMod <= 1? modulesCount: selMod-1;
					 }
					 redraw = true;
				 } else {
					 selMod = 0;
				 }
		 }

		//encoder
		last = value;

	} //while
}

