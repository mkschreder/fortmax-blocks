#pragma once

#define FAIL (0)
#define SUCCESS (1)

#include <avr/io.h>

#define DEVICE_DECLARE(driver) \
	void driver##_init(); \
	int16_t driver##_ioctl(handle_t inst, uint8_t num, int32_t data); \
	int16_t driver##_write(handle_t inst, const uint8_t *buffer, uint16_t size);\
	int16_t driver##_read(handle_t inst, uint8_t *buffer, uint16_t size);\
	handle_t driver##_open(id_t id);\
	int16_t driver##_close(handle_t inst);\
/*	static struct driver##_init__ {\
		public: driver##_init(){driver##_init();}\
	} driver##_static_init__; \*/

typedef void* handle_t;
#define INVALID_HANDLE 0
#define DEFAULT_HANDLE ((handle_t)0xffff)

typedef int16_t id_t;

#define dev_ioctl(driver, instance, num, data) driver##_ioctl((handle_t)instance, num, (int32_t)data)
#define dev_read(driver, inst, buf, size) driver##_read((handle_t)inst, (uint8_t*)buf, size)
#define dev_write(driver, inst, buf, size) driver##_write((handle_t)inst, (uint8_t*)buf, size)
//#define dev_open(driver) driver##_open()
#define dev_open(driver, num) driver##_open((id_t)num)
#define dev_close(driver, inst) driver##_close((handle_t)inst)

typedef struct driver {
	const char *name; 
	void (*tick)();
	
	void (*init)();
	int16_t (*ioctl)(handle_t, uint8_t num, int32_t data); 
	int16_t (*write)(handle_t, const uint8_t *buffer, uint16_t size);
	int16_t (*read)(handle_t,uint8_t *buffer, uint16_t size);
	handle_t (*open)(id_t id);
	int16_t (*close)(handle_t);

	struct driver *next, *prev; 
} driver_t;

#define DECLARE_DRIVER(_name) \
static struct driver _name##_struct = { \
	.name = "" #_name "", \
	.tick = &_name##_tick,\
	.init = &_name##_init,\
	.ioctl = &_name##_ioctl, \
	.write = &_name##_write,\
	.read = &_name##_read,\
	.open = &_name##_open,\
	.close = &_name##_close\
};\
driver_init(_name##_struct);

//extern void device_register(struct device *dev);
extern void driver_tick();
extern void driver_register(struct driver *drv); 
//extern struct device* device(const char *dev); 

// pass dev as pointer to the driver struct
#define driver_init(drv) void __attribute__((constructor)) drv##_run__(); void drv##_run__()  { driver_register(&drv); }
/*
class __auto_init {
	public:
	__auto_init(void (*ptr)()){
		if(ptr) ptr(); 
	}
};
*/
//#define USE(driver) extern device 
//#define INIT_DRIVER(DEV) static __auto_init DEV ## _instance(&DEV##_init); 