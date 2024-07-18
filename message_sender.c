#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "message_slot.h"

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
    msg_size = strlen(message);

    printf("before file open\n");
    file_open = open(file_path, O_RDWR);
    printf("after file open\n");
    if (file_open < 0){
        perror("opening the device file has failed");
        exit(1);
    }

    printf("before ioctl\n");
    chID_set = ioctl(file_open, MSG_SLOT_CHANNEL, &channel_id);
    printf("after ioctl\n");
    if (chID_set != 0){
        close(file_open);
        perror("setting channel id to the id you provided has failed");
        exit(1);
    }

    printf("before write\n");
    bytes_num = write(file_open, message, msg_size);
    printf("after write\n");
    if (bytes_num != msg_size){
        close(file_open);
        perror("writing the message has failed");
        exit(1);
    }

    close(file_open);
    return SUCCESS;

}
