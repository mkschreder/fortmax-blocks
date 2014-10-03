#pragma once

#include "device.h"
#include "async.h"

typedef struct i2c_command {
	uint8_t addr;	// i2c device address
	uint8_t *buf; // buffer that contains bytes read and bytes to write
	uint8_t wcount; // bytes to write into the i2c device
	uint8_t rcount; // bytes to read from i2c device
	async_callback_t callback; // callbeck called upon completion
	void *arg;  // argument to callback 
} i2c_command_t; 

handle_t 	i2c_open(id_t id); 
int8_t    i2c_transfer(handle_t dev, i2c_command_t cmd);
