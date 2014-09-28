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

#include <drivers/avrbus.h>
#include <drivers/uart.h>

#include <string.h>

#include "device.hpp"
#include "adc.hpp"
#include "time.hpp"
#include "gpio.hpp"

/*
struct adc_buffer_s{
	uint8_t adc[6];
}; 
*/


int main(){
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	bus_master_init();
	//time_init();
	
	//init interrupt
	sei();

	uart_printf("...");
	_delay_us(1000);
	
	device_t *adc = device("adc");
	device_t *timer = device("timer1_hp_timer");
	device_t *gpio = device("gpio");
	device_t *i2c = device("i2c");
	
	if(adc)
		uart_printf("ADC Initialized!\n");
	else {
		uart_printf("ADC FAILED init!\n");
		while(1);
	}
	if(timer)
		uart_printf("TIMER Initialized!\n");
	else {
		uart_printf("TIMER FAILED init!\n");
		while(1);
	}
	if(!i2c){
		uart_printf("ERROR initializing i2c!\n");
		while(1);
	}
	
	adc->ioctl(IOC_ADC_START_CONV, 0);

	timeout_t timeout = timeout_from_now(timer, 1000000UL);

	uint8_t chan = 0;
	if(gpio->ioctl(IOC_GPIO_LOCK_PIN, GPIO_PB0) != SUCCESS){
		uart_printf("MAIN: could not lock PB0 pin!\n");
		while(1);
	}
	
	while(1){
		if(timeout_expired(timer, timeout)){
			uint16_t value = 0;

			DDRB |= (1 << 0); 
			
			if(adc->read((uint8_t*)&value, 1) == SUCCESS){   
				uart_printf("ADC chan %d val: %d\n", chan, value);
				do {
					chan = (chan + 1) & 0x07;
					if(adc->ioctl(IOC_ADC_SET_CHANNEL, chan) != FAIL){
						break;
					} else {
						uart_printf("ADC chan %d: not available!\n", chan);
					} 
				} while(1); 
				timeout = timeout_from_now(timer, 500000UL);
			}
		}
		
		PORTB |= (1 << 0);
		//device_tick();
		PORTB &= ~(1 << 0); 
	}
	//uint32_t clock = time_get_clock();
	DDRB |= _BV(1);

	//adc_buffer_s adc;
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
		
		//_delay_ms(10); */
	}
	
}

