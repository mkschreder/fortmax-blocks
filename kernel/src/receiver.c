#include <kernel/receiver.h>
#include <kernel/gpio.h>

static handle_t gpio = 0;
static handle_t timer1 = 0;

#define RC_NUM_CHANNELS 6

static uint8_t rc_pin[RC_NUM_CHANNELS] = {
	GPIO_PD0,
	GPIO_PD1,
	GPIO_PD2,
	GPIO_PD3,
	GPIO_PD4,
	GPIO_PD5
};

struct rc_channel {
	timeout_t pulse_start_time; 
	uint16_t value;
};

struct rc_device {
	struct rc_channel channels[RC_NUM_CHANNELS];
}; 

static struct rc_device rc_receiver[1];

static void interrupt(pcint_config_t *conf, void *ptr){
	struct rc_channel *data = (struct rc_channel*) ptr;

	if(conf->flags.leading){
		data->pulse_start_time = conf->time;
	} else {
		data->value = timer_clock_to_us(timer1, conf->time - data->pulse_start_time);
	}
}

handle_t rc_open(id_t id){
	struct rc_device *dev = &rc_receiver[0]; 
	for(int c = 0; c < RC_NUM_CHANNELS; c++){
		if(FAIL == gpio_configure(gpio, rc_pin[c], (gpio_flags_t){
			.output = 0,
			.pullup = 1
		})){
			return INVALID_HANDLE;
		}
		gpio_enable_pcint(gpio, rc_pin[c], &interrupt, &dev->channels[c]);
	}
	return &rc_receiver[0]; 
}

void rc_close(handle_t h){
	
}

uint16_t rc_get_channel(handle_t h, uint8_t chan){
	struct rc_device *rc = (struct rc_device*)h;
	return rc->channels[chan].value; 
}

CONSTRUCTOR(rc_init){
	timer1 = timer_open(1);
	gpio = gpio_open(0); 
}
