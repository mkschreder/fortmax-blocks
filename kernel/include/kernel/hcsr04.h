#pragma once

#include <kernel/gpio.h>

struct hcsr04_config {
	uint8_t mode; 
	uint8_t trig;
	uint8_t echo;
}; 

//#define IOC_HCSR_TRIGGER 1
enum hcsr_op {
	HCSR04_READ
};

#define IOC_HCSR_CONFIGURE 2

#define HCSR_GPIO 1

enum {
	HCSR04_DEFAULT, 
	HCSR04_01, 
	HCSR04_02, 
	HCSR04_03, 
	HCSR04_04, 
	HCSR04_05,
	HCSR04_MAX
};

enum {
	E_HCSR_NO_GPIO = -2,
	E_HCSR_NO_TIMER1 = -3,
	E_HCSR_TRIGGER_NA = -4,
	E_HCSR_ECHO_NA = -5,
	E_HCSR_PCINT_NA = -6
}; 

handle_t hcsr04_open(id_t id);
int16_t hcsr04_close(handle_t h);

int8_t hcsr04_trigger(handle_t dev, void (*callback)(void* arg), void *arg); 
int16_t hcsr04_configure(handle_t dev, gpio_pin_t trigger_pin, gpio_pin_t echo_pin);
uint32_t hcsr04_get_distance(handle_t dev);

DEVICE_DECLARE(hcsr04); 