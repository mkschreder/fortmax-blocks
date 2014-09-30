#include <avr/io.h>
#include <util/delay.h>

#include <string.h>

#include "device.h"
#include "hcsr04.h"
#include "gpio.h"
#include "timer.h"

typedef struct hcsr04 {
	int8_t mode; 
	int8_t trig;
	int8_t echo;
	
	uint32_t distance;
	uint32_t echo_timeout;
	uint32_t echo_up_time; 
	uint32_t echo_down_time;

	uint8_t state;
	
	struct {
		uint8_t in_use : 1;
	} flags; 
		
	pcint_config_t interrupt;
	timeout_t trigger_ticks; 
	struct async_op _current_op;
	
} hcsr04_t;

static hcsr04_t _sensors[HCSR04_MAX];
static handle_t _gpio;
static handle_t _timer;

enum hcsr_state {
	STATE_IDLE, 
	STATE_TRIGGERING,
	STATE_TRIGGERED,
	STATE_COUNT
};

enum hcsr_event {
	EV_TICK,
	EV_ECHO_UP,
	EV_ECHO_DOWN,
	EV_TRIGGER,
	EV_COUNT
};

static void (*const state_table[STATE_COUNT][EV_COUNT])(hcsr04_t *hc);

static void _hcsr_idle(hcsr04_t *hc){
	if(hc->echo_timeout && timeout_expired(timer1, hc->echo_timeout)){
		// simulate an echo using the ordinary state machine path (cleaner) 
		state_table[hc->state][EV_ECHO_UP](hc);
		state_table[hc->state][EV_ECHO_DOWN](hc); 
		hc->distance = 4999;
	}
}

static void _hcsr_echo_up(hcsr04_t *hc){
	hc->echo_timeout = 0; 
	hc->echo_up_time = timer_get_clock(timer1);
}

static void _hcsr_echo_down(hcsr04_t *hc){
	hc->distance = timer_clock_to_us(timer1, timer_get_clock(timer1) - hc->echo_up_time);
	hc->distance = hc->distance;
	hc->state = STATE_IDLE;
	hc->_current_op.flags.busy = 0; 
}

static void _hcsr_trigger(hcsr04_t *hc){
	dev_ioctl(gpio, 0, IOC_GPIO_SET_OUTPUT, hc->trig); 
	dev_ioctl(gpio, 0, IOC_GPIO_SET_HI, hc->trig);
	hc->trigger_ticks = timer_get_clock(timer1);
	hc->echo_timeout = timeout_from_now(timer1, 100000UL);
	hc->state = STATE_TRIGGERED;
	dev_ioctl(gpio, 0, IOC_GPIO_SET_LO, hc->trig); 
}

static void _hcsr_idle_tick(hcsr04_t *hc){
	struct async_op *op = &hc->_current_op; 
	if(!op->flags.busy && !op->flags.cb_called){
		op->flags.cb_called = 1;
		if(op->callback)
			op->callback(op->arg); 
	}
}

static void (*const state_table[STATE_COUNT][EV_COUNT])(hcsr04_t *hc) = {
	// tick 					echo up				echo down				trigger
	{ _hcsr_idle_tick,_hcsr_idle, 	_hcsr_idle, 	_hcsr_trigger}, // idle
	{ _hcsr_idle, 	_hcsr_idle, 		_hcsr_idle,			_hcsr_idle},		// triggering
	{ _hcsr_idle, 	_hcsr_echo_up, 	_hcsr_echo_down, _hcsr_idle}		// triggered
};

static void interrupt(pcint_config_t *conf){
	hcsr04_t *hc = (hcsr04_t*)conf->arg;
	if(!hc) return;

	if(hc->interrupt.leading)
		state_table[hc->state][EV_ECHO_UP](hc);
	else {
		state_table[hc->state][EV_ECHO_DOWN](hc);
	}
}

