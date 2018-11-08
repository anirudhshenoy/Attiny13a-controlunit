/*
 * ATtiny13a Control Unit
 * File: main.c
 * Author: Anirudh
 * Pin Configuration
 * PB0 and PB3 switch
 * Pulse mode: PB2 HIGH PB0 HIGH Mode 0
 * Toggle Mode: PB2 LOW PB0 HIGH Mode 1
 * Timer Mode: PB2 PULLED LOW PB0 LOW Mode 2
 * Fuses OK (E:FF, H:FF, L:7A)
 * Before burning : Change ALL PB3 instances to PB2
 * Check ADMUX
 */

#include <avr/io.h>
#include <avr/interrupt.h>

unsigned int count=0;
volatile int timer_overflow_count = 0;
volatile int time_keeper_count = 0;
volatile int mode = 0;
volatile unsigned char trigger=0;
unsigned char toggle=0b0;


ISR(INT0_vect){
	trigger=1;
}

void clear_PB2 (void) {
	DDRB &= ~(1<<DDB3);
	PORTB |= (1<<DDB3);
	
}
void set_PB2_OP (void) {
	DDRB|= (1<<DDB3);
	PORTB &= ~(1<<DDB3);
}
ISR(TIM0_OVF_vect) {																
	if (mode==1){											//Toggle Mode
		if (++timer_overflow_count < count) PORTB |= (1 << DDB4);
		else PORTB &= ~(1 << DDB4);											//Software PWM Algo
		if (timer_overflow_count>256) timer_overflow_count=0;
		
	}
	else if (mode==2){												//Time Mode
		if(++timer_overflow_count > 17){											//0.464 secs
			if (++time_keeper_count > count) {
				PORTB &= ~(1<<DDB4);
				TCCR0B=0x00;
				time_keeper_count = 0;
			}
			timer_overflow_count=0;
		}
	}
	else {
		if (++timer_overflow_count > count) {									//Pulse Mode
			PORTB ^= (1<<DDB4);
			timer_overflow_count = 0;
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
	//TCCR0A |= (1<<WGM01);			// CTC Mode
    //TIMSK0 = 0b00000100;			// enable timer overflow interrupt
	TCCR0A=0x00;
	TIMSK0=0b00000010;
    sei();
    while (1) {
		if (trigger==1) {
			set_PB2_OP();
			if ((PINB & 0b00000001)==0){							//Time keeper mode
				clear_PB2();
				mode=2;
				ADCSRA |= 0b01000000;
				while (!(ADCSRA & (1 << ADIF)));
				ADCSRA |= (1 << ADIF);
				count = ADCH;
				timer_overflow_count = 0;
				time_keeper_count = 0;
				PORTB|=(1<<DDB4);									//Set O/P to high
				count =count>>1;									//Calculate pulse high time 
				trigger=0;
				//OCR0A=234;				
				TCCR0B |= (1<<CS02) | (1<<CS00);								// Pre-scale tim9er to 1/1024 the clock rate	
				
				
			}
			else {
					clear_PB2();
					if((PINB & 0b00001000)==0){						//Pulse Mode
					mode =0;
					PORTB|=(1<<DDB4);
					//OCR0A=234;
					trigger=0;
				}
					else {
						mode=1;													// Toggle Mode
						toggle^=0b1;
						timer_overflow_count=0;								//Start Timer with no pre-scaler
						//OCR0A=1;
						trigger=0;
				}	
			}
		}
		if(((PINB & 0b00001000)==0) && (PINB & 0b00000010)){		//Pulse Mode and I/P is high
			ADCSRA |= 0b01000000;
			while (!(ADCSRA & (1 << ADIF)));
			ADCSRA |= (1 << ADIF);
			count = ADCH;
			count =count>>2;											//Calculate pulse duty cycle
			TCCR0B|= (1<<CS02) | (1<<CS00);										// Pre-scale timer to 1/256th the clock rate
				
		}
		else if ((PINB & 0b00001000) && (PINB & 0b00000001)){			//Toggle Mode
			if(toggle){										// Toggle variable O/P is high (420 everyday)
				ADCSRA |= 0b01000000;
				while (!(ADCSRA & (1 << ADIF)));
				ADCSRA |= (1 << ADIF);
				count = ADCH;
				TCCR0B|= (1<<CS00);
				
			}
			else{
				TCCR0B=0x00;
				PORTB &= ~(1<<DDB4);
			}
		}
		else if (!(PINB & 0b00001000)){							//Pulse Mode but button off
			TCCR0B=0x00;
			timer_overflow_count = 0;
			PORTB &= ~(1<<DDB4);
		}
			
		

	}
    return 0;
}

