#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define SERVER_PORT 4444
#define SERVER_IP   "127.0.0.1"
#define TIMEOUT_SEC 2

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[256];
    const char *message = "Hello UDP Server";

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[client] Sent: %s\n", message);

    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(server_addr);
    int len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                       (struct sockaddr *)&server_addr, &addr_len);
    if (len < 0) {
        perror("[client] No response from server (timeout or unreachable)");
    } else {
        buffer[len] = '\0';
        printf("[client] Received: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