static int16_t _configure(hcsr04_t *hc, struct hcsr04_config *conf){
	if(!hc) return FAIL;

	int16_t ret = SUCCESS;

	if(conf->mode == HCSR_GPIO){
		// TODO: need to add gpio TRYLOCK_PIN or IS_LOCKED
		
		if(SUCCESS != dev_ioctl(gpio, _gpio, IOC_GPIO_LOCK_PIN, conf->trig))
			goto no_trigger;
		dev_ioctl(gpio, _gpio, IOC_GPIO_SET_OUTPUT, conf->trig);
		
		if(SUCCESS != dev_ioctl(gpio, _gpio, IOC_GPIO_LOCK_PIN, conf->echo))
			goto no_echo; 
		dev_ioctl(gpio, _gpio, IOC_GPIO_SET_INPUT, conf->echo); 
		dev_ioctl(gpio, _gpio, IOC_GPIO_SET_PULLUP, conf->echo);
		
		hc->mode = conf->mode; 
		hc->trig = conf->trig;
		hc->echo = conf->echo; 
		hc->interrupt.pin = hc->echo,
		hc->interrupt.handler = &interrupt;
		hc->interrupt.arg = hc;
		
		if(SUCCESS != dev_ioctl(gpio, _gpio, IOC_GPIO_ENABLE_PCINT, &hc->interrupt))
			goto no_pcint;
			
		hc->state = STATE_IDLE;
		goto success; 
no_trigger:
	ret = E_HCSR_TRIGGER_NA;
	goto fail; 
no_echo:
	ret = E_HCSR_ECHO_NA;
	goto fail;
no_pcint:
	ret = E_HCSR_PCINT_NA;
	goto fail; 
fail:
	return ret;
	} else {
		return FAIL;
	}
success:
	return SUCCESS; 
};


void hcsr04_init() {
	for(int c = 0; c < HCSR04_MAX; c++){
		_sensors[c].flags.in_use = 0;
	}
	
	_gpio = dev_open(gpio, 0);
	_timer = dev_open(timer1, 0);
}

handle_t hcsr04_open(id_t id){
	if(id < 0 || id >= HCSR04_MAX) return INVALID_HANDLE;
	if(_sensors[id].flags.in_use) return INVALID_HANDLE;

	hcsr04_t *s = &_sensors[id];
	memset(s, 0, sizeof(s));

	s->distance = 4998; 
	s->state = STATE_IDLE;
	s->flags.in_use = 1;

	s->_current_op = (struct async_op){
		.flags = 0
	}; 
	return s; 
}

int16_t hcsr04_close(void* arg){
	hcsr04_t *hc = (hcsr04_t*)arg;

	if(!hc) return FAIL;
	dev_ioctl(gpio, 0, IOC_GPIO_UNLOCK_PIN, hc->trig);
	dev_ioctl(gpio, 0, IOC_GPIO_UNLOCK_PIN, hc->echo);
	hc->flags.in_use = 0; 
}

int16_t hcsr04_ioctl(handle_t arg, uint8_t ioc, int32_t param){
	hcsr04_t *hc = (hcsr04_t*)arg;
	if(!hc) return FAIL;

	switch(ioc){
		case IOC_HCSR_CONFIGURE:
			if(!param) return FAIL;
			_configure(hc, (struct hcsr04_config*)param);
			break; 
		default:
			return FAIL; 
	}
	
	return SUCCESS; 
}

int16_t hcsr04_read(handle_t arg, uint8_t *buf, uint16_t size){
	hcsr04_t *hc = (hcsr04_t*)arg;
	if(!hc) return FAIL;
	(*(uint32_t*)buf) = hc->distance;
	return SUCCESS; 
}

int16_t hcsr04_write(handle_t arg, const uint8_t *buf, uint16_t size){
	if(!buf || size != sizeof(struct async_op)) return FAIL;
	hcsr04_t *hc = (hcsr04_t*)arg;

	if(hc->_current_op.flags.busy) return EBUSY;

	hc->_current_op = *((struct async_op*)buf);
	hc->_current_op.flags.busy = 1; 
	hc->_current_op.flags.cb_called = 0;
	
	state_table[STATE_IDLE][EV_TRIGGER](hc);
	
	return SUCCESS; 
}

void hcsr04_tick(){
	for(int c = 0; c < HCSR04_MAX; c++){
		hcsr04_t *hc = (hcsr04_t*)&_sensors[c];
		
		if(hc->flags.in_use){
			state_table[hc->state][EV_TICK](hc);
		}
	}
}

DECLARE_DRIVER(hcsr04); 

//INIT_DRIVER(hcsr04); 
