/*
 * FatfsAVR.c
 *
 * Created: 11/07/2013 16:16:42
 *  Author: cliff
 */ 
#include <stdio.h>				//Printf and other file stream operators
#include <avr/io.h>				//IO specific info for particular AVR uC
#include <avr/interrupt.h>		//Include interrupt library
#include "../avr/diskio.h"		//Fatfs library
#include "../avr/ff.h"			//Fatfs library
#include "../avr/uart.h"		//Uart initialization etc.
#include <util/delay.h>			//Allow us and ms delays
//#define F_CPU 16000000		//Note F_CPU is defined in symbols section of Atmel GUI
#define ADCBUFFERSIZE 512		//Good size for SD storage
uint16_t adcbuffer[ADCBUFFERSIZE]; // Create buffer to store data during interrupt
uint16_t adcbufferindex; // Create index for the buffer
uint32_t timetotal; // Create a variable to hold the value of the TIMER0 register
uint8_t timediff; // variable to hold the difference in time since last ADC call
uint8_t lasttime, currenttime; // globals for last and current time

void uart_puts(char * str);

FATFS fs; // Declare a file system instance
FIL fin;  // Decale a file instance

//char line[80];
//char sec[512];
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
void InitADC()
{
	// Select Vref=AVcc
	ADMUX |= (1<<REFS0);
	//set prescaller to 128 and enable ADC
	ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);
	//Enable interrupts
	ADCSRA |= (1<<ADIE);
	//Start conversion
	ADCSRA |= (1<<ADSC);
}
ISR(ADC_vect)
{
	// Assign contents of ADC buffer to databuffer,
	// we know the data is ready because of the interrupt
	// (storing 10 bits in 16 bit space for now)
	//Determine the time interval since last ADC call
	
	currenttime = TCNT0;
	if (currenttime > lasttime)
	{
		timediff = currenttime - lasttime;
	}
	else
	{
		timediff = 256 - (lasttime - currenttime);
	}

	timetotal = timetotal + timediff; //Add the elapsed time to total time
	lasttime = currenttime;
	
	
	adcbuffer[adcbufferindex] = ADC;
	adcbufferindex++;
	
	if (adcbufferindex==ADCBUFFERSIZE)
	{
		adcbufferindex=0;
	}
	
}
void timer0_init()
{
	
	// set up timer with prescaler = 64
	TCCR0B |= (1 << CS01)|(1 << CS00);
	// set up timer with prescaler = 256
	//TCCR0B |= (1 << CS02);
	// set up timer with no prescaling
	//TCCR0B |= (1 << CS00);
	
	// initialize counter
	TCNT0 = 0;
}

void timer2_init()
{
	
	// set up timer with prescaler = 1024
	TCCR2B |= (1 << CS22)|(1 << CS21)|(1 << CS20);
	
	// initialize counter
	TCNT2 = 0;
}
ISR(TIMER2_COMPA_vect) {
	disk_timerproc(); //FatFs uses it internally to countdown some soft timers to implement time outs (needed during file functions), also CardPresent and WriteProtect
}


int main(void)
{
	
	uint16_t sensorval = 0;
	//Set buffer index to zero before enabling interrupts
	adcbufferindex = 0;
	timetotal = 0;
	timediff = 0;
	lasttime = 0;
	currenttime = 0;
	
	FRESULT res;
	OCR2A = 0x9B; // avrcalc says that at 16MHz that /1024 and CTC 0x9B aka 156 ticks will give 10ms
	TIMSK2 = (1 << OCIE2A); // enable Timer0 Compare Match A interrupt, executed if a compare match occurs
	TCCR2A = (1 << WGM21) | (1 << COM2A1); // CTC mode WGM01 set to 1, clear OC0A pin (not OCR0A! which is the number to compare to!) on compare match
	
	//initialize ADC
	InitADC();
	// initialize timer
	timer0_init();
	// initialize timer
	timer2_init();
	//uart init
	USART0Init();
	
	sei(); //Enable global interrupt flag (tell CPU)
	
	uint16_t localbufferindex=0;
/* Working read file code
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
*/
	res = f_mount(0, &fs);
	if (res == FR_OK) {
		res = f_open(&fin, "record.txt", FA_CREATE_ALWAYS);
		if (res == FR_OK) {
			USART0SendByte('C',&usart0_str);
		}
	}

	while(1)
	{
		if(adcbufferindex==0)
		{
		}
		while(localbufferindex != adcbufferindex)
		{
			sensorval = adcbuffer[localbufferindex];
			//sending sensor value to terminal
			f_printf(&fin,"%u\n", (uint16_t)sensorval);
			//Increment local buffer index
			localbufferindex++;
			if (localbufferindex==ADCBUFFERSIZE)
			{
				localbufferindex=0;
			}
		}
		f_close(&fin);
	}
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