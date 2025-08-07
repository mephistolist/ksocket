# ksocket project
# BSD-style socket APIs for kernel 2.6 developers
# 
# @2007-2008, China
# @song.xian-guang@hotmail.com (MSN Accounts)
# 
# This code is licenced under the GPL
# Feel free to contact me if any questions
# 
# @2017
# Hardik Bagdi (hbagdi1@binghamton.edu)
# Changes for Compatibility with Linux 4.9 to use iov_iter
#
# @2025
# Mephistolist (cloneozone@gmail.com)
# Updated for kernels 5.11-6.16
#
KBUILD_EXTRA_SYMBOLS := ../../src/Module.symvers

obj-m := tcp_server.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

EXTRA_CFLAGS += -I$(PWD)

# Name of the userland binary
USER_BINARY := tcp_client
USER_SOURCE := tcp_client.c

default: modules user

# Build the kernel module
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Build the userland binary
user:
	gcc $(USER_SOURCE) -o $(USER_BINARY)

# Clean both kernel module and userland binary
clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order .*cmd $(USER_BINARY)
