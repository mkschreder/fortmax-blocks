#include <avr/io.h>

#include "device.h"
#include "i2c.h"
#include "bmp085.h"

static i2c_command_t trans;

void bmp085_init(){
	
}

int16_t bmp085_ioctl(handle_t h, uint8_t ioc, int32_t data){
	return SUCCESS; 
}

// reads all sensor values into buffer
// buffer must point to bmp085_t struct!
int16_t bmp085_read(handle_t h, uint8_t *buffer, uint16_t size){
	//bmp085_t *bmp = (bmp085_t*) buffer;
	return SUCCESS; 
}

int16_t bmp085_write(handle_t h, const uint8_t *buffer, uint16_t size){
	return SUCCESS; 
}

handle_t bmp085_open(id_t id){
	//_i2c = device("i2c");
	//if(!_i2c) return FAIL;
	return 0; 
}

int16_t bmp085_close(handle_t h){
	//if(_i2c) _i2c->close();
	//_i2c = 0; 
	return SUCCESS; 
}

static void bmp085_tick(){
	
}

DECLARE_DRIVER(bmp085); 
