# ksocket API  

BSD-style socket API in kernel space for TCP/IP networking.
Original ksocket for v2.6 was published at [http://ksocket.sourceforge.net/](http://ksocket.sourceforge.net/).
This repository contains changes to make it compatible with new kernel versions.
The ksocket API has been updated to incorporate the changes related to the [iov_iter](https://lwn.net/Articles/625077/) interface introduced in kernel v3.19+.

### Getting started
```
$ git clone https://github.com/mephistolist/ksocket.git
$ cd ksocket/src
$ make # make sure you have the kernel headers/tree installed first
$ sudo insmod ksocket.ko
#now you can use the exported symbols from this kernel module
```

### Support across kernel versions
The original ksocket work was to support Linux 2.6, however support for v5.4.0 was later 
included. This version of ksocket was designed for kernels 5.11-6.16. It may work in verions beyond 6.16, but we do not know what future kernel versions will entail. If you need this for an older kernel, see the links below:

#### v2.6 original development
https://github.com/hbagdi/ksocket

#### v5.4.0
https://github.com/hbagdi/ksocket/tree/linux-5.4.0

### Contributing/Reporting Bugs
- Feel free to open Pull-Requests here for any enhancements/fixes.
- Open an issue in the repository for any help or bugs. Make sure to mention Kernel version.

### Contact
Email at hardikbagdi@gmail.com for any further help.
