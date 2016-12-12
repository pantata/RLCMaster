/*
 * uart.h
 *
 * UART  for ATMega328P clocked at 16 MHz
 *
 */

#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>

/* Probably already defined somewhere else. Define here, if isn't. */
#ifndef FOSC
#define	FOSC			16000000UL
#endif

/* Settings */
#define _BAUD			115200					// Baud rate (9600 is default)
#define _DATA			0x03					// Number of data bits in frame = byte tranmission
#define _UBRR			(FOSC/16)/_BAUD - 1		// Used for UBRRL and UBRRH

#define RX_BUFF			8
#define UART_RX0_BUFFER_MASK ( RX_BUFF - 1)

/* Useful macros */
#define TX_START()		UCSR0B |= _BV(TXEN0)	// Enable TX
#define TX_STOP()		UCSR0B &= ~_BV(TXEN0)	// Disable TX
#define RX_START()		UCSR0B |= _BV(RXEN0)	// Enable RX
#define RX_STOP()		UCSR0B &= ~_BV(RXEN0)	// Disable RX
#define COMM_START()		TX_START(); RX_START()	// Enable communications

/* Interrupt macros; Remember to set the GIE bit in SREG before using (see datasheet) */
#define RX_INTEN()		UCSR0B |= _BV(RXCIE0)	// Enable interrupt on RX complete
#define RX_INTDIS()		UCSR0B &= ~_BV(RXCIE0)	// Disable RX interrupt
#define TX_INTEN()		UCSR0B |= _BV(TXCIE0)	// Enable interrupt on TX complete
#define TX_INTDIS()		UCSR0B &= ~_BV(TXCIE0)	// Disable TX interrupt

static volatile uint16_t UART_RxHead;
static volatile uint16_t UART_RxTail;
static volatile uint16_t UART_TxHead;
static volatile uint16_t UART_TxTail;
static volatile uint8_t UART_LastRxError;
static volatile uint8_t UART_RxBuf[RX_BUFF];
static volatile bool pckCmplt;
/*
** high byte error return code of uart_getc()
*/
#define UART_FRAME_ERROR      0x0800              /**< Framing Error by UART       */
#define UART_OVERRUN_ERROR    0x0400              /**< Overrun condition by UART   */
#define UART_BUFFER_OVERFLOW  0x0200              /**< receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /**< no receive data available   */

#ifdef __cplusplus
extern "C" {
#endif

/* Prototypes */
void uart_init(void);
void uart_putB(unsigned char data);
void uart_write(char *str);
uint8_t *uart_getPckt(void);
bool uart_available(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_H_ */
