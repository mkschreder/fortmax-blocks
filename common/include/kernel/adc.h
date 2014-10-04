#pragma once

#include <kernel/object.h>

#define IOC_ADC_SET_CHANNEL 1
#define IOC_ADC_SET_PRECISION 2
#define IOC_ADC_START_CONV 3
#define IOC_ADC_READY 4

handle_t adc_open(id_t id);
int8_t adc_measure(handle_t dev, void (*callback)(void *arg), void *arg); 
uint16_t adc_read(handle_t dev);

//DEVICE_DECLARE(adc); 
