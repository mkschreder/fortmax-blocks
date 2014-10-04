#pragma once

/** Macros that one should use to correctly deal with timeouts **/
#include <kernel/types.h>

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

timeout_t timer_get_clock(handle_t h);
timeout_t timer_us_to_clock(handle_t h, uint32_t us);
uint32_t timer_clock_to_us(handle_t h, timeout_t clock);

handle_t timer_open(id_t id);
int16_t timer_close(handle_t h);

//DEVICE_DECLARE(timer1); 
