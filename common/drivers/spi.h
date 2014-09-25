/*
spi lib 0x01

copyright (c) Davide Gironi, 2012

References:
  -  This library is based upon SPI avr lib by Stefan Engelke
     http://www.tinkerer.eu/AVRLib/SPI

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef _SPI_H_
#define _SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>

//spi ports
#define SPI_DDR DDRB
#define SPI_PORT PORTB
#define SPI_PIN PINB

#define SPI_CE PB0
#define SPI_RDY PB1
#define SPI_SS PB2
#define SPI_MOSI PB3
#define SPI_MISO PB4
#define SPI_SCK PB5

uint8_t bus_exchange_byte(uint8_t data);

void bus_master_init(); 
void bus_slave_init(uint16_t base_addr, char *buf, uint8_t size);
uint8_t bus_read(uint16_t addr, char *data, uint8_t size);
uint8_t bus_write(uint16_t addr, const char *data, uint8_t size);

#ifdef __cplusplus
} 
#endif

#endif
