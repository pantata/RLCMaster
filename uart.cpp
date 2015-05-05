/*
 * uart.c
 *
 * Asynchronous UART for ATMega328P (16 MHz)
 * TODO: buffer overflow
 */

#include "uart.h"
#include <string.h>
#include <stddef.h>
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

/*! \brief Configures baud rate (refer datasheet) */
void uart_init(void)
{
	UART_TxHead = 0;
	UART_TxTail = 0;
	UART_RxHead = 0;
	UART_RxTail = 0;
	pckCmplt = false;

	DDRD |= _BV(PD1);
	DDRD &= ~_BV(PD0);

	// Set baud rate; lower byte and top nibble
	UBRR0H = ((_UBRR) & 0xF00);
	UBRR0L = (uint8_t) ((_UBRR) & 0xFF);

	TX_START();
	RX_START();

	// Set frame format = 8-N-1
	UCSR0C = (_DATA << UCSZ00);

	//enable RX interrupt
	RX_INTEN();
}

/*! \brief Transmits a byte
 * 	Use this function if the TX interrupt is not enabled.
 * 	Blocks the serial port while TX completes
 */
void uart_putB(unsigned char data)
{
	// Stay here until data buffer is empty
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = (unsigned char) data;

}

/*! \brief Writes an ASCII string to the TX buffer */
void uart_write(char *str)
{
	while (*str != '\0')
	{
		uart_putB(*str);
		++str;
	}
}

/*! \brief return true if packet is in buffer */
bool uart_available(void)
{
	return pckCmplt;
}

/*! \brief return 8byte packet  */
uint8_t *uart_getPckt(void) {

	static uint8_t data[8];

	if (pckCmplt) {
		 ATOMIC_BLOCK(ATOMIC_FORCEON) {
			 UART_RxHead = 0;
			 memcpy((void*)data,(void*)UART_RxBuf,8);
			 memset((void*)UART_RxBuf,0,8);
			 pckCmplt = false;
		 }
		return data;
	} else {
		return NULL;
	}
}

ISR(USART_RX_vect)
{
	    uint16_t tmphead;
	    uint8_t data;
	    uint8_t usr;
	    uint8_t lastRxError;

	    /* read UART status register and UART data register */
		usr  = UCSR0A;
		data = UDR0;
		lastRxError = (usr & (_BV(FE0)|_BV(DOR0)) );

		/* calculate buffer index */
		tmphead = ( UART_RxHead + 1);

	    if ( tmphead > RX_BUFF ) {
	        /* error: receive buffer overflow */
	        lastRxError = UART_BUFFER_OVERFLOW >> 8;
	    } else {
	        /* store new index */
	        UART_RxHead = tmphead;
	        /* store received data in buffer */
	        UART_RxBuf[tmphead-1] = data;
	    }

	    if (UART_RxHead == RX_BUFF) pckCmplt = true;

	    UART_LastRxError = lastRxError;
}

