#pragma once

#include <kernel/device.h>
#include <kernel/timer.h>


typedef enum gpio_pin {
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
} gpio_pin_t;

/*
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
*/
typedef struct pcint_info {
	void (*handler)(struct pcint_info *conf, void *arg);
	timeout_t time;
	uint8_t pin;
	struct {
		uint8_t enabled : 1;
		uint8_t leading : 1;
	} flags; 
	void *arg;
} pcint_config_t;

typedef struct gpio_flags {
	uint8_t output : 1; 
	uint8_t pullup : 1; 
} gpio_flags_t;

handle_t gpio_open(id_t id);
int16_t gpio_close(handle_t dev);

int8_t gpio_configure(handle_t dev, gpio_pin_t pin, gpio_flags_t flags); 
int8_t gpio_enable_pcint(handle_t dev, gpio_pin_t pin, void (*handler)(struct pcint_info *info, void *arg), void *arg);
int8_t gpio_set(handle_t dev, gpio_pin_t pin, uint8_t value);
int8_t gpio_get(handle_t dev, gpio_pin_t pin);
int8_t gpio_register(handle_t dev, gpio_pin_t pin);
int8_t gpio_release(handle_t dev, gpio_pin_t pin);

//DEVICE_DECLARE(gpio); 
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
