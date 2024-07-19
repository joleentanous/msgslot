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
file_data* find_data(unsigned int minor) {
    int i;
    for (i = 0; i < 256; i++) {
        if (devices[i]){
            if (devices[i]->minor == minor){
                return devices[i];
            }
        }
    }
    return NULL;
}
*/

file_data* create_data(unsigned int minor) {
    int i;
    printk(KERN_INFO "1\n");

    for (i = 0; i < 256; i++) {
        if (devices[i] == NULL) {
            file_data *data = kmalloc(sizeof(file_data), GFP_KERNEL);
            if (data == NULL) {
                return NULL;
            }
            data->minor = minor;
            data->channels_head = NULL;
            data->working_channel = NULL;
            devices[i] = data;
            return data;
        }
    }
    return NULL; // No available slot
}


static int device_open(struct inode* inode, struct file* file) {

    unsigned int minor;
    int i;
    file_data *data;
    minor = iminor(inode);    
    //data = find_data(minor);
    data = NULL;
    for (i = 0; i < 256; i++) {
        if (devices[i]){
            if (devices[i]->minor == minor){
                data = devices[i];
            }
        }
    }


    if (data == NULL) {
        data = create_data(minor);
        if (data == NULL) {
            return -ENOMEM;
        }
    }
    file->private_data = (void*)data;

    return SUCCESS;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset) {

    file_data *data;
    unsigned long copy_val;
    channel *channel;    
	printk(KERN_INFO "4\n");
   
    
    data = (file_data*)file->private_data;
    channel = data->working_channel;

    
    if (channel == NULL) {
        printk(KERN_ERR "No channel set.\n");
        return -EINVAL;  
    }

    if (channel->message_length == 0) {
        printk(KERN_INFO "No message to read\n");
        return -EWOULDBLOCK;
    }

    if (length < channel->message_length) {
        return -ENOSPC;
    }

    if (!access_ok(buffer, length)) {
        printk(KERN_ERR "User space memory access check failed\n");
        return -EFAULT;
    }

    copy_val = copy_to_user(buffer, channel->message, channel->message_length);
    if (copy_val) {
        printk(KERN_ERR "Failed to copy message to user space\n");
        return -EFAULT;
    }
    printk(KERN_INFO "5\n");

    return channel->message_length;
}



static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset) {
    file_data *data;
    unsigned long copy_val;
    channel *channel;    
    
    data = (file_data*)file->private_data;
    channel = data->working_channel;

    printk(KERN_INFO "6\n");

    if ((channel == NULL) || buffer == NULL) {
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

    /*
    channel->message = kmalloc(MAX_MSG_LEN, GFP_KERNEL);
    if (channel->message == NULL) {
        printk(KERN_ERR "Failed to allocate memory for message\n");
        return -ENOMEM;
    }
    */

    //printk(KERN_ERR "before copy from user\n");
    copy_val = copy_from_user(channel->message, buffer, length);
    if (copy_val) {
        printk(KERN_ERR "Failed to copy message from user space\n");
        return -EFAULT;  // Failed to copy from user space
    }
    channel->message_length = length;
    printk(KERN_INFO "7\n");

    return length;
}

static long device_ioctl(struct file* file, unsigned int cmd_id, unsigned long arg_id) {
    file_data *data;
    channel *head_ch;


    if (cmd_id != MSG_SLOT_CHANNEL || arg_id == 0) {
        printk(KERN_ERR "Invalid IOCTL command or channel ID equals zero\n");
        return -EINVAL;
    }

    arg_id = (unsigned int)arg_id;

    data = (file_data*)file->private_data;
    if (data == NULL) {
        return -ENODEV;  // No device found
    }

    // Check if the channel already exists
    head_ch = data->channels_head;
    while (head_ch != NULL) {
        if (head_ch->ch_id == arg_id){
            data->working_channel = head_ch;
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
    data->channels_head = head_ch;
    data->working_channel = head_ch;

    return SUCCESS;
}


static int device_release(struct inode *inode, struct file *file) {
    file_data *data = (file_data *)file->private_data;
    if (data) {
        file->private_data = NULL;
    }

    printk(KERN_INFO "Close device\n");
    return SUCCESS;  
}



struct file_operations Fops = {
  .owner = THIS_MODULE, 
  .read  = device_read,
  .write = device_write,
  .open  = device_open,
  .unlocked_ioctl = device_ioctl,
  .release = device_release,
};

static int __init message_slot_init(void){ 
    int i;
    int reg_result;
    reg_result = register_chrdev(MJR, "message_slot", &Fops);

    if (reg_result < 0) {
        printk(KERN_ERR "Failed to register the device with major number %d\n", MJR);
        return reg_result;
    }
    for(i = 0 ; i < 256 ;i++){
        devices[i] = NULL;
    }
    printk(KERN_INFO "Message slot device has been registered with major number %d\n", MJR);
    return SUCCESS;
}


static void free_data(void){
    channel *head, *tmp;
    int i;
    // Loop over the devices array
    for (i = 0; i < 256; i++) {
        if (devices[i]) {  
            head = devices[i]->channels_head;
            while (head != NULL) {
                tmp = head;
                head = head->next_ch;
                kfree(tmp->message); // Free the message buffer
                kfree(tmp);
            }
        kfree(devices[i]);
        devices[i] = NULL;
        }
    }
}

static void __exit message_slot_exit(void) { //didnt change
    free_data();
    unregister_chrdev(MJR, "message_slot");
    printk(KERN_INFO "Message slot device has been unregistered\n");
}


module_init(message_slot_init);
module_exit(message_slot_exit);

