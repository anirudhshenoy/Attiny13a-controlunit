/*
 * ATtiny13a Control Unit
 * File: main.c
 * Author: Anirudh
 * Pin Configuration
 * PB0 and PB3 switch
 */

#include <avr/io.h>
#include <avr/interrupt.h>

unsigned int count=0;
volatile int timer_overflow_count_pulse = 0;
volatile int timer_overflow_count_time = 0;
volatile unsigned char trigger=0;


ISR(INT0_vect){
	trigger=1;
}

ISR(TIM0_COMPA_vect) {
	if ((PINB & 0b00001000)==0){								//Pulse Mode
		if (++timer_overflow_count_pulse > count) {
			PORTB ^= (1<<PB4);
			timer_overflow_count_pulse = 0;
		}
	}
	else{														//Time Mode
		if (++timer_overflow_count_pulse > count*10) {
			PORTB &= ~(1<<PB4);
			TCCR0B=0x00;
			timer_overflow_count_time = 0;
		}
	}
	
}

int main(void)
{
    cli();
    DDRB = 0b00010000;	// Set up PB4 as output, PB3 as input 
	PORTB = 0b00001001;
	GIMSK = 0b01000000;
	PCMSK = 0b00000010;
	MCUCR = 0b00000011;
	ADMUX = 0b00100001;				//PORTB |= 1<<PB4;
	ADCSRA = 0b10000111;			//Enable ADC, div 16
	TCCR0A |= (1<<WGM01);			// CTC Mode
	OCR0A=234;						//Set top, compare match ~0.2secs
    TIMSK0 |=1<<OCIE0A;				// enable timer overflow interrupt
    sei();
    while (1) {
		if (trigger==1) {	
			if ((PINB & 0b00000001)==0){							//Time keeper mode
				ADCSRA |= 0b01000000;
				while (!(ADCSRA & (1 << ADIF)));
				ADCSRA |= (1 << ADIF);
				count = ADCH;
				TCCR0B |= (1<<CS02);								// Pre-scale timer to 1/256th the clock rate
				PORTB|=(1<<PB4);									//Set O/P tohigh
				count =count>>2;									//Calculate pulse high time 
				trigger=0;					
				timer_overflow_count_pulse = 0;
			}
			else{													// Toggle Mode
				PORTB^=(1<<PB4);
				trigger=0;
			}
			
		}
		else {
			if(((PINB & 0b00001000)==0) && (PINB & 0b00000010)){		//Pulse Mode and I/P is high
				ADCSRA |= 0b01000000;
				while (!(ADCSRA & (1 << ADIF)));
				ADCSRA |= (1 << ADIF);
				count = ADCH;
				TCCR0B |= (1<<CS02);										// Pre-scale timer to 1/256th the clock rate
				count =count>>3;											//Calculate pulse duty cycle
				timer_overflow_count_time = 0;
				trigger = 0; 
			}
			else if (!(PINB & 0b00001000)){
				TCCR0B=0x00;
				PORTB &= ~(1<<PB4);
			}
		}
	}
    return 0;
}

