obj-m += ksocket.o

MARCH=$(gcc -Q -march=native --help=target|grep -m1 march=|awk '{print $2}')
EXTRA_CFLAGS += -Wall -Wextra -Wno-unused-parameter -march=$(MARCH) -pedantic -pipe -O2 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
