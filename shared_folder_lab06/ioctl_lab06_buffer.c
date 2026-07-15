#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lab06_buffer_ioctl.h"

#define DEVICE_PATH "/dev/lab06_buffer"

static void fail(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	struct lab06_buffer_info info;
	uint32_t mode;
	int fd;

	if (argc != 2) {
		fprintf(stderr, "uso: %s clear|info|append|replace\n", argv[0]);
		return EXIT_FAILURE;
	}

	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0)
		fail("open");

	if (strcmp(argv[1], "clear") == 0) {
		if (ioctl(fd, LAB06_BUFFER_IOCTL_CLEAR) < 0)
			fail("ioctl clear");
	} else if (strcmp(argv[1], "info") == 0) {
		if (ioctl(fd, LAB06_BUFFER_IOCTL_GET_INFO, &info) < 0)
			fail("ioctl get_info");

		printf("mode=%u\n", info.mode);
		printf("size=%llu\n", (unsigned long long)info.size);
		printf("capacity=%llu\n", (unsigned long long)info.capacity);
		printf("read_ops=%llu\n", (unsigned long long)info.read_ops);
		printf("write_ops=%llu\n", (unsigned long long)info.write_ops);
		printf("bytes_read=%llu\n", (unsigned long long)info.bytes_read);
		printf("bytes_written=%llu\n", (unsigned long long)info.bytes_written);
		printf("no_space_errors=%llu\n",
		       (unsigned long long)info.no_space_errors);
	} else if (strcmp(argv[1], "append") == 0) {
		mode = LAB06_BUFFER_MODE_APPEND;
		if (ioctl(fd, LAB06_BUFFER_IOCTL_SET_MODE, &mode) < 0)
			fail("ioctl set_mode append");
	} else if (strcmp(argv[1], "replace") == 0) {
		mode = LAB06_BUFFER_MODE_REPLACE;
		if (ioctl(fd, LAB06_BUFFER_IOCTL_SET_MODE, &mode) < 0)
			fail("ioctl set_mode replace");
	} else {
		fprintf(stderr, "comando invalido: %s\n", argv[1]);
		close(fd);
		return EXIT_FAILURE;
	}

	close(fd);
	return EXIT_SUCCESS;
}
