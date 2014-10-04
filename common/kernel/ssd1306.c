/*******
 * I2C asynchronous driver for ssd1306 series 128x64 point displays
 *
 * License: GPLv3
 * Author: Martin K. Schröder
 * Email: info@fortmax.se
 * Website: http://oskit.se
 */

#include <avr/pgmspace.h>
#include <stdlib.h>

#include <kernel/device.h>
#include <kernel/i2c.h>
#include <kernel/ssd1306.h>

#include "../drivers/uart.h"

#include <string.h>

#define DISPLAY_ADDRESS 0x78

#define U8G_ESC_CS(x) 255, (0xd0 | ((x)&0x0f))
#define U8G_ESC_ADR(x) 255, (0xe0 | ((x)&0x0f))
#define U8G_ESC_RST(x) 255, (0xc0 | ((x)&0x0f))
#define U8G_ESC_END 255, 254

typedef struct ssd1306_device {
	struct {
		uint8_t in_use : 1; 
		uint8_t busy : 1;
	} flags;

	// user callback
	async_callback_t callback;
	void *arg;

	// callbacks used for internal ops
	async_callback_t __callback;
	void *__arg;
	
	uint8_t _commands[40]; // command buffer
	uint8_t _data[64];
	uint8_t _data_size;

	uint16_t addr;  // byte address in the display ram
} ssd1306_device_t;

static ssd1306_device_t _device[1];
static handle_t i2c = 0;

static void __init_display(handle_t dev, async_callback_t cb, void *arg);

handle_t ssd1306_open(id_t id){
	ssd1306_device_t *dev = &_device[0];
	if(dev->flags.in_use) return INVALID_HANDLE;
	dev->flags.in_use = 1;
	dev->flags.busy = 0;
	return dev;
}

int8_t ssd1306_init(handle_t dev, async_callback_t callback, void *ptr){
	__init_display(dev, callback, ptr);
	return SUCCESS; 
}

/// asynchronously sends one command to the display
static void __each_command(void *block, uint8_t *it, uint8_t last, void *arg, void (*callback)(void *block)) {
	ssd1306_device_t *dev = (ssd1306_device_t*)arg;

	static uint8_t buffer[2];

	buffer[0] = 0x00;
	buffer[1] = (*it);

	// send the i2c command to the i2c driver and have it call
	// supplied callback when the transfer is completed
	i2c_command_t cmd = {
		.addr = DISPLAY_ADDRESS,
		.buf = buffer,
		.wcount = 2,
		.rcount = 0,
		.callback = callback,
		.arg = block
	}; 
	i2c_transfer(i2c, cmd);

	// if it is the last byte then call user callback 
	if(last){
		dev->flags.busy = 0;
		// schedule callback for async execution on next tick
		async_schedule(dev->callback, dev->arg, 0); 
	}
	// Aruduino equivalent (blocking)
	//uint8_t control = 0x00;   // Co = 0, D/C = 0
	//Wire.beginTransmission(_i2caddr);
	//Wire.write(control);
	//Wire.write(c);
	//Wire.endTransmission();
}

/// executes a series of commands using async foreach loop
/// one command per main loop tick. 
static void __run_commands(handle_t arg, uint8_t *buffer, uint8_t size, async_callback_t callback, void *cbarg){
	ssd1306_device_t *dev = (ssd1306_device_t*)arg;
	
	memcpy(dev->_commands, buffer, size);
	dev->callback = callback;
	dev->arg = cbarg;
	
	async_schedule_each(dev->_commands, 1, size, __each_command, dev); 
}

static void __each_data(void *block, uint8_t *it, uint8_t last, void *arg, void (*callback)(void *block)){
	ssd1306_device_t *dev = (ssd1306_device_t*)arg;
	
	static uint8_t buffer[2];
	buffer[0] = 0x40;
	buffer[1] = (*it); 
	i2c_transfer(i2c, (i2c_command_t){
		.addr = DISPLAY_ADDRESS,
		.buf = buffer,
		.wcount = 2,
		.rcount = 0,
		.callback = callback,
		.arg = block
	});
	if(last){
		dev->flags.busy = 0;

		// data is always preceded by command sequence to set address
		// so the user callback is placed in __ pointers and ordinary
		// pointers are used for internal callbacks. 
		async_schedule(dev->__callback, dev->__arg, 0); 
	}
}

