#include "kernel.h"

struct kernel_export_data kdata;
static handle_t hcsr1, hcsr2;
static timeout_t timeout;

static struct async_op read1;
static struct async_op read2;
	
void _fetch_hcsr1(long arg){
	struct kernel_export_data *d = (struct kernel_export_data*)arg;
	dev_read(hcsr04, hcsr1, &kdata.distance[0], sizeof(kdata.distance[0])); 
	dev_write(hcsr04, hcsr1, &read1, sizeof(read1)); 
}

void _fetch_hcsr2(long arg){
	struct kernel_export_data *d = (struct kernel_export_data*)arg;
	dev_read(hcsr04, hcsr2, &kdata.distance[1], sizeof(kdata.distance[1])); 
	dev_write(hcsr04, hcsr2, &read2, sizeof(read2)); 
}


void kernel_init(){
	read1 = (struct async_op){
		.op = HCSR04_READ,
		.callback = &_fetch_hcsr1,
		.arg = (long)&kdata
	};
	read2 = (struct async_op){
		.op = HCSR04_READ,
		.callback = &_fetch_hcsr2,
		.arg = (long)&kdata
	};
	
}

int16_t kernel_ioctl(handle_t inst, uint8_t num, int32_t data){

}

int16_t kernel_write(handle_t inst, const uint8_t *buffer, uint16_t size){

}

int16_t kernel_read(handle_t inst, uint8_t *buffer, uint16_t size){

}

handle_t kernel_open(id_t id){
	hcsr1 = dev_open(hcsr04, 0);
	hcsr2 = dev_open(hcsr04, 1);
	
	if(!hcsr1 || !hcsr2){
		kdata.last_error = "NO_HC_DEV"; 
		//uart_printf("Could not allocate an HCSR device!\n");
		return INVALID_HANDLE; 
	}
	
	struct hcsr04_config conf = {
		.mode = HCSR_GPIO, 
		.trig = GPIO_PB0,
		.echo = GPIO_PB1
	};
	
	if(SUCCESS != dev_ioctl(hcsr04, hcsr1, IOC_HCSR_CONFIGURE, &conf)){
		kdata.last_error = "NO_HC1"; 
		//uart_printf("CANT SET UP HC1\n");
		return INVALID_HANDLE; 
	}
	
	conf.mode = HCSR_GPIO; 
	conf.trig = GPIO_PD6;
	conf.echo = GPIO_PD3;
	
	if(SUCCESS != dev_ioctl(hcsr04, hcsr2, IOC_HCSR_CONFIGURE, &conf)){
		kdata.last_error = "NO_HC2"; 
		//uart_printf("CANT SET UP HC2\n");
		return INVALID_HANDLE; 
	}
	int16_t ret = 0; 
	
	if(!dev_open(adc, 0)){
		kdata.last_error = "NO_ADC"; 
		//uart_printf("ADC FAILED init!\n");
		//while(1);
		return INVALID_HANDLE; 
	}
	if(!dev_open(timer1, 0)){
		kdata.last_error = "NO_TIMER"; 
		//uart_printf("TIMER FAILED init!\n");
		//while(1);
		return INVALID_HANDLE; 
	}
	
	if(!dev_open(gpio, 0)){
		kdata.last_error = "NO_GPIO";
		return INVALID_HANDLE;
	}
	
	if(!dev_open(i2c, 0)){
		//uart_printf("ERROR initializing i2c!\n");
		//while(1);
		return INVALID_HANDLE; 
	}
	
	dev_ioctl(adc, 0, IOC_ADC_START_CONV, 0);

	_fetch_hcsr1(&kdata);
	_fetch_hcsr2(&kdata);
	
/*
	if(SUCCESS != hcsr04_ioctl(hcsr1, IOC_HCSR_TRIGGER, 0))
		kdata.last_error = "NO_TRIG1"; 
	if(SUCCESS != hcsr04_ioctl(hcsr2, IOC_HCSR_TRIGGER, 0))
		kdata.last_error = "NO_TRIG2";
		*/
	timeout = timeout_from_now(timer1, 10000UL);

	return DEFAULT_HANDLE; 
}

int16_t kernel_close(handle_t inst){

}

void kernel_tick(){
	static int8_t chan = 0;

	kdata.last_error = "OK";
	
	if(timeout_expired(timer1, timeout)){
		uint16_t value = 0;
		//_delay_ms(500);
		
		//DDRB |= (1 << 0); 

		if(dev_read(adc, 0, (uint8_t*)&value, 1) == SUCCESS){
			kdata.adc[chan] = value;
			//uart_printf("ADC chan %d val: %d, dist: %d\n", chan, value, distance1);
			//uint16_t d1 = distance1, d2 = distance2; 
			//uart_printf("DIS: %d\t%d\n", d2, d1); 
			do {
				chan = (chan + 1) & 0x07;
				if(dev_ioctl(adc, 0, IOC_ADC_SET_CHANNEL, chan) != FAIL){
					break;
				} else {
					//uart_printf("ADC chan %d: not available!\n", chan);
				} 
			} while(1); 
		}

		timeout = timeout_from_now(timer1, 50000UL);
	}
}

DECLARE_DRIVER(kernel); 
