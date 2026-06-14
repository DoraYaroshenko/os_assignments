#include "message_slot.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    int fd;
    unsigned int channel_id;
    unsigned int censorship_mode;
    int len;

    if (argc != 5)
        return 1;

    fd = open(argv[1], O_WRONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    channel_id = atoi(argv[2]);
    censorship_mode = atoi(argv[3]);
    len = strlen(argv[4]);

    if (ioctl(fd, MSG_SLOT_SET_CEN, censorship_mode) < 0 ||
        ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0 ||
        write(fd, argv[4], len) < 0)
    {
        perror("operation failed");
        close(fd);
        exit(1);
    }
    close(fd);
    return 0;
}