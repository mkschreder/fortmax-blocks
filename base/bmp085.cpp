#include <avr/io.h>

#include "device.hpp"
#include "i2c.hpp"
#include "bmp085.hpp"

static device_t *_i2c = 0;
static i2c_command_t trans;

static void bmp085_init(){
	
}

static int16_t bmp085_ioctl(uint8_t ioc, int32_t data){
	return SUCCESS; 
}

// reads all sensor values into buffer
// buffer must point to bmp085_t struct!
static int16_t bmp085_read(uint8_t *buffer, uint16_t size){
	//bmp085_t *bmp = (bmp085_t*) buffer;
	return SUCCESS; 
}

static int16_t bmp085_write(const uint8_t *buffer, uint16_t size){
	return SUCCESS; 
}

static int16_t bmp085_open(){
	//_i2c = device("i2c");
	//if(!_i2c) return FAIL;
	return SUCCESS; 
}

static int16_t bmp085_close(){
	//if(_i2c) _i2c->close();
	_i2c = 0; 
	return SUCCESS; 
}

static void bmp085_tick(){
	
}

static struct device bmp085s {
	.name = "bmp085", 
	.init = &bmp085_init,
	.ioctl = &bmp085_ioctl, 
	.write = &bmp085_write,
	.read = &bmp085_read,
	.open = &bmp085_open,
	.close = &bmp085_close,
	.tick = &bmp085_tick
};

DRIVER(bmp085s); 
