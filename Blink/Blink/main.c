/*
 * Blink.c
 *
 * Created: 5/17/2016 1:43:33 PM
 * Author : Adam
      
     ATmega168
     
     Example Blink
     Toggle PIN PC0 at 1Hz
 */ 

#include <avr/io.h>
#define F_CPU 8E6 //This corresponds to the 8Mhz clock on the microcontroller and is needed for the delay function timing, changed to 16MHz for external crystal (remember to change fuses)
#include <util/delay.h> //This allows use of the ms and us delays

#define LED_ON	PORTC |= 0b11111111 //Macros are useful for repeated statements that are called often similar to functions
#define LED_OFF	PORTC &= 0b11111110


//Define functions
//======================
void ioinit(void);      //Initializes IO
//======================

int main (void)
{
	ioinit(); //Setup IO pins and defaults

	while(1)
	{
		LED_ON;
		_delay_ms(100);

		LED_OFF;
		_delay_ms(100);
	}
	
	return(0);
}

void ioinit (void)
{
	//1 = output, 0 = input
	DDRB = 0b00000000; //All inputs (for power consumption purposes)
	DDRC = 0b00000001; //All inputs except PC0 which is the LED output
	DDRD = 0b00000000;
	
	PORTB = 0b11111111; //Set to high (for power consumption purposes)
	PORTC = 0b11111110; 
	PORTD = 0b11111111; 
}

