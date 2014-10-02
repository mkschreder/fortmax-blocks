#pragma once

#include "types.h"
#include "timer.h"
//#include "object.h"
#include "list.h"

struct async_op;

typedef void (*next_callback_t)(void); 
typedef void (*async_callback_t)(void *arg) ;

struct async_op {
	uint8_t op;
	const char *name; 
	async_callback_t callback;
	async_callback_t failed; 
	void* arg;
	volatile uint8_t *wait_on; 
	struct {
		uint8_t busy : 1;
		uint8_t cb_called : 1;
		uint8_t scheduled : 1; 
	} flags;
	timeout_t timeout;
	struct list_head list;
};

int8_t async_schedule(async_callback_t cb, void *arg, timeout_t timeout); 
int8_t async_wait(
	async_callback_t success, async_callback_t fail,
	void *arg, volatile uint8_t *flag, timeout_t timeout); 
