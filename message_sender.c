#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "message_slot.h"


/*
Command line arguments:
• argv[1]: message slot file path.
• argv[2]: the target message channel id. Assume a non-negative integer.
• argv[3]: the message to pass.



static int device_open(struct inode *inode, struct file *file) {
static long device_ioctl(struct file *file, unsigned int cmd_id, unsigned long arg) {
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
*/


int main(int argc, char const *argv[]) {
    int file_open, chID_set, bytes_num, msg_size;
    unsigned int channel_id;
    const char* file_path;
    const char *message;

    if (argc != 4){
        perror("invalid number of arguements provided!");
        exit(1);
    }
    
    file_path = argv[1];
    channel_id = atoi(argv[2]);
    message = argv[3];

    file_open = open(file_path, O_WRONLY);
    if (file_open < 0){
        perror("opening the device file has failed");
        exit(1);
    }

    chID_set = ioctl(file_open, MSG_SLOT_CHANNEL, &channel_id);
    if (chID_set !=0 ){
        perror("setting channel id to the id you provided has failed");
        exit(1);
    }

    msg_size = strlen(message);
    bytes_num = write(file_open, message, msg_size);
    if (bytes_num < 0){
        perror("writing the message has failed");
        exit(1);
    }

    close(file_open);
    return SUCCESS;

}
