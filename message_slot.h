#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>


#define MAX_MSG_LEN 128
#define MJR 235
#define SUCCESS 0
#define MSG_SLOT_CHANNEL _IOW(MJR, 0, unsigned int)



typedef struct channel {
    unsigned long ch_id;
    char *message;
    int message_length;
    struct channel *next_ch;
} channel;

typedef struct file_data {
    int minor;
    channel *channels_head;
    channel *working_channel;
} file_data;


/*
typedef struct channel_s {
    unsigned long id;
    char message[MAX_MSG_LEN];
    int message_length;
    struct channel *next;
} channel;


typedef struct device_f {
	int minor;
	device_f *next_device;
	channel **channels;
} device_f;

*/



/*
struct message_slot_channel_s {
    char *message;
    size_t message_length;
} message_slot_channel;

typedef struct message_slot_s {
    channel_t *channels;
} message_slot;

*/






#endif
