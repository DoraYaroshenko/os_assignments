#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235

#define MSG_SLOT_CHANNEL  _IOW(MAJOR_NUM, 0, unsigned int)
#define MSG_SLOT_SET_CEN  _IOW(MAJOR_NUM, 1, unsigned int)

#define MAX_MSG_LEN 128

#endif
