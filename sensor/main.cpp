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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <drivers/spi.h>
#include <drivers/uart.h>

#include <string.h>


extern "C" uint16_t ext_get_address();

int main(){
	
	PORTC |= _BV(6);
	
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	
	//init interrupt
	sei();

	// slave
	char buffer[100];
	memset(buffer, 0, sizeof(buffer)); 
	strcpy(buffer, "Hello World!"); 
	bus_slave_init(0x4bef, buffer, 100);

	uart_printf("Ready!\r\n"); 
	while(1){
		//uart_printf("S:%d, A: %04x\r\n", ext_state(), ext_get_address());
	}
}
