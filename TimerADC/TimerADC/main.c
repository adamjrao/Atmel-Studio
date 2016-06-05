#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h> //Include interrupt library
#define F_CPU 16000000
#include <util/delay.h>
#define USART_BAUDRATE 57600 // Change to bluetooth baud rate
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define DATABUFFERSIZE 512	// 512 is a nice size because its sd ready
uint16_t DATABUFFER[DATABUFFERSIZE]; // Create buffer to store data during interrupt
uint16_t BUFFERINDEX; // Create index for the buffer
uint32_t TIME; // Create a variable to hold the value of the TIMER0 register
uint8_t TIMEDIFF; // variable to hold the difference in time since last ADC call
uint8_t lasttime, currenttime; // globals for last and current time
volatile uint8_t TOT_OVERFLOW; //Global variable to count the number of overflows

void USART0Init(void)
{
	// Set baud rate
	UBRR0H = (uint8_t)(UBRR_VALUE>>8);
	UBRR0L = (uint8_t)UBRR_VALUE;
	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
	//enable transmission and reception
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}
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
//set stream pointer
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
//This function is never called since we are using interrupts instead
uint16_t ReadADC(uint8_t ADCchannel)
{
	//select ADC channel with safety mask
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
	//single conversion mode
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADC;
}
void ioinit(void)
{
	DDRB = 0b00000001; //All inputs except PB0 which is the LED output
	
	PORTB = 0b11111111; //Set to high (for power consumption purposes)
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
		TIMEDIFF = currenttime - lasttime;
	}
	else
	{
		TIMEDIFF = 256 - (lasttime - currenttime);
	}

	TIME = TIME + TIMEDIFF; //Add the elapsed time to total time
	TOT_OVERFLOW = 0; // Reset overflow counter
	//TCNT0 = 0;
	lasttime = currenttime;
	
	
	DATABUFFER[BUFFERINDEX] = ADC;
	BUFFERINDEX++;
	
	if (BUFFERINDEX==DATABUFFERSIZE)
	{
		BUFFERINDEX=0;
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
	
	//Initialize overflow counter variable
	TOT_OVERFLOW = 0;
	
	// initialize counter
	TCNT0 = 0;
}

// TIMER0 overflow interrupt service routine
// called whenever TCNT0 overflows
ISR(TIMER0_OVF_vect)
{
	// keep a track of number of overflows
	TOT_OVERFLOW++;
}


int main()
{
	
	uint16_t sensorval = 0;
	
	//Initialize IO
	ioinit();
	//initialize ADC
	InitADC();
	//Initialize USART0
	USART0Init();
	// initialize timer
	timer0_init();

	//Set buffer index to zero before enabling interrupts
	BUFFERINDEX = 0;
	TIME = 0;
	TIMEDIFF = 0;
	lasttime = 0;
	currenttime = 0;
	PORTB &= ~(1<<PORTB0); //Turn off led
	//Enable global interrupt flag (tell CPU)
	sei();
	//assign our stream to standart I/O streams
	stdout=&usart0_str;
	
	uint16_t localbufferindex=0;

	while(1)
	{
//printf("%u\n", (uint16_t)TIMEDIFF);
		//This is needed for some reason for the code to work
		if(BUFFERINDEX==0)
		{
		}
		while(localbufferindex != BUFFERINDEX)
		{
			sensorval = DATABUFFER[localbufferindex];
			//Start conversion NO THIS MESSES UP TIMING
			ADCSRA |= (1<<ADSC);

			//sending sensor value to terminal
			printf("%u\n", (uint16_t)sensorval);
			//Increment local buffer index
			localbufferindex++;
			if (localbufferindex==DATABUFFERSIZE)
			{
				localbufferindex=0;
			}
		}
	}
}
