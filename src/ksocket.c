/* 
 * ksocket project
 * BSD-style socket APIs for kernel 2.6 developers
 * 
 * @2007-2008, China
 * @song.xian-guang@hotmail.com (MSN Accounts)
 * 
 * This code is licenced under the GPL
 * Feel free to contact me if any questions
 *
 * @2017
 * Hardik Bagdi (hbagdi1@binghamton.edu)
 * Changes for Compatibility with Linux 4.9 to use iov_iter
 *
 * @2025
 * Mephistolist (cloneozone@gmail.com)
 * Changes for kernels 5.11 through at least 6.16. 
 */
#include <linux/module.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/sockptr.h>
#include <linux/net.h>
#include <linux/in.h>
#include <net/sock.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <linux/uio.h>
#include "ksocket.h"

#define KSOCKET_NAME	"ksocket"
#define KSOCKET_VERSION	"0.0.3"
#define KSOCKET_DESCPT	"BSD-style socket APIs for kernels 5.11 - 6.16.x"
#define KSOCKET_AUTHOR	"mail : cloneozone@gmail.com\n"\
						"blog: https://myresume.sh"
#define KSOCKET_DATE	"2025-08-03"

MODULE_AUTHOR(KSOCKET_AUTHOR);
MODULE_DESCRIPTION(KSOCKET_NAME"-"KSOCKET_VERSION"\n"KSOCKET_DESCPT);
MODULE_LICENSE("Dual BSD/GPL");

ksocket_t ksocket(int domain, int type, int protocol) {
	struct socket *sk = NULL;
	int ret = 0;
	
	ret = sock_create(domain, type, protocol, &sk);
	if (ret < 0) {
		printk(KERN_INFO "sock_create failed\n");
		return NULL;
	}

	printk("sock_create sk= 0x%p\n", sk);
	
	return sk;
}

int kbind(ksocket_t socket, struct sockaddr *address, int address_len) {
	struct socket *sk;
	int ret = 0;

	sk = (struct socket *)socket;
	ret = sk->ops->bind(sk, address, address_len);
	printk("kbind ret = %d\n", ret);
	
	return ret;
}

int klisten(ksocket_t socket, int backlog) {
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	
	if ((unsigned)backlog > SOMAXCONN) {
		backlog = SOMAXCONN;
	}
	
	ret = sk->ops->listen(sk, backlog);
	
	return ret;
}

int kconnect(ksocket_t socket, struct sockaddr *address, int address_len) {
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	ret = sk->ops->connect(sk, address, address_len, 0/*sk->file->f_flags*/);
	
	return ret;
}

ksocket_t kaccept(ksocket_t socket, struct sockaddr *address, int *address_len) {
    struct socket *sk = (struct socket *)socket;
    struct socket *new_sk = NULL;
    int ret;

    printk("family = %d, type = %d, protocol = %d\n",
           sk->sk->sk_family, sk->type, sk->sk->sk_protocol);

    // Create a new socket
    ret = sock_create(sk->sk->sk_family, sk->type, sk->sk->sk_protocol, &new_sk);
    if (ret < 0 || !new_sk)
        return NULL;

    new_sk->type = sk->type;
    new_sk->ops = sk->ops;

    // Accept connection
    ret = sk->ops->accept(sk, new_sk, 0 /*sk->file->f_flags*/);
    if (ret < 0) {
        sock_release(new_sk);
        return NULL;
    }

    // Retrieve peer address if requested
    if (address) {
        ret = new_sk->ops->getname(new_sk, address, 1);
        if (ret < 0) {
            sock_release(new_sk);
            return NULL;
        }
    }

    return new_sk;
}

ssize_t krecv(ksocket_t socket, void *buffer, size_t length, int flags) {
    struct socket *sk = (struct socket *)socket;
    struct msghdr msg = { 0 };
    struct kvec iov;

    iov.iov_base = buffer;
    iov.iov_len = length;

    return kernel_recvmsg(sk, &msg, &iov, 1, length, flags);
}

ssize_t ksend(ksocket_t socket, const void *buffer, size_t length, int flags) {
	struct socket *sk;
	struct msghdr msg = {0};
	struct kvec iov;
	int len;

	sk = (struct socket *)socket;

	iov.iov_base = (void *)buffer;
	iov.iov_len = length;

	len = kernel_sendmsg(sk, &msg, &iov, 1, length);
	return len;
}

int kshutdown(ksocket_t socket, int how) {
	struct socket *sk;
	int ret = 0;

	sk = (struct socket *)socket;
	if (sk) {
		ret = sk->ops->shutdown(sk, how);
	}
	return ret;
}

//TODO: ?
int kclose(ksocket_t socket) {
	struct socket *sk;
	int ret;

	sk = (struct socket *)socket;
	ret = sk->ops->release(sk);

	if (sk) {
		sock_release(sk);
        }
	return ret;
}

