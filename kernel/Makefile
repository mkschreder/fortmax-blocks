srctree		:= $(CURDIR)

INCLUDES := 

VPATH := arch:boards:crypto:disp:hid:io:motors:net:radio:rfid:sensors:tty

include .config 
include Makefile.build 

BUILD_DIR := build

ARCH_CPU := atmega328p
CPU_FREQ := 16000000
CROSSCXX := avr-g++
CROSSCC  := avr-gcc
CROSSLD  := avr-g++

INCLUDES += -I. -Iinclude -Iinclude/c++ -Ikernel

CROSSCXXFLAGS := $(INCLUDES) -ffunction-sections -fpermissive  -std=c++11 -fdata-sections -Os -Wl,--relax,--gc-sections -DBOARD_MULTIWII -DF_CPU=$(CPU_FREQ) -mmcu=$(ARCH_CPU)

CROSSCFLAGS := $(INCLUDES) -ffunction-sections -Wall -Wstrict-prototypes -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -std=c99 -fdata-sections -Os -Wl,--relax,--gc-sections -DBOARD_MULTIWII -DF_CPU=$(CPU_FREQ) -mmcu=$(ARCH_CPU)

CROSSLDFLAGS := $(INCLUDES) -ffunction-sections -fpermissive  -std=c++11 -fdata-sections -Os -DBOARD_MULTIWII -DF_CPU=$(CPU_FREQ) -mmcu=$(ARCH_CPU)

APPNAME := built-in.o

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)
	  
# Look for make include files relative to root of kernel src
MAKEFLAGS += --include-dir=$(srctree)

HOSTCC  	= gcc
HOSTCXX  	= g++
HOSTCFLAGS	:=
HOSTCXXFLAGS	:=
# We need some generic definitions
include $(srctree)/scripts/Kbuild.include

HOSTCFLAGS	+= $(call hostcc-option,-Wall -Wstrict-prototypes -O2 -fomit-frame-pointer,)
HOSTCXXFLAGS	+= -O2

# For maximum performance (+ possibly random breakage, uncomment
# the following)

MAKEFLAGS += -rR

export srctree CONFIG_SHELL HOSTCC HOSTCFLAGS HOSTCXX HOSTCXXFLAGS 
export quiet Q KBUILD_VERBOSE

# Basic helpers built in scripts/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

config %config: scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

app: build
	make -C $(APP) build

build: $(obj-y)
	ar rs $(APPNAME) $(obj-y)

%.o: %.cpp
	$(CROSSCXX) -c $(CROSSCXXFLAGS) $< 

%.o: %.c
	$(CROSSCC) -c $(CROSSCFLAGS) $< 

clean: 
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
		-type f -print | xargs rm -f	
	@find . \( -name 'fixdep' -o -name 'docproc' -o -name 'split-include' \
		-o -name 'autoconf.h'  \
		-o -name 'qconf' -o -name 'gconf' -o -name 'kxgettext' \
		-o -name 'mconf' -o -name 'conf' -o -name 'lxdialog' \) \
		-type f -print | xargs rm -f	


PHONY += FORCE
FORCE:


# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable se we can use it in if_changed and friends.
.PHONY: $(PHONY)
