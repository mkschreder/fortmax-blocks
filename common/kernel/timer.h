#pragma once

/** Macros that one should use to correctly deal with timeouts **/

#define IOC_TIMER_GET_TICKS 1
#define IOC_TIMER_GET_PRESCALER 2

typedef int32_t timeout_t;

#define time_before(unknown, known) ((timeout_t)(unknown) - (timeout_t)(known) < 0)
#define time_after(a,b) time_before(b, a)
#define timeout_from_now(timer, us) (timer_get_clock(timer) + timer_us_to_clock(timer, us))
#define timeout_expired(timer, timeout) (time_after(timer_get_clock(timer), timeout))
#define delay_us(timer, del) {timeout_t t = timeout_from_now(timer, del); while(!timeout_expired(timer, t));}
// returns current number of clock cycles (will overflow!)
//timeout_t timer_get_clock(struct device *timer); 

inline timeout_t _timer_get_clock(int16_t (*ioctl)(void *inst, uint8_t ioc, int32_t data)){
	timeout_t clock;
	ioctl(0, IOC_TIMER_GET_TICKS, (int16_t)&clock); 
	return clock;
}

// converts a value in microseconds to number of clock ticks
inline uint32_t _timer_us_to_clock(int16_t (*ioctl)(void *inst, uint8_t ioc, int32_t data), uint32_t us){
	uint8_t ps = 1; 
	ioctl(0, IOC_TIMER_GET_PRESCALER, (int16_t)&ps); 
	return (F_CPU / 1000000 / ps) * us; 
}

inline uint32_t _timer_clock_to_us(int16_t (*ioctl)(void *inst, uint8_t ioc, int32_t data), uint32_t ticks){
	uint8_t ps = 1; 
	ioctl(0, IOC_TIMER_GET_PRESCALER, (int16_t)&ps); 
	return ticks / (F_CPU / 1000000 / ps); 
}

#define timer_get_clock(timer) (_timer_get_clock(&timer##_ioctl))
#define timer_us_to_clock(timer, us) (_timer_us_to_clock(&timer##_ioctl, us))
#define timer_clock_to_us(timer, ticks) (_timer_clock_to_us(&timer##_ioctl, ticks))

DEVICE_DECLARE(timer1); 
