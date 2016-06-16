#ifndef UART_DEFINED
#define UART_DEFINED

#include <stdint.h>

#define USART_BAUDRATE 57600 // Change to bluetooth baud rate
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)	//Careful to ensure integer based on clock choice

void USART0Init(void);          /* Initialize USART */
uint16_t uart_test (void);		/* Check number of data in UART Rx FIFO */
uint8_t uart_getc (void);		/* Get a byte from UART Rx FIFO */
void uart_putc (uint8_t d);		/* Put a byte into UART Tx FIFO */

#endif
