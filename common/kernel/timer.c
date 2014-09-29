#include <avr/io.h>
#include <avr/interrupt.h>

#include "device.h"
#include "timer.h"

/// this holds the number of overflows of timer 1 (which counts clock ticks)
static volatile uint32_t _timer1_ovf = 0;
static uint8_t _timer1_in_use = 0;


ISR (TIMER1_OVF_vect)
{
	_timer1_ovf++;
}

void timer1_init(){
	TCCR1A = 0;
	TCCR1B = _BV(CS11);
	TIMSK1 |= _BV(TOIE0);
}

int16_t timer1_write(handle_t h, const uint8_t *buffer, uint16_t size){
	return 0; 
}

int16_t timer1_read(handle_t h, uint8_t *buffer, uint16_t size){
	return 0; 
}

handle_t timer1_open(id_t id){
	// only allow one instance
	//if(_timer1_in_use) return FAIL;
	
	//_timer1_in_use = 1;
	
	_timer1_ovf = 0;
	TCNT1 = 0; 
	return DEFAULT_HANDLE; 
}

int16_t timer1_close(handle_t h){
	TCCR1A = 0; 
	TCCR1B = 0;
	TIMSK1 = 0;
	//_timer1_in_use = 0; 
	return SUCCESS; 
}

void timer1_tick(){

}

int16_t timer1_ioctl(handle_t h, uint8_t ioc, long data){
	switch(ioc){
		case IOC_TIMER_GET_TICKS:
			*((timeout_t*)data) = TCNT1 + _timer1_ovf * 65535;
			return SUCCESS;
		case IOC_TIMER_GET_PRESCALER:
			*((uint8_t*)data) = 8; 
			return SUCCESS; 
	}
	return FAIL; 
}

DECLARE_DRIVER(timer1);

