# kernel_module/Makefile

obj-m := sniffer.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
OUT := ../build/kernel_module

all:
	@mkdir -p $(OUT)
	$(MAKE) -C $(KDIR) M=$(PWD) modules
# Copy kernel modules to build directory
	@cp *.ko $(OUT) 

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -rf $(OUT)/*
