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

    file_open = open(file_path, O_WRONLY);
    if (file_open < 0){
        perror("opening the device file has failed");
        exit(1);
    }

    chID_set = ioctl(file_open, MSG_SLOT_CHANNEL, &channel_id);
    if (chID_set != 0){
        close(file_open);
        perror("setting channel id to the id you provided has failed");
        exit(1);
    }

    
    bytes_num = write(file_open, message, msg_size);
    if (bytes_num < 0){
        close(file_open);
        perror("writing the message has failed");
        exit(1);
    }

    close(file_open);
    return SUCCESS;

}
