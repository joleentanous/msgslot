
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>  


#include "message_slot.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Message Slot Module Implementation");

file_data* devices[256];

/*
file_data* find_file(unsigned int minor) {
    int i;
    for (i = 0; i < MAX_MSG_LEN; i++) {
        if (devices[i]->minor == minor) {
            return devices[i];
        }
    }
    return NULL;
}
*/

static int device_open(struct inode *inode, struct file *file) {
    unsigned int minor;
    file_data *data;
    minor = iminor(inode);
    data = devices[minor];
    if (data == NULL){
        data = kmalloc(sizeof(file_data), GFP_KERNEL);
        if (data == NULL) {
        printk(KERN_ERR "file data kmalloc failed on device_open\n");
        return -ENOMEM;
        }
        data->minor = minor;
        data->channels_head = NULL;
        data->working_channel = NULL;
        devices[minor] = data;
    }
    file->private_data = (void*)data;
    printk(KERN_INFO "Opened device with minor number %d\n", data->minor);
    return 0; // Use 0 for success
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) {
    file_data *data;
    char *msg;
    int msg_size, copy_val;
    channel *cur_channel;

    data = (file_data *)(file->private_data);
    cur_channel = data->working_channel;
    if (!(cur_channel) || ((cur_channel)->ch_id) == 0 || buffer == NULL) {
        printk(KERN_ERR "No channel set or buffer is invalid\n");
        return -EINVAL;
    }

    msg = (cur_channel)->message;
    msg_size = (cur_channel)->message_length;

    printk(KERN_INFO "msg is: %p\n", msg);
    if (msg_size == 0) {
        printk(KERN_INFO "No message to read\n");
        return -EWOULDBLOCK;
    }

    if (length < msg_size) {
        printk(KERN_WARNING "Buffer size %zu is smaller than message size %d\n", length, msg_size);
        return -ENOSPC;
    }

    if (!access_ok(buffer, length)) {
        printk(KERN_ERR "User space memory access check failed\n");
        return -EFAULT;
    }
    printk(KERN_INFO "before copy to user\n");
    printk(KERN_INFO "msg is: %p\n", msg);
    copy_val = copy_to_user(buffer, msg, msg_size);
    if (copy_val != 0) {
        printk(KERN_ERR "Failed to copy message to user space\n");
        return -EFAULT;
    }

    printk(KERN_INFO "Read message of size %d\n", msg_size);

    return msg_size;
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    file_data *data;
    int copy_val;
    channel *working_ch;

    printk(KERN_INFO "before file_data parsing\n");
    data = (file_data *)(file->private_data);
    working_ch = data->working_channel;

    if (!(working_ch) || buffer == NULL) {
        printk(KERN_ERR "Invalid write parameters\n");
        return -EINVAL;
    }

    if (length == 0 || length > MAX_MSG_LEN) {
        printk(KERN_ERR "Invalid message size %zu\n", length);
        return -EMSGSIZE;
    }

    if (!access_ok(buffer, length)) {
        printk(KERN_ERR "User space memory access check failed\n");
        return -EFAULT;
    }

    printk(KERN_INFO "Pointer data: %p\n", data);
    printk(KERN_INFO "Pointer data->working_ch: %p\n", data->working_channel);
    printk(KERN_INFO "Pointer data->working_ch->message: %p\n", data->working_channel->message);
    printk(KERN_INFO "after file_data parsing\n");

    
    working_ch->message = kmalloc(length, GFP_KERNEL);
        if (working_ch->message == NULL) {
            printk(KERN_ERR "Failed to allocate memory for message\n");
            return -ENOMEM;
        }
    
    printk(KERN_INFO "Pointer working_ch->message: %p\n", working_ch->message);

    printk(KERN_INFO "before copy from user\n");
    copy_val = copy_from_user((working_ch)->message, buffer, length);
    printk(KERN_INFO "after copy from user\n");
    if (copy_val != 0) {
        printk(KERN_ERR "Failed to copy message from user space\n");
        return -EFAULT;
    }

    working_ch->message_length = length;

    printk(KERN_INFO "Wrote message of size %zu\n", length);
    printk(KERN_INFO "Message contents: %s\n", (working_ch)->message);


    return length;
}


