/*
l3g4200d asynchronous driver for fortmax kernel
https://github.com/mkschreder/fortmax-blocks

copyright (c) Martin K. Schröder (async code)
(Original driver by Davice Gironi - plain version)

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>

#include <kernel/i2c.h>

#include <kernel/l3g4200d.h>
#include <kernel/l3g4200d_priv.h>

#define debug(msg) {} //uart_puts(msg);

enum {
	STATE_IDLE,
	STATE_BUSY,
	MAX_STATES
};

enum {
	EV_TICK,
	MAX_EVENTS
};

typedef struct l3g4200d_request {
	union {
		uint8_t buffer[6];
		uint16_t raw[3];
	} data; 
	// callbacks invoked when request succeeds or fails
	async_callback_t success, error;
	void * 				arg;
} l3g4200d_request_t;

struct l3g4200d {
	struct {
		uint8_t 		busy : 1;
		uint8_t 		initialized : 1; 
	} flags;

	l3g4200d_request_t request;

	int16_t 			gxraw, gyraw, gzraw; 
	int16_t 			tempdiff;
};

handle_t i2c = 0;

struct l3g4200d _sensors[L3G4200D_COUNT];

//static void (*state_table[MAX_STATES][MAX_EVENTS])(void *ptr);

//offset variables
volatile double l3g4200d_offsetx = 0.0f;
volatile double l3g4200d_offsety = 0.0f;
volatile double l3g4200d_offsetz = 0.0f;

//reference temperature
static int8_t l3g4200d_temperatureref = 0;

#if L3G4200D_CALIBRATED == 1 && L3G4200D_CALIBRATEDDOTEMPCOMP == 1
static double l3g4200d_gtemp = 0; //temperature used for compensation
#endif

static void __rawdata_get_completed(void *ptr){
	struct l3g4200d *dev = (struct l3g4200d*)ptr;
	uint8_t *buff = dev->request.data.buffer;
	
	dev->gxraw = ((buff[1] << 8) | buff[0]);
	dev->gyraw = ((buff[3] << 8) | buff[2]);
	dev->gzraw = ((buff[5] << 8) | buff[4]);

	//dev->state = STATE_IDLE;
	dev->flags.busy = 0;
	
	if(dev->request.success){
		async_schedule(dev->request.success, dev->request.arg, 0);
	}
}

static void _l3g4200d_get_corrected_data(handle_t handle, double *gx, double *gy, double *gz){
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	
	#if L3G4200D_CALIBRATED == 1 && L3G4200D_CALIBRATEDDOTEMPCOMP == 1
	l3g4200d_gtemp = l3g4200d_gtemp*0.95 + 0.05*l3g4200d_gettemperaturediff(); //filtered temperature compansation
	#endif

	#if L3G4200D_CALIBRATED == 1
		#if L3G4200D_CALIBRATEDDOTEMPCOMP == 1
		*gx = (dev->gxraw - (double)((L3G4200D_TEMPCOMPX*l3g4200d_gtemp) + (double)l3g4200d_offsetx)) * (double)L3G4200D_GAINX;
		*gy = (dev->gyraw - (double)((L3G4200D_TEMPCOMPY*l3g4200d_gtemp) + (double)l3g4200d_offsety)) * (double)L3G4200D_GAINY;
		*gz = (dev->gzraw - (double)((L3G4200D_TEMPCOMPZ*l3g4200d_gtemp) + (double)l3g4200d_offsetz)) * (double)L3G4200D_GAINZ;
		#else
		*gx = (dev->gxraw-(double)l3g4200d_offsetx) * (double)L3G4200D_GAINX;
		*gy = (dev->gyraw-(double)l3g4200d_offsety) * (double)L3G4200D_GAINY;
		*gz = (dev->gzraw-(double)l3g4200d_offsetz) * (double)L3G4200D_GAINZ;
		#endif
	#else
	*gx = (dev->gxraw-(double)l3g4200d_offsetx) * (double)L3G4200D_GAIN;
	*gy = (dev->gyraw-(double)l3g4200d_offsety) * (double)L3G4200D_GAIN;
	*gz = (dev->gzraw-(double)l3g4200d_offsetz) * (double)L3G4200D_GAIN;
	#endif
}

static int8_t _l3g4200d_readdata(handle_t handle, async_callback_t callback, void *arg) {
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	if(dev->flags.busy) return EBUSY;

	dev->request.success = callback;
	dev->request.error = 0; 
	dev->request.arg = arg;

	//dev->state = STATE_BUSY;
	dev->flags.busy = 1;

	dev->request.data.buffer[0] = L3G4200D_OUT_X_L | (1 << 7); 

	debug("L3G: read data\n"); 
	i2c_transfer(i2c, (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = dev->request.data.buffer,
		.wcount = 1,
		.rcount = 6, 
		.callback = __rawdata_get_completed,
		.arg = dev
	});
	
	return SUCCESS; 
}

int8_t l3g4200d_read(handle_t handle, int16_t *gx, int16_t *gy, int16_t *gz){
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	*gx = dev->gxraw; *gy = dev->gyraw; *gz = dev->gzraw;
	return SUCCESS; 
}

static void _repeat_read(void *ptr){
	_l3g4200d_readdata(ptr, _repeat_read, ptr); 
}

static void _tempref_set_complete(void *ptr){
	struct l3g4200d *dev = (struct l3g4200d*)ptr;
	uint8_t rawtemp = dev->request.data.buffer[0]; 
	l3g4200d_temperatureref = (int8_t)rawtemp;
	#if L3G4200D_CALIBRATED == 1 && L3G4200D_CALIBRATEDDOTEMPCOMP == 1
	l3g4200d_gtemp = (double)rawtemp;
	#endif
	
	dev->flags.busy = 0;
	dev->flags.initialized = 1;

	debug("L3G: init done!\n"); 
	//uart_puts("init done!\n"); 
	if(dev->request.success){
		async_schedule(dev->request.success, dev->request.arg, 0);
	}
	
	_l3g4200d_readdata(dev, _repeat_read, dev);
}

static void __l3g4200d_init_set_tempref(handle_t handle) {
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	//if(dev->flags.busy) return EBUSY;
	//uart_puts("set_temp\n");
	debug("L3G: set tempref\n"); 
	dev->request.data.buffer[0] = L3G4200D_OUT_TEMP; 
	i2c_transfer(i2c, (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = dev->request.data.buffer,
		.wcount = 1,
		.rcount = 1, 
		.callback = _tempref_set_complete,
		.arg = dev
	});
	
}

// second stage of init process
static void __init_set_range(void *ptr){
	//uart_puts("setrange\n"); 
	struct l3g4200d *s = (struct l3g4200d*)ptr;
	s->request.data.buffer[0] = L3G4200D_CTRL_REG4;
	s->request.data.buffer[1] = L3G4200D_RANGE<<4;
	debug("L3G: set range\n");
	_i2c_command = (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = s->request.data.buffer,
		.wcount = 2,
		.callback = __l3g4200d_init_set_tempref,
		.arg = s
	});
	i2c_transfer(i2c, &_i2c_buffer)
}

static void __l3g4200d_init(struct l3g4200d *dev){
	//uart_puts("init\n"); 
	// fireoff the init sequence
	debug("L3G: init\n"); 
	dev->request.data.buffer[0] = L3G4200D_CTRL_REG1;
	dev->request.data.buffer[1] = 0x0f; 
	i2c_transfer(i2c, (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = dev->request.data.buffer,
		.wcount = 2,
		.callback = __init_set_range,
		.arg = dev
	});
}

static void _l3g4200d_configure(handle_t handle,
	async_callback_t success, async_callback_t fail, void *arg){
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	//uart_puts("conf\n");
	//DDRD &= ~_BV(5);
	
	dev->request.success = success;
	dev->request.error = fail;
	dev->request.arg = arg;

	debug("L3G: configure\n"); 
	__l3g4200d_init(dev);
}

handle_t l3g4200d_open(id_t id) {
	if(!i2c) i2c = i2c_open(0);
	if(!i2c) return INVALID_HANDLE; 
	if(id < 0 || id >= L3G4200D_COUNT) return INVALID_HANDLE;
	
	struct l3g4200d *s = &_sensors[id];
	//s->state = STATE_BUSY;
	s->flags.busy = 1;
	s->flags.initialized = 0;
	
	#if L3G4200D_CALIBRATED == 1
	//init offset
	l3g4200d_offsetx = L3G4200D_OFFSETX;
	l3g4200d_offsety = L3G4200D_OFFSETY;
	l3g4200d_offsetz = L3G4200D_OFFSETZ;
	#endif

	debug("L3G: open\n"); 
	_l3g4200d_configure(s, 0, 0, 0); 
	return s; 
}
