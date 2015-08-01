#ifndef LANG_H_
#define LANG_H_

#include <avr/pgmspace.h>

char p_buffer[20];
#define P(str) (strcpy_P(p_buffer, str), p_buffer)

//display text
const char NONE[]	 PROGMEM 	=  {"                "};
const char AUTO[]	 PROGMEM	=  {"    LED AUTO    "};
const char MANUAL[] PROGMEM	=  {"   LED  MANUAL  "};
const char OFF[]	PROGMEM	=  	   {"   LED VYPNUTO  "};
const char SETTINGS[]	PROGMEM	=  {"  Nastaveni LED "};
const char SETTIME[]	PROGMEM	=  {" Nastaveni hodin"};
const char DDMMYY1[]	PROGMEM	=  {"DD-MM-YY"};
const char YYMMDD1[]	PROGMEM	=  {"YY-MM-DD"};
const char MMDDYY1[]	PROGMEM	=  {"MM/DD/YY"};
const char DDMMYY2[]	PROGMEM	=  {"DD.MM.YY"};
const char H12[]	PROGMEM	=  {"12H"};
const char H24[]	PROGMEM	=  {"24H"};
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

const char *menuIcon[] = {"0","1","2","3","6","4","7","5"};
const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

#endif /* LANG_H_ */
