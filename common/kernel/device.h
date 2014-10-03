#pragma once

#include "types.h"

#define DEVICE_DECLARE(driver)
/*\
	void driver##_init(void); \
	int16_t driver##_ioctl(handle_t inst, uint8_t num, int32_t data); \
	int16_t driver##_write(handle_t inst, const uint8_t *buffer, uint16_t size);\
	int16_t driver##_read(handle_t inst, uint8_t *buffer, uint16_t size);\
	handle_t driver##_open(id_t id);\
	int16_t driver##_close(handle_t inst);\
*/


#define dev_ioctl(driver, instance, num, data) driver##_ioctl((handle_t)instance, num, (int32_t)data)
#define dev_read(driver, inst, buf, size) driver##_read((handle_t)inst, (uint8_t*)buf, size)
#define dev_write(driver, inst, buf, size) driver##_write((handle_t)inst, (uint8_t*)buf, size)
//#define dev_open(driver) driver##_open()
#define dev_open(driver, num) driver##_open((id_t)num)
#define dev_close(driver, inst) driver##_close((handle_t)inst)

typedef struct driver {
	const char *name; 
	void (*tick)(void);
	void (*init)(void);

	/*
	int16_t (*ioctl)(handle_t, uint8_t num, int32_t data); 
	int16_t (*write)(handle_t, const uint8_t *buffer, uint16_t size);
	int16_t (*read)(handle_t,uint8_t *buffer, uint16_t size);
	handle_t (*open)(id_t id);
	int16_t (*close)(handle_t);*/

	struct driver *next, *prev; 
} driver_t;

#define DECLARE_DRIVER(_name) \
static struct driver _name##_struct = { \
	.name = "" #_name "", \
	.tick = &_name##_tick,\
	.init = &_name##_init\
};\
driver_init(_name##_struct);

//extern void device_register(struct device *dev);
extern void driver_tick(void);
extern void driver_register(struct driver *drv); 
//extern struct device* device(const char *dev); 

// pass dev as pointer to the driver struct
#define driver_init(drv) void __attribute__((constructor)) drv##_run__(void); void drv##_run__(void)  { driver_register(&drv); }

#define CONSTRUCTOR(name) void __attribute__((constructor)) name(void)

enum device_errors {
	EBUSY = -100,
	EINVAL = -99
}; 
