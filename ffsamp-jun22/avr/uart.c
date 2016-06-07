/*------------------------------------------------*/
/* UART functions                                 */
/*------------------------------------------------*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

#define	UART_BPS		57600
#define	UART_BUFF		64


typedef struct {
	uint16_t	wi, ri, ct;
	uint8_t buff[UART_BUFF];
}FIFO;
static
volatile FIFO TxFifo, RxFifo;



/* Initialize UART */
void USART0Init(void)
{
	// Set baud rate
	UBRR0H = (uint8_t)(UBRR_VALUE>>8);
	UBRR0L = (uint8_t)UBRR_VALUE;
	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);

	// Reset FIFO
	RxFifo.ct = 0; RxFifo.ri = 0; RxFifo.wi = 0;
	TxFifo.ct = 0; TxFifo.ri = 0; TxFifo.wi = 0;

	//enable transmission and reception
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
}


/* Get a received character */

uint16_t uart_test (void)
{
	return RxFifo.ct;
}


uint8_t uart_getc (void)
{
	uint8_t d, i;


	while (RxFifo.ct == 0) ;

	i = RxFifo.ri;
	d = RxFifo.buff[i];
	cli();
	RxFifo.ct--;
	sei();
	RxFifo.ri = (i + 1) % sizeof RxFifo.buff;

	return d;
}

/* Put a character to transmit */

void uart_putc (uint8_t d)
{
	uint8_t i;


	while (TxFifo.ct >= sizeof TxFifo.buff) ;

	i = TxFifo.wi;
	TxFifo.buff[i] = d;
	cli();
	TxFifo.ct++;
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<< UDRE0) | (1<<RXCIE0);
	sei();
	TxFifo.wi = (i + 1) % sizeof TxFifo.buff;
}


/* UART RXC interrupt */

ISR(USART_RX_vect)
{
	uint8_t d, n, i;


	d = UDR0;
	n = RxFifo.ct;
	if (n < sizeof RxFifo.buff) {
		RxFifo.ct = ++n;
		i = RxFifo.wi;
		RxFifo.buff[i] = d;
		RxFifo.wi = (i + 1) % sizeof RxFifo.buff;
	}
}


/* UART UDRE interrupt */

ISR(USART_UDRE_vect)
{
	uint8_t n, i;

	n = TxFifo.ct;
	if (n) {
		TxFifo.ct = --n;
		i = TxFifo.ri;
		UDR0 = TxFifo.buff[i];
		TxFifo.ri = (i + 1) % sizeof TxFifo.buff;
	}
	if (n == 0) UCSR0B = (1<<RXEN0) | (1<<RXCIE0) | (1<<TXEN0);
}

