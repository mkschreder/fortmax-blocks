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

#include <drivers/avrbus.h>
#include <drivers/uart.h>

#include <kernel/kernel.h>

#include <string.h>

int main(){
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	bus_master_init();

	handle_t kern = kernel_open(0);
	
	__asm("sei"); 

	uart_printf("...");

	delay_us(timer1, 1000000UL);

	if(!kern){
		uart_printf("ERROR INIT: %s\n", kdata.last_error);
		while(1);
	}
	
	while(1){
		DDRD |= _BV(5);
		PORTD |= _BV(5); 
		//uart_printf("DISTANCE: %d\t%d, err: %s\n",
		//	kdata.distance[0], kdata.distance[1], kdata.last_error);
		//for(int c = 0; c < 6; c++)
		//	uart_printf("ADC %d: %d\n", c, kdata.adc[c]); 
		driver_tick();
		PORTD &= ~_BV(5); 
	}
	/*
	uint8_t buffer[16];
	memset(buffer, 0, 16);
	
	uint16_t reads = 0, err = 0; 
	while(1){
		//uint16_t speed[4] = {400, 500, 600, 700};
		
		//uint8_t w = bus_write(0x5000, (char*)&speed, sizeof(speed));
		//uart_printf("Written %d\n", w); 

		// master
		//adc.adc[0] = 0;
		uint8_t rd = bus_read(0x5000, (char*)buffer, 4); 
		//if(bus_read(0x5000, (char*)&adc, sizeof(adc))){ // read ext address 0x4004 - slave buffer bytes 4-20
			//uart_printf("YES! %x %x %x %x\n", b[0], b[1], b[2], b[3]);
			uart_printf("RD: %d, ADC: %d\n", rd, buffer[0]);
			for(int c = 0; c < 16; c++){
				uart_printf("%d ", buffer[c]);
			}
			reads++; 
		//} else {
		//	uart_printf("Error rate: %d, %d\r\n", reads, ++err); 
		//}
		
		//_delay_ms(10); 
	}*/
	
}

