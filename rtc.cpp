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

/*
 * DS1307 register map
 *  
 *  00h-06h: seconds, minutes, hours, day-of-week, date, month, year (all in BCD)
 *     bit 7 of seconds enables/disables clock
 *     bit 6 of hours toggles 12/24h mode (1 for 12h, 0 for 24h)
 *       when 12h mode is selected bit 5 is high for PM, low for AM
 *  07h: control
 *      bit7: OUT
 *      bit6: 0
 *      bit5: 0
 *      bit4: SQWE
 *      bit3: 0
 *      bit2: 0
 *      bit1: RS0
 *      bit0: RS1
 *  08h-3fh: 56 bytes of SRAM
 *
 * DS3231 register map
 *
 *  00h-06h: seconds, minutes, hours, day-of-week, date, month, year (all in BCD)
 *       bit 7 should be set to zero: The DS3231 clock is always running
 *  07h: A1M1  Alarm 1 seconds
 *  08h: A1M2  Alarm 1 minutes
 *  09h: A1M3  Alarm 1 hour (bit6 is am/pm flag in 12h mode)
 *  0ah: A1M4  Alarm 1 day/date (bit6: 1 for day, 0 for date)
 *  0bh: A2M2  Alarm 2 minutes
 *  0ch: A2M3  Alarm 2 hour (bit6 is am/pm flag in 12h mode)
 *  0dh: A2M4  Alarm 2 day/data (bit6: 1 for day, 0 for date)
 *       <see data sheet page12 for Alarm register mask bit tables:
 *        for alarm when hours, minutes and seconds match set 1000 for alarm 1>
 *  0eh: control
 *      bit7: !EOSC
 *      bit6: BBSQW
 *      bit5: CONV
 *      bit4: RS2
 *      bit3: RS1
 *      bit2: INTCN
 *      bit1: A2IE
 *      bit0: A1IE
 *  0fh: control/status
 *      bit7: OSF
 *      bit6: 0
 *      bit5: 0
 *      bit4: 0
 *      bit3: EN32kHz
 *      bit2: BSY
 *      bit1: A2F alarm 2 flag
 *      bit0: A1F alarm 1 flag
 * 10h: aging offset (signed)
 * 11h: MSB of temp (signed)
 * 12h: LSB of temp in bits 7 and 6 (0.25 degrees for each 00, 01, 10, 11)
 *
 */

#include <avr/io.h>

#define TRUE 1
#define FALSE 0

#include "rtc.h"

#define RTC_ADDR 0x68 // I2C address
#define CH_BIT 7 // clock halt bit

// statically allocated structure for time value
struct tm _tm;

uint8_t dec2bcd(uint8_t d)
{
  return ((d/10 * 16) + (d % 10));
}

uint8_t bcd2dec(uint8_t b)
{
  return ((b/16 * 10) + (b % 16));
}

uint8_t rtc_read_byte(uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(offset);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 1);
	return twi_receive();
}

void rtc_write_byte(uint8_t b, uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(offset);
	twi_send_byte(b);
	twi_end_transmission();
}

struct tm* rtc_get_time(void)
{
	uint8_t rtc[9];
	//uint8_t century = 0;

	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 7);

	for (uint8_t i = 0; i < 7; i++) {
		rtc[i] = twi_receive();
	}

	twi_end_transmission();

	// Clear clock halt bit from read data
	// This starts the clock for a DS1307, and has no effect for a DS3231
	rtc[0] &= ~(_BV(CH_BIT)); // clear bit

	_tm.sec = bcd2dec(rtc[0]);
	_tm.min = bcd2dec(rtc[1]);
	_tm.hour = bcd2dec(rtc[2]);
	_tm.mday = bcd2dec(rtc[4]);
	_tm.mon = bcd2dec(rtc[5] & 0x1F); // returns 1-12
	//century = (rtc[5] & 0x80) >> 7;
	//_tm.year = century == 1 ? 2000 + bcd2dec(rtc[6]) : 1900 + bcd2dec(rtc[6]); // year 0-99
	_tm.year = bcd2dec(rtc[6]);
	_tm.wday = bcd2dec(rtc[3]); // returns 1-7

	if (_tm.hour == 0) {
		_tm.twelveHour = 0;
		_tm.am = 1;
	} else if (_tm.hour < 12) {
		_tm.twelveHour = _tm.hour;
		_tm.am = 1;
	} else {
		_tm.twelveHour = _tm.hour - 12;
		_tm.am = 0;
	}

	return &_tm;
}

void rtc_set_time(struct tm* tm_)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);

	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	twi_send_byte(dec2bcd(tm_->sec)); // seconds
	twi_send_byte(dec2bcd(tm_->min)); // minutes
	twi_send_byte(dec2bcd(tm_->hour)); // hours
	twi_send_byte(dec2bcd(tm_->wday)); // day of week
	twi_send_byte(dec2bcd(tm_->mday)); // day
	twi_send_byte(dec2bcd(tm_->mon) + 0x80); // month
	twi_send_byte(dec2bcd(tm_->year)); // year

	twi_end_transmission();
}

// DS1307 only (has no effect when run on DS3231)
// halt/start the clock
// 7th bit of register 0 (second register)
// 0 = clock is running
// 1 = clock is not running
void rtc_run_clock(bool run)
{
  
  uint8_t b = rtc_read_byte(0x0);

  if (run)
    b &= ~(_BV(CH_BIT)); // clear bit
  else
    b |= _BV(CH_BIT); // set bit
    
    rtc_write_byte(b, 0x0);
}

// DS1307 only
// Returns true if the clock is running, false otherwise
// For DS3231, it always returns true
bool rtc_is_clock_running(void)
{
  uint8_t b = rtc_read_byte(0x0);

  if (b & _BV(CH_BIT)) return false;
  return true;
}


#define DS1307_SRAM_ADDR 0x08

// SRAM: 56 bytes from address 0x08 to 0x3f (DS1307-only)
void rtc_get_sram(uint8_t* data)
{
	// cannot receive 56 bytes in one go, because of the TWI library buffer limit
	// so just receive one at a time for simplicity
  	for(int i=0;i<56;i++)
		data[i] = rtc_get_sram_byte(i);
}

void rtc_set_sram(uint8_t *data)
{
	// cannot send 56 bytes in one go, because of the TWI library buffer limit
	// so just send one at a time for simplicity
  	for(int i=0;i<56;i++)
		rtc_set_sram_byte(data[i], i);
}

uint8_t rtc_get_sram_byte(uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(DS1307_SRAM_ADDR + offset);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 1);
	return twi_receive();
}

void rtc_set_sram_byte(uint8_t b, uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(DS1307_SRAM_ADDR + offset);
	twi_send_byte(b);
	twi_end_transmission();
}






