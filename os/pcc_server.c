#define _GNU_SOURCE
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

__sig_atomic_t done_flag;

void handleSigint(int signo)
{
    done_flag = 1;
}

unsigned int receiveFileSize(int connfd)
{
    unsigned int fileSize;
    if (recv(connfd, (void *)&fileSize, 4, 0) < 0)
    {
        perror("Server failed to receive file size");
        exit(1);
    }
    fileSize = ntohl(fileSize);
    return fileSize;
}

// counts printable characters sent by this connection
int count(ssize_t received, char *buffer, unsigned int *connectionPrintable)
{
    unsigned int charactersInConnection = 0;
    for (int j = 0; j < received; j++)
    {
        if (buffer[j] >= 32 && buffer[j] <= 126)
        {
            connectionPrintable[buffer[j] - 32]++;
            charactersInConnection++;
        }
    }
    return charactersInConnection;
}

void updateTotal(unsigned int *connectionPrintable, unsigned int *total)
{
    for (int i = 0; i < 95; i++)
    {
        total[i] += connectionPrintable[i];
    }
}

int readAndCount(int connfd, char *buffer, unsigned int bufferSize, unsigned int fileSize, unsigned int *connectionPrintable)
{
    unsigned int bytesRead = 0;
    unsigned int charactersInConnection = 0;

    while (bytesRead < fileSize)
    {
        ssize_t received = recv(connfd, buffer, bufferSize, 0);

        if (received == 0)
        {
            fprintf(stderr, "Client closed connection unexpectedly\n");
            return -1;
        }

        if (received < 0)
        {
            if (errno == ECONNRESET || errno == EPIPE || errno == ETIMEDOUT)
            {
                perror("TCP error while receiving data");
                return -1;
            }
            perror("recv failed");
            return -1;
        }

        charactersInConnection += count(received, buffer, connectionPrintable);
        bytesRead += received;
    }

    return charactersInConnection;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Invalid number of arguments");
    }
    struct sigaction newAction = {.sa_handler = handleSigint};
    sigemptyset(&newAction.sa_mask);
    if (sigaction(SIGINT, &newAction, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    unsigned int total[95] = {0};
    unsigned int clientsServed = 0;

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    socklen_t addrsize = sizeof(struct sockaddr_in);

    int bufferSize = 1024 * 1024;
    char *buffer = (char *)malloc(bufferSize);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        perror("socket");
        exit(1);
    }
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (0 != bind(listenfd, (struct sockaddr *)&serv_addr, addrsize))
    {
        perror("Server failed to bind");
        exit(1);
    }

    if (0 != listen(listenfd, 10))
    {
        perror("Server failed to listen");
        exit(1);
    }

    while (!done_flag)
    {
        int connfd = accept(listenfd, (struct sockaddr *)&peer_addr, &addrsize);
        if (connfd < 0)
        {
            if (errno == EINTR)
                break;
            perror("Server failed to accept connection");
            exit(1);
        }
        unsigned int fileSize = receiveFileSize(connfd);
        unsigned int connectionPrintable[95] = {0};
        int charactersInConnection = readAndCount(connfd, buffer, bufferSize, fileSize, connectionPrintable);
        if (charactersInConnection >= 0)
        {
            unsigned int netCount = htonl((unsigned int)charactersInConnection);
            if (send(connfd, &netCount, 4, 0) < 0)
            {
                perror("Server failed to send count");
                close(connfd);
                continue;
            }
            updateTotal(connectionPrintable, total);
            clientsServed++;
        }
        close(connfd);
    }
    for (int i = 0; i < 95; i++)
    {
        if (total[i] > 0)
            printf("char '%c' : %u times\n", (char)(i + 32), total[i]);
    }
    printf("Served %u client(s) successfully\n", clientsServed);
    free(buffer);
    close(listenfd);
}
