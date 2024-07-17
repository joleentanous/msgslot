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
You should validate that the correct number of command line arguments is passed.
The flow:
1. Open the specified message slot device file.
2. Set the channel id to the id specified on the command line.
3. Read a message from the message slot file to a buffer.
4. Close the device.
5. Print the message to standard output (using the write() system call). Print only the message,
without any additional text.
6. Exit the program with exit value 0
*/

int main(int argc, char const *argv[]) {
    int file_open, chID_set, bytes_num;
    unsigned int channel_id;
    const char* file_path;
    char message_buf[MAX_MSG_LEN + 1];

    if (argc != 3){
        perror("invalid number of arguements provided!");
        exit(1);
    }
    
    file_path = argv[1];
    channel_id = atoi(argv[2]);

    file_open = open(file_path, O_RDONLY);
    if (file_open < 0){
        perror("opening the device file has failed");
        exit(1);
    }

    chID_set = ioctl(file_open, MSG_SLOT_CHANNEL, &channel_id);
    if (chID_set !=0 ){
        perror("setting channel id to the id you provided has failed");
        exit(1);
    }

    bytes_num = read(file_open, message_buf, MAX_MSG_LEN);
    if (bytes_num < 0){
        perror("reading the message has failed");
        exit(1);
    }
    message_buf[bytes_num] = '\0'; // null-termination

    close(file_open);
    return SUCCESS;

}
