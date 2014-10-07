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

#include <util/delay.h>

#include <drivers/avrbus.h>
#include <drivers/uart.h>
#include <drivers/nrf24l01.h>

#include <kernel/kernel.h>
#include <kernel/nrf24l01.h>
#include <kernel/ssd1306.h>
#include <kernel/timer.h>

#include <string.h>

//********************
//** Board settings **
//********************

#define RADIO_CE_PIN GPIO_PB0
#define RADIO_CS_PIN GPIO_PB1

//********************
int main(void){
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	
	__asm("sei"); 

	handle_t kern = kernel_open(0);

	uart_printf("FORTMAX Flight Control v1.0\n");

	if(!kern){
		uart_printf("ERROR INIT: %s\n", kdata.last_error);
		while(1);
	}

	//handle_t radio = nrf24l01_open(0); 
	handle_t disp = ssd1306_open(0);

	uint8_t bufferin[NRF24L01_PAYLOAD];
	const char *status = "n/a";
	uint16_t packets = 0;
	static uint8_t addr[] = {0xE8, 0xE8, 0xF0, 0xF0, 0xE2};
	
	nrf24l01_init();
	nrf24l01_settxaddr(addr);
	
	while(1){
		timeout_t time = timer_get_clock(0);
		
		driver_tick();

		//rx
		uint8_t pipe = 0;
		if(nrf24l01_readready(&pipe)) { //if data is ready
			//read buffer
			nrf24l01_read(bufferin);
			
			status = "  Packet received!  ";
			packets++; 
			for(int i=0; i<sizeof(bufferin); i++)
				bufferin[i] = 0;
		} else {
			//status = "     Waiting...     ";
		}

		ssd1306_xy_printf(disp, 0, 0, "       OSKit.se     ");
		ssd1306_xy_printf(disp, 0, 1, "  Flight Controller ");
		ssd1306_xy_printf(disp, 0, 3, "    Loop: %dus  ", timer_clock_to_us(0, timer_get_clock(0) - time));
		ssd1306_xy_printf(disp, 0, 5, "        Status:     ");
		ssd1306_xy_printf(disp, 0, 6, "   Received: %04d", packets);
	}
}

