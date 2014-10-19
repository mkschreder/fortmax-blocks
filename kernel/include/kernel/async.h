#pragma once

#include <kernel/types.h>
#include <kernel/timer.h>
//#include "object.h"
#include <kernel/list.h>

struct async_op;
 
typedef void (*async_callback_t)(void *arg) ;
typedef void (*async_iterator_t)(void *block, uint8_t *it, uint8_t last, void *arg,
		void (*next)(void *block));

typedef struct async_process {
	struct {
		uint8_t sleeping : 1;
		uint8_t scheduled : 1;
		uint8_t completed : 1;
	} flags;

	timeout_t sleep_timeout;

	void (*start)(handle_t *proc);
	void (*done)(handle_t *proc);
	void (*signal)(handle_t *proc, uint16_t sig, long data); 
	void (*failed)(handle_t *proc, const char *err);
	
	struct list_head list; 
} async_process_t;

struct myprocess {
	uint8_t mybuffer; 
	struct async_process process;
};

typedef struct async_task {
	async_callback_t 	completed;
	void 							*completed_arg;
	
	volatile uint8_t 	*wait_on;
	struct {
		uint8_t busy : 1;
		uint8_t cb_called : 1;
		uint8_t scheduled : 1;
		uint8_t done : 1;
		uint8_t iterator_called : 1; 
	} flags;
	timeout_t timeout;

	// loop tasks
	uint8_t *data; 	// pointer to data
	uint8_t ptr; 		// current item pointer
	uint8_t stride;	// how many bytes between items?
	uint8_t size; 	// total size of the array
	async_iterator_t iterator; // iterator that is called for each block
	void *iterator_arg;

	struct list_head list; 
} async_task_t;

typedef struct async_semaphore {
	uint8_t 	value;
} semaphore_t;

/*
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
*/
int8_t async_schedule(async_callback_t cb, void *arg, timeout_t timeout); 
int8_t async_wait(
	async_callback_t success, async_callback_t fail,
	void *arg, volatile uint8_t *flag, timeout_t timeout); 
int8_t async_schedule_each(uint8_t *array, uint8_t stride, uint8_t size, async_iterator_t iterator, void *arg);

#define async_each(_array, _each, _arg) async_schedule_each(_array, sizeof(_array[0]), sizeof(_array), _each, _arg)

inline void 	sem_init(semaphore_t *sem, uint8_t value){
	sem->value = value;
}

inline uint8_t sem_take(semaphore_t *sem){
	if(sem->value == 0) return 0;
	sem->value--;
	return 1;
}

inline void 	sem_give(semaphore_t *sem){
	sem->value++;
}

inline uint8_t 	sem_value(semaphore_t *sem){
	return sem->value; 
}
