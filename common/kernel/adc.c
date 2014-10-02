#include <avr/io.h>
#include <avr/interrupt.h>

#include "device.h"
#include "adc.h"
#include "gpio.h"
#include "kvar.h"
#include "async.h"

static uint8_t _in_use = 0; 
static volatile uint16_t _adc = 0;
static volatile uint8_t _completed; 
static uint8_t _adc_current_pin = 0; 
static handle_t _gpio = 0;

#define ADC_CH0 GPIO_PC0

/*ADC Conversion Complete Interrupt Service Routine (ISR)*/
ISR(ADC_vect)
{
	_adc = ADCH;
	_adc |= 0x8000; //(ADCH << 8) | ADCL;			// Output ADCH to PortD
	ADCSRA |= 1<<ADSC;		// Start Conversion
	_completed = 1; 
}

void adc_init(void){
	//_adc_port = PORTC; 
}


uint16_t adc_read(handle_t h){
	uint8_t sreg = SREG;
	cli(); 
	uint16_t val = _adc & 0x7fff;
	SREG = sreg;
	return val; 
}

int8_t adc_set_channel(handle_t dev, uint8_t chan){
	uint8_t pin = ADC_CH0+chan;

	// try to reserve the pin for our own use

	if(gpio_register(_gpio, pin) != SUCCESS)
		return FAIL;

	gpio_release(_gpio, _adc_current_pin);

	_adc_current_pin = pin;

	// set the pin as input
	gpio_configure(_gpio, pin, (gpio_flags_t){.output = 0, .pullup = 0}); 

	ADMUX = (ADMUX & 0xf0) | chan;
	ADCSRA |= 1<<ADSC;

	return SUCCESS; 
}

int8_t adc_measure(handle_t dev, void (*cb)(void*), void *arg){
	_completed = 0; 
	ADCSRA |= (1<<ADSC);
	async_wait(cb, 0, arg, &_completed, 100000);
	return 1; 
}

int16_t adc_ioctl(handle_t inst, uint8_t ioc, long data){
	switch(ioc){
		case IOC_ADC_READY:
			return _adc & 0x8000;
		case IOC_ADC_SET_CHANNEL:
		{
			
			break;
		}
		case IOC_ADC_START_CONV:
			ADCSRA |= (1<<ADSC);
			return SUCCESS; 
			break; 
		default:
			return FAIL;
	};
	return SUCCESS; 
}

handle_t adc_open(id_t id){
	if(_in_use) return INVALID_HANDLE;

	_gpio = gpio_open(0); 
	if(!_gpio) return INVALID_HANDLE; 

	dev_ioctl(adc, 0, IOC_ADC_SET_CHANNEL, 0); 
	
	ADCSRA = (1 << ADEN) | (1 << ADIE) |
		(0 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
		
	ADMUX = (0 << REFS1) | (1 << REFS0) | (1 << ADLAR) |
		(0 << MUX0) | (0 << MUX1) | (0 << MUX2) | (0 << MUX3);

	DIDR0 = 0xff;
	
	ADCSRA |= 1<<ADATE;
	ADCSRA |= 1<<ADSC;		// Start Conversion
	ADCSRB = 0;

	_in_use = 1;
	
	return DEFAULT_HANDLE; 
}

int16_t adc_close(handle_t inst){
	_in_use = 0;
	return SUCCESS; 
}

void adc_tick(void){

}

DECLARE_DRIVER(adc); 
