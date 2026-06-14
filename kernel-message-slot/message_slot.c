#include "message_slot.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define MAX_SLOTS 256
#define SUCCESS 0

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dora Yaroshenko");
MODULE_DESCRIPTION("Message Slot Kernel Module");

struct channel
{
    unsigned int id;
    char message[MAX_MSG_LEN];
    size_t len;
    struct channel *next;
};

struct slot
{
    struct channel *head;
};

struct fd_state
{
    unsigned int channel_id;
    unsigned int censorship_mode;
};

static struct slot *slots[MAX_SLOTS];
static int major;

static struct channel *find_channel(struct slot *s, unsigned int id)
{
    struct channel *c = s->head;
    while (c)
    {
        if (c->id == id)
            return c;
        c = c->next;
    }
    return NULL;
}

static int device_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);

    if (!slots[minor])
    {
        slots[minor] = kmalloc(sizeof(struct slot), GFP_KERNEL);
        if (!slots[minor])
            return -ENOMEM;
        slots[minor]->head = NULL;
    }

    file->private_data = kmalloc(sizeof(struct fd_state), GFP_KERNEL);
    if (!file->private_data)
        return -ENOMEM;

    ((struct fd_state *)file->private_data)->channel_id = 0;
    ((struct fd_state *)file->private_data)->censorship_mode = 0;
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    return SUCCESS;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct fd_state *state = file->private_data;

    if (cmd == MSG_SLOT_CHANNEL)
    {
        if (arg == 0)
            return -EINVAL;
        state->channel_id = arg;
        return SUCCESS;
    }
    if (cmd == MSG_SLOT_SET_CEN)
    {
        state->censorship_mode = (int)arg;
        return SUCCESS;
    }

    return -EINVAL;
}

static ssize_t device_write(struct file *file,
                            const char __user *buf,
                            size_t len,
                            loff_t *off)
{
    struct fd_state *state = file->private_data;
    struct slot *s;
    struct channel *c;
    char ch;
    size_t i;

    if (state->channel_id == 0)
        return -EINVAL;
    if (len == 0 || len > MAX_MSG_LEN)
        return -EMSGSIZE;

    s = slots[iminor(file->f_inode)];
    c = find_channel(s, state->channel_id);

    if (!c)
    {
        c = kmalloc(sizeof(struct channel), GFP_KERNEL);
        if (!c)
            return -ENOMEM;
        c->id = state->channel_id;
        c->next = s->head;
        s->head = c;
    }

    for (i = 0; i < len; i++)
    {
        if (get_user(ch, &buf[i]))
            return -EFAULT;

        if (state->censorship_mode == 1 && (i % 4 == 3))
        {
            ch = '#';
        }

        c->message[i] = ch;
    }

    c->len = len;
    return len;
}

static ssize_t device_read(struct file *file,
                           char __user *buf,
                           size_t len,
                           loff_t *off)
{
    struct fd_state *state = file->private_data;
    struct channel *c;
    size_t i;

    if (state->channel_id == 0)
        return -EINVAL;

    c = find_channel(slots[iminor(file->f_inode)], state->channel_id);
    if (!c)
        return -EWOULDBLOCK;
    if (len < c->len)
        return -ENOSPC;

    for (i = 0; i < c->len; i++)
    {
        if (put_user(c->message[i], &buf[i]))
            return -EFAULT;
    }

    return c->len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
};

static int __init message_slot_init(void)
{
    major = register_chrdev(MAJOR_NUM, "message_slot", &fops);
    if (major < 0)
    {
        printk(KERN_ALERT "Failed to register device\n");
        return major;
    }
    printk(KERN_ALERT "Message slot loaded, major=%d\n", MAJOR_NUM);
    return 0;
}

static void __exit message_slot_exit(void)
{
    int i;
    struct channel *c, *next_c;
    for (i = 0; i < MAX_SLOTS; i++)
    {
        if (slots[i])
        {
            c = slots[i]->head;
            while (c)
            {
                next_c = c->next;
                kfree(c);
                c = next_c;
            }
            kfree(slots[i]);
        }
    }

    unregister_chrdev(MAJOR_NUM, "message_slot");
    printk(KERN_ALERT "Message slot unloaded\n");
}

module_init(message_slot_init);
module_exit(message_slot_exit);
