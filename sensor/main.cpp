/*********************************************

For more projects, visit https://github.com/mkschreder/

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************/

/*
 * Author:  AVR Tutoruals
 * Website: www.AVR-Tutorials.com
 *
 * Written in AVR Studio 5
 * Compiler: AVR GNU C Compiler (GCC)
 */ 
 
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint16_t _adc = 0; 
/*ADC Conversion Complete Interrupt Service Routine (ISR)*/
ISR(ADC_vect)
{
	//DDRB |= _BV(1);
	//PORTB |= _BV(1); 
	_adc = ADCH; //(ADCH << 8) | ADCL;			// Output ADCH to PortD
	ADCSRA |= 1<<ADSC;		// Start Conversion
	//PORTB &= ~_BV(1); 
}

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//#include <drivers/avrbus.h>
//#include <drivers/uart.h>
//#include <drivers/time.h>

#include <string.h>

extern "C" uint16_t ext_get_address();

int main(){
/*
	PORTC |= _BV(6);
	
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	time_init();
	
	//init interrupt
	sei();

	DDRC &= ~_BV(PC0);
	
	ADCSRA = (1 << ADEN) | (1 << ADIE) |
		(1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
		
	ADMUX = (0 << REFS1) | (1 << REFS0) | (1 << ADLAR) |
		(0 << MUX0) | (0 << MUX1) | (0 << MUX2) | (0 << MUX3);

	DIDR0 = 0xff;
	
	ADCSRA |= 1<<ADATE;
	ADCSRA |= 1<<ADSC;		// Start Conversion
	ADCSRB = 0;
	
	// slave
	char buffer[100];
	const char *str = "\x01\x02\x03\x04\x05\x06\x07\x08\x09"; 
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, str, 9); 
	bus_slave_init(0x5000, buffer, 16);

	uart_printf("Ready!\r\n");

	DDRD |= _BV(PD2); // pin 4 - trigger
	DDRD |= _BV(PD3); // pin 5 - echo

	uint32_t timeout = timeout_from_now(100000);
	while(1){
		if(timeout_expired(timeout)){
			PORTD |= _BV(PD2);
			_delay_us(10);
			PORTD &= ~_BV(PD2);
			timeout = timeout_from_now(100000);
		}
		//ADCSRA |= 1<<ADSC;	
		//while(ADCSRA == (1<<ADSC));
		//_adc = ADCH;			// Output ADCH to PortD
		//ADCSRA |= 1<<ADSC;
		
	}
	*/
}
