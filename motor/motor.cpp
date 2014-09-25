
#include <avr/io.h>
#include <avr/interrupt.h>

#include "motor.hpp"

#include <drivers/time.h>

#include <string.h>

typedef enum {
	MOTOR_PWM_HI,
	MOTOR_PWM_LO,
	MOTOR_NUM_STATES
} motor_state;

enum events {
	EV_TIMEOUT,
	EV_COUNT
};

const int16_t servo_correction[MOTOR_COUNT] = {-250, -10, -40, -10}; 
const uint16_t servo_default_speed[MOTOR_COUNT] = {500, 500, 500, 500};

static motor_exported_params _export;

static struct motor_s{
	timeout_t timeout;
	timeout_t cycle_timeout;
	motor_state state;
	uint8_t id; 
} motors[MOTOR_COUNT]; 

static inline void motor_out(uint8_t motor, uint8_t val){
	switch(motor){
		case 0:
			if(val) MOTOR_PORT |= _BV(MOTOR_0_PIN);
			else MOTOR_PORT &= ~_BV(MOTOR_0_PIN);
			break; 
		case 1:
			if(val) MOTOR_PORT |= _BV(MOTOR_1_PIN);
			else MOTOR_PORT &= ~_BV(MOTOR_1_PIN);
			break; 
		case 2: 
			if(val) MOTOR_PORT |= _BV(MOTOR_2_PIN);
			else MOTOR_PORT &= ~_BV(MOTOR_2_PIN);
			break; 
		case 3:
			if(val) MOTOR_PORT |= _BV(MOTOR_3_PIN);
			else MOTOR_PORT &= ~_BV(MOTOR_3_PIN);
			break; 
	}; 
}

void motor_init(){
	memcpy(_export.speed, servo_default_speed, sizeof(servo_default_speed));

	for(uint8_t c = 0; c < MOTOR_COUNT; c++){
		motors[c].id = c;
		
		switch(c){
			case 0:
				MOTOR_DDR |= _BV(MOTOR_0_PIN); 
				break; 
			case 1:
				MOTOR_DDR |= _BV(MOTOR_1_PIN); 
				break; 
			case 2: 
				MOTOR_DDR |= _BV(MOTOR_2_PIN); 
				break; 
			case 3:
				MOTOR_DDR |= _BV(MOTOR_3_PIN); 
				break;
		}
	}
}

void motor_set_speed(uint8_t motor, uint16_t speed){
	if(motor >= MOTOR_COUNT)
		motor = 0;

	//speed += servo_correction[motor];
	
	if(speed > 2000) speed = 2000;
	if(speed < 500) speed = 500;
	
	//servo = servo & 0x03;
	_export.speed[motor] = speed;
}

void motor_hi_timeout(motor_s *motor){
	motor_out(motor->id, 0);
	motor->timeout = motor->cycle_timeout; 
	motor->state = MOTOR_PWM_LO;
}

void motor_lo_timeout(motor_s *motor){
	// pull the motor line high and set timeout to number of micros for pulse length
	//MOTOR_PORT |= _BV(MOTOR_0_PIN);
	uint16_t sp = _export.speed[motor->id];
	if(sp > 2000) sp = 2000; else if(sp < 500) sp = 500;
	
	motor_out(motor->id, 1);
	
	motor->timeout = timeout_from_now(sp);
	motor->cycle_timeout = timeout_from_now(MOTOR_DUTY_TIME); 
	motor->state = MOTOR_PWM_HI;
}

void (*const state_table [MOTOR_NUM_STATES][EV_COUNT]) (motor_s *motor) = {
	{ motor_hi_timeout }, 
	{ motor_lo_timeout }, 
};

void motor_do_epoch(){
	for(uint8_t c = 0; c < MOTOR_COUNT; c++){
		motor_s *motor = &motors[c];
		
		if(timeout_expired(motor->timeout)){
			state_table[motor->state][EV_TIMEOUT](motor); 
		}
	}
}

