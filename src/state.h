/*
 * state.h
 *
 *  Created on: 19. 4. 2017
 *      Author: slouf
 */

#ifndef STATE_H_
#define STATE_H_

#define PING         '0' //find wifi
#define GETCHANGE    '1' //is any change?
#define GETTIME      '2' //get unixtime
#define GETCONFIG    '3' //"3%c%c%c%c%c\xff\xff"      //send modules count and get config data from wifi
#define GETLEDVALUES '4' //"4\xff\xff\xff\xff\xff\xff\xff" //getLedValues
#define GETNETVALUES '5' //"5\xff\xff\xff\xff\xff\xff\xff" //getNetValues (ip, name, wifi, pwd ...)
#define LEDMANUAL    '6' //"6%c\xff\xff\xff\xff\xff\xff"  //set manual, nasleduje 224 byte s hodnotami led pro kazdy kanal
#define LEDOFF       '7' //"7\xff\xff\xff\xff\xff\xff\xff" //set manual,
#define GETLOGO      'Z' //"Z\xff\xff\xff\xff\xff\xff\xff"
#define TEMPERATUREINFO  '8' //"8%c%c%c%c\xff\xff\xff"
#define VERSIONINFO  '9' //"9%c%c%c%c\xff\xff\xff"

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
	change,
	change_s,
	reset,
	temperature,
	temperature_s,
	version,
	version_s
} wifiState_t;

#endif /* STATE_H_ */
