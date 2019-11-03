#ifndef LANG_H_
#define LANG_H_

#include <avr/pgmspace.h>

char p_buffer[20];
#define P(str) (strcpy_P(p_buffer, str), p_buffer)

//display text
const char NONE[]	 PROGMEM 	=  {"                "};
const char AUTO[]	 PROGMEM	=  {"    LED AUTO    "};
const char MANUAL[] PROGMEM	=      {"   LED  MANUAL  "};
const char OFF[]	PROGMEM	=  	   {"   LED VYPNUTO  "};
const char SETTINGS[]	PROGMEM	=  {"  Nastaveni LED "};
const char SETTIME[]	PROGMEM	=  {" Nastaveni hodin"};
const char DDMMYY0[]	PROGMEM	=  {"DD.MM.YY"};
const char DDMMYY1[]	PROGMEM	=  {"DD/MM/YY"};
const char DDMMYY2[]	PROGMEM	=  {"DD-MM-YY"};
const char DDMMYY3[]	PROGMEM	=  {"YY/MM/DD"};
const char DDMMYY4[]	PROGMEM	=  {"YY-MM-DD"};

const char HH0[]	PROGMEM	=  {"HH:MI:SS P"};
const char HH1[]	PROGMEM	=  {"HH.MI:SS P"};
const char HH2[]	PROGMEM	=  {"HH24:MI:SS"};
const char HH3[]	PROGMEM	=  {"HH24.MI:SS"};
const char HH4[]	PROGMEM	=  {"HH:MI P   "};
const char HH5[]	PROGMEM	=  {"HH.MI P   "};
const char HH6[]	PROGMEM	=  {"HH24:MI   "};
const char HH7[]	PROGMEM	=  {"HH24.MI   "};

const char DAY[]	PROGMEM	=  {"Den:"};
const char MONTH[]	PROGMEM	=  {"Mesic:"};
const char YEAR[]	PROGMEM	=  {"Rok:"};
const char HOUR[]	PROGMEM	=  {"Hod.:"};
const char MINUTE[]	PROGMEM	=  {"Min.:"};
const char SECOND[]	PROGMEM	=  {"Sec.:"};
const char DTFORMAT[]	PROGMEM	=  {"Format:"};
const char AM[]	PROGMEM	=  {"Dop."};
const char PM[]	PROGMEM	=  {"Odp."};

const char BATTERY[] PROGMEM =  {"8"};
const char *wifi[]  = {"5","6","7"};
const char *menuIcon[] = {"0","1","2","3","6","4","7","5"};
const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

const char CRC_ERROR[] PROGMEM = {"CRC Error"};

const char NET1[]	PROGMEM	=  {"IP: %03d.%03d.%03d.%03d"};
const char NET2[]	PROGMEM	=  {"WIFI :%s"};
const char NET3[]	PROGMEM	=  {"HESLO:%s"};
const char NET4[]	PROGMEM	=  {"URL: http://%s"};
#endif /* LANG_H_ */
