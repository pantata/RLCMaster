/*
 * DS RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

#ifndef DS1307_H
#define DS1307_H

#include <stdbool.h>
#include <avr/io.h>
#include "twi.h"

#define DS1307_SLAVE_ADDR 0b11010000

/** Time structure
 * 
 * Both 24-hour and 12-hour time is stored, and is always updated when rtc_get_time is called.
 * 
 * When setting time and alarm, 24-hour mode is always used.
 *
 * If you run your clock in 12-hour mode:
 * - set time hour to store in twelveHour and set am to true or false.
 * - call rtc_12h_translate (this will put the correct value in hour, so you don't have to
 *   calculate it yourself.
 * - call rtc_set_alarm or rtc_set_clock
 *
 * Note that rtc_set_clock_s, rtc_set_alarm_s, rtc_get_time_s, rtc_set_alarm_s always operate in 24-hour mode
 * and translation has to be done manually (you can call rtc_24h_to_12h to perform the calculation)
 *
 */
struct tm {
	uint8_t sec;      // 0 to 59
	uint8_t min;      // 0 to 59
	uint8_t hour;     // 0 to 23
	uint8_t mday;     // 1 to 31
	uint8_t mon;      // 1 to 12
	uint8_t year;     // year-99
	uint8_t wday;     // 1-7

    // 12-hour clock data
    bool am; // true for AM, false for PM
    int twelveHour; // 12 hour clock time
};

// statically allocated 
extern struct tm _tm;

// Get/set time
struct tm* rtc_get_time(void);
void rtc_set_time(struct tm* tm_);

// start/stop clock running (DS1307 only)
void rtc_run_clock(bool run);
bool rtc_is_clock_running(void);

// SRAM read/write DS1307 only
void rtc_get_sram(uint8_t* data);
void rtc_set_sram(uint8_t *data);
uint8_t rtc_get_sram_byte(uint8_t offset);
void rtc_set_sram_byte(uint8_t b, uint8_t offset);




#endif
