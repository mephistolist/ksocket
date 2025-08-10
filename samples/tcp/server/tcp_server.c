#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/uio.h>

#define SERVER_PORT 12345
#define BACKLOG     5
#define RECV_BUF_SZ 1024

static struct task_struct *server_thread;
static struct socket *listen_sock;

static int tcp_server_thread(void *data) {
    struct socket *newsock = NULL;
    struct sockaddr_in addr;
    int ret;

    pr_info("tcp_server: thread starting\n");

    /* Create kernel socket properly for kernel context */
    ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &listen_sock);
    if (ret) {
        pr_err("tcp_server: sock_create_kern failed: %d\n", ret);
        listen_sock = NULL;
        return ret;
    }
    pr_debug("tcp_server: created listen_sock=%p\n", listen_sock);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    ret = kernel_bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret) {
        pr_err("tcp_server: kernel_bind failed: %d\n", ret);
        goto out_release_listen;
    }

    ret = kernel_listen(listen_sock, BACKLOG);
    if (ret) {
        pr_err("tcp_server: kernel_listen failed: %d\n", ret);
        goto out_release_listen;
    }

    pr_info("tcp_server: listening on port %d\n", SERVER_PORT);

    /* main accept loop (will exit when thread asked to stop) */
    while (!kthread_should_stop()) {
        /* kernel_accept may block; that's fine in this thread context */
        ret = kernel_accept(listen_sock, &newsock, 0);
        if (ret < 0) {
            /* If interrupted or shutting down, break cleanly */
            if (ret == -EINTR || ret == -ERESTARTSYS || kthread_should_stop()) {
                pr_info("tcp_server: accept interrupted or thread stopping: %d\n", ret);
                break;
            }
            pr_warn("tcp_server: kernel_accept returned: %d - sleeping then retry\n", ret);
            msleep(100);
            continue;
        }

        if (!newsock) {
            pr_warn("tcp_server: kernel_accept returned NULL newsock\n");
            msleep(100);
            continue;
        }

        pr_info("tcp_server: accepted newsock=%p newsock->sk=%p\n", newsock, newsock->sk);

        /* Receive a single message (blocking) and then close */
        {
            char *buf = kmalloc(RECV_BUF_SZ, GFP_KERNEL);
            if (!buf) {
                pr_err("tcp_server: kmalloc failed\n");
                sock_release(newsock);
                newsock = NULL;
                continue;
            }

            {
                struct kvec iov = { .iov_base = buf, .iov_len = RECV_BUF_SZ - 1 };
                struct msghdr msg;
                int n;

                memset(&msg, 0, sizeof(msg));
                n = kernel_recvmsg(newsock, &msg, &iov, 1, RECV_BUF_SZ - 1, 0);
                if (n > 0) {
                    buf[n] = '\0';
                    pr_info("tcp_server: received (%d bytes): %s\n", n, buf);
                } else {
                    pr_info("tcp_server: kernel_recvmsg returned %d\n", n);
                }
            }

            kfree(buf);
        }

        /* close connection */
        sock_release(newsock);
        newsock = NULL;
    }

out_release_listen:
    if (listen_sock) {
        /* release listening socket */
        pr_info("tcp_server: closing listen socket %p\n", listen_sock);
        sock_release(listen_sock);
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

    /* Shutdown listening socket to interrupt accept */
    if (listen_sock) {
        kernel_sock_shutdown(listen_sock, SHUT_RDWR);
    }

    /* Stop the thread (waits for it to exit) */
    if (server_thread) {
        kthread_stop(server_thread);
        server_thread = NULL;
    }

    /* If still present, release listen socket (thread may have done it) */
    if (listen_sock) {
        sock_release(listen_sock);
        listen_sock = NULL;
    }

    pr_info("tcp_server: Unloaded\n");
}

module_init(tcp_server_init);
module_exit(tcp_server_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mephistolist");
MODULE_DESCRIPTION("Kernel-space TCP server using ksocket");
