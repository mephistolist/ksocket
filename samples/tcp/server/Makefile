KBUILD_EXTRA_SYMBOLS := ../../../src/Module.symvers

obj-m := tcp_server.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

EXTRA_CFLAGS += -I$(PWD)

default: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order .*cmd $(USER_BINARY)
