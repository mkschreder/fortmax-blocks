VPATH += arch/m328p

CROSSCC := avr-gcc
CROSSCXX := avr-g++
CROSSLD := avr-gcc

obj-$(CONFIG_ATMEGA328P) += adc.o i2cmaster.o i2csoft.o i2csw.o i2csw_slave.o random.o spi.o stack.o time.o twi_slave.o uart.o

CROSSCFLAGS += -DF_CPU=$(CPU_FREQ) -mmcu=$(ARCH_CPU)
