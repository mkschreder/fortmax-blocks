#pragma once

#define Y 1
#define N 0

// onboard peripherals
#define CONFIG_GPIO Y
#define CONFIG_ADC Y
#define CONFIG_TIMERS Y

// external device support
#define CONFIG_HCSR04 Y

#include "device.h"
#include "adc.h"
#include "timer.h"
#include "gpio.h"
#include "hcsr04.h"
#include "i2c.h"

// data structures for supported kernel functions
struct kernel_export_data {
	uint16_t adc[6];
	uint32_t distance[2];

	char *last_error; 
};

extern struct kernel_export_data kdata; 

DEVIDE_DECLARE(kernel); 
