#include <kernel/kernel.h>
#include <kernel/kvar.h>

struct kernel_export_data kdata;
static handle_t hcsr1, hcsr2;
static handle_t adc, timer1, i2c, gpio; 

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

void _adc_completed(void *arg); 

void _do_measure(void *arg){
	handle_t adc = (handle_t)arg;
	adc_measure(adc, _adc_completed, 0); 
}

void _adc_completed(void *arg){
	static uint8_t chan = 0;
	handle_t adc = (handle_t)arg;
	
	kdata.adc[chan] = adc_read(adc);

	chan = (chan + 1) & 0x07;
	
	adc_set_channel(adc, chan);
	
	async_schedule(_do_measure, adc, 100000); 
}

handle_t kernel_open(id_t id){
	hcsr1 = hcsr04_open(0);
	hcsr2 = hcsr04_open(1);

	if(!hcsr1 || !hcsr2 ||
		FAIL == hcsr04_configure(hcsr1, GPIO_PB0, GPIO_PB1) || 
		FAIL == hcsr04_configure(hcsr2, GPIO_PD6, GPIO_PD3)){
		kdata.last_error = "NO_HC1/2"; 
		return INVALID_HANDLE;
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
	
	return DEFAULT_HANDLE; 
}

int16_t kernel_close(handle_t inst){
	return SUCCESS; 
}

void kernel_tick(void){
	kdata.last_error = "OK";
}

DECLARE_DRIVER(kernel); 
