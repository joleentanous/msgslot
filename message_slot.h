#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>


#define MAX_MSG_LEN 128
#define MJR 235
#define SUCCESS 0
#define MSG_SLOT_CHANNEL _IOW(MJR, 0, unsigned int)

typedef struct channel_s {
    long message_length;
    char message[MAX_MSG_LEN];
    unsigned int ch_id;
    struct channel_s *next_ch;
} channel;

typedef struct file_data_s {
    unsigned int minor;
    channel *channels_head;
    channel *working_channel;
} file_data;


#endif
        
