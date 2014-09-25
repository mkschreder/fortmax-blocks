/*********************************************

For more projects, visit https://github.com/fantachip/

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
#define F_CPU 11059200

#include <avr/io.h>
#include <avr/interrupt.h>


volatile uint8_t PWM;

int main(void)
{
  DDRB = 0b11111111; // Saidas

  PWM = 0;
  // Configure ADC prescaler to 16. ADC clock = 11059200 / 16 = 691kHz.
  ADMUX = (0<<REFS0) | (0<<REFS1) | (1<<ADLAR) ;
  ADCSRA = (1<<ADEN) | (0<<ADIE) | (1<<ADPS2) | (0<<ADPS1) | (1<<ADPS0);

  // Fast PWM mode, prescaler 256, no compare match output. Timer ticks at 43.2kHz, PWM frequency is 168Hz.
  TCCR2A = (0 << COM2A1) | (0 << COM2A0) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (0 << WGM22) | (1 << CS22) | (1 << CS21) | (0 << CS20);
  TIMSK2 = (0 << OCIE2A) | (1 << TOIE2);
  TCNT2 = 0;
  sei();

  while(1)
  {
    if(TCNT2 > PWM) PORTB &= ~(1 << PB1);
  }
}


ISR(TIMER2_OVF_vect)
{
  ADCSRA |= (1 << ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);
  if(PWM > ADCH) PWM--; else PWM++;
  PORTB |= (1 << PB1);
  TCNT2 = 1;
}*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <drivers/spi.h>
#include <drivers/uart.h>
#include <drivers/time.h>


int main(){
	_delay_ms(10);

	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	bus_master_init();
	time_init();
	
	//init interrupt
	sei();
	
	// Hello W
	char buffer[20];
	uint16_t err = 0;
	uint16_t reads = 0; 
	const char *str = "FOOOBAAR";

	uint32_t clock = time_get_clock();
	DDRB |= _BV(1); 
	while(1){
		/*time_reset();
		
		uint32_t end = time_get_clock() + time_us_to_clock(1500); 
		while(end > time_get_clock());
		PORTB |= _BV(1);
		end = time_get_clock() + time_us_to_clock(18500);
		while(end > time_get_clock());
		PORTB &= ~_BV(1); */
		
		//ext_write(0x4bef, str, 8); 
		// master
		if(bus_read(0x4bef, buffer, 16) == 16){ // read ext address 0x4004 - slave buffer bytes 4-20
			buffer[16] = 0; 
			uart_printf("YES! ");
			for(int c = 0; c < 16; c++){
				uart_putc(buffer[c]);
			}
			uart_puts("\r\n"); 
			reads++; 
		} else {
			uart_printf("Error rate: %d, %d\r\n", reads, ++err); 
		}
		
		//_delay_ms(10); */
	}
	
}

