// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef __NR_lab5_get_value
#define __NR_lab5_get_value 472
#endif

#ifndef __NR_lab5_set_value
#define __NR_lab5_set_value 473
#endif

#ifndef __NR_lab5_stats
#define __NR_lab5_stats 474
#endif

struct lab5_stats {
	int current_value;
	unsigned long long get_calls;
	unsigned long long set_calls;
	int last_written_value;
	int last_set_pid;
};

static long checked_syscall(long nr, long arg, const char *name)
{
	long ret;

	errno = 0;
	ret = syscall(nr, arg);
	if (ret == -1) {
		perror(name);
		exit(EXIT_FAILURE);
	}

	return ret;
}

static void get_stats(struct lab5_stats *stats)
{
	checked_syscall(__NR_lab5_stats, (long)stats, "lab5_stats");
}

int main(int argc, char **argv)
{
	struct lab5_stats before;
	struct lab5_stats after;
	int initial;
	int current;
	long ret;
	long value = 42;

	if (argc > 1)
		value = strtol(argv[1], NULL, 10);

	get_stats(&before);

	errno = 0;
	ret = syscall(__NR_lab5_get_value, &initial);
	if (ret == -1) {
		perror("lab5_get_value");
		return EXIT_FAILURE;
	}

	checked_syscall(__NR_lab5_set_value, value, "lab5_set_value");

	errno = 0;
	ret = syscall(__NR_lab5_get_value, &current);
	if (ret == -1) {
		perror("lab5_get_value");
		return EXIT_FAILURE;
	}

	printf("initial=%d\n", initial);
	printf("set_value=%ld\n", value);
	printf("current=%d\n", current);

	get_stats(&after);

	printf("stats.current_value=%d\n", after.current_value);
	printf("stats.get_calls=%llu\n", after.get_calls);
	printf("stats.set_calls=%llu\n", after.set_calls);
	printf("stats.last_written_value=%d\n", after.last_written_value);
	printf("stats.last_set_pid=%d\n", after.last_set_pid);

	if (current != value) {
		fprintf(stderr, "erro: get retornou %d, esperado %ld\n",
			current, value);
		return EXIT_FAILURE;
	}

	if (after.current_value != value) {
		fprintf(stderr, "erro: stats.current_value=%d, esperado %ld\n",
			after.current_value, value);
		return EXIT_FAILURE;
	}

	if (after.last_written_value != value) {
		fprintf(stderr,
			"erro: stats.last_written_value=%d, esperado %ld\n",
			after.last_written_value, value);
		return EXIT_FAILURE;
	}

	if (after.last_set_pid != getpid()) {
		fprintf(stderr, "erro: stats.last_set_pid=%d, esperado %d\n",
			after.last_set_pid, getpid());
		return EXIT_FAILURE;
	}

	if (after.get_calls != before.get_calls + 2) {
		fprintf(stderr, "erro: get_calls=%llu, esperado %llu\n",
			after.get_calls, before.get_calls + 2);
		return EXIT_FAILURE;
	}

	if (after.set_calls != before.set_calls + 1) {
		fprintf(stderr, "erro: set_calls=%llu, esperado %llu\n",
			after.set_calls, before.set_calls + 1);
		return EXIT_FAILURE;
	}

	puts("resultado=ok");
	return EXIT_SUCCESS;
}
