ifneq ($(KERNELRELEASE),)
obj-m += lab06_char.o
obj-m += lab06_cmd_char.o
obj-m += lab06_buffer.o
else
CC ?= gcc
CFLAGS += -Wall -Wextra -Werror

USER_BINS := ioctl_lab06_buffer test_lab06_buffer

.PHONY: all clean modules userbins

all: modules userbins

modules:
	$(MAKE) -C ../linux M=$(CURDIR) modules

userbins: $(USER_BINS)

ioctl_lab06_buffer: ioctl_lab06_buffer.c lab06_buffer_ioctl.h
	$(CC) $(CFLAGS) -o $@ ioctl_lab06_buffer.c

test_lab06_buffer: test_lab06_buffer.c lab06_buffer_ioctl.h
	$(CC) $(CFLAGS) -o $@ test_lab06_buffer.c

clean:
	$(MAKE) -C ../linux M=$(CURDIR) clean
	$(RM) $(USER_BINS)
endif
