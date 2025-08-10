#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>  
#include <linux/net.h>
#include <linux/in.h>
#include "ksocket.h"

static struct task_struct *server_thread;
static ksocket_t sockfd = NULL;  /* global so exit can access it */

/* TCP server thread function */
static int tcp_server_fn(void *data) {
    ksocket_t client_sock;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    char buf[128];
    int len;

    sockfd = ksocket(AF_INET, SOCK_STREAM, 0);
    if (IS_ERR(sockfd)) {
        long err = PTR_ERR(sockfd);
        printk(KERN_ERR "ksocket: ksocket() failed: %ld\n", err);
        sockfd = NULL;
        return (int)err;
    }

    /* allow immediate reuse of the port */
    {
        int optval = 1;
        ksetsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(12345);

    if (kbind(sockfd, (struct sockaddr *)&server_addr, addr_len) < 0) {
        printk(KERN_ERR "ksocket: bind failed\n");
        kclose(sockfd);
        sockfd = NULL;
        return -EINVAL;
    }

    if (klisten(sockfd, 5) < 0) {
        printk(KERN_ERR "ksocket: listen failed\n");
        kclose(sockfd);
        sockfd = NULL;
        return -EINVAL;
    }

    printk(KERN_INFO "ksocket: TCP server listening on port 12345...\n");

    while (!kthread_should_stop()) {
        if (!sockfd)
            break;

	client_sock = kaccept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
	if (IS_ERR(client_sock)) {
	   long err = PTR_ERR(client_sock);
	   /* if interrupted or shutdown, will get -ERESTARTSYS/-EINTR/-ECONNABORTED etc */
	   if (kthread_should_stop()) {
	       break;
	   }	
	   printk(KERN_DEBUG "ksocket: kaccept() returned err %ld\n", err);
	   msleep(100);
	   continue;
	}

        memset(buf, 0, sizeof(buf));
        len = krecv(client_sock, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';
            printk(KERN_INFO "ksocket: Received: %s\n", buf);
        }

        kclose(client_sock);
    }

    /* clean up listen socket if still present */
    if (sockfd) {
        kclose(sockfd);
        sockfd = NULL;
    }

    return 0;
}

/* module init: start the server thread */
static int __init tcp_server_init(void) {
    printk(KERN_INFO "ksocket: Loading TCP server module...\n");

    server_thread = kthread_run(tcp_server_fn, NULL, "tcp_server_thread");
    if (IS_ERR(server_thread)) {
        printk(KERN_ERR "ksocket: Failed to start server thread\n");
        server_thread = NULL;
        return PTR_ERR(server_thread);
    }

    return 0;
}

/* module exit: shutdown socket (to unblock accept) and stop thread */
static void __exit tcp_server_exit(void) {
    printk(KERN_INFO "ksocket: Unloading TCP server module...\n");

    /* Ask the socket to shutdown so blocking accept/recv is interrupted */
    if (sockfd) {
        kshutdown(sockfd, 2); /* SHUT_RDWR */
    }

    /* Stop the thread (waits for it to exit) */
    if (server_thread) {
        kthread_stop(server_thread);
        server_thread = NULL;
    }

    /* If socket wasn't closed by thread, close it here */
    if (sockfd) {
        kclose(sockfd);
        sockfd = NULL;
    }
}

module_init(tcp_server_init);
module_exit(tcp_server_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple TCP Server Example");
MODULE_AUTHOR("Mephistolist");
