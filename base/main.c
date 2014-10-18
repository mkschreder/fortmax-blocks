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
#include <stdio.h>
#include "basic.h"


void kwdhook_(char *msg) {
	//if (!strcmp(msg,"PRINTS"))
	//	expr(), emit(PRINTS_);
	//else	return 0;
	return 1;
}
/*
int dobasic(int argc, char **argv) {
	initbasic(0);
	kwdhook=kwdhook_;
	return interp(stdin);
}
*/
#include <util/delay.h>

#include <drivers/avrbus.h>
#include <drivers/uart.h>

#include <kernel/kernel.h>
#include <kernel/l3g4200d.h>
#include <kernel/ssd1306.h>
#include <kernel/timer.h>

#include <string.h>

void _start_measure(void *ptr); 
void _gyro_read_completed(void *ptr){
	handle_t dev = ptr;
	int16_t gx = 0, gy = 0, gz = 0; 
	l3g4200d_readraw(dev, &gx, &gy, &gz);
	uart_printf("GYRO: %d %d %d\n", gx, gy, gz);
	async_schedule(_start_measure, dev, 0); 
}

void _start_measure(void *ptr){
	uart_printf("START\n"); 
	handle_t dev = (handle_t)ptr;
	l3g4200d_readdata(dev, _gyro_read_completed, dev);
}

/*
void _heartbeat(void *ptr){
	uart_printf("X"); 
	async_schedule(_heartbeat, 0, 100000UL);
}

void _init_completed(void *ptr); 
void _reinit_display(void *ptr){
	ssd1306_init(ptr, _init_completed, ptr);
}

void _data_sent(void *ptr){
	uart_puts("datadone\n");
}
*/
/*
void _send_string(void *ptr){
	static const char data[170] =
		"Hello World! Now we have a worki"
		"ng OLED display driver for the S"
		"SSD1306 display controller. It's"
		" all async and runs on bg! Enjoy"
		"! 2014 Martin K. Schroder. info@"
		"fortmax.se"; 
	static uint8_t idx = 0;
	uint16_t len = strlen(data); 
	if(idx >= len)
		async_schedule(_data_sent, ptr, 0);
	else {
		uint8_t size = 64;
		if((len - idx) < 64) size = len - idx; 
		ssd1306_putstring(ptr, data + idx, size, _send_string, ptr);
	}
	idx += 64;
}

void _clear_display(void *ptr){
	uart_puts("clear 1/2 page\n");
	static uint8_t buffer[64];
	static const uint8_t total = 16 + 1;
	static int8_t iteration = 0;
	if(iteration == -1) iteration = 0; 
	uint8_t j = 0; 
	for(int i = 0; i < sizeof(buffer); i++){
		buffer[i] = 0x0; //(uint8_t)(1 << j);
		j++; if(j >= 8) j = 0; 
	}
	j = 0;
	for(int i = sizeof(buffer)-1; i >= 0; i--){
		buffer[i] = 0x0; //|= (uint8_t)(7 << j);
		j++; if(j >= 8) j = 0; 
	}
	iteration ++;
	if(iteration >= total) {
		iteration = -1;
		ssd1306_seek(ptr, 0); 
		async_schedule(_send_string, ptr, 0);
		return;
	} else {
		ssd1306_putraw(ptr, buffer, sizeof(buffer), _clear_display, ptr);
	} 
}
*/
int main(void){
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	bus_master_init();

	__asm("sei"); 

	handle_t kern = kernel_open(0);

	uart_printf("...\n");

	//delay_us(timer1, 1000000UL);

	if(!kern){
		uart_printf("ERROR INIT: %s\n", kdata.last_error);
		while(1);
	}

	//uart_puts("\e[H\e[2J");

	uart_printf("Gyro init..\n");

	/*
	handle_t l3g = l3g4200d_open(0);

	if(!l3g){
		uart_printf("NO GYRO!\n");
		while(1);
	}
	*/
	//l3g4200d_configure(l3g, _start_measure, 0, l3g); 

	//async_schedule(_heartbeat, 0, 100000UL); 
	
	handle_t disp = ssd1306_open(0);
	
	//ssd1306_init(disp, _clear_display, disp);

	uint16_t count = 0;
	
	//ssd1306_xy_printf(disp, 0, 3, "Loop time: %4d usec", 0);

	while(1){
		timeout_t time = timer_get_clock(0);
		//DDRD |= _BV(5);
		//PORTD |= _BV(5);
		
		driver_tick();

		ssd1306_xy_printf(disp, 0, 0, "       OSKit.se     ");
		async_tick(); 
		ssd1306_xy_printf(disp, 0, 2, "   Flight Controller");
		async_tick();
		ssd1306_xy_printf(disp, 0, 4, "        Board");
		async_tick();
		ssd1306_xy_printf(disp, 0, 6, "      Time: %dus    ", timer_clock_to_us(0, timer_get_clock(0) - time));
		/*
		ssd1306_xy_printf(disp, 0, 0, "Timer: %04x%04x", (uint16_t)(time >> 16), (uint16_t)(time & 0xffff));
		async_tick(); 
		ssd1306_xy_printf(disp, 0, 1, "Loop:  %u", count++);
		async_tick();
		ssd1306_xy_printf(disp, 0, 2, "Tasks: %d/%d",
			async_get_active_tasks(), async_get_max_tasks());
		async_tick();
		ssd1306_xy_printf(disp, 0, 3, "Loop time: %4d usec", timer_clock_to_us(0, timer_get_clock(0) - time));
*/
		//PORTD &= ~_BV(5); 
		//uart_puts("\e[H\e[?25l");
		//uart_printf("KVARS: \n"); 
		//struct kvar *vars = kvar_get_all();

		//uart_printf("Decl: %d %d     \n", (uint16_t)kdata.distance[0], (uint16_t)kdata.distance[1]); 
		//uart_printf("TIME: %04x%04x\n", (uint16_t)(time >> 16), (uint16_t)time); 
		/*if(vars){
			list_for_each(i, &vars->list) {
				struct kvar *ent = list_entry(i, struct kvar, list); 
				uart_printf("%02x %s\n", (uint8_t)(ent->id), ent->name);
			}
		}*/

		/*uart_puts("\e[H\e[?25l"); 
		uart_printf("D: %08d %08d, err: %s\n",
			kdata.distance[0], kdata.distance[1], kdata.last_error);
		for(int c = 0; c < 6; c++)
			uart_printf("ADC %d: %d\n", c, kdata.adc[c]);*/
		
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

