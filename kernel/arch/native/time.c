/** 
 * 	Author: Martin K. Schröder 
 *  Date: 2014
 * 
 * 	info@fortmax.se
 */
#include <stdint.h>
#include "time.h"

// update this for correct amount of ticks in microsecon
#define TICKS_PER_US 2

/// this holds the number of overflows of timer 1 (which counts clock ticks)
static volatile uint32_t _timer1_ovf = 0;
static uint16_t TCNT1; 

void time_init(void){
	
}

void time_reset(void){
	
}

timeout_t time_us_to_clock(timeout_t us){
	return TICKS_PER_US * us; 
}

timeout_t time_clock_to_us(timeout_t clock){
	return clock / TICKS_PER_US; 
}

timeout_t time_get_clock(void){
	return TCNT1 + _timer1_ovf * 65535;
}

timeout_t time_clock_since(timeout_t clock){
	return time_get_clock() - clock; 
}
