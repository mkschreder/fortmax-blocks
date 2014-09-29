#include "device.h"
#include <string.h>

struct driver *_drv_list = 0; 

static struct driver *driver_find(const struct driver *drv){
	if(!_drv_list || !drv) return 0;
	struct driver *i = _drv_list;
	while(i){
		if(i == drv) {
			return i; 
		}
		i = i->next; 
	}
	return 0;
}

void driver_register(struct driver *drv){
	if(!drv) return;
	if(driver_find(drv)) return;
	
	drv->next = 0; drv->prev = 0;
	if(drv->init) drv->init(); 
	if(!_drv_list) {
		_drv_list = drv;
		return;
	} else {
		drv->next = _drv_list;
		_drv_list->prev = drv; 
		_drv_list = drv;
	}
}


void driver_tick(){
	if(!_drv_list) return;
	for(struct driver *i = _drv_list; i != 0; i = i->next){
		if(i->tick != 0) i->tick(); 
	}
}
/*
struct device *device(const char *name){
	struct device *i = device_find(name);
	if(i && i->open && i->open() == SUCCESS)
		return i;
	return 0; 
}
*/
