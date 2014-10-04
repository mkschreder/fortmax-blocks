#include <avr/io.h>
#include <util/delay.h>

#include <string.h>

#include <kernel/device.h>
#include <kernel/hcsr04.h>
#include <kernel/gpio.h>
#include <kernel/timer.h>

typedef struct hcsr04 {
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

	volatile uint8_t completed; 
	
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
	
}

static void _hcsr_echo_up(hcsr04_t *hc){
	hc->echo_timeout = 0; 
	hc->echo_up_time = timer_get_clock(_timer);
	hc->echo_down_time = hc->echo_up_time + 1; 
}

static void _hcsr_echo_down(hcsr04_t *hc){
	hc->distance = timer_clock_to_us(_timer, hc->echo_down_time - hc->echo_up_time);
	hc->state = STATE_IDLE;
	hc->completed = 1; 
}

static void _hcsr_trigger(hcsr04_t *hc){
	gpio_set(_gpio, hc->trig, 1);
	hc->echo_timeout = timeout_from_now(_timer, 100000UL);
	hc->state = STATE_TRIGGERED;
	gpio_set(_gpio, hc->trig, 0); 
	//dev_ioctl(gpio, 0, IOC_GPIO_SET_LO, hc->trig); 
}

static void _hcsr_idle_tick(hcsr04_t *hc){
	//struct async_op *op = &hc->op; 
	/*if(!op->flags.busy && !op->flags.cb_called){
		op->flags.cb_called = 1;
		if(op->callback)
			op->callback(op->arg); 
	}*/
	if(hc->echo_timeout && timeout_expired(_timer, hc->echo_timeout)){
		// simulate an echo using the ordinary state machine path (cleaner)
		state_table[hc->state][EV_ECHO_UP](hc);
		state_table[hc->state][EV_ECHO_DOWN](hc); 
		hc->distance = 4999;
	}
}

static void (*const state_table[STATE_COUNT][EV_COUNT])(hcsr04_t *hc) = {
	// tick 					echo up				echo down				trigger
	{ _hcsr_idle_tick,	_hcsr_idle, 	_hcsr_idle, 	_hcsr_trigger}, // idle
	{ _hcsr_idle_tick, 	_hcsr_idle, 		_hcsr_idle,			_hcsr_idle},		// triggering
	{ _hcsr_idle_tick, 	_hcsr_echo_up, 	_hcsr_echo_down, _hcsr_idle}		// triggered
};

static void interrupt(pcint_config_t *conf){
	hcsr04_t *hc = (hcsr04_t*)conf->arg;
	if(!hc) return;
	
	if(conf->leading)
		state_table[hc->state][EV_ECHO_UP](hc);
	else {
		hc->echo_down_time = conf->time; 
		state_table[hc->state][EV_ECHO_DOWN](hc);
	}
}

int16_t hcsr04_configure(handle_t dev,
	gpio_pin_t trig,
	gpio_pin_t echo){
	hcsr04_t *hc = (hcsr04_t*)dev;

	if(!hc || FAIL == gpio_register(_gpio, trig) || FAIL == gpio_register(_gpio, echo)){
		return E_HCSR_NO_GPIO; 
	}

	if(FAIL == gpio_configure(_gpio, echo, (gpio_flags_t){.output = 0, .pullup = 1})){
		return E_HCSR_NO_GPIO;
	}
	if(FAIL == gpio_configure(_gpio, hc->trig, (gpio_flags_t){ .output = 1, .pullup = 0})){
		return E_HCSR_NO_GPIO;
	}
	
	gpio_enable_pcint(_gpio, echo, &interrupt, hc);

	hc->trig = trig;
	hc->echo = echo;
	hc->state = STATE_IDLE;
	hc->completed = 0;
	
	return SUCCESS; 
}

int8_t hcsr04_trigger(handle_t dev, void (*callback)(void* arg), void *arg){
	if(!dev) return FAIL; 
	hcsr04_t *hc = (hcsr04_t*)dev;
	if(hc->state != STATE_IDLE) return EBUSY;

	hc->completed = 0;
	
	async_wait(callback, 0, arg, &hc->completed, 100000); 

	state_table[hc->state][EV_TRIGGER](hc);

	return SUCCESS; 
}

void hcsr04_init(void) {
	for(int c = 0; c < HCSR04_MAX; c++){
		_sensors[c].flags.in_use = 0;
	}
	
	_gpio = gpio_open(0);
	_timer = timer_open(1);
}

handle_t hcsr04_open(id_t id){
	if(id < 0 || id >= HCSR04_MAX) return INVALID_HANDLE;
	if(_sensors[id].flags.in_use) return INVALID_HANDLE;

	hcsr04_t *s = &_sensors[id];
	memset(s, 0, sizeof(*s));

	s->distance = 4998; 
	s->state = STATE_IDLE;
	s->flags.in_use = 1;

	return s; 
}

int16_t hcsr04_close(void* arg){
	hcsr04_t *hc = (hcsr04_t*)arg;

	if(!hc) return FAIL;
	gpio_release(_gpio, hc->trig);
	gpio_release(_gpio, hc->echo); 
	//dev_ioctl(gpio, 0, IOC_GPIO_UNLOCK_PIN, hc->trig);
	//dev_ioctl(gpio, 0, IOC_GPIO_UNLOCK_PIN, hc->echo);
	hc->flags.in_use = 0;

	return SUCCESS; 
}

int16_t hcsr04_ioctl(handle_t arg, uint8_t ioc, int32_t param){
	/*hcsr04_t *hc = (hcsr04_t*)arg;
	if(!hc) return FAIL;

	switch(ioc){
		case IOC_HCSR_CONFIGURE:
			if(!param) return FAIL;
			_configure(hc, (struct hcsr04_config*)param);
			break; 
		default:
			return FAIL; 
	}
	*/
	return SUCCESS; 
}

uint32_t hcsr04_get_distance(handle_t dev){
	hcsr04_t *hc = (hcsr04_t*)dev;
	if(!hc) return 0;
	return hc->distance; 
}

int16_t hcsr04_read(handle_t arg, uint8_t *buf, uint16_t size){
	hcsr04_t *hc = (hcsr04_t*)arg;
	if(!hc) return FAIL;
	(*(uint32_t*)buf) = hc->distance;
	return SUCCESS; 
}

int16_t hcsr04_write(handle_t arg, const uint8_t *buf, uint16_t size){
	
	return SUCCESS; 
}

void hcsr04_tick(void){
	for(int c = 0; c < HCSR04_MAX; c++){
		hcsr04_t *hc = (hcsr04_t*)&_sensors[c];
		
		if(hc->flags.in_use){
			state_table[hc->state][EV_TICK](hc);
		}
	}
}

DECLARE_DRIVER(hcsr04); 

//INIT_DRIVER(hcsr04); 
