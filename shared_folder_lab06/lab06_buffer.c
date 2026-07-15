// SPDX-License-Identifier: GPL-2.0-only
/*
 * lab06_buffer.c - buffered pseudo char device with ioctl-based configuration.
 */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "lab06_buffer_ioctl.h"

#define LAB06_BUFFER_DEVICE_NAME "lab06_buffer"
#define LAB06_BUFFER_CAPACITY 256

struct lab06_buffer_device {
	dev_t devt;
	struct cdev cdev;
	struct mutex lock;
	char *data;
	size_t size;
	size_t capacity;
	u32 mode;
	u64 read_ops;
	u64 write_ops;
	u64 bytes_read;
	u64 bytes_written;
	u64 no_space_errors;
};

static struct lab06_buffer_device lab06_buffer_dev = {
	.capacity = LAB06_BUFFER_CAPACITY,
	.mode = LAB06_BUFFER_MODE_APPEND,
};

static loff_t lab06_buffer_llseek(struct file *file, loff_t off, int whence)
{
	struct lab06_buffer_device *dev = &lab06_buffer_dev;
	loff_t new_pos;

	mutex_lock(&dev->lock);

	switch (whence) {
	case SEEK_SET:
		new_pos = off;
		break;
	case SEEK_CUR:
		new_pos = file->f_pos + off;
		break;
	case SEEK_END:
		new_pos = dev->size + off;
		break;
	default:
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}

	if (new_pos < 0) {
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}

	file->f_pos = new_pos;
	mutex_unlock(&dev->lock);
	return new_pos;
}

static ssize_t lab06_buffer_read(struct file *file, char __user *buf, size_t count,
				 loff_t *ppos)
{
	struct lab06_buffer_device *dev = &lab06_buffer_dev;
	size_t available;
	size_t to_copy;
	ssize_t ret;

	if (count == 0)
		return 0;

	mutex_lock(&dev->lock);

	if (*ppos < 0) {
		ret = -EINVAL;
		goto out_unlock;
	}

	if (*ppos >= dev->size) {
		ret = 0;
		goto out_unlock;
	}

	available = dev->size - *ppos;
	to_copy = min(count, available);

	if (copy_to_user(buf, dev->data + *ppos, to_copy)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	*ppos += to_copy;
	dev->read_ops++;
	dev->bytes_read += to_copy;
	ret = to_copy;

out_unlock:
	mutex_unlock(&dev->lock);
	return ret;
}

static ssize_t lab06_buffer_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	struct lab06_buffer_device *dev = &lab06_buffer_dev;
	size_t offset;
	size_t available;
	size_t new_size;
	ssize_t ret;

	if (count == 0)
		return 0;

	mutex_lock(&dev->lock);

	switch (dev->mode) {
	case LAB06_BUFFER_MODE_APPEND:
		offset = dev->size;
		available = dev->capacity - dev->size;
		new_size = dev->size + count;
		break;
	case LAB06_BUFFER_MODE_REPLACE:
		offset = 0;
		available = dev->capacity;
		new_size = count;
		break;
	default:
		ret = -EINVAL;
		goto out_unlock;
	}

	if (count > available) {
		dev->no_space_errors++;
		ret = -ENOSPC;
		goto out_unlock;
	}

	if (copy_from_user(dev->data + offset, buf, count)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	dev->size = new_size;
	dev->write_ops++;
	dev->bytes_written += count;

	if (dev->mode == LAB06_BUFFER_MODE_REPLACE && dev->size < dev->capacity)
		memset(dev->data + dev->size, 0, dev->capacity - dev->size);

	ret = count;

out_unlock:
	mutex_unlock(&dev->lock);
	return ret;
}

static long lab06_buffer_ioctl(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	struct lab06_buffer_device *dev = &lab06_buffer_dev;
	struct lab06_buffer_info info;
	u32 mode;
	long ret = 0;

	switch (cmd) {
	case LAB06_BUFFER_IOCTL_CLEAR:
		mutex_lock(&dev->lock);
		memset(dev->data, 0, dev->capacity);
		dev->size = 0;
		mutex_unlock(&dev->lock);
		break;
	case LAB06_BUFFER_IOCTL_GET_INFO:
		mutex_lock(&dev->lock);
		info.mode = dev->mode;
		info.reserved = 0;
		info.size = dev->size;
		info.capacity = dev->capacity;
		info.read_ops = dev->read_ops;
		info.write_ops = dev->write_ops;
		info.bytes_read = dev->bytes_read;
		info.bytes_written = dev->bytes_written;
		info.no_space_errors = dev->no_space_errors;
		mutex_unlock(&dev->lock);

		if (copy_to_user((void __user *)arg, &info, sizeof(info)))
			ret = -EFAULT;
		break;
	case LAB06_BUFFER_IOCTL_SET_MODE:
		if (copy_from_user(&mode, (void __user *)arg, sizeof(mode)))
			return -EFAULT;

		if (mode != LAB06_BUFFER_MODE_APPEND &&
		    mode != LAB06_BUFFER_MODE_REPLACE)
			return -EINVAL;

		mutex_lock(&dev->lock);
		dev->mode = mode;
		mutex_unlock(&dev->lock);
		break;
	default:
		ret = -ENOTTY;
	}

	return ret;
}

static const struct file_operations lab06_buffer_fops = {
	.owner = THIS_MODULE,
	.read = lab06_buffer_read,
	.write = lab06_buffer_write,
	.unlocked_ioctl = lab06_buffer_ioctl,
	.llseek = lab06_buffer_llseek,
};

static int __init lab06_buffer_init(void)
{
	struct lab06_buffer_device *dev = &lab06_buffer_dev;
	int ret;

	dev->data = kzalloc(dev->capacity, GFP_KERNEL);
	if (!dev->data)
		return -ENOMEM;

	mutex_init(&dev->lock);

	ret = alloc_chrdev_region(&dev->devt, 0, 1, LAB06_BUFFER_DEVICE_NAME);
	if (ret)
		goto err_free_data;

	cdev_init(&dev->cdev, &lab06_buffer_fops);
	dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&dev->cdev, dev->devt, 1);
	if (ret)
		goto err_unregister_region;

	pr_info("lab06_buffer: initialized with major=%d minor=%d capacity=%zu\n",
		MAJOR(dev->devt), MINOR(dev->devt), dev->capacity);
	return 0;

err_unregister_region:
	unregister_chrdev_region(dev->devt, 1);
err_free_data:
	kfree(dev->data);
	return ret;
}

static void __exit lab06_buffer_exit(void)
{
	struct lab06_buffer_device *dev = &lab06_buffer_dev;

	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->devt, 1);
	kfree(dev->data);
	pr_info("lab06_buffer: exiting\n");
}

module_init(lab06_buffer_init);
module_exit(lab06_buffer_exit);

MODULE_AUTHOR("Lab5");
MODULE_DESCRIPTION("Buffered pseudo char device with ioctl-based configuration");
MODULE_LICENSE("GPL");
