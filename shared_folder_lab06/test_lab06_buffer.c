#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "lab06_buffer_ioctl.h"

#define DEVICE_PATH "/dev/lab06_buffer"

static void fail(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static void expect_equal_str(const char *expected, const char *actual,
			     const char *label)
{
	if (strcmp(expected, actual) != 0) {
		fprintf(stderr, "erro: %s: esperado '%s', obtido '%s'\n",
			label, expected, actual);
		exit(EXIT_FAILURE);
	}
}

static void expect_equal_u64(uint64_t expected, uint64_t actual,
			     const char *label)
{
	if (expected != actual) {
		fprintf(stderr, "erro: %s: esperado %llu, obtido %llu\n",
			label,
			(unsigned long long)expected,
			(unsigned long long)actual);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	struct lab06_buffer_info info;
	char buf[64];
	uint32_t mode;
	ssize_t n;
	int fd;

	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0)
		fail("open");

	if (ioctl(fd, LAB06_BUFFER_IOCTL_CLEAR) < 0)
		fail("ioctl clear");

	n = write(fd, "abc", 3);
	if (n != 3)
		fail("write abc");

	if (lseek(fd, 0, SEEK_SET) < 0)
		fail("lseek after abc");

	memset(buf, 0, sizeof(buf));
	n = read(fd, buf, sizeof(buf) - 1);
	if (n < 0)
		fail("read abc");
	buf[n] = '\0';
	expect_equal_str("abc", buf, "conteudo apos append inicial");

	if (ioctl(fd, LAB06_BUFFER_IOCTL_GET_INFO, &info) < 0)
		fail("ioctl get_info");

	expect_equal_u64(3, info.size, "size apos append");
	expect_equal_u64(256, info.capacity, "capacity");
	expect_equal_u64(LAB06_BUFFER_MODE_APPEND, info.mode, "modo inicial");

	mode = LAB06_BUFFER_MODE_REPLACE;
	if (ioctl(fd, LAB06_BUFFER_IOCTL_SET_MODE, &mode) < 0)
		fail("ioctl set_mode replace");

	n = write(fd, "xyz", 3);
	if (n != 3)
		fail("write xyz");

	if (lseek(fd, 0, SEEK_SET) < 0)
		fail("lseek after xyz");

	memset(buf, 0, sizeof(buf));
	n = read(fd, buf, sizeof(buf) - 1);
	if (n < 0)
		fail("read xyz");
	buf[n] = '\0';
	expect_equal_str("xyz", buf, "conteudo apos replace");

	if (ioctl(fd, LAB06_BUFFER_IOCTL_GET_INFO, &info) < 0)
		fail("ioctl get_info final");

	expect_equal_u64(3, info.size, "size final");
	expect_equal_u64(LAB06_BUFFER_MODE_REPLACE, info.mode, "modo final");

	if (ioctl(fd, LAB06_BUFFER_IOCTL_CLEAR) < 0)
		fail("ioctl clear final");

	if (lseek(fd, 0, SEEK_SET) < 0)
		fail("lseek after clear");

	n = read(fd, buf, sizeof(buf));
	if (n < 0)
		fail("read after clear");
	expect_equal_u64(0, n, "leitura apos clear");

	close(fd);

	printf("resultado=ok\n");
	printf("device=%s\n", DEVICE_PATH);
	return 0;
}
