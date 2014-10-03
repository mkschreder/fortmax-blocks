#pragma once

//definitions
void l3g4200d_init(void);
handle_t l3g4200d_open(id_t id); 
int8_t l3g4200d_getdata(handle_t handle, async_callback_t callback, void *arg);
int8_t l3g4200d_readraw(handle_t handle, int16_t *gx, int16_t *gy, int16_t *gz);

//functions
//extern void l3g4200d_init(void);
//extern void l3g4200d_setoffset(double offsetx, double offsety, double offsetz);
//extern void l3g4200d_getrawdata(int16_t *gxraw, int16_t *gyraw, int16_t *gzraw);
//extern void l3g4200d_getdata(double* gx, double* gy, double* gz);
//extern void l3g4200d_settemperatureref(void);
//extern int8_t l3g4200d_gettemperaturediff(void);
