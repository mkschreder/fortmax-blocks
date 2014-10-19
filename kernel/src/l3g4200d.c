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

struct l3g4200d {
	struct {
		uint8_t 		busy : 1;
		uint8_t 		initialized : 1; 
	} flags;
	
	struct i2c_command i2c_request;

	int16_t 			gxraw, gyraw, gzraw; 
	int16_t 			tempdiff;

	unsigned char buffer[6]; 
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

static int8_t _l3g4200d_read_loop(handle_t handle);

static void __rawdata_get_completed(void *ptr){
	struct l3g4200d *dev = (struct l3g4200d*)ptr;
	uint8_t *buff = dev->buffer;
	
	dev->gxraw = ((buff[1] << 8) | buff[0]);
	dev->gyraw = ((buff[3] << 8) | buff[2]);
	dev->gzraw = ((buff[5] << 8) | buff[4]);

	//dev->state = STATE_IDLE;
	dev->flags.busy = 0;

	async_schedule(_l3g4200d_read_loop, dev, 10000); 
	//_l3g4200d_read_loop(dev); 
}

static int8_t _l3g4200d_read_loop(handle_t handle) {
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	if(dev->flags.busy) return EBUSY;

	//dev->state = STATE_BUSY;
	dev->flags.busy = 1;

	dev->buffer[0] = L3G4200D_OUT_X_L | (1 << 7); 

	debug("L3G: read data\n"); 
	dev->i2c_request = (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = dev->buffer,
		.wcount = 1,
		.rcount = 6, 
		.callback = __rawdata_get_completed,
		.arg = dev
	};
	i2c_transfer(i2c, &dev->i2c_request);
	
	return SUCCESS; 
}

int8_t l3g4200d_read(handle_t handle, int16_t *gx, int16_t *gy, int16_t *gz){
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	*gx = dev->gxraw; *gy = dev->gyraw; *gz = dev->gzraw;
	return SUCCESS; 
}

static void __l3g4200d_tempref_set_complete(void *ptr){
	struct l3g4200d *dev = (struct l3g4200d*)ptr;
	uint8_t rawtemp = dev->buffer[0]; 
	l3g4200d_temperatureref = (int8_t)rawtemp;
	#if L3G4200D_CALIBRATED == 1 && L3G4200D_CALIBRATEDDOTEMPCOMP == 1
	l3g4200d_gtemp = (double)rawtemp;
	#endif
	
	dev->flags.busy = 0;
	dev->flags.initialized = 1;

	debug("L3G: init done!\n"); 
	
	_l3g4200d_read_loop(dev);
}

static void __l3g4200d_init_set_tempref(handle_t handle) {
	struct l3g4200d *dev = (struct l3g4200d*)handle;
	//if(dev->flags.busy) return EBUSY;
	//uart_puts("set_temp\n");
	debug("L3G: set tempref\n"); 
	dev->buffer[0] = L3G4200D_OUT_TEMP; 
	dev->i2c_request = (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = dev->buffer,
		.wcount = 1,
		.rcount = 1, 
		.callback = __l3g4200d_tempref_set_complete,
		.arg = dev
	};
	i2c_transfer(i2c, &dev->i2c_request); 
}

// second stage of init process
static void __init_set_range(void *ptr){
	//uart_puts("setrange\n"); 
	struct l3g4200d *s = (struct l3g4200d*)ptr;
	s->buffer[0] = L3G4200D_CTRL_REG4;
	s->buffer[1] = L3G4200D_RANGE<<4;
	debug("L3G: set range\n");
	s->i2c_request = (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = s->buffer,
		.wcount = 2,
		.rcount = 0,
		.callback = __l3g4200d_init_set_tempref,
		.arg = s
	};
	i2c_transfer(i2c, &s->i2c_request); 
}

static void __l3g4200d_init(struct l3g4200d *dev){
	debug("L3G: init\n");
	
	dev->flags.busy = 1;
	
	dev->buffer[0] = L3G4200D_CTRL_REG1;
	dev->buffer[1] = 0x0f;
	
	dev->i2c_request = (i2c_command_t){
		.addr = L3G4200D_ADDR,
		.buf = dev->buffer,
		.wcount = 2,
		.rcount = 0,
		.callback = __init_set_range,
		.arg = dev
	};

	i2c_transfer(i2c, &dev->i2c_request); 
}

handle_t l3g4200d_open(id_t id) {
	if(!i2c) i2c = i2c_open(0);
	if(i2c == INVALID_HANDLE) return INVALID_HANDLE; 
	if(id < 0 || id >= L3G4200D_COUNT) return INVALID_HANDLE;
	
	struct l3g4200d *s = &_sensors[id];
	//s->state = STATE_BUSY;
	s->flags.busy = 0;
	s->flags.initialized = 0;
	INIT_LIST_HEAD(&s->i2c_request._list);
	
	#if L3G4200D_CALIBRATED == 1
	//init offset
	l3g4200d_offsetx = L3G4200D_OFFSETX;
	l3g4200d_offsety = L3G4200D_OFFSETY;
	l3g4200d_offsetz = L3G4200D_OFFSETZ;
	#endif

	debug("L3G: open\n");
	
	__l3g4200d_init(s);
	return s; 
}
