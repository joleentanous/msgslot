#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "message_slot.h"


int main(int argc, char *argv[]) {
    int file_open;
    long chID_set;
    unsigned int channel_id;
    char message_buf[MAX_MSG_LEN + 1];
    ssize_t bytes_num;

    channel_id = atoi(argv[2]);

    // Validate the number of command line arguments
    if (argc != 3) {
        perror("invalid number of arguements provided!");
        exit(1);
    }
    // open the file
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
    if (chID_set < 0 ){
        perror("setting channel id to the id you provided has failed");
        close(file_open);
        exit(1);
    }

    bytes_num = read(file_open, message_buf, MAX_MSG_LEN);
    if (bytes_num < 0){
        perror("reading the message has failed");
        close(file_open);
        exit(1);
    }

    // Print the message to standard output
    if (write(STDOUT_FILENO, message_buf, bytes_num) != bytes_num) {
        perror("Failed to write message to stdout");
        exit(1);
    }
    message_buf[bytes_num] = '\0'; // null-termination
    close(file_open);

    return SUCCESS;
}
