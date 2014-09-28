#include <avr/io.h>
#include <avr/interrupt.h>

#include "device.hpp"
#include "time.hpp"

/// this holds the number of overflows of timer 1 (which counts clock ticks)
static volatile uint32_t _timer1_ovf = 0;
static uint8_t _timer1_in_use = 0;

timeout_t timer_get_clock(struct device *timer){
	timeout_t clock;
	if(timer) timer->ioctl(IOC_TIMER_GET_TICKS, (long)&clock); 
	return clock; 
}

ISR (TIMER1_OVF_vect)
{
	_timer1_ovf++;
}

void timer1_init(){
	
}

int16_t timer1_write(const uint8_t *buffer, uint16_t size){
	return 0; 
}

int16_t timer1_read(uint8_t *buffer, uint16_t size){
	return 0; 
}

int16_t timer1_open(){
	// only allow one instance
	if(_timer1_in_use) return FAIL;
	
	_timer1_in_use = 1;
	TCCR1A = 0;
	TCCR1B = _BV(CS10);
	TIMSK1 |= _BV(TOIE0);
	_timer1_ovf = 0;
	TCNT1 = 0; 
	return SUCCESS; 
}

int16_t timer1_close(){
	TCCR1A = 0; 
	TCCR1B = 0;
	TIMSK1 = 0;
	_timer1_in_use = 0; 
	return SUCCESS; 
}

int16_t timer1_ioctl(uint8_t ioc, long data){
	switch(ioc){
		case IOC_TIMER_GET_TICKS:
			*((timeout_t*)data) = TCNT1 + _timer1_ovf * 65535;
			return SUCCESS;
		case IOC_TIMER_GET_PRESCALER:
			*((uint8_t*)data) = 1; 
			return SUCCESS; 
	}
	return 0; 
}

static struct device timer1 {
	.name = "timer1_hp_timer", 
	.init = &timer1_init,
	.ioctl = &timer1_ioctl, 
	.write = &timer1_write,
	.read = &timer1_read,
	.open = &timer1_open,
	.close = &timer1_close,
	.tick = 0
};

DRIVER(timer1); 
