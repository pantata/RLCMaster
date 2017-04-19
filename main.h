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

#define VERSION  5

#define MODULES 16
#define CHANNELS 7
#define PWMSTEPS 4000
#define MAXPWMCOUNT 100

#define LCDREFRESH  3000
#define SECINTERVAL 1000
#define LEDINTERVAL 5000
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

#define PING         "0      " //find wifi
#define GETCHANGE    "1      " //is any change?
#define GETTIME      "2      " //get unixtime
#define GETCONFIG    "3%c%c%c%c%c " //send modules count and get config data from wifi
#define GETLEDVALUES "4      " //getLedValues
#define GETNETVALUES "5      " //getNetValues (ip, name, wifi, pwd ...)
#define LEDMANUAL    "6%c    %c" //set manual, nasleduje 224 byte s hodnotami led pro kazdy kanal
#define LEDOFF       "7      " //set manual,
#define GETLOGO      "Z      "

union Unixtime {
    uint32_t time;
    uint8_t btime[4];
};

typedef enum {
	none,
	ping,
	ping_s,
	config,
	config_s,
	time,
	time_s,
	setup,
	setup_s,
	ledTimeSlots,
	ledTimeSlots_s,
	ledValues,
	ledValues_s,
	netInfo,
	netInfo_s,
	ledOff,
	ledOff_s,
	ledManual,
	ledManual_s,
	ledManualOverride_s,
	change,
	change_s,
} wifiState_t;

#define UARTTIMEOUT    2000L
uint32_t uartTimeout = 0;

wifiState_t wifiState=none;

union Unixtime unixtime;

uint8_t slaveAddr[16];
uint8_t modulesCount = 0;

uint8_t ip[4] = {0,0,0,0};
char ssid[33];
char pwd[33];

struct SetupData {
    bool  firstRun;
    uint8_t  version;
    int8_t menuTimeout;
    int8_t lcdTimeout;
    uint8_t dt_fmt;
    uint8_t tm_fmt;
    bool useDST;
    int16_t overrideVal[MODULES][CHANNELS];
} setupData;


unsigned long lastSetupDataSum;

// MENU
uint8_t menuItem = 0;
uint8_t menuPos = 0;
int8_t selMod = 1;

uint8_t selMenu = 0;
bool isMenu = false;
bool resetMenu = false;
bool isWiFi = false;

//LED
int16_t channelVal[MODULES][CHANNELS]; //16x7x2

bool override = false;
bool manualOFF = false;

//DATETIME
DateTime t;
unsigned long currentTime;

//UART
bool wifiClear = true;
bool isUartData = false;
bool uDataIsComplete = false;
uint16_t uDataLength = 0;
uint8_t uart_data[8];
uint8_t uart_cmd = 0;
uint16_t uart_DataLength = 0;
uint8_t _data[300];
uint16_t _bytes = 0;
uint8_t _address = 0;
int8_t lcdTimeout;
int8_t menuTimeout;

//FUNKCE
void wifiFound();
void setTimeFromWiFi(uint8_t *data);
void setLedVal();

//LED MANUAL OVVERIDE
uint8_t override_y = 0;
uint8_t override_x = 0;
bool ledLowValuesOK = false;
bool ledHighValuesOK = false;
bool ledValuesOK = false;

//
void printTemperature();

static void tftClearWin(const char *title);

uint16_t checkCrc(uint8_t *data, bool r=true);
#endif /* MAIN_H_ */
