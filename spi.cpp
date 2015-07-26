/*
 * spi.c
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */

#include <avr/io.h>
#include "spi.h"

#define SS   PB2
#define SCK  PB5
#define MOSI PB3

void spi_init() {

    //SS na high
    PORTB |= (1 << SS);
    DDRB |= (1<<SS);

    //SPI clock
    SPCR = (SPCR & ~SPI_CLOCK_MASK) | (SPI_CLOCK_DIV4 & SPI_CLOCK_MASK);
    SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((SPI_CLOCK_DIV4 >> 2) & SPI_2XCLOCK_MASK);

    // Enable SPI, Set as Master
    SPCR |= (1 << MSTR);
    SPCR |= (1 << SPE);

    // Set SS, MOSI, SCK as Output
    DDRB |= (1<<SCK)|(1<<MOSI);
}


//Function to send and receive data for both master and slave
unsigned char spi_rw (uint8_t _data) {
    // Load data into the buffer
    SPDR = _data;

    //Wait until transmission complete
    while (!(SPSR & _BV(SPIF)))
        ;
    // Return received data
    return(SPDR);
}
