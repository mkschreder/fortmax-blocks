#include <kernel/device.h>
#include <kernel/timer.h>
#include <kernel/list.h>
#include <kernel/async.h>

/**
* Async driver - runs tasks asynchronously inside the main loop
*
* Functions:
* - each: run a task for each element in an array
* - map: produces a new array of values after running each element through iterator
* - filter: filters values from an array
* - reduce: reduces values of an array to a single value
**/

#define POOL_SIZE 16

static LIST_HEAD(_tasks);
static LIST_HEAD(_to_schedule);
static LIST_HEAD(_empty);
static struct async_task _pool[POOL_SIZE];
static semaphore_t _semaphore;

static handle_t timer1 = 0;


struct async_task *__alloc_task(void){
	//printf("schedule...\n");
	if(list_empty(&_empty)) return 0;
	
	struct async_task *op = list_first_entry(&_empty, struct async_task, list);
	
	//printf("Scheduling task %s\n", op->name);
	
	list_del_init(&op->list);
	list_add_tail(&op->list, &_to_schedule);

	op->wait_on = 0; 
	op->completed = 0;
	op->timeout = timeout_from_now(timer1, 0);
	op->flags.busy = 1;
	op->flags.cb_called = 0;
	op->flags.done = 0;
	
	sem_take(&_semaphore);
	
	return op;
}

static void __async_done(struct async_task *op, uint8_t success){
	if(success && op->completed)
		op->completed(op->completed_arg);
	else {
		op->flags.done = 1;
	}
	
	//else if(!success && op->failed)
	//	op->failed(op->arg);
	if(op->flags.done){
		list_del_init(&op->list);
		list_add_tail(&op->list, &_empty);
	
		op->flags.busy = 0;
		
		sem_give(&_semaphore); 
	}
}

void __reschedule(struct async_task *op, timeout_t timeout){
	op->timeout = timeout_from_now(timer1, timeout);
	list_del_init(&op->list);
	list_add_tail(&op->list, &_to_schedule);
}

int8_t async_wait(
	async_callback_t success, async_callback_t fail,
	void *arg, volatile uint8_t *flag, timeout_t timeout){
	if(!success) return FAIL;
	/*if(*flag) {
		success(arg);
		return SUCCESS;
	}*/
	
	struct async_task *op = __alloc_task();
	//op->failed = fail;
	op->wait_on = flag;
	op->completed = success; 
	op->completed_arg = arg;
	op->timeout = timeout_from_now(timer1, timeout);
	
	return SUCCESS; 
}

int8_t async_schedule(async_callback_t cb, void *arg, timeout_t time){
	if(!cb) return FAIL; 
	struct async_task *op = __alloc_task();
	if(!op) return FAIL;
	op->completed = cb;
	op->completed_arg = arg; 
	op->timeout = timeout_from_now(timer1, time); 
	return SUCCESS; 
}

 void async_tick(void){
	//uart_printf("TICK!\n");
	struct list_head *i, *n;

	//if(!list_empty(&_to_schedule)){
		list_for_each_safe(i, n, &_to_schedule){
			list_del_init(i);
			list_add_tail(i, &_tasks);
		}
	//}
	
	// iterates thorugh the tasks and calls the callbacks if 
	list_for_each_safe(i, n, &_tasks) {
		struct async_task *op = list_entry(i, struct async_task, list);
		//printf("tick %s %u\n", op->name, timer_get_clock(0));

		// set the done flag (this flag can be reset by callback!) 
		op->flags.done = 1; // tell system to free the async slot
		
		if(op->wait_on && (*(op->wait_on))){
			// if the flag we are waiting on has changed to 1
			__async_done(op, 1);
		} else if(op->wait_on && timeout_expired(timer1, op->timeout)){
			// if our timeout has expired before the flag changed to 1
			__async_done(op, 0);
		} else if(!op->wait_on && timeout_expired(timer1, op->timeout)){
			// if our timeout has expired
			__async_done(op, 1); 
		}
	}
}

void _block_iterate(void *ptr); 
void _block_iterate_next(void *ptr){
	async_task_t *block = (async_task_t*)ptr;
	block->ptr += block->stride;
	uint8_t last = block->ptr > (block->size - block->stride); 
	if(last){
		// we are done
		//block->completed = 0;
		//block->completed_arg = 0;
		block->flags.done = 1; 
		//__async_done(block, 1); 
	} else {
		__reschedule(block, 0);
	}
	block->flags.iterator_called = 0; 
}

void _block_iterate(void *ptr){
	async_task_t *block = (async_task_t*)ptr;
	block->flags.done = 0;
	uint8_t last = block->ptr >= (block->size - block->stride); 
	if(block->iterator && !block->flags.iterator_called){
		block->flags.iterator_called = 1;
		block->iterator(
			block,
			block->data + block->ptr,
			last,
			block->iterator_arg,
			_block_iterate_next);
		//block->flags.iterator_called = 1; 
	} 
	if(!block->iterator || last){
		block->flags.done = 1;
	}
}

static int8_t __schedule_block(async_task_t block){
	if(!block.iterator || block.size == 0 || block.stride == 0){
		//printf("iterator, size and stride must be set!\n"); 
		return FAIL;
	}
	
	async_task_t *b = __alloc_task();

	if(!b) return FAIL;
	
	b->data = block.data; 
	b->stride = block.stride; 
	b->size = block.size; 
	b->iterator = block.iterator; 
	b->iterator_arg = block.iterator_arg; 
	b->ptr = 0;
	b->completed = _block_iterate;
	b->completed_arg = b;
	b->timeout = timeout_from_now(timer1, 0);
	b->flags.iterator_called = 0;
	
	return SUCCESS; 
}

int8_t async_schedule_each(uint8_t *array, uint8_t stride, uint8_t size, async_iterator_t iterator, void *arg){
	return __schedule_block((async_task_t){
		.data = array,
		.stride = stride,
		.size = size,
		.iterator = iterator,
		.iterator_arg = arg, 
	});
}

uint16_t async_get_max_tasks(void){
	return POOL_SIZE; 
}

uint16_t async_get_active_tasks(void){
	return POOL_SIZE - sem_value(&_semaphore);
}

void async_stats(void){
	uart_printf("Memory/task: %d\n", sizeof(async_task_t));
	uart_printf("Total: %d\n", sizeof(async_task_t) * 2 + sizeof(_pool));
	uart_printf("Free: %d\n", sem_value(&_semaphore));
	uart_printf("Active: %d\n", POOL_SIZE - sem_value(&_semaphore)); 
}

CONSTRUCTOR(async_init){
	timer1 = timer_open(1);

	sem_init(&_semaphore, POOL_SIZE);
	
	for(int c = 0; c < POOL_SIZE; c++){
		//_pool[c].name = "reserved"; 
		INIT_LIST_HEAD(&_pool[c].list);
		list_add_tail(&_pool[c].list, &_empty);
	}
	
	static struct driver drv = {
		.name = "async",
		.tick = &async_tick,
		.init = 0
	};
	
	driver_register(&drv); 
}
