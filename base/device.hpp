#pragma once

#define FAIL (-1)
#define SUCCESS 0

#include <avr/io.h>

typedef struct device {
	const char *name; 
	void (*init)();
	int16_t (*ioctl)(uint8_t num, int32_t data); 
	int16_t (*write)(const uint8_t *buffer, uint16_t size);
	int16_t (*read)(uint8_t *buffer, uint16_t size);
	int16_t (*open)();
	int16_t (*close)();

	void (*tick)();
	
	struct device *next, *prev; 
} device_t;

extern void device_register(struct device *dev);
extern void device_tick(); 
extern struct device* device(const char *dev); 


class __init {
	public:
	__init(struct device *dev){
		device_register(dev);
	}
};

//#define USE(driver) extern device 
#define DRIVER(DEV) static __init DEV ## _instance(&DEV); 
