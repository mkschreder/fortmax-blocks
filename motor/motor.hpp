#pragma once


#ifndef MOTOR_COUNT
#define MOTOR_COUNT 4
#endif

#define MOTOR_DUTY_TIME 20000

#define MOTOR_PORT PORTB
#define MOTOR_DDR DDRB

#define MOTOR_0_PIN PB1
#define MOTOR_1_PIN PB0
#define MOTOR_2_PIN PB7
#define MOTOR_3_PIN PB6

struct motor_exported_params {
	uint16_t speed[MOTOR_COUNT];
};

void motor_init();

void motor_set_speed(uint8_t motor, uint16_t speed);

/// updates all motor outputs. Should be called very frequently
void motor_do_epoch(); 
