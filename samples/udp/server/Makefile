KBUILD_EXTRA_SYMBOLS := ../../../src/Module.symvers

obj-m := udp_server.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
EXTRA_CFLAGS += -I$(PWD)

.PHONY: all clean
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
