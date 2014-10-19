#include <kernel/timer.h>
#include <util/atomic.h>

/*
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif 

#include <time.h>

uint32_t getTick()
{
	struct timespec timespec_time;
	__uint32_t theTick = 0;

	clock_gettime(CLOCK_REALTIME, &timespec_time);

	theTick = timespec_time.tv_nsec / 1000;
	theTick += timespec_time.tv_sec * 1000000;
	return theTick; 
}


handle_t timer_open(id_t id){
	return DEFAULT_HANDLE; 
}

int16_t timer_close(handle_t h){
	return SUCCESS; 
}


timeout_t timer_get_clock(handle_t h){
	return getTick(); 
}

timeout_t timer_us_to_clock(handle_t h, uint32_t us){
	return us; 
}

uint32_t timer_clock_to_us(handle_t h, timeout_t ticks){
	return ticks; 
}
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#include <kernel/device.h>
#include <kernel/timer.h>

/// this holds the number of overflows of timer 1 (which counts clock ticks)
static volatile uint32_t _timer1_ovf = 0;

ISR (TIMER1_OVF_vect)
{
	_timer1_ovf++;
}

CONSTRUCTOR(timer_init){
	TCCR1A = 0;
	TCCR1B = _BV(CS10);
	TIMSK1 |= _BV(TOIE0);
}

handle_t timer_open(id_t id){
	_timer1_ovf = 0;
	TCNT1 = 0; 
	return DEFAULT_HANDLE; 
}

int16_t timer_close(handle_t h){
	TCCR1A = 0; 
	TCCR1B = 0;
	TIMSK1 = 0;
	//_timer1_in_use = 0; 
	return SUCCESS; 
}


timeout_t timer_get_clock(handle_t h){
	timeout_t clock; 
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		clock = TCNT1 + _timer1_ovf * 65535;
	}
	return clock; 
}

timeout_t timer_us_to_clock(handle_t h, uint32_t us){
	uint8_t ps = 1; 
	return (F_CPU / 1000000 / ps) * us; 
}

uint32_t timer_clock_to_us(handle_t h, timeout_t ticks){
	uint8_t ps = 1; 
	return ticks / (F_CPU / 1000000 / ps); 
}

//DECLARE_DRIVER(timer);

