#include "message_slot.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int fd;
    unsigned int channel;
    char buffer[MAX_MSG_LEN];
    ssize_t n;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device> <channel>\n", argv[0]);
        return 1;
    }

    channel = atoi(argv[2]);

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, MSG_SLOT_CHANNEL, channel) < 0) {
        perror("ioctl MSG_SLOT_CHANNEL");
        close(fd);
        return 1;
    }

    n = read(fd, buffer, MAX_MSG_LEN);
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    write(STDOUT_FILENO, buffer, n);

    close(fd);
    return 0;
}
