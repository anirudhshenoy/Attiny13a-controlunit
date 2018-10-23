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
volatile int timer_overflow_count = 0;
volatile unsigned char trigger=0;
unsigned char toggle=0b0;


ISR(INT0_vect){
	trigger=1;
}

ISR(TIM0_COMPA_vect) {
	if ((PINB & 0b00001000)==0){													//Pulse Mode
		if (++timer_overflow_count > count) {
			PORTB ^= (1<<DDB4);
			timer_overflow_count = 0;
		}
	}
	else if ((PINB & 0b00000001)==0){												//Time Mode
		if (++timer_overflow_count > count*10) {
			PORTB &= ~(1<<DDB4);
			TCCR0B=0x00;
			timer_overflow_count = 0;
		}
	}
	else {																			//Toggle Mode
		if (++timer_overflow_count > count) PORTB |= (1 << DDB4);
		else PORTB &= ~(1 << DDB4);										//Software PWM Algo	
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
	OCR0A=234;						//Set top, compare match ~0.05secs
    TIMSK0 |=1<<OCIE0A;				// enable timer overflow interrupt
    sei();
    while (1) {
		if (trigger==1) {
			if ((PINB & 0b00000001)==0){							//Time keeper mode
				ADCSRA |= 0b01000000;
				while (!(ADCSRA & (1 << ADIF)));
				ADCSRA |= (1 << ADIF);
				count = ADCH;
				timer_overflow_count = 0;
				PORTB|=(1<<DDB4);									//Set O/P to high
				count =count>>2;									//Calculate pulse high time 
				trigger=0;				
				TCCR0B |= (1<<CS02);								// Pre-scale timer to 1/256th the clock rate	
				
			}
			else{													// Toggle Mode
				toggle^=0b1;
				trigger=0;
			}
			
		}
		else {
			if(((PINB & 0b00001000)==0) && (PINB & 0b00000010)){		//Pulse Mode and I/P is high
				ADCSRA |= 0b01000000;
				while (!(ADCSRA & (1 << ADIF)));
				ADCSRA |= (1 << ADIF);
				count = ADCH;
				timer_overflow_count = 0;
				count =count>>3;											//Calculate pulse duty cycle
				trigger = 0; 
				TCCR0B |= (1<<CS02);										// Pre-scale timer to 1/256th the clock rate
				
			}
			else if (!(PINB & 0b00001000)){							//Pulse Mode but button off
				TCCR0B=0x00;
				PORTB &= ~(1<<DDB4);
			}
			
		}
		if ((PINB & 0b00001000) && (PINB & 0b00000001)){			//Toggle Mode
			if(toggle){										// Toggle variable O/P is high (420 everyday)
				ADCSRA |= 0b01000000;
				while (!(ADCSRA & (1 << ADIF)));
				ADCSRA |= (1 << ADIF);
				count = ADCH;
				timer_overflow_count=0;
				TCCR0B|= (1<<CS00);							//Start Timer with no pre-scaler
			}
			else{
				TCCR0B=0x00;
				PORTB &= ~(1<<DDB4);
			}
		}
	}
    return 0;
}

