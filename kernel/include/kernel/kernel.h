#pragma once

#define Y 1
#define N 0

// onboard peripherals
#define CONFIG_GPIO Y
#define CONFIG_ADC Y
#define CONFIG_TIMERS Y

// external device support
#define CONFIG_HCSR04 Y

#include <kernel/object.h>
#include <kernel/async.h>
#include <kernel/kvar.h>
#include <kernel/device.h>
#include <kernel/adc.h>
#include <kernel/timer.h>
#include <kernel/gpio.h>
#include <kernel/hcsr04.h>
#include <kernel/i2c.h>

struct kernel_export {
	struct {
		int16_t x, y, z;
	} gyro; 
};

extern struct kernel_export sys;

handle_t kernel_open(id_t id);

//DEVICE_DECLARE(kernel); 

