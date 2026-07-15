#ifndef LAB06_BUFFER_IOCTL_H
#define LAB06_BUFFER_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define LAB06_BUFFER_IOC_MAGIC 'L'

enum lab06_buffer_mode {
	LAB06_BUFFER_MODE_APPEND = 0,
	LAB06_BUFFER_MODE_REPLACE = 1,
};

struct lab06_buffer_info {
	__u32 mode;
	__u32 reserved;
	__u64 size;
	__u64 capacity;
	__u64 read_ops;
	__u64 write_ops;
	__u64 bytes_read;
	__u64 bytes_written;
	__u64 no_space_errors;
};

#define LAB06_BUFFER_IOCTL_CLEAR _IO(LAB06_BUFFER_IOC_MAGIC, 0)
#define LAB06_BUFFER_IOCTL_GET_INFO \
	_IOR(LAB06_BUFFER_IOC_MAGIC, 1, struct lab06_buffer_info)
#define LAB06_BUFFER_IOCTL_SET_MODE \
	_IOW(LAB06_BUFFER_IOC_MAGIC, 2, __u32)

#endif
