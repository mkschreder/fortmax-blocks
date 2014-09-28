#pragma once

/** Macros that one should use to correctly deal with timeouts **/

#define IOC_TIMER_GET_TICKS 1
#define IOC_TIMER_GET_PRESCALER 2

typedef int32_t timeout_t;

#define time_before(unknown, known) ((timeout_t)(unknown) - (timeout_t)(known) < 0)
#define time_after(a,b) time_before(b, a)
#define timeout_from_now(timer, us) (timer_get_clock(timer) + timer_us_to_clock(timer, us))
#define timeout_expired(timer, timeout) (time_after(timer_get_clock(timer), timeout))

// returns current number of clock cycles (will overflow!)
timeout_t timer_get_clock(struct device *timer); 

// converts a value in microseconds to number of clock ticks
inline uint32_t timer_us_to_clock(struct device *timer, uint32_t us){
	uint8_t ps = 1; 
	timer->ioctl(IOC_TIMER_GET_PRESCALER, (long)&ps); 
	return (F_CPU / 1000000 * ps) * us; 
}
