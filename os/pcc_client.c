#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int createSocketAndConnect(const char *ip, const char *port)
{
    struct sockaddr_in server_adr;
    socklen_t addrsize = sizeof(server_adr);
    memset(&server_adr, 0, addrsize);

    server_adr.sin_family = AF_INET;
    server_adr.sin_port = htons(atoi(port));

    if (inet_pton(AF_INET, ip, &server_adr.sin_addr) != 1)
    {
        perror("inet_pton");
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Client failed to create socket");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&server_adr, addrsize) < 0)
    {
        perror("Client failed to connect");
        exit(1);
    }

    return sockfd;
}

unsigned int getFileSize(const char *path)
{
    unsigned int fileSize;
    struct stat st;
    if (stat(path, &st) == 0)
    {
        fileSize = st.st_size;
    }
    else
    {
        perror("Client failed to get file size");
        exit(1);
    }
    return fileSize;
}

void sendFileSize(unsigned int fileSize, int sockfd)
{
    unsigned int fileSizeInNetwork = htonl(fileSize);
    if (send(sockfd, (void *)&fileSizeInNetwork, 4, 0) < 0)
    {
        perror("Client failed to send the file size");
        exit(1);
    }
}

void sendFile(int fd, char *buffer, unsigned int bufferSize, int sockfd)
{
    int readBytes;
    while ((readBytes = read(fd, buffer, bufferSize)) > 0)
    {
        if (send(sockfd, (void *)buffer, readBytes, 0) < 0)
        {
            perror("Client failed to send the file size");
            exit(1);
        }
    }
    if (readBytes < 0)
    {
        perror("Client failed reading file");
        exit(1);
    }
    if (close(fd) < 0)
    {
        perror("Client failed closing file");
        exit(1);
    }
}

void receiveAndPrintCharacters(int sockfd)
{
    unsigned int printable;
    if (recv(sockfd, (void *)&printable, 4, 0) < 0)
    {
        perror("Client failed to receive the number of printable characters");
        exit(1);
    }
    printable = ntohl(printable);
    printf("# of printable characters: %u\n", printable);
}

void checkValidArgs(int argc)
{
    if (argc != 4)
    {
        printf("Invalid number of arguments");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    checkValidArgs(argc);

    int fd = open(argv[3], O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening file");
        exit(1);
    }

    unsigned int fileSize = getFileSize(argv[3]);
    int bufferSize = 1024 * 1024;
    char *buffer = (char *)malloc(bufferSize);

    int sockfd = createSocketAndConnect(argv[1], argv[2]);
    sendFileSize(fileSize, sockfd);
    sendFile(fd, buffer, bufferSize, sockfd);
    receiveAndPrintCharacters(sockfd);

    if (close(sockfd) < 0)
    {
        perror("Client failed closing socket");
        exit(1);
    };
    free(buffer);
    exit(0);
}
