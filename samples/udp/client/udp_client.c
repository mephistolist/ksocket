#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <linux/string.h>
#include <linux/net.h>     
#include <linux/socket.h>  
#include "ksocket.h"

#define SERVER_PORT 4444
#define SERVER_IP   "127.0.0.1"
#define TIMEOUT_MS  2000  // 2 seconds

static struct task_struct *client_thread;

static int udp_client_fn(void *data) {
    struct socket *sock;
    struct sockaddr_in server_addr;
    char buffer[256];
    const char *message = "Hello UDP Server";
    int ret;

    sock = ksocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!sock) {
        printk(KERN_ERR "[udp_client] Failed to create socket\n");
        return -ENOMEM;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); // <- use your exported symbol

    // Send message
    ret = ksendto(sock, (void *)message, strlen(message), 0,
                  (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        printk(KERN_ERR "[udp_client] Failed to send message (%d)\n", ret);
        kclose(sock);
        return ret;
    }

    printk(KERN_INFO "[udp_client] Sent: %s\n", message);

    // Try receive (non-blocking with manual timeout loop)
    {
        unsigned long timeout = jiffies + msecs_to_jiffies(TIMEOUT_MS);
        int addr_len = sizeof(server_addr);
        ret = -EAGAIN;

        while (time_before(jiffies, timeout) && !kthread_should_stop()) {
            ret = krecvfrom(sock, buffer, sizeof(buffer) - 1, MSG_DONTWAIT,
                            (struct sockaddr *)&server_addr, &addr_len);
            if (ret >= 0) {
                buffer[ret] = '\0';
                printk(KERN_INFO "[udp_client] Received: %s\n", buffer);
                break;
            }
            msleep(50);
        }

        if (ret < 0)
            printk(KERN_WARNING "[udp_client] No response from server (timeout)\n");
    }

    kclose(sock);
    return 0;
}

static int __init udp_client_init(void) {
    printk(KERN_INFO "[udp_client] Initializing\n");
    client_thread = kthread_run(udp_client_fn, NULL, "udp_client");
    if (IS_ERR(client_thread)) {
        printk(KERN_ERR "[udp_client] Failed to create thread\n");
        return PTR_ERR(client_thread);
    }
    return 0;
}

static void __exit udp_client_exit(void) {
    if (client_thread)
        kthread_stop(client_thread);
    printk(KERN_INFO "[udp_client] Exited\n");
}

module_init(udp_client_init);
module_exit(udp_client_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mephistolist");
MODULE_DESCRIPTION("Simple UDP Client using ksocket API");
