# Makefile for the sniffer module: make ko file instead of .o file
# This Makefile is used to compile the sniffer module for Linux kernel
# It is assumed that the kernel source is installed in /lib/modules/$(uname -r)/build

obj-m += sniffer.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# The following lines are used to compile the sniffer module
# The $(MAKE) command is used to invoke the kernel build system
# The -C option specifies the directory to change to before executing the make command
# The M option specifies the directory containing the module source files
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
