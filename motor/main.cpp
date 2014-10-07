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
	
	time_init();
	motor_init();
	
	//init interrupt
	sei();
	
	uint16_t motor_speed[4];
	
	//bus_slave_init(0x5000, (char*)&motor_speed, sizeof(motor_speed));
	
	uint16_t err = 0;
	uint16_t reads = 0; 

	uint32_t clock = time_get_clock();
	//DDRB |= _BV(1);
	
	for(int c = 0; c < 4; c++)
		motor_speed[c] = 1500;
		
	while(1){
		for(int c = 0; c < 4; c++){
			motor_set_speed(c, motor_speed[c]);
			motor_speed[c]++;
			if(motor_speed[c] > 2000) motor_speed[c] = 1000; 
		}
		for(int c = 0; c < 1000; c++){
			//_delay_us(1); 
			motor_do_epoch();
		}
		//_delay_ms(10); 
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

