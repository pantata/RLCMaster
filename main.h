/*
 * main.h
 *
 *  Created on: 27. 3. 2015
 *      Author: ludek
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <avr/pgmspace.h>
#include "rtc8563.h"

#define VERSION  3

#define CHANNELS 7
#define PWMSTEPS 1000
#define MAXPWMCOUNT 100

#define LCDREFRESH  3000
#define SECINTERVAL 1000
#define LEDINTERVAL 1000
#define LEDSTEP 1

#define BTPOSX    138
#define BTPOSY    0

//display & menu timeout
#define TIMEOUTMENU         120
#define TIMEOUTPODSVICENI  120

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

RTC_8563 RTC;



struct _pwmValue {
	uint8_t  timeSlot;
	uint8_t  pwmChannel;
	int16_t pwmValue;
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
    int8_t dt_fmt; //0 = DD.MM.YY,   1 = dd-mm-yy ,   2 = mm/dd/yy,  3 = yy-mm-dd
    bool tm_fmt;  // 0 = 24H, 1 = 12H
} setupData;

unsigned long lastSetupDataSum;

// MENU
uint8_t menuItem = 0;
uint8_t menuPos = 0;

uint8_t selMenu = 0;
bool isMenu = false;
bool resetMenu = false;

//LED
bool moon = false;

int16_t channelVal[CHANNELS];
int16_t lastChannelVal[CHANNELS];

bool override = false;
bool manualOFF = false;

//DATETIME
DateTime t;

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
static void sortPwmValues(uint8_t k);
static void initPwmValue();

#endif /* MAIN_H_ */
