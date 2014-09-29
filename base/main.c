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
	__asm("sei"); 

	uart_printf("...");

	delay_us(timer1, 1000000UL);
	
	//device_t *adc = device("adc");
	//device_t *timer = device("timer1_hp_timer");
	//device_t *gpio = device("gpio");
	//device_t *i2c = device("i2c");

	handle_t hcsr1 = dev_open(hcsr04, 0);
	handle_t hcsr2 = dev_open(hcsr04, 1);
	
	if(!hcsr1 || !hcsr2){
		uart_printf("Could not allocate an HCSR device!\n");
		while(1); 
	}
	struct hcsr04_config conf = {
		.mode = HCSR_GPIO, 
		.trig = GPIO_PB0,
		.echo = GPIO_PB1
	};
	if(SUCCESS != dev_ioctl(hcsr04, hcsr1, IOC_HCSR_CONFIGURE, &conf)){
		uart_printf("CANT SET UP HC1\n");
	}
	
	conf.mode = HCSR_GPIO; 
	conf.trig = GPIO_PD6;
	conf.echo = GPIO_PD3;
	
	if(SUCCESS != dev_ioctl(hcsr04, hcsr2, IOC_HCSR_CONFIGURE, &conf)){
		uart_printf("CANT SET UP HC2\n");
	}
	int16_t ret = 0; 
	
	if(!dev_open(adc, 0)){
		uart_printf("ADC FAILED init!\n");
		while(1);
	}
	if(!dev_open(timer1, 0)){
		uart_printf("TIMER FAILED init!\n");
		while(1);
	}
	dev_open(gpio, 0);
	
	if(!dev_open(i2c, 0)){
		uart_printf("ERROR initializing i2c!\n");
		while(1);
	}
	
	dev_ioctl(adc, 0, IOC_ADC_START_CONV, 0);

	timeout_t timeout = timeout_from_now(timer1, 1000000UL);

	uint8_t chan = 0;
	if(dev_ioctl(gpio, 0, IOC_GPIO_LOCK_PIN, GPIO_PB0) != SUCCESS){
		uart_printf("MAIN: could not lock PB0 pin!\n");
		while(1);
	}

	uint32_t distance1 = 0, distance2 = 0;
	
	while(1){
		//PORTB |= (1 << 0);
		hcsr04_ioctl(hcsr1, IOC_HCSR_TRIGGER, 0);
		hcsr04_ioctl(hcsr2, IOC_HCSR_TRIGGER, 0);
		
		if(timeout_expired(timer1, timeout)){
			uint16_t value = 0;
			//_delay_ms(500);
			
			//DDRB |= (1 << 0); 

			if(dev_read(adc, 0, (uint8_t*)&value, 1) == SUCCESS){
				dev_read(hcsr04, hcsr1, &distance1, sizeof(distance1));
				dev_read(hcsr04, hcsr2, &distance2, sizeof(distance2));
				
				//uart_printf("ADC chan %d val: %d, dist: %d\n", chan, value, distance1);
				uint16_t d1 = distance1, d2 = distance2; 
				uart_printf("DIS: %d\t%d\n", d2, d1); 
				do {
					chan = (chan + 1) & 0x07;
					if(dev_ioctl(adc, 0, IOC_ADC_SET_CHANNEL, chan) != FAIL){
						break;
					} else {
						//uart_printf("ADC chan %d: not available!\n", chan);
					} 
				} while(1); 
				timeout = timeout_from_now(timer1, 50000UL);
			}
		}
		
		driver_tick();
		//PORTB &= ~(1 << 0); 
	}
	//uint32_t clock = time_get_clock();
	//DDRB |= _BV(1);

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

