#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "device.h"
#include "gpio.h"
#include "timer.h"

static uint8_t _locked_pins[GPIO_COUNT / 8];
static struct pcint_config *_pcint_table[GPIO_COUNT];
static uint8_t _prev_pinc = 0;

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



static inline void _pcint(uint8_t cur, uint8_t changed, uint8_t base){
	register uint8_t bit = 0; 
	while(changed){
		if((changed & 1) && _pcint_table[base + bit]){
			 pcint_config_t *conf = _pcint_table[base + bit];
			 conf->leading = cur & _BV(bit); 
			 conf->time = timer_get_clock(timer1); 
			 if(conf->handler) conf->handler(conf); 
		}
		changed >>= 1; 
		bit++;
	}
}

ISR(PCINT0_vect)
{
	static uint8_t _prev = 0;
	register uint8_t cur = PINB; 
	//PORTD |= _BV(7);
	_pcint(cur, cur ^ _prev, GPIO_PB0);
	_prev = cur;
	
	/*
	DDRD &= ~_BV(PD3); 
	PCICR |= (1<<PCIE2);  
	PCMSK2 |= (1 << PCINT19);*/
}

ISR(PCINT1_vect)
{
	static uint8_t _prev = 0;
	register uint8_t cur = PINC; 
	_pcint(cur, cur ^ _prev, GPIO_PC0);
	_prev = cur; 
}

ISR(PCINT2_vect)
{
	//DDRD |= _BV(5);
	//PORTD |= _BV(5); 
	static uint8_t _prev = 0;
	register uint8_t cur = PIND;
	_pcint(cur, cur ^ _prev, GPIO_PD0);
	_prev = cur; 
	//PORTD &= ~_BV(5);
}

int16_t gpio_ioctl(void *inst, uint8_t ioc, int32_t data){
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
			} else {
				return FAIL;
			}
			return SUCCESS; 
			break;
		}
		case IOC_GPIO_SET_HI:
		case IOC_GPIO_SET_LO: 
		case IOC_GPIO_SET_PULLUP:
		case IOC_GPIO_SET_HIQ: 
		{
			uint8_t pin = data & 0xff; 
			uint8_t mode = ((ioc == IOC_GPIO_SET_PULLUP) || (ioc == IOC_GPIO_SET_HI))?1:0; 
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
				pcint_config_t *conf = (pcint_config_t*)data;
				uint8_t pin = conf->pin; 
				
				if(pin > GPIO_COUNT || pin < 0) return FAIL;
				
				_pcint_table[conf->pin] = conf;
				
				if(pin >= GPIO_PB0 && pin <= GPIO_PB7){
					PCICR |= (1<<PCIE0);  
					PCMSK0 |= (1 << (pin - GPIO_PB0));
				} else if(pin >= GPIO_PC0 && pin <= GPIO_PC7){
					PCICR |= (1<<PCIE1);  
					PCMSK1 |= (1 << (pin - GPIO_PC0));
				} else if(pin >= GPIO_PD0 && pin <= GPIO_PD7){
					PCICR |= (1<<PCIE2);  
					PCMSK2 |= (1 << (pin - GPIO_PD0));
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
int16_t gpio_read(handle_t handle, uint8_t *buffer, uint16_t size){
	
}

int16_t gpio_write(handle_t handle, const uint8_t *buffer, uint16_t size){
	return SUCCESS; 
}

handle_t gpio_open(id_t id){
	dev_open(timer1, 0); 
	return DEFAULT_HANDLE; 
}

int16_t gpio_close(handle_t handle){
	return SUCCESS; 
}

static void gpio_tick(){
	
}

#include <util/delay.h>
void gpio_init(){
	pin_lock(GPIO_PB6);
	pin_lock(GPIO_PB7);
	memset(_pcint_table, 0, sizeof(_pcint_table)); 
}

DECLARE_DRIVER(gpio); 
