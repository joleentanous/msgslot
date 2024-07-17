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


file_data devices[256];



static int device_open(struct inode *inode, struct file *file) {
    file_data *data;
    data = kmalloc(sizeof(file_data), GFP_KERNEL);
    if (data == NULL) {
        printk("file data kmalloc failed on device_open");
        return -ENOMEM;
    }

    data->minor = iminor(inode);
    data->channels_head = NULL;
    data->working_channel = NULL;
    file->private_data = data; // (*void)data

    return SUCCESS;
}




static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) {
    file_data *data;
    char *msg;
    int msg_size, copy_val;

    data = (file_data *)(file->private_data);

    if (!(data->working_channel) || buffer == NULL) {
        return -EINVAL;
    }

    msg = data->working_channel->message;
    msg_size = data->working_channel->message_length;


    if (msg_size == 0) {
        return -EWOULDBLOCK;
    }

    if (length < msg_size) {
        return -ENOSPC;
    }
    copy_val = copy_to_user(buffer, msg, msg_size);
    if (copy_val != 0) {
        return -EFAULT;
    }

    return msg_size; //if we get here then all of the bytes were read from the buffer
}




static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    file_data *data;
    int copy_val;

    data = (file_data *)file->private_data;

    if (!(data->working_channel) || buffer == NULL) {
        return -EINVAL;
    }

    if (length == 0 || length > MAX_MSG_LEN) {
        return -EMSGSIZE;
    }
    
    copy_val = copy_from_user(data->working_channel->message, buffer, length); //copies from user buffer to kernel
    if (copy_val != 0) {
        return -EFAULT;
    }

    data->working_channel->message_length = length;

    return length; //if we get here then all of the bytes were written to the buffer
}



static long device_ioctl(struct file *file, unsigned int cmd_id, unsigned long arg_id) {
    file_data *data;
    channel *head_ch;
	channel *tmp;

    if (cmd_id != MSG_SLOT_CHANNEL || arg_id == 0) {
        return -EINVAL;
    }

    data = (file_data *)(file->private_data);
    head_ch = devices[data->minor].channels_head;

    if (head_ch == NULL){ //channel_list is empty
        head_ch = kmalloc(sizeof(channel), GFP_KERNEL);
        if (head_ch == NULL) {
            printk("new channel kmalloc has failed in ioctl");
            return -ENOMEM;
        }
        // initialize the new head for the empty linked list of channels
        head_ch->ch_id = arg_id;
        head_ch->message_length = 0;
        head_ch->next_ch = NULL;
        // link with the device
		data->working_channel = head_ch;
		devices[data->minor].channels_head = head_ch;
		return SUCCESS;
    }
    tmp = head_ch;
    while (tmp->next_ch != NULL) {
        if (tmp->ch_id == arg_id) {
            data->working_channel = tmp;
            return SUCCESS;
        }
        tmp = tmp->next_ch;
    }
    if (tmp->ch_id == arg_id){ //the last channel is the target
        data->working_channel = tmp;
        return SUCCESS;
    }
    //channel does not exist, creates a new one
    tmp->next_ch = kmalloc(sizeof(channel), GFP_KERNEL);
    if (tmp->next_ch == NULL) {
        return -ENOMEM;
    }

    (tmp->next_ch)->ch_id = arg_id;
    (tmp->next_ch)->message_length = 0;
    (tmp->next_ch)->next_ch = NULL;
    data->working_channel = tmp->next_ch;
    return SUCCESS;
}


static int device_release(struct inode *inode, struct file *file) {
    kfree(file->private_data);
    return SUCCESS;
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
    return SUCCESS;
}


static void free_data(void){
    channel *head, *tmp;
	int i;
	//loop over the devices array
	for(i = 0; i < 256; i++){
		head = devices[i].channels_head;
		while (head != NULL)
		{
			tmp = head;
			head = head->next_ch;
			kfree(tmp);
		}
	}
	
}


static void __exit message_slot_exit(void) {
    free_data();
    unregister_chrdev(MJR, "message_slot");
    printk(KERN_INFO "Message slot device has been unregistered\n");
}



module_init(message_slot_init);
module_exit(message_slot_exit);

