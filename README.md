# ksocket API  

A BSD-style socket API in kernel space for TCP/IP networking on Linux kernels 5.11-6.16. This is not backwards compatible with older kernels, but links will be provided to those later in this documentation. 

### Getting started
```
$ git clone https://github.com/mephistolist/ksocket.git
$ cd ksocket/src
$ make # make sure you have the kernel headers/tree installed first
$ sudo insmod ksocket.ko
#now you can use the exported symbols from this kernel module
```
You can then go to the samples directory in the root of this project. Inside it are TCP and UDP of servers and client examples you may build and insert into the kernel. After inserting the server and client for TCP, you should see something like the following with dmesg:
```
# dmesg
[  137.259240] tcp_server: Loading (starting thread)
[  137.260232] tcp_server: thread starting
[  137.260634] tcp_server: listening on port 12345
[  151.256262] [tcp_client] Initializing
[  151.257245] sock_create sk= 0x00000000db4a0058
[  151.264826] [tcp_client] Connected to 127.0.0.1:12345
[  151.264886] tcp_server: accepted newsock=000000000aae9c53 newsock->sk=00000000844b247d
[  151.265311] tcp_server: received (29 bytes): Hello from kernel TCP client!
[  151.265396] [tcp_client] Sent: Hello from kernel TCP client!
```
Or with the UDP modules, the dmesg should resemble this:
```
# dmesg
[  501.520772] sock_create sk= 0x00000000295150e8
[  501.522605] kbind ret = 0
[  501.523914] UDP Server: Listening on port 4444
[  590.998621] [udp_client] Initializing
[  591.001210] sock_create sk= 0x000000006da0c70b
[  591.003466] [udp_client] Sent: Hello UDP Server
[  591.003636] UDP Server: Received 'Hello UDP Server'
[  591.062269] [udp_client] Received: ACK from UDP Server
```
### Support across kernel versions
The original ksocket work was to support Linux 2.6, and later versions came for later kernels. This version of ksocket was designed for kernels 5.11-6.16. It may work in verions beyond 6.16, but we do not know what future kernel versions will entail. If you need this for an older kernel, see the links below:

#### v2.6 original development
https://github.com/hbagdi/ksocket

#### v5.4.0
https://github.com/hbagdi/ksocket/tree/linux-5.4.0

### Contributing/Reporting Bugs
- Feel free to open Pull-Requests here for any enhancements/fixes.
- Open an issue in the repository for any help or bugs. Make sure to mention Kernel version. 

### Stream-lining
If you wish to not have to load the ksocket module, or link its symbols in Makefiles you can statically link ksocket when building a kernel from source. Considering we have kernel 6.16 inside /usr/src and this ksocket project in /home/user/ksocket, we can do something like the following:

```
# export VERSION=6.16
# mkdir /usr/src/linux-$VERSION/drivers/ksocket
# rsync -avP /home/user/ksocket/ /usr/src/linux-$VERSION/drivers/ksocket/
# sed -i '$ s|^endmenu$|source "drivers/ksocket/Kconfig"\nendmenu|' /usr/src/linux-$VERSION/drivers/Kconfig
# echo "obj-$(CONFIG_KSOCKET)   += ksocket/" >> /usr/src/linux-$VERSION/drivers/Makefile
```
If you proceed with then building and loading your kernel, you will not have to build or insert the ksocket module as it will already be in the kernel. You will also no longer need this line in your Makefiles to use or call kscoket's API:
``` 
KBUILD_EXTRA_SYMBOLS := ../../../src/Module.symvers
```
### Contact
For this version of kscoket you may reach out to cloneozone@gmail.com.
