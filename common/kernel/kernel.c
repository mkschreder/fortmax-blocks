#include "kernel.h"
#include "kvar.h"

struct kernel_export_data kdata;
static handle_t hcsr1, hcsr2;
static handle_t adc, timer1, i2c, gpio; 
static timeout_t timeout;

void _hcsr1_complete(void *arg){
	kdata.distance[0] = hcsr04_get_distance(hcsr1); 
	hcsr04_trigger(hcsr1, _hcsr1_complete, 0);
}

void _hcsr2_complete(void *arg){
	kdata.distance[1] = hcsr04_get_distance(hcsr2); 
	hcsr04_trigger(hcsr2, _hcsr2_complete, 0); 
}

void kernel_init(void){
	
}

int16_t kernel_ioctl(handle_t inst, uint8_t num, int32_t data){
	return FAIL; 
}

int16_t kernel_write(handle_t inst, const uint8_t *buffer, uint16_t size){
	return FAIL; 
}

int16_t kernel_read(handle_t inst, uint8_t *buffer, uint16_t size){
	return FAIL; 
}

void _adc_measure(void *p){
	
}

void _adc_completed(void *p){
	static uint8_t chan = 0; 
	//_delay_ms(500);
	
	//DDRB |= (1 << 0); 
	kdata.adc[chan] = adc_read(chan);
	
	//uart_printf("ADC chan %d val: %d, dist: %d\n", chan, value, distance1);
	//uint16_t d1 = distance1, d2 = distance2; 
	//uart_printf("DIS: %d\t%d\n", d2, d1); 
	do {
		chan = (chan + 1) & 0x07;
		if(adc_set_channel(adc, chan) != FAIL){
			break;
		} else {
			//uart_printf("ADC chan %d: not available!\n", chan);
		} 
	} while(1);
	
	timeout = timeout_from_now(timer1, 50000UL);
}

handle_t kernel_open(id_t id){
	hcsr1 = hcsr04_open(0);
	hcsr2 = hcsr04_open(1);

	if(!hcsr1 || !hcsr2 ||
		FAIL == hcsr04_configure(hcsr1, GPIO_PB0, GPIO_PB1) || 
		FAIL == hcsr04_configure(hcsr2, GPIO_PD6, GPIO_PD3)){
		kdata.last_error = "NO_HC1/2"; 
		return FAIL;
	}

	hcsr04_trigger(hcsr1, _hcsr1_complete, 0);
	hcsr04_trigger(hcsr2, _hcsr2_complete, 0);

	adc = adc_open(0);
	timer1 = timer_open(1);
	gpio = gpio_open(0);
	i2c = gpio_open(0);
	
	if(!adc){
		kdata.last_error = "NO_ADC"; 
		return INVALID_HANDLE; 
	}
	if(!timer1){
		kdata.last_error = "NO_TIMER"; 
		return INVALID_HANDLE; 
	}
	
	if(!gpio){
		kdata.last_error = "NO_GPIO";
		return INVALID_HANDLE;
	}
	
	if(!i2c){
		kdata.last_error = "NO I2C"; 
		return INVALID_HANDLE; 
	}

	adc_measure(0, _adc_completed, 0);

	timeout = timeout_from_now(timer1, 10000UL);

	return DEFAULT_HANDLE; 
}

int16_t kernel_close(handle_t inst){
	return SUCCESS; 
}

void kernel_tick(void){
	static int8_t chan = 0;

	kdata.last_error = "OK";
	
	if(timeout && timeout_expired(timer1, timeout)){
		adc_measure(adc, _adc_completed, 0);
		timeout = 0; 
	}
}

DECLARE_DRIVER(kernel); 
