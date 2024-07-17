obj-m += message_slot.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all: message_sender message_reader message_slot.ko

message_sender: message_sender.c
	gcc -O3 -Wall -std=c11 -o message_sender message_sender.c

message_reader: message_reader.c
	gcc -O3 -Wall -std=c11 -o message_reader message_reader.c

message_slot.ko:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f message_sender message_reader
