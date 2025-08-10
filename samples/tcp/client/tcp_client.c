#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include "ksocket.h"

#define DEFAULT_IP   "127.0.0.1"
#define DEFAULT_PORT 12345

static struct task_struct *client_thread;

static int tcp_client_fn(void *data) {
    struct socket *sock;
    struct sockaddr_in server_addr;
    char *ip = (char *)data ? (char *)data : DEFAULT_IP;
    int port = DEFAULT_PORT;
    const char *message = "Hello from kernel TCP client!";
    int ret;

    // Create TCP socket
    sock = ksocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!sock) {
        printk(KERN_ERR "[tcp_client] Failed to create socket\n");
        return -ENOMEM;
    }

    // Prepare server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Connect to server
    ret = kconnect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        printk(KERN_ERR "[tcp_client] Failed to connect to %s:%d (err=%d)\n", ip, port, ret);
        kclose(sock);
        return ret;
    }

    printk(KERN_INFO "[tcp_client] Connected to %s:%d\n", ip, port);

    // Send message
    ret = ksend(sock, (void *)message, strlen(message), 0);
    if (ret < 0) {
        printk(KERN_ERR "[tcp_client] Failed to send message (err=%d)\n", ret);
        kclose(sock);
        return ret;
    }

    printk(KERN_INFO "[tcp_client] Sent: %s\n", message);

    kclose(sock);
    return 0;
}

static int __init tcp_client_init(void) {
    printk(KERN_INFO "[tcp_client] Initializing\n");
    client_thread = kthread_run(tcp_client_fn, NULL, "tcp_client");
    if (IS_ERR(client_thread)) {
        printk(KERN_ERR "[tcp_client] Failed to create thread\n");
        return PTR_ERR(client_thread);
    }
    return 0;
}

static void __exit tcp_client_exit(void) {
    if (client_thread)
        kthread_stop(client_thread);
    printk(KERN_INFO "[tcp_client] Exited\n");
}

module_init(tcp_client_init);
module_exit(tcp_client_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mephistolist");
MODULE_DESCRIPTION("Simple TCP Client using ksocket");
