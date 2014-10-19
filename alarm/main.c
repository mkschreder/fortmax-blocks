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

//#include <drivers/avrbus.h>
#include <kernel/uart.h>
//#include <drivers/nrf24l01.h>
//#include <drivers/bmp085.h>
//#include <drivers/l3g4200d.h>
//#include <drivers/adxl345.h>

#include <kernel/kernel.h>
#include <kernel/nrf24l01.h>
#include <kernel/ssd1306.h>
#include <kernel/l3g4200d.h>
#include <kernel/timer.h>

#include <string.h>

//********************
//** Board settings **
//********************

#define RADIO_CE_PIN GPIO_PB0
#define RADIO_CS_PIN GPIO_PB1

int main(void){
	uart_init(UART_BAUD_SELECT(38400, F_CPU));
	
	__asm("sei"); 

	uart_printf("\nINIT\n"); 
	handle_t kern = kernel_open(0);
	//handle_t radio = nrf24l01_open(0); 
	handle_t disp = ssd1306_open(0);
	handle_t gyro = l3g4200d_open(0); 

	int16_t gx, gy, gz;
	timeout_t frame_time = 0;
	timeout_t read_timeout = timeout_from_now(0, 20000); 
	while(1){
		timeout_t time = timer_get_clock(0); 

		driver_tick();

		if(timeout_expired(0, read_timeout)){
			l3g4200d_read(gyro, &gx, &gy, &gz);

			//uart_printf("GYRO: %d %d %d\n", gx, gy, gz);
			
			ssd1306_xy_printf(disp, 0, 0, "       ALARM        ");
			ssd1306_xy_printf(disp, 0, 1, "G: %4d, %4d, %4d", gx, gy, gz);
			ssd1306_xy_printf(disp, 0, 3, "    Loop: %dus  ", frame_time);

			read_timeout = timeout_from_now(0, 20000);
		}
		//uart_printf("%d\n", frame_time);
		
		frame_time = timer_clock_to_us(0, timer_get_clock(0) - time); 
	}
}

