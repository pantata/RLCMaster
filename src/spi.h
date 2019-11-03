/*
 * spi.h
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */

#ifndef SPI_H_
#define SPI_H_

#include <inttypes.h>

#define MOSI   PORTB3  //na out
#define MISO   PORTB4  //na in
#define SCK    PORTB5  //na out

#define SPI_CLOCK_MASK 0x03    // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR
#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV2 0x04

void spi_init();
unsigned char spi_rw (uint8_t _data);

#endif /* SPI_H_ */