static void __push_data(handle_t arg, uint8_t *buffer, uint8_t size, async_callback_t callback, void *ptr){
	ssd1306_device_t *dev = (ssd1306_device_t*)arg;
	
	memcpy(dev->_commands, buffer, size);
	dev->callback = callback;
	dev->arg = arg;
	
	async_schedule_each(dev->_commands, 1, size, __each_data, dev); 
}

static void _fill_display_data(void *arg){
	struct ssd1306_device *dev = (ssd1306_device_t*)arg;
	
	__push_data(arg, dev->_data, dev->_data_size, dev->__callback, dev->__arg);
}

/// Write raw buffer into the desplay. 
int8_t ssd1306_putraw(handle_t handle, const uint8_t *data, uint8_t size, async_callback_t callback, void *ptr){
	struct ssd1306_device *dev = (ssd1306_device_t*)handle;

	// copy the data into the internal buffer
	memcpy(dev->_data, data, size);
	dev->_data_size = size;
	
	dev->__callback = callback;
	dev->__arg = ptr;

	uint8_t col = dev->addr & 0x7f;
	
	uint8_t buffer[] = {
		U8G_ESC_ADR(0),           /* instruction mode */
		U8G_ESC_CS(1),             /* enable chip */
		0x000 | (col & 0x0f),		// set lower 4 bit of the col adr to 0 
		0x010 | ((col >> 4) & 0x0f),		// set higher 4 bit of the col adr to 0 
		0x0b0 | (dev->addr >> 7) & 0x0f,  	// page address
		U8G_ESC_END,                /* end of sequence */
	};
	
	dev->addr += dev->_data_size;

	__run_commands(handle, buffer, sizeof(buffer), _fill_display_data, handle); 
}

/// executes the display init sequence that powers on the display
static void __init_display(handle_t dev, async_callback_t callback, void *arg) {
	uint8_t buffer[] = {
		U8G_ESC_CS(0),             /* disable chip */
		U8G_ESC_ADR(0),           /* instruction mode */
		U8G_ESC_RST(1),           /* do reset low pulse with (1*16)+2 milliseconds */
		U8G_ESC_CS(1),             /* enable chip */
		
		0x0ae,				/* display off, sleep mode */
		0x0d5, 0x081,		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
		0x0a8, 0x03f,		/* multiplex ratio */
		0x0d3, 0x000,	0x00,	/* display offset */
		//0x040,				/* start line */
		0x08d, 0x014,		/* charge pump setting (p62): 0x014 enable, 0x010 disable */
		0x20, 0x00, // memory addr mode
		0x0a1,				/* segment remap a0/a1*/
		0xa5, // display on
		0x0c8,				/* c0: scan dir normal, c8: reverse */
		0x0da, 0x012,		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */
		0x081, 0x09f,		/* set contrast control */
		0x0d9, 0x011,		/* pre-charge period */
		0x0db, 0x020,		/* vcomh deselect level */
		//0x022, 0x000,		/* page addressing mode WRONG: 3 byte cmd! */
		0x0a4,				/* output ram to display */
		0x0a6,				/* none inverted normal display mode */
		0x0af,				/* display on */
  
		//U8G_ESC_CS(0),             /* disable chip */
		U8G_ESC_END                /* end of sequence */
		
	};
	
	__run_commands(dev, buffer, sizeof(buffer), callback, arg); 
}


void ssd1306_invertDisplay(handle_t dev, uint8_t i) {
	uint8_t command[1]; 
  if (i) {
    command[0] = SSD1306_INVERTDISPLAY;
  } else {
    command[0] = SSD1306_NORMALDISPLAY;
  }
  __run_commands(dev, command, 1, 0, 0); 
}


