#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "ksocket.h"

int udp_server_fn(void *data);
static struct task_struct *server_thread;

int udp_server_fn(void *data) {
    struct socket *sock;
    struct sockaddr_in addr;
    int ret;
    char buffer[256];
    
    sock = ksocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!sock) {
        printk(KERN_ERR "UDP Server: Failed to create socket\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(4444);

    ret = kbind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printk(KERN_ERR "UDP Server: Bind failed (%d)\n", ret);
        kclose(sock);
        return ret;
    }

    printk(KERN_INFO "UDP Server: Listening on port 4444\n");

    while (!kthread_should_stop()) {
        struct sockaddr_in src_addr;
        int len = sizeof(src_addr);

        memset(buffer, 0, sizeof(buffer));
        ret = krecvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                        (struct sockaddr *)&src_addr, &len);
        if (ret > 0) {
            buffer[ret] = '\0';
            printk(KERN_INFO "UDP Server: Received '%s'\n", buffer);

            // Send back acknowledgement
            const char *reply = "ACK from UDP Server";
	   ksendto(sock, (void *)reply, strlen(reply), 0,
	       (struct sockaddr *)&src_addr, sizeof(src_addr));
        }
        msleep(100);
    }

    kclose(sock);
    return 0;
}

static int __init udp_server_init(void) {
    server_thread = kthread_run(udp_server_fn, NULL, "udp_server");
    if (IS_ERR(server_thread)) {
        printk(KERN_ERR "UDP Server: Failed to create thread\n");
        return PTR_ERR(server_thread);
    }
    return 0;
}

static void __exit udp_server_exit(void) {
    if (server_thread)
        kthread_stop(server_thread);
    printk(KERN_INFO "UDP Server: Exited\n");
}

module_init(udp_server_init);
module_exit(udp_server_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple UDP Server Example with ACK");
MODULE_AUTHOR("Mephistolist");
