#include <kernel/device.h>
#include <kernel/kvar.h>
#include <kernel/list.h>
#include <kernel/object.h>

#include <string.h>

static LIST_HEAD(_kvar_list);
static uint16_t _kvar_id = 0;

struct kvar *kvar_get_all(void){
	if(list_empty(&_kvar_list)) return 0; 
	return list_first_entry(&_kvar_list, struct kvar, list);
}

struct kvar * kvar_register(struct kvar *var){
	struct kvar *exist = kvar_find((var)->name); 
	if(exist){
		return exist;
	}
	
	var->id = _kvar_id++;

	list_add_tail(&var->list, &_kvar_list);
	
	return var; 
}

struct kvar *kvar_find(const char *name){
	if(list_empty(&_kvar_list)) return 0;

	list_for_each(i, &_kvar_list) {
		struct kvar *var = list_entry(i, struct kvar, list);
		if(strcmp(var->name, name) == 0) return var; 
	}
	return 0; 
}

uint32_t hash(const char *str){
    uint32_t hash = 5381;
    uint32_t c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}