static long device_ioctl(struct file *file, unsigned int cmd_id, unsigned long arg_id) {
    file_data *data;
    channel *head_ch;
    channel *tmp;

    if (cmd_id != MSG_SLOT_CHANNEL || arg_id == 0) {
        printk(KERN_ERR "Invalid IOCTL command or channel ID equals zero\n");
        return -EINVAL;
    }

    data = (file_data *)(file->private_data);
    if (!data) {
        return -ENODEV;  // No device found
    }

    head_ch = data->channels_head;
    arg_id = (unsigned int)arg_id;

    while (head_ch !=NULL){
        if (head_ch->ch_id == arg_id){
            data->working_channel = tmp;
            return SUCCESS;
        }
        head_ch = head_ch->next_ch;
    }

    head_ch = kmalloc(sizeof(channel), GFP_KERNEL); //insert first
        if (head_ch == NULL) {
            printk(KERN_ERR "Failed to allocate memory for new channel\n");
            return -ENOMEM;
        }
    head_ch->ch_id = arg_id;
    head_ch->message_length = 0;
    head_ch->next_ch = data->channels_head;
    data->working_channel = head_ch;
    data->channels_head = head_ch;

    return SUCCESS;

    /* insert last
    if (head_ch == NULL) {
        printk(KERN_INFO "Creating new channel list for minor %d\n", data->minor);
        head_ch = kmalloc(sizeof(channel), GFP_KERNEL);
        if (head_ch == NULL) {
            printk(KERN_ERR "Failed to allocate memory for new channel\n");
            return -ENOMEM;
        }
        head_ch->ch_id = arg_id;
        head_ch->message_length = 0;
        head_ch->next_ch = NULL;
        data->working_channel = head_ch;
        data->channels_head = head_ch;
        printk(KERN_INFO "created new Head_ch, head_ch id: %lu\n", head_ch->ch_id);
        return SUCCESS; 
    }

    tmp = head_ch;
    printk(KERN_INFO "head_ch channel id: %lu\n", head_ch->ch_id);
    printk(KERN_INFO "arg id: %lu\n", arg_id);


    while (tmp->next_ch != NULL) {
        if (tmp->ch_id == arg_id) {
            data->working_channel = tmp;
            printk(KERN_INFO "Switched to existing channel with id %lu\n", arg_id);
            printk(KERN_INFO "Message in tmp in ioctl: %s\n", tmp->message);

            return 0; // Use 0 for success
        }
        tmp = tmp->next_ch;
    }

    if (tmp->ch_id == arg_id) { // The last channel is the target
        data->working_channel = tmp;
        printk(KERN_INFO "Switched to existing channel with id %lu\n", arg_id);
        printk(KERN_INFO "Message in tmp in ioctl: %s\n", tmp->message);
        return 0; // Use 0 for success
    }

    tmp->next_ch = kmalloc(sizeof(channel), GFP_KERNEL);
    if (tmp->next_ch == NULL) {
        printk(KERN_ERR "Failed to allocate memory for new channel\n");
        return -ENOMEM;
    }

    tmp->next_ch->ch_id = arg_id;
    tmp->next_ch->message_length = 0;
    tmp->next_ch->next_ch = NULL;
    data->working_channel = tmp->next_ch;

    printk(KERN_INFO "Created new channel with id %lu\n", arg_id);

    return 0; // Use 0 for success
    */
}

static int device_release(struct inode *inode, struct file *file) {
    //int minor;

    file_data *data = (file_data *)file->private_data;
    //minor = iminor(inode);

    if (data) {
        kfree(file->private_data);
    }
    //devices[minor] = NULL; //not sure
    printk(KERN_INFO "Closed device\n");
    return 0; // Use 0 for success
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

static int __init message_slot_init(void) {
    int reg_result = register_chrdev(MJR, "message_slot", &fops);

    if (reg_result < 0) {
        printk(KERN_ERR "Failed to register the device with major number %d\n", MJR);
        return reg_result;
    }

    printk(KERN_INFO "Message slot device has been registered with major number %d\n", MJR);
    return 0; // Use 0 for success
}

static void free_data(void) {
    channel *head, *tmp;
    int i;
    // Loop over the devices array
    for (i = 0; i < 256; i++) {
        head = devices[i]->channels_head;
        while (head != NULL) {
            tmp = head;
            head = head->next_ch;
            kfree(tmp);
        }
        kfree(devices[i]);
        devices[i] = NULL;
    }
}

static void __exit message_slot_exit(void) {
    free_data();
    unregister_chrdev(MJR, "message_slot");
    printk(KERN_INFO "Message slot device has been unregistered\n");
}

module_init(message_slot_init);
module_exit(message_slot_exit);
