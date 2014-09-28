#include "device.hpp"
#include <string.h>

struct device *_dev_list = 0; 

static struct device *device_find(const char *name){
	if(!_dev_list || !name) return 0;
	struct device *i = _dev_list;
	while(i){
		if(i && strcmp(i->name, name) == 0) {
			return i; 
		}
		i = i->next; 
	}
	return 0;
}

void device_register(struct device *dev){
	if(!dev) return;
	if(device_find(dev->name)) return;
	
	dev->next = 0; dev->prev = 0;
	if(dev->init) dev->init(); 
	if(!_dev_list) {
		_dev_list = dev;
		return;
	} else {
		dev->next = _dev_list;
		_dev_list->prev = dev; 
		_dev_list = dev;
	}
}

void device_tick(){
	if(!_dev_list) return;
	for(struct device *i = _dev_list; i != 0; i = i->next){
		if(i->tick != 0) i->tick(); 
	}
}

struct device *device(const char *name){
	struct device *i = device_find(name);
	if(i && i->open && i->open() == SUCCESS)
		return i;
	return 0; 
}
