#include <stddef.h>
#include <stdint.h>

#include "device.h"
#include "list.h"
/*
int8_t list_push(struct list_head **list, struct list_head *i){
	if(i->sync) return FAIL; 
	if(!*list){
		*list = i;
		return SUCCESS;
	}
	i->next = *list;
	i->prev = 0; 
	(*list)->prev = i; 
	(*list) = i;
	i->sync = 1; 
	return SUCCESS; 
}

struct list_head *list_unlink(struct list_head *item){
	item->sync = 0; 
	if(!item->next && !item->prev) return 0; 
	if(!item->next) {
		item->prev->next = 0;
		return item->prev; 
	}
	if(!item->prev) {
		item->next->prev = 0;
		return item->next; 
	} else {
		item->next->prev = item->prev;
		item->prev->next = item->next;
		return item->next;
	}
}*/
