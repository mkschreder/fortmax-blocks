#include <avr/io.h>
#include <avr/interrupt.h>

#include "device.hpp"
#include "adc.hpp"
#include "gpio.hpp"

static uint8_t _in_use = 0; 
static volatile uint16_t _adc = 0;
static struct device *_gpio = 0;
static uint8_t _adc_current_pin = 0; 

#define ADC_CH0 GPIO_PC0

/*ADC Conversion Complete Interrupt Service Routine (ISR)*/
ISR(ADC_vect)
{
	_adc = ADCH;
	_adc |= 0x8000; //(ADCH << 8) | ADCL;			// Output ADCH to PortD
	ADCSRA |= 1<<ADSC;		// Start Conversion
}

static void adc_init(){
	//_adc_port = PORTC; 
}

static int16_t adc_write(const uint8_t *buffer, uint16_t size){
	return 0; 
}

static int16_t adc_read(uint8_t *buffer, uint16_t size){
	//if(ADCSRA == (1<<ADSC)) return FAIL;
	if(!(_adc & 0x8000)) return FAIL;
	*((uint16_t*)buffer) = _adc & 0x7fff;
	_adc = 0;
	return SUCCESS; 
}

static int16_t adc_ioctl(uint8_t ioc, long data){
	switch(ioc){
		case IOC_ADC_READY:
			return _adc & 0x8000;
		case IOC_ADC_SET_CHANNEL:
		{
			uint8_t chan = data & 0xff;
			uint8_t pin = ADC_CH0+chan;

			// try to reserve the pin for our own use
			if(_gpio->ioctl(IOC_GPIO_LOCK_PIN, pin) != SUCCESS)
				return FAIL;
			
			_gpio->ioctl(IOC_GPIO_UNLOCK_PIN, _adc_current_pin);
			
			_adc_current_pin = pin;
			
			// set the pin as input
			_gpio->ioctl(IOC_GPIO_SET_INPUT, pin);
			
			ADMUX = (ADMUX & 0xf0) | chan;
			ADCSRA |= 1<<ADSC;

			return SUCCESS; 
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

static int16_t adc_open(){
	if(_in_use) return FAIL;

	if(!_gpio) _gpio = device("gpio");
	if(!_gpio) return FAIL;

	adc_ioctl(IOC_ADC_SET_CHANNEL, 0); 
	
	ADCSRA = (1 << ADEN) | (1 << ADIE) |
		(0 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
		
	ADMUX = (0 << REFS1) | (1 << REFS0) | (1 << ADLAR) |
		(0 << MUX0) | (0 << MUX1) | (0 << MUX2) | (0 << MUX3);

	DIDR0 = 0xff;
	
	ADCSRA |= 1<<ADATE;
	ADCSRA |= 1<<ADSC;		// Start Conversion
	ADCSRB = 0;

	_in_use = 1;
	
	return SUCCESS; 
}

static int16_t adc_close(){
	_in_use = 0;
	return SUCCESS; 
}

static struct device adc {
	.name = "adc", 
	.init = &adc_init,
	.ioctl = &adc_ioctl, 
	.write = &adc_write,
	.read = &adc_read,
	.open = &adc_open,
	.close = &adc_close,
	.tick = 0
};

DRIVER(adc);
