#include <avr/io.h>
#include <avr/interrupt.h>

#include "device.h"
#include "adc.h"
#include "gpio.h"

static uint8_t _in_use = 0; 
static volatile uint16_t _adc = 0;
static uint8_t _adc_current_pin = 0; 
static handle_t _gpio = 0;

#define ADC_CH0 GPIO_PC0

/*ADC Conversion Complete Interrupt Service Routine (ISR)*/
ISR(ADC_vect)
{
	_adc = ADCH;
	_adc |= 0x8000; //(ADCH << 8) | ADCL;			// Output ADCH to PortD
	ADCSRA |= 1<<ADSC;		// Start Conversion
}

void adc_init(){
	//_adc_port = PORTC; 
}

int16_t adc_write(handle_t h, const uint8_t *buffer, uint16_t size){
	return 0; 
}

int16_t adc_read(handle_t h, uint8_t *buffer, uint16_t size){
	//if(ADCSRA == (1<<ADSC)) return FAIL;
	if(!(_adc & 0x8000)) return FAIL;
	*((uint16_t*)buffer) = _adc & 0x7fff;
	_adc = 0;
	return SUCCESS; 
}

int16_t adc_ioctl(handle_t inst, uint8_t ioc, long data){
	switch(ioc){
		case IOC_ADC_READY:
			return _adc & 0x8000;
		case IOC_ADC_SET_CHANNEL:
		{
			uint8_t chan = data & 0xff;
			uint8_t pin = ADC_CH0+chan;

			// try to reserve the pin for our own use
			if(dev_ioctl(gpio, 0, IOC_GPIO_LOCK_PIN, pin) != SUCCESS)
				return FAIL;
			
			dev_ioctl(gpio, 0, IOC_GPIO_UNLOCK_PIN, _adc_current_pin);
			
			_adc_current_pin = pin;
			
			// set the pin as input
			dev_ioctl(gpio, 0, IOC_GPIO_SET_INPUT, pin);
			
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

handle_t adc_open(id_t id){
	if(_in_use) return FAIL;

	_gpio = dev_open(gpio, 0); 
	if(!_gpio) return FAIL; 

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
	
	return (handle_t)-1; 
}

int16_t adc_close(handle_t inst){
	_in_use = 0;
	return SUCCESS; 
}

void adc_tick(){

}

DECLARE_DRIVER(adc); 