// startscrollright
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void ssd1306_startscrollright(handle_t dev, uint8_t start, uint8_t stop){
	uint8_t buffer[] = {
			SSD1306_RIGHT_HORIZONTAL_SCROLL,
			0x00, start, 0x00, stop, 0x01, 0xff,
			SSD1306_ACTIVATE_SCROLL
	};
	__run_commands(dev, buffer, sizeof(buffer), 0, 0); 
	/*
	ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
	ssd1306_command(0X00);
	ssd1306_command(start);
	ssd1306_command(0X00);
	ssd1306_command(stop);
	ssd1306_command(0X01);
	ssd1306_command(0XFF);
	ssd1306_command(SSD1306_ACTIVATE_SCROLL);*/
}

// startscrollleft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void ssd1306_startscrollleft(handle_t dev, uint8_t start, uint8_t stop){
	uint8_t buffer[] = {
			SSD1306_RIGHT_HORIZONTAL_SCROLL,
			0x00,
			start,
			0x00,
			stop,
			0x01,
			0xff,
			SSD1306_ACTIVATE_SCROLL
	};
	__run_commands(dev, buffer, sizeof(buffer), 0, 0); 
	/*ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
	ssd1306_command(0X00);
	ssd1306_command(start);
	ssd1306_command(0X00);
	ssd1306_command(stop);
	ssd1306_command(0X01);
	ssd1306_command(0XFF);
	ssd1306_command(SSD1306_ACTIVATE_SCROLL);*/
}

// startscrolldiagright
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void ssd1306_startscrolldiagright(handle_t dev, uint8_t start, uint8_t stop){
	uint8_t buffer[] = {
			SSD1306_SET_VERTICAL_SCROLL_AREA,
			0x00,
			SSD1306_LCDHEIGHT,
			SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL,
			0x0,
			start,
			0x00,
			stop,
			0x01,
			SSD1306_ACTIVATE_SCROLL
	};
	__run_commands(dev, buffer, sizeof(buffer), 0, 0);
	/*
	ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
	ssd1306_command(0X00);
	ssd1306_command(SSD1306_LCDHEIGHT);
	ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
	ssd1306_command(0X00);
	ssd1306_command(start);
	ssd1306_command(0X00);
	ssd1306_command(stop);
	ssd1306_command(0X01);
	ssd1306_command(SSD1306_ACTIVATE_SCROLL);*/
}

// startscrolldiagleft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
/*void SSD1306::startscrolldiagleft(uint8_t start, uint8_t stop){
	ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
	ssd1306_command(0X00);
	ssd1306_command(SSD1306_LCDHEIGHT);
	ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
	ssd1306_command(0X00);
	ssd1306_command(start);
	ssd1306_command(0X00);
	ssd1306_command(stop);
	ssd1306_command(0X01);
	ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306::stopscroll(void){
	ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
}*/

/*
static void ssd1306_fill(unsigned char dat)
{
	unsigned char i,j;

	ssd1306_command(0x00);//set lower column address
	ssd1306_command(0x10);//set higher column address
	ssd1306_command(0xB0);//set page address

	for (byte i=0; i<(SSD1306_LCDHEIGHT/8); i++)
	{
			// send a bunch of data in one xmission
			ssd1306_command(0xB0 + i);//set page address
			ssd1306_command(0);//set lower column address
			ssd1306_command(0x10);//set higher column address

			for(byte j = 0; j < 8; j++){
					Wire.beginTransmission(_i2caddr);
					Wire.write(0x40);
					for (byte k = 0; k < 16; k++) {
							Wire.write(dat);
					}
					Wire.endTransmission();
			}
	}
}

void ssd1306_draw8x8(uint8_t* buffer, uint8_t x, uint8_t y){
	// send a bunch of data in one xmission
	ssd1306_command(0xB0 + y);//set page address
	ssd1306_command(x & 0xf);//set lower column address
	ssd1306_command(0x10 | (x >> 4));//set higher column address

	Wire.beginTransmission(_i2caddr);
	Wire.write(0x40);
	Wire.write(buffer, 8);
	Wire.endTransmission();
}
*/

CONSTRUCTOR(ssd1306_setup){
	i2c = i2c_open(0);
}
