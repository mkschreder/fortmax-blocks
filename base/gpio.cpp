#include <avr/io.h>

#include "device.hpp"
#include "gpio.hpp"

static uint8_t _locked_pins[GPIO_COUNT / 8];
static void (*_pcint_table[GPIO_COUNT])();

#define SR_BIT(reg, bit, val) { if(val) reg |= (1 << bit); else reg &= ~(1 << bit);}

static inline void pin_lock(uint8_t pin){
	_locked_pins[pin >> 3] |= (1 << (pin % 8)); 
}

static inline void pin_unlock(uint8_t pin){
	_locked_pins[pin >> 3] &= ~(1 << (pin % 8)); 
}

static inline uint8_t pin_is_locked(uint8_t pin){
	return ((_locked_pins[pin >> 3] & (1 << (pin % 8))) == 0)?0:1; 
}

static void gpio_init(){
	// these are XTAL pins so they should not be used 
	pin_lock(GPIO_PB6);
	pin_lock(GPIO_PB7);
	
}

static int16_t gpio_ioctl(uint8_t ioc, int32_t data){
	switch(ioc){
		case IOC_GPIO_LOCK_PIN:
			if(pin_is_locked(data))
				return FAIL;
			else {
				pin_lock(data); 
				return SUCCESS;
			}
			return FAIL; 
		case IOC_GPIO_UNLOCK_PIN:
			pin_unlock(data);
			return SUCCESS;
		case IOC_GPIO_SET_INPUT:
		case IOC_GPIO_SET_OUTPUT: 
		{
			uint8_t pin = data & 0xff; 
			uint8_t out = (ioc == IOC_GPIO_SET_OUTPUT)?1:0; 
			if(pin >= GPIO_PB0 && pin <= GPIO_PB7){
				SR_BIT(DDRB, pin - GPIO_PB0, out); 
			} else if(pin >= GPIO_PC0 && pin <= GPIO_PC7){
				SR_BIT(DDRC, pin - GPIO_PC0, out); 
			} else if(pin >= GPIO_PD0 && pin <= GPIO_PD7){
				SR_BIT(DDRD, pin - GPIO_PD0, out); 
			}
			break;
		}
		case IOC_GPIO_SET_PULLUP:
		case IOC_GPIO_SET_HIQ: 
		{
			uint8_t pin = data & 0xff; 
			uint8_t mode = (ioc == IOC_GPIO_SET_PULLUP)?1:0; 
			if(pin >= GPIO_PB0 && pin <= GPIO_PB7){
				SR_BIT(PORTB, pin - GPIO_PB0, mode); 
			} else if(pin >= GPIO_PC0 && pin <= GPIO_PC7){
				SR_BIT(PORTC, pin - GPIO_PC0, mode); 
			} else if(pin >= GPIO_PD0 && pin <= GPIO_PD7){
				SR_BIT(PORTD, pin - GPIO_PD0, mode); 
			}
			break;
		}
		case IOC_GPIO_ENABLE_PCINT:
			{
				if(data > GPIO_COUNT || data < 0) return FAIL;
				void(*cb)() = (void(*)())(data & 0x00ffffff);
				uint8_t pin = (data & 0xff000000) >> 24; 
				_pcint_table[data] = (void(*)()) data;
				if(pin >= GPIO_PB0 && pin <= GPIO_PB7){
					PCICR |= (1<<PCIE0);  
					PCMSK0 = (1 << (pin - GPIO_PB0));
				} else {
					return FAIL;
				}
				return SUCCESS;
			}
		case IOC_GPIO_DISABLE_PCINT:
			if(data > GPIO_COUNT || data < 0) return FAIL;
			_pcint_table[data] = 0;
			return SUCCESS; 
	}
	return FAIL; 
}

// reads all sensor values into buffer
// buffer must point to gpio_t struct!
static int16_t gpio_read(uint8_t *buffer, uint16_t size){
	
}

static int16_t gpio_write(const uint8_t *buffer, uint16_t size){
	return SUCCESS; 
}

static int16_t gpio_open(){
	return SUCCESS; 
}

static int16_t gpio_close(){
	return SUCCESS; 
}

static void gpio_tick(){
	
}

static struct device gpio {
	.name = "gpio", 
	.init = &gpio_init,
	.ioctl = &gpio_ioctl, 
	.write = &gpio_write,
	.read = &gpio_read,
	.open = &gpio_open,
	.close = &gpio_close,
	.tick = &gpio_tick
};

DRIVER(gpio); 
