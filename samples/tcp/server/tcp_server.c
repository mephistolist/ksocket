#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/in.h>
#include <linux/net.h>     
#include <linux/socket.h>   
#include "ksocket.h"   

#define SERVER_PORT 12345
#define BACKLOG     5
#define RECV_BUF_SZ 1024

static struct task_struct *server_thread;
static ksocket_t listen_sock = NULL;

static int tcp_server_thread(void *data) {
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    int ret;

    pr_info("tcp_server: thread starting (ksocket API)\n");

    /* Create listening socket via ksocket() */
    listen_sock = ksocket(AF_INET, SOCK_STREAM, 0);
    if (IS_ERR(listen_sock)) {
        long err = PTR_ERR(listen_sock);
        pr_err("tcp_server: ksocket() failed: %ld\n", err);
        listen_sock = NULL;
        return (int)err;
    }

    /* (Optional) allow immediate port reuse */
    {
        int optval = 1;
        ksetsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
                    (void *)&optval, sizeof(optval));
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(SERVER_PORT);

    ret = kbind(listen_sock, (struct sockaddr *)&addr, addrlen);
    if (ret < 0) {
        pr_err("tcp_server: kbind() failed: %d\n", ret);
        kclose(listen_sock);
        listen_sock = NULL;
        return ret;
    }

    ret = klisten(listen_sock, BACKLOG);
    if (ret < 0) {
        pr_err("tcp_server: klisten() failed: %d\n", ret);
        kclose(listen_sock);
        listen_sock = NULL;
        return ret;
    }

    pr_info("tcp_server: listening on port %d\n", SERVER_PORT);

    /* Accept loop */
    while (!kthread_should_stop()) {
        ksocket_t client = NULL;
        struct sockaddr_in peer;
        int peerlen = sizeof(peer);
        int n;
        char *buf;

        /* Accept (blocking) */
        client = kaccept(listen_sock, (struct sockaddr *)&peer, &peerlen);
        if (IS_ERR_OR_NULL(client)) {
            long err = IS_ERR(client) ? PTR_ERR(client) : -ENOTCONN;

            /* Exit cleanly if we’re stopping */
            if (kthread_should_stop())
                break;

            /* Transient failure: back off a bit and retry */
            pr_debug("tcp_server: kaccept() error ptr=%p err=%ld; retrying\n",
                     client, err);
            msleep(100);
            continue;
        }

        pr_info("tcp_server: accepted client=%p\n", client);

        /* One recv then close (demo behavior) */
        buf = kmalloc(RECV_BUF_SZ, GFP_KERNEL);
        if (!buf) {
            pr_err("tcp_server: kmalloc failed\n");
            kclose(client);
            continue;
        }

        n = krecv(client, buf, RECV_BUF_SZ - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            pr_info("tcp_server: received (%d bytes): %s\n", n, buf);
        } else {
            pr_info("tcp_server: krecv() returned %d\n", n);
        }

        kfree(buf);
        kclose(client);
    }

    /* Cleanup listening socket */
    if (listen_sock) {
        pr_info("tcp_server: closing listen socket %p\n", listen_sock);
        kclose(listen_sock);
        listen_sock = NULL;
    }

    pr_info("tcp_server: thread exiting\n");
    return 0;
}

static int __init tcp_server_init(void) {
    pr_info("tcp_server: Loading (starting thread)\n");
    server_thread = kthread_run(tcp_server_thread, NULL, "tcp_server_thread");
    if (IS_ERR(server_thread)) {
        int err = PTR_ERR(server_thread);
        pr_err("tcp_server: kthread_run failed: %d\n", err);
        server_thread = NULL;
        return err;
    }
    return 0;
}

static void __exit tcp_server_exit(void) {
    pr_info("tcp_server: Unloading\n");

    /* Ask the listen socket to shut down so accept/recv unblocks */
    if (listen_sock) {
        kshutdown(listen_sock, 2 /* SHUT_RDWR */);
    }

    if (server_thread) {
        kthread_stop(server_thread);
        server_thread = NULL;
    }

    if (listen_sock) {
        kclose(listen_sock);
        listen_sock = NULL;
    }

    pr_info("tcp_server: Unloaded\n");
}

module_init(tcp_server_init);
module_exit(tcp_server_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mephistolist");
MODULE_DESCRIPTION("Kernel-space TCP server using ksocket wrapper API");
