#pragma once 

#include "kconfig.h"
#include "kdepends.h"

#include <inttypes.h>

#include "arch/m328p.h"

//#include "accel_docalibration.h"

#ifdef __ATMega328P__
#include "arch/m328p/adc.h"
#endif

#ifdef I2CMASTER
#include "arch/m328p/i2cmaster.h"
#endif

#ifdef ACS712
#include "sensors/acs712.h"
#endif

#ifdef ADXL345
#include "sensors/adxl345.h"
#endif

#ifdef ATM1001
#include "sensors/amt1001.h"
#endif

//#include "audioget.h"
#ifdef AVRBUS
#include "avrbus.h"
#endif

#ifdef BH1750
#include "sensors/bh1750.h"
#endif

//#include "bldcsensored.h"
//#include "bldcsensorless.h"

#ifdef BMP085
#include "sensors/bmp085.h"
#endif

//#include "dcmotor.h"
//#include "dcmotorpwm.h"
//#include "dcmotorpwmsoft.h"

#ifdef DHT11
#include "sensors/dht.h"
#endif

#ifdef DS18B20
#include "sensors/ds18b20.h"
#endif

#ifdef ENC28J60
#include "net/enc28j60.h"
#endif

#ifdef FFT
#include "fftradix4.h"
#endif

#ifdef FS300A
#include "sensors/fs300a.h"
#endif

//#include "gyro_docalibration.h"
#ifdef HCSR04
#include "sensors/hcsr04.h"
#endif

#ifdef HMC5883L
#include "sensors/hmc5883l.h"
#endif

#ifdef I2CSLAVE
#include "arch/m328p/twi_slave.h"
#endif

//#include "i2csoft.h"
//#include "i2csw.h"
//#include "i2csw_slave.h"

#ifdef ILI9340
#include "disp/ili9340.h"
#endif

#ifdef IMU10DOF01
#include "sensors/imu10dof01.h"
#endif

#ifdef L3G4200D
#include "sensors/l3g4200d.h"
#endif

#ifdef L74HC165
#include "io/l74hc165.h"
#endif

#ifdef L74HC4051
#include "io/l74hc4051.h"
#endif

#ifdef L74HC595
#include "io/l74hc595.h"
#endif

//#include "lcdpcf8574.h"
//#include "ldr.h"
//#include "ledmatrix88.h"
//#include "magn_docalibration.h"
#ifdef MFRC522
#include "sensors/mfrc522.h"
#endif

#ifdef MMA7455
#include "sensors/mma7455.h"
#endif

#ifdef MPU6050
#include "sensors/mpu6050.h"
#include "sensors/mpu6050registers.h"
#endif

#ifdef NRF24L01
#include "radio/nrf24l01.h"
#include "radio/nrf24l01registers.h"
#endif

//#include "ntctemp.h"
#ifdef PCF8574
#include "io/pcf8574.h"
#endif

//#include "pwm.h"
//#include "pwmcd4017.h"
//#include "pwmcd4017servo.h"
#ifdef RFNET
#include "net/rfnet.h"
#endif

//#include "sevseg.h"
//#include "softi2c.h"
#ifdef SPI
#include "arch/m328p/spi.h"
#endif

#ifdef SSD1306
#include "disp/ssd1306.h"
#include "disp/ssd1306_priv.h"
#endif

//#include "stepper02.h"
//#include "stepper04multi.h"
#if defined(TCPIP) | defined(TCP) | defined(UDP)
#include "net/tcpip.h"
#endif

//#include "temt6000.h"
//#include "tsl235.h"
#ifdef UART
#include "arch/m328p/uart.h"
#endif

#ifdef VT100
#include "tty/vt100.h"
#endif

//#include "wiinunchuck.h"
