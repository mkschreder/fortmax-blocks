#pragma once

#include "device.h"
#include "timer.h"


enum gpio_pin {
	GPIO_PB0,
	GPIO_PB1,
	GPIO_PB2,
	GPIO_PB3,
	GPIO_PB4,
	GPIO_PB5,
	GPIO_PB6, //XTAL
	GPIO_PB7, //XTAL

	GPIO_PC0,
	GPIO_PC1,
	GPIO_PC2,
	GPIO_PC3,
	GPIO_PC4,
	GPIO_PC5,
	GPIO_PC6,
	GPIO_PC7,

	GPIO_PD0,
	GPIO_PD1,
	GPIO_PD2,
	GPIO_PD3,
	GPIO_PD4,
	GPIO_PD5,
	GPIO_PD6,
	GPIO_PD7,

	GPIO_COUNT
};

enum gpio_ioc {
	IOC_GPIO_LOCK_PIN,
	IOC_GPIO_UNLOCK_PIN,
	IOC_GPIO_ENABLE_PCINT,
	IOC_GPIO_DISABLE_PCINT,
	IOC_GPIO_SET_INPUT,
	IOC_GPIO_SET_OUTPUT,
	IOC_GPIO_SET_PULLUP,
	IOC_GPIO_SET_HIQ,
	IOC_GPIO_SET_HI,
	IOC_GPIO_SET_LO
};

typedef struct pcint_config {
	void (*handler)(struct pcint_config *conf);
	timeout_t time;
	uint8_t pin;
	uint8_t leading; 
	void *arg;
} pcint_config_t;

DEVICE_DECLARE(gpio); 
/*
#define GPIO_INPUT 0
#define GPIO_OUTPUT 1
#define GPIO_DISABLED 0
#define GPIO_ENABLED 1
*/
/*
#define GPIO_PIN_MODE_PACK(pin, mode) ((pin & 0xff) | ((mode & 1)<< 8))
#define GPIO_PIN(pinmode) (pinmode & 0xff)
#define GPIO_MODE(pinmode) ((pinmode & 0x100) >> 8); 
*/
