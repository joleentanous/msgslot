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


int main(int argc, char *argv[]) {
    int file_open;
    long chID_set;
    unsigned int channel_id;
    ssize_t bytes_num;
    size_t msg_size;

    channel_id = atoi(argv[2]);
    msg_size = strlen(argv[3]);

    if (argc != 4) {
        perror("invalid number of arguements provided!");
        exit(1);
    }
    //open the file
    file_open = open(argv[1], O_RDWR);
    if (file_open < 0) {
        perror("opening the device file has failed");
        exit(1);
    }

    if (channel_id == 0) {
        fprintf(stderr, "Invalid channel ID\n");
        close(file_open);
        exit(1);
    }

    chID_set = ioctl(file_open, MSG_SLOT_CHANNEL, channel_id);
    if (chID_set < 0){
        perror("setting channel id to the id you provided has failed");
        close(file_open);
        exit(1);
    }

    if (msg_size == 0 || msg_size > MAX_MSG_LEN) {
        fprintf(stderr, "Message length is invalid\n");
        close(file_open);
        exit(1);
    }

    bytes_num = write(file_open, argv[3], msg_size);
    if (bytes_num < 0) {
        perror("writing the message has failed");
        close(file_open);
        exit(1);
    }

    close(file_open);
    return SUCCESS;
}
