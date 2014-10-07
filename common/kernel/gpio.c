#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include <kernel/device.h>
#include <kernel/gpio.h>
#include <kernel/timer.h>

static uint8_t _locked_pins[GPIO_COUNT / 8];
static struct pcint_info _pcint_table[GPIO_COUNT];
static handle_t timer1 = 0;

#define SR_BIT(reg, bit, val) { if(val) reg |= (1 << (bit)); else reg &= ~(1 << (bit));}

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
		if((changed & 1) && _pcint_table[base + bit].flags.enabled){
			 pcint_config_t *conf = &_pcint_table[base + bit];
			 conf->flags.leading = cur & _BV(bit); 
			 conf->time = timer_get_clock(timer1); 
			 if(conf->handler) conf->handler(conf, conf->arg); 
		}
		changed >>= 1; 
		bit++;
	}
}

ISR(PCINT0_vect)
{
	static uint8_t _prev = 0;
	register uint8_t cur = PINB; 
	_pcint(cur, cur ^ _prev, GPIO_PB0);
	_prev = cur;
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
	static uint8_t _prev = 0;
	register uint8_t cur = PIND;
	_pcint(cur, cur ^ _prev, GPIO_PD0);
	_prev = cur; 
}

int8_t gpio_configure(handle_t dev, gpio_pin_t pin, gpio_flags_t flags){
	uint8_t out = flags.output; 
	if(pin >= GPIO_PB0 && pin <= GPIO_PB7){
		SR_BIT(DDRB, pin - GPIO_PB0, out); 
	} else if(pin >= GPIO_PC0 && pin <= GPIO_PC7){
		SR_BIT(DDRC, pin - GPIO_PC0, out); 
	} else if(pin >= GPIO_PD0 && pin <= GPIO_PD7){
		SR_BIT(DDRD, pin - GPIO_PD0, out); 
	} else {
		return FAIL;
	}
	uint8_t mode = flags.pullup; 
	gpio_set(dev, pin, mode);
	
	return SUCCESS; 
}

int8_t gpio_enable_pcint(handle_t dev, gpio_pin_t pin, void (*handler)(struct pcint_info *info, void *arg), void *arg){
	pcint_config_t *conf = &_pcint_table[pin];
	
	if(pin > GPIO_COUNT || pin < 0) return FAIL;

	conf->pin = pin;
	conf->handler = handler;
	conf->arg = arg;
	conf->flags.enabled = 1;
	
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

int8_t gpio_disable_pcint(handle_t dev, gpio_pin_t pin){
	if(pin > GPIO_COUNT || pin < 0) return FAIL;
		_pcint_table[pin].flags.enabled = 0;
	return SUCCESS; 
}

int8_t gpio_set(handle_t dev, gpio_pin_t pin, uint8_t value){
	if(pin >= GPIO_PB0 && pin <= GPIO_PB7){
		SR_BIT(PORTB, pin - GPIO_PB0, value); 
	} else if(pin >= GPIO_PC0 && pin <= GPIO_PC7){
		SR_BIT(PORTC, pin - GPIO_PC0, value); 
	} else if(pin >= GPIO_PD0 && pin <= GPIO_PD7){
		SR_BIT(PORTD, pin - GPIO_PD0, value); 
	} else {
		return FAIL;
	}
	return SUCCESS;
}

int8_t gpio_get(handle_t dev, gpio_pin_t pin){
	if(pin >= GPIO_PB0 && pin <= GPIO_PB7){
		return PINB & _BV(pin - GPIO_PB0); 
	} else if(pin >= GPIO_PC0 && pin <= GPIO_PC7){
		return PINC & _BV(pin - GPIO_PC0);  
	} else if(pin >= GPIO_PD0 && pin <= GPIO_PD7){
		return PIND & _BV(pin - GPIO_PD0); 
	} 
	return 0; 
}

int8_t gpio_register(handle_t dev, gpio_pin_t pin){
	if(pin_is_locked(pin))
		return FAIL;
		
	pin_lock(pin); 
	return SUCCESS;
}

int8_t gpio_release(handle_t dev, gpio_pin_t pin){
	pin_unlock(pin);
	return SUCCESS; 
}

handle_t gpio_open(id_t id){
	timer1 = timer_open(1);
	return DEFAULT_HANDLE; 
}
int16_t gpio_read(handle_t h, uint8_t *buffer, uint16_t size){
	//bmp085_t *bmp = (bmp085_t*) buffer;
	return SUCCESS; 
}

int16_t gpio_write(handle_t h, const uint8_t *buffer, uint16_t size){
	return SUCCESS; 
}

int16_t gpio_close(handle_t handle){
	return SUCCESS; 
}

int16_t gpio_ioctl(handle_t h, uint8_t ioc, int32_t data){
	return SUCCESS; 
}

CONSTRUCTOR(gpio_init){
	pin_lock(GPIO_PB6);
	pin_lock(GPIO_PB7);
	memset(_pcint_table, 0, sizeof(_pcint_table)); 
}
