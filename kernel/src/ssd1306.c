/*******
 * I2C asynchronous driver for ssd1306 series 128x64 point displays
 *
 * License: GPLv3
 * Author: Martin K. Schröder
 * Email: info@fortmax.se
 * Website: http://oskit.se
 */

#ifdef __AVR__
#include <avr/io.h>
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif

#include <stdlib.h>

#include <kernel/device.h>
#include <kernel/i2c.h>
#include <kernel/ssd1306.h>
#include <kernel/ssd1306_priv.h>
#include <kernel/uart.h>

#include <string.h>

#define DISPLAY_ADDRESS 0x78

#define TEXT_W (21)
#define TEXT_H (8)
#define TEXTSZ (TEXT_W * TEXT_H)
#define PACKETSZ (8 * 6 + 1) // 8 chars * 6 bytes each + 1 command byte
#define debug(msg) {} //uart_printf("SSD1306: %s\n", msg);


enum {
	SSD_IDLE, 
	SSD_TX_INIT,
	SSD_TX_START,
	SSD_TX_DATA
};

// types of requests:
// init: one long command request
// scroll: one small command request
// data: one command, then data request
// text: 
typedef struct ssd1306_device {
	struct {
		uint8_t in_use : 1; 
		uint8_t busy : 1;
		uint8_t need_sync : 1;
	} flags;

	uint8_t front[TEXTSZ];
	uint8_t back[TEXTSZ];
	//uint8_t _commands[32]; // command buffer

	struct i2c_command i2c_request;
	uint8_t _buffer[PACKETSZ];
	
	uint8_t state;
	const uint8_t *cur_buf;
	uint16_t cur_idx;
	uint16_t cur_length;
	uint8_t cur_char; // buffering for current char
	
} ssd1306_device_t;

static ssd1306_device_t _device[1];
static handle_t i2c = 0;

static void __update(void *arg);

handle_t ssd1306_open(id_t id){
	ssd1306_device_t *dev = &_device[0];
	if(dev->flags.in_use) return INVALID_HANDLE;
	dev->flags.in_use = 1;
	dev->flags.busy = 0;

	dev->cur_idx = 0;
	dev->state = SSD_TX_INIT;
	dev->cur_buf = cmd_init;
	dev->cur_length = sizeof(cmd_init);

	debug("OPEN"); 
	__update(dev); 
	
	return dev;
}

static uint16_t _frame_count = 0; 
uint16_t ssd1306_get_frame_count(){
	return _frame_count;
}

static void __update(void *arg) {
	//DDRD |= _BV(4);
	//PORTD |= _BV(4);
	
	ssd1306_device_t *dev = (ssd1306_device_t*)arg;

	uint8_t tx_count = 2; 
	switch(dev->state){
		case SSD_TX_START:
			debug("START");
			dev->_buffer[0] = 0x00; // command
			dev->_buffer[1] = pgm_read_byte(&dev->cur_buf[dev->cur_idx++]);
			tx_count = 2; 
			break; 
		case SSD_TX_INIT:
			debug("INIT"); 
			dev->_buffer[0] = 0x00; // command
			dev->_buffer[1] = pgm_read_byte(&dev->cur_buf[dev->cur_idx++]);
			tx_count = 2; 
			break; 
		case SSD_TX_DATA:
			//debug("DATA");
			tx_count = PACKETSZ;
			dev->_buffer[0] = 0x40;
			for(int c = 1; c < tx_count; c++)
			{
				if(dev->cur_idx == 0) dev->cur_char = dev->cur_buf[0];
				uint8_t char_col = dev->cur_idx & 0x07;
				
				if(char_col < 5){ // next four cols are just font stuff
					dev->_buffer[c] = pgm_read_byte(&font[dev->cur_char * 5 + (char_col)]);
					dev->cur_idx++;
				} else if(char_col == 5){ // last one is letter separator
					dev->_buffer[c] = 0;
					dev->cur_idx &= ~0x07; // next char;
					dev->cur_idx += 8;
					dev->cur_char = dev->cur_buf[dev->cur_idx >> 3];
					//dev->cur_idx++;
				}
			}
			break; 
	};

	dev->i2c_request = (i2c_command_t){
		.addr = DISPLAY_ADDRESS,
		.buf = dev->_buffer,
		.wcount = tx_count,
		.rcount = 0,
		.callback = __update,
		.arg = dev
	};

	// update state
	if(dev->cur_idx >= dev->cur_length){
		dev->cur_idx = 0;
		
		switch(dev->state){
			case SSD_TX_START:
				debug("START -> DATA"); 
				dev->state = SSD_TX_DATA;
				// swap buffers
				memcpy(dev->front, dev->back, TEXTSZ);
				dev->cur_buf = dev->front; 
				dev->cur_length = TEXT_H * TEXT_W * 8;
				_frame_count++; 
				break; 
			case SSD_TX_INIT:
			case SSD_TX_DATA:
				debug("INIT|DATA -> START"); 
				dev->state = SSD_TX_START;
				dev->cur_buf = cmd_home;
				dev->cur_length = sizeof(cmd_home);
				break; 
		};
	}
	
	i2c_transfer(i2c, &dev->i2c_request);
}

int16_t ssd1306_xy_printf(handle_t h, uint8_t x, uint8_t y, const char *fmt, ...){
	uint16_t n; 
	va_list vl; 
	va_start(vl, fmt);
	struct ssd1306_device *dev = (struct ssd1306_device*)h;
	uint8_t start = y * TEXT_W + x;
	
	n = vsnprintf((char*)(dev->back + start), TEXTSZ-start, fmt, vl);
	
	va_end(vl);

	dev->flags.need_sync = 1; 
	return n; 
}

int16_t ssd1306_xy_puts(handle_t h,
	uint8_t x, uint8_t y, const char *str){
	struct ssd1306_device *dev = (struct ssd1306_device*)h;

	ssd1306_xy_printf(dev, x, y, str); 

	dev->flags.need_sync = 1; 
	return 1; 
}

void ssd1306_reset(handle_t h){
	struct ssd1306_device *dev = (struct ssd1306_device*)h;
	
	for(int c = 0; c < TEXTSZ; c++){
		dev->back[c] = ' ';
	}
}

CONSTRUCTOR(ssd1306_setup){
	i2c = i2c_open(0);
}

