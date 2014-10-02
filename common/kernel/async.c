#include "device.h"
#include "timer.h"
#include "list.h"
#include "async.h"

#define POOL_SIZE 16

static LIST_HEAD(_tasks);
static LIST_HEAD(_to_schedule);
static LIST_HEAD(_empty);
static struct async_op _pool[POOL_SIZE];

static handle_t timer1 = 0;

int8_t async_begin(struct async_op *op){
	op->flags.busy = 1;
	op->flags.cb_called = 0;
	return SUCCESS; 
}

struct async_op *__async_schedule(async_callback_t cb, timeout_t time){
	//printf("schedule...\n");
	if(list_empty(&_empty)) return 0;
	
	struct async_op *op = list_first_entry(&_empty, struct async_op, list);
	
	printf("Scheduling task %s\n", op->name);
	
	list_del_init(&op->list);
	list_add_tail(&op->list, &_to_schedule);

	op->wait_on = 0; 
	op->callback = cb;
	op->timeout = timeout_from_now(timer1, time);
	
	async_begin(op);
	
	return op;
}

void __async_done(struct async_op *op, uint8_t success){
	list_del_init(&op->list);
	list_add_tail(&op->list, &_empty);
	
	if(success && op->callback)
		op->callback(op->arg);
	else if(!success && op->failed)
		op->failed(op->arg);
		
	op->flags.busy = 0;
}

int8_t async_wait(
	async_callback_t success, async_callback_t fail,
	void *arg, volatile uint8_t *flag, timeout_t timeout){
	struct async_op *op = __async_schedule(success, timeout);
	op->failed = fail;
	op->wait_on = flag;
	op->arg = arg;
	return SUCCESS; 
}

int8_t async_schedule(async_callback_t cb, void *arg, timeout_t time){
	struct async_op *op = __async_schedule(cb, time);
	op->arg = arg; 
	if(op) return SUCCESS;
	return FAIL; 
}

static void async_tick(void){
	//printf("TICK!\n");
	struct list_head *i, *n;
	
	list_for_each_safe(i, n, &_to_schedule){
		list_del_init(i);
		list_add_tail(i, &_tasks);
	}
	
	list_for_each_safe(i, n, &_tasks) {
		struct async_op *op = list_entry(i, struct async_op, list);
		//printf("tick %s %u\n", op->name, timer_get_clock(0));
		if(op->wait_on && (*(op->wait_on))){
			__async_done(op, 1);
		} else if(op->wait_on && timeout_expired(timer1, op->timeout)){
			__async_done(op, 0);
		} else if(timeout_expired(timer1, op->timeout)){
			__async_done(op, 1); 
		}
	}
}


CONSTRUCTOR(async_init){
	timer1 = timer_open(1);

	for(int c = 0; c < POOL_SIZE; c++){
		_pool[c].name = "reserved"; 
		INIT_LIST_HEAD(&_pool[c].list);
		list_add_tail(&_pool[c].list, &_empty);
	}
	
	static struct driver drv = {
		.name = "async",
		.tick = async_tick,
		.init = 0
	};
	
	driver_register(&drv); 
}

