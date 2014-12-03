srctree		:= $(CURDIR)

VPATH := arch:boards:build:crypto:disp:hid:io:motors:net:radio:rfid:sensors:tty

# define defaults that can be added to in submakefiles
INCLUDES := -I. -Iinclude -Iinclude/c++ -Ikernel
COMMON_FLAGS := -MD -ffunction-sections -Wall -Wstrict-prototypes -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -fdata-sections -Os -Wl,--relax,--gc-sections
CFLAGS := 
CXXFLAGS := 
LDFLAGS := 
BUILD_DIR := build

include .config 
include Makefile.build 

# append flags defined in arch/
COMMON_FLAGS += $(CPU_FLAGS)

# add includes to the make
CFLAGS 		+= $(INCLUDES) $(COMMON_FLAGS) -std=gnu99 
CXXFLAGS 	+= $(INCLUDES) $(COMMON_FLAGS) -fpermissive  -std=c++11 
LDFLAGS 	+= $(COMMON_FLAGS) 
OUTDIRS := build build/crypto/aes
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

obj-y := $(patsubst %, $(BUILD_DIR)/%, $(obj-y))

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

config %config: scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

app: build
	make -C $(APP) build

fixdep: 
	find build -type f -iname '*.d' -exec sh -c 'scripts/basic/fixdep "$${1%.*}.d" "$${1%.*}.o" "" > $${1%.*}.cmd' convert {} \;
	
build: fixdep $(obj-y)
	ar rs $(APPNAME) $(obj-y)

$(BUILD_DIR)/%.o: %.cpp 
	mkdir -p `dirname $@`
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c 
	mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) $< -o $@

clean: 
	@find . \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
		-type f -print | xargs rm -f	
	@find . \( -name 'docproc' -o -name 'split-include' \
		  \
		-o -name 'qconf' -o -name 'gconf' -o -name 'kxgettext' \
		-o -name 'mconf' -o -name 'conf' -o -name 'lxdialog' \) \
		-type f -print | xargs rm -f	

-include $(obj-y:%.o=%.cmd)

PHONY += FORCE
FORCE:


# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable se we can use it in if_changed and friends.
.PHONY: $(PHONY) directories
