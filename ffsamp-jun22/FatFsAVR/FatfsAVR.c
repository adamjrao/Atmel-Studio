/*
 * FatfsAVR.c
 *
 * Created: 11/07/2013 16:16:42
 *  Author: cliff
 */ 

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

int main(void)
{
	FRESULT res;
	OCR0 = 0xB3; // avrcalc says that at 3.6864MHz that /8 and CTC 0xB3 will give 10ms
	TIMSK = (1 << OCIE0); // use COMP interrupt
	TCCR0 = (1 << WGM01) | (1 << CS01); // CTC with div 8
	sei();
	if (disk_initialize(0) == STA_NOINIT) {
		while(1);
	}
	res = f_mount(0, &fs);
	if (res == FR_OK) {
		res = f_open(&fin, "poem.txt", FA_OPEN_EXISTING | FA_READ);
		if (res == FR_OK) {
			do {
				f_gets(line, sizeof(line), &fin);
				uart_puts(line);
				uart_putc('\r');
			} while (!f_eof(&fin));
			f_close(&fin);
		}
	}
	while(1)
	{
	}
}

ISR(TIMER0_COMP_vect) {
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