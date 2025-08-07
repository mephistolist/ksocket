#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    const char *ip = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? atoi(argv[2]) : 12345;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv = {0};

    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        return 1;
    }

    char *msg = "Hello from user-space TCP client!";
    send(sock, msg, strlen(msg), 0);
    printf("Sent message to %s:%d\n", ip, port);

    close(sock);
    return 0;
}
