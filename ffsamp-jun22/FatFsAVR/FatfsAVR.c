/*
 * FatfsAVR.c
 *
 * Created: 11/07/2013 16:16:42
 *  Author: cliff
 */ 
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../avr/diskio.h"
#include "../avr/ff.h"
#include "../avr/uart.h"

void uart_puts(char * str);

FATFS fs;
FIL fin;

char line[80];
char sec[512];
int USART0SendByte(char u8Data, FILE *stream)
{
	if(u8Data == '\n')
	{
		USART0SendByte('\r', stream);
	}
	//wait while previous byte is completed
	while(!(UCSR0A&(1<<UDRE0))){};
	// Transmit data
	UDR0 = u8Data;
	return 0;
}

FILE usart0_str = FDEV_SETUP_STREAM(USART0SendByte, NULL, _FDEV_SETUP_WRITE);

void timer0_init()
{
	
	// set up timer with prescaler = 1024
	TCCR0B |= (1 << CS02)|(1 << CS00);
	// set up timer with prescaler = 256
	//TCCR0B |= (1 << CS02);
	// set up timer with no prescaling
	//TCCR0B |= (1 << CS00);
	
	// initialize counter
	TCNT0 = 0;
}

int main(void)
{
	FRESULT res;
	OCR0A = 0x9B; // avrcalc says that at 16MHz that /1024 and CTC 0x9B aka 156 ticks will give 10ms
	TIMSK0 = (1 << OCIE0A); // enable Timer0 Compare Match A interrupt, executed if a compare match occurs
	TCCR0A = (1 << WGM01) | (1 << COM0A1); // CTC mode WGM01 set to 1, clear OC0A pin (not OCR0A! which is the number to compare to!) on compare match
	sei();
	// initialize timer
	timer0_init();
	//uart init
	USART0Init();

	USART0SendByte('A',&usart0_str);
	if (disk_initialize(0) == STA_NOINIT) {
		USART0SendByte('F',&usart0_str);
		while(1);
	}else{
		USART0SendByte('B',&usart0_str);
	}
	res = f_mount(0, &fs);
	if (res == FR_OK) {
		res = f_open(&fin, "poem.txt", FA_OPEN_EXISTING | FA_READ);
		if (res == FR_OK) {
			do {
				f_gets(line, sizeof(line), &fin);
				USART0SendByte('C',&usart0_str);
				uart_puts(line);
				uart_putc('\r');
			} while (!f_eof(&fin));
			f_close(&fin);
		} else{
			USART0SendByte('G',&usart0_str);
		}
	}
	while(1)
	{
	}
}

ISR(TIMER0_COMPA_vect) {
	disk_timerproc();
}

void uart_puts(char * str) {
	while (*str) {
		uart_putc(*str++);
	}
}

DWORD get_fattime (void)
{
	/* Pack date and time into a DWORD variable */
	return     ((DWORD)(2013 - 1980) << 25)
	| ((DWORD)3 << 21)
	| ((DWORD)23 << 16)
	| ((DWORD)12 << 11)
	| ((DWORD)0 << 5)
	| ((DWORD)0 >> 1);
}