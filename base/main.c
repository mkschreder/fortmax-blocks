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

struct task_list {
	struct task_list *next;
	void (*func)(long arg);
	long arg;
	uint8_t sync; 
};

typedef struct task_list task_t;

task_t task[10];

#define OP(_func, _arg, _next) (task_t){.next = _next, .func = _func, .arg = _arg}; 

void _read_byte(long arg){

}

int main(void){
	task[0] = OP(_read_byte, 0, &task[1]);
	
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	bus_master_init();
	
	handle_t kern = kernel_open(0);

	__asm("sei"); 

	uart_printf("...\n");

	//delay_us(timer1, 1000000UL);

	if(!kern){
		uart_printf("ERROR INIT: %s\n", kdata.last_error);
		while(1);
	}

	uart_puts("\e[H\e[2J");
	
	while(1){
		uart_puts("\e[H\e[?25l");
		uart_printf("KVARS: \n"); 
		struct kvar *vars = kvar_get_all();

		uart_printf("Decl: %d %d     \n", (uint16_t)kdata.distance[0], (uint16_t)kdata.distance[1]);
		timeout_t time = timer_get_clock(0); 
		uart_printf("TIME: %04x%04x\n", (uint16_t)(time >> 16), (uint16_t)time); 
		if(vars){
			list_for_each(i, &vars->list) {
				struct kvar *ent = list_entry(i, struct kvar, list); 
				uart_printf("%02x %s\n", (uint8_t)(ent->id), ent->name);
			}
		}

		/*uart_puts("\e[H\e[?25l"); 
		uart_printf("D: %08d %08d, err: %s\n",
			kdata.distance[0], kdata.distance[1], kdata.last_error);
		for(int c = 0; c < 6; c++)
			uart_printf("ADC %d: %d\n", c, kdata.adc[c]);*/
		driver_tick();
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

