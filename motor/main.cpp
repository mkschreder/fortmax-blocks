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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <drivers/spi.h>
#include <drivers/uart.h>
#include <drivers/time.h>

#include "motor.hpp"

int main(){
	_delay_ms(10);

	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	bus_master_init();
	time_init();
	motor_init();
	
	//init interrupt
	sei();
	
	// Hello W
	char buffer[20];
	uint16_t err = 0;
	uint16_t reads = 0; 
	const char *str = "FOOOBAAR";

	uint32_t clock = time_get_clock();
	DDRB |= _BV(1);
	motor_set_speed(0, 1000);
	
	while(1){
		motor_do_epoch();
		/*
		uint32_t end = time_get_clock() + time_us_to_clock(1500); 
		while(end > time_get_clock());
		PORTB |= _BV(1);
		end = time_get_clock() + time_us_to_clock(18500);
		while(end > time_get_clock());
		PORTB &= ~_BV(1); 
		*/
		//ext_write(0x4bef, str, 8); 
		// master
		/*if(ext_read(0x4bef, buffer, 16) == 16){ // read ext address 0x4004 - slave buffer bytes 4-20
			buffer[16] = 0; 
			//uart_printf("Read success! ");
			for(int c = 0; c < 16; c++){
				//uart_putc(buffer[c]);
			}
			reads++; 
		} else {
			uart_printf("Error rate: %d, %d\r\n", reads, ++err); 
		}
		
		//_delay_ms(10); */
	}
	
}

