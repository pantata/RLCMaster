/*
 * main.h
 *
 *  Created on: 27. 3. 2015
 *      Author: ludek
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <avr/pgmspace.h>
#include "rtc.h"

#define VERSION  2

#define CHANNELS 7
#define MAXPWMCOUNT 100

#define LCDREFRESH  3000
#define SECINTERVAL 1000
#define LEDINTERVAL 100
#define LEDSTEP 2

//display text
#define NONE	 ( char *) "                "
#define AUTO     ( char *) "    LED AUTO    "
#define MANUAL   ( char *) "   LED  MANUAL  "
#define OFF      ( char *) "   LED VYPNUTO  "
#define CANCEL   ( char *) "Zpet  "
#define SAVE     ( char *) "Ulozit"
#define SETTINGS ( char *) "  Nastaveni LED "
#define SETTIME  ( char *) " Nastaveni hodin"
#define RTCERR   ( char *) "  Chyba hodin   "

#define BTPOSX    138
#define BTPOSY    0

//display & menu timeout
#define TIMEOUTMENU         200
#define TIMEOUTPODSVICENI  400

#define MENU_OFF     0
#define MENU_MANUAL  1
#define MENU_LEDOFF  2
#define MENU_TIME    3
#define MENU_PREF    4
// menu
#define MENUDEFAULT 4
#define MENUEDIT 2
#define MENUHOME 3

#define UART_CRCERROR 	0x0200
#define UART_OK 		0xFFFF
#define UART_BADCMD     0x0100

const char *menuIcon[] = {"0","1","2","3","6","4","7","5"};

struct _pwmValue {
	uint8_t  timeSlot;
	uint8_t  pwmChannel;
	uint16_t pwmValue;
};

//_pwmValue pwmValues[MAXPWMCOUNT];

uint8_t slaveAddr[16];
uint8_t modulesCount = 0;

struct SetupData {
    uint8_t  version;
    int16_t menuTimeout;
    int16_t lcdTimeout;
    _pwmValue pwmValues[MAXPWMCOUNT];
    int16_t overrideVal[CHANNELS];
} setupData;

unsigned long lastSetupDataSum;

// MENU
uint8_t menuItem = 0;
uint8_t menuPos = 0;
int16_t menuTimeout = TIMEOUTMENU;
uint8_t selMenu = 0;
bool isMenu = false;
bool resetMenu = false;

//LED
bool moon = false;

int16_t channelVal[CHANNELS];
int16_t lastChannelVal[CHANNELS];

bool override = false;
bool manualOFF = false;

//LCD
int16_t lcdTimeout=TIMEOUTPODSVICENI;

//RIZENI
uint8_t _dateTimeStamp = 0;
long timeStamp01 = 0;
long timeStamp02 = 0;
long timeStamp03 = 0;

//DATETIME
struct tm* t = NULL;

//UART
bool isUartData = false;
uint16_t uDataLength = 0;
uint8_t tmpBuffer[8];
uint8_t uart_cmd = 0;
uint16_t uart_DataLength = 0;
uint8_t _data[400];
uint16_t _bytes = 0;
uint8_t _address = 0;

//FUNKCE
void sortPwmValues(uint8_t k);
void initInterrupts(void);
void initTimer(void);
void initPwmValue();

#endif /* MAIN_H_ */
