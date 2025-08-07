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
 * Updated for kernels 5.11-6.16
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/net.h>
#include <linux/in.h>
#include "ksocket.h"

extern unsigned int inet_addr(char *ip);

static struct task_struct *server_thread;
static ksocket_t sockfd = NULL;  // now global so we can access it in exit()

static int tcp_server_fn(void *data) {
    ksocket_t client_sock;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    char buf[128];
    int len;

    sockfd = ksocket(AF_INET, SOCK_STREAM, 0);
    if (!sockfd) {
        printk(KERN_ERR "ksocket: Failed to create TCP socket\n");
        return -1;
    }

    int optval = 1;
    ksetsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(12345);

    if (kbind(sockfd, (struct sockaddr *)&server_addr, addr_len) < 0) {
        printk(KERN_ERR "ksocket: bind failed\n");
        kclose(sockfd);
        return -1;
    }

    if (klisten(sockfd, 5) < 0) {
        printk(KERN_ERR "ksocket: listen failed\n");
        kclose(sockfd);
        return -1;
    }

    printk(KERN_INFO "ksocket: TCP server listening on port 12345...\n");

    while (!kthread_should_stop()) {
        client_sock = kaccept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (!client_sock) {
            if (kthread_should_stop())
                break;

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

    if (sockfd) {
        kclose(sockfd);
        sockfd = NULL;
    }

    return 0;
}

static int __init tcp_server_init(void) {
    printk(KERN_INFO "ksocket: Loading TCP server module...\n");
    server_thread = kthread_run(tcp_server_fn, NULL, "tcp_server_thread");
    return 0;
}

static void __exit tcp_server_exit(void) {
    printk(KERN_INFO "ksocket: Unloading TCP server module...\n");

    if (server_thread) {
        if (sockfd) {
            kshutdown(sockfd, 2);  // SHUT_RDWR: stop reads and writes
        }

        kthread_stop(server_thread);
        server_thread = NULL;
    }
}

module_init(tcp_server_init);
module_exit(tcp_server_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple TCP Server Example");
MODULE_AUTHOR("Mephistolist");