ssize_t krecvfrom(ksocket_t socket, void *buffer, size_t length,
                  int flags, struct sockaddr *address, int *address_len) {
	struct socket *sk;
	struct msghdr msg = {0};
	struct kvec iov;
	int ret;

	sk = (struct socket *)socket;

	// Set up kvec for kernel buffer
	iov.iov_base = buffer;
	iov.iov_len = length;

	// Set the msg_name and msg_namelen if address is requested
	if (address && address_len) {
		msg.msg_name = address;
		msg.msg_namelen = *address_len;
	}

	ret = kernel_recvmsg(sk, &msg, &iov, 1, length, flags);

	// Update actual received address length
	if (ret >= 0 && address_len && msg.msg_namelen > 0) {
		*address_len = msg.msg_namelen;
	}
	return ret;
}

ssize_t ksendto(ksocket_t socket, void *message, size_t length,
                int flags, const struct sockaddr *dest_addr, int dest_len) {
	struct socket *sk = (struct socket *)socket;
	struct msghdr msg = {0};
	struct kvec iov;
	int ret;

	// Set up kvec for kernel-safe buffer
	iov.iov_base = message;
	iov.iov_len = length;

	// Populate destination if provided
	if (dest_addr) {
		msg.msg_name = (void *)dest_addr;
		msg.msg_namelen = dest_len;
	}

	msg.msg_flags = flags;

	// Use kernel_sendmsg for modern compatibility
	ret = kernel_sendmsg(sk, &msg, &iov, 1, length);
	return ret;
}

int kgetsockname(ksocket_t socket, struct sockaddr *address, int *address_len) {
	struct socket *sk = (struct socket *)socket;
	struct sockaddr_storage addr;
	int ret;

	ret = kernel_getsockname(sk, (struct sockaddr *)&addr);
	if (ret < 0) {
		return ret;
	}
	if (address) {
		memcpy(address, &addr, sizeof(addr));
		if (address_len) {
			*address_len = sizeof(addr);
		}
	}

	return 0;
}

int kgetpeername(ksocket_t socket, struct sockaddr *address, int *address_len) {
	struct socket *sk = (struct socket *)socket;
	struct sockaddr_storage addr;
	int ret;

	ret = kernel_getpeername(sk, (struct sockaddr *)&addr);
	if (ret < 0) {
		return ret;
	}
	if (address) {
		memcpy(address, &addr, sizeof(addr));
		if (address_len)
			*address_len = sizeof(addr);
	}

	return 0;
}

int ksetsockopt(ksocket_t socket, int level, int optname, void *optval, int optlen) {
	struct socket *sk = (struct socket *)socket;
	sockptr_t opt_ptr;
	int ret;

	// Wrap kernel pointer as a safe sockptr_t
	opt_ptr = KERNEL_SOCKPTR(optval);

	if (level == SOL_SOCKET) {
		ret = sock_setsockopt(sk, level, optname, opt_ptr, optlen);
	}
	else {
		ret = sk->ops->setsockopt(sk, level, optname, opt_ptr, optlen);
	}
	return ret;
}

int kgetsockopt(ksocket_t socket, int level, int optname, void *optval, int *optlen) {
	struct socket *sk = (struct socket *)socket;
	int ret;

	if (!sk || !sk->ops || !sk->ops->getsockopt) {
		return -ENOSYS;
	}
	ret = sk->ops->getsockopt(sk, level, optname, (char *)optval, optlen);

	return ret;
}

//helper functions
unsigned int inet_addr(char* ip) {
	int a, b, c, d;
	char addr[4];
	
	sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
	addr[0] = a;
	addr[1] = b;
	addr[2] = c;
	addr[3] = d;
	
	return *(unsigned int *)addr;
}

char *inet_ntoa(struct in_addr *in) {
	char* str_ip = NULL;
	u_int32_t int_ip = 0;
	
	str_ip = kmalloc(16 * sizeof(char), GFP_KERNEL);
	if (!str_ip) {
		return NULL;
	}
	else {
		memset(str_ip, 0, 16);
	}
	int_ip = in->s_addr;
	
	sprintf(str_ip, "%d.%d.%d.%d",  (int_ip      ) & 0xFF,
					(int_ip >> 8 ) & 0xFF,
					(int_ip >> 16) & 0xFF,
					(int_ip >> 24) & 0xFF);
	return str_ip;
}

//module init and cleanup procedure
static int ksocket_init(void) {
	printk("%s version %s\n%s\n%s\n", 
		KSOCKET_NAME, KSOCKET_VERSION,
		KSOCKET_DESCPT, KSOCKET_AUTHOR);

	return 0;
}

static void ksocket_exit(void) {
	printk("ksocket exit\n");
}

module_init(ksocket_init);
module_exit(ksocket_exit);

EXPORT_SYMBOL(ksocket);
EXPORT_SYMBOL(kbind);
EXPORT_SYMBOL(klisten);
EXPORT_SYMBOL(kconnect);
EXPORT_SYMBOL(kaccept);
EXPORT_SYMBOL(krecv);
EXPORT_SYMBOL(ksend);
EXPORT_SYMBOL(kshutdown);
EXPORT_SYMBOL(kclose);
EXPORT_SYMBOL(krecvfrom);
EXPORT_SYMBOL(ksendto);
EXPORT_SYMBOL(kgetsockname);
EXPORT_SYMBOL(kgetpeername);
EXPORT_SYMBOL(ksetsockopt);
EXPORT_SYMBOL(kgetsockopt);
EXPORT_SYMBOL(inet_addr);
EXPORT_SYMBOL(inet_ntoa);
