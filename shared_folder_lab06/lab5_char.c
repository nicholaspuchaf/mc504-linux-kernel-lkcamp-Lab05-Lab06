// SPDX-License-Identifier: GPL-2.0-only
/*
 * lab5_char.c - simple character device for the LKCamp driver tutorial.
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

#define LAB5_DEVICE_NAME "lab5_char"

enum lab5_status {
	LAB5_STATUS_OFF = 0,
	LAB5_STATUS_ON = 1,
};

static dev_t lab5_dev;
static struct cdev lab5_cdev;
static DEFINE_MUTEX(lab5_lock);
static enum lab5_status lab5_status = LAB5_STATUS_OFF;

static ssize_t lab5_read(struct file *file, char __user *buf, size_t count,
			 loff_t *ppos)
{
	char value;

	if (*ppos > 0)
		return 0;

	mutex_lock(&lab5_lock);
	value = lab5_status == LAB5_STATUS_ON ? '1' : '0';
	mutex_unlock(&lab5_lock);

	if (count == 0)
		return 0;

	if (copy_to_user(buf, &value, 1))
		return -EFAULT;

	*ppos = 1;
	return 1;
}

static ssize_t lab5_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	char value;

	if (count == 0)
		return 0;

	if (copy_from_user(&value, buf, 1))
		return -EFAULT;

	mutex_lock(&lab5_lock);
	if (value == '0')
		lab5_status = LAB5_STATUS_OFF;
	else if (value == '1')
		lab5_status = LAB5_STATUS_ON;
	else {
		mutex_unlock(&lab5_lock);
		return -EINVAL;
	}
	mutex_unlock(&lab5_lock);

	return 1;
}

static const struct file_operations lab5_fops = {
	.owner = THIS_MODULE,
	.read = lab5_read,
	.write = lab5_write,
};

static int __init lab5_char_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&lab5_dev, 0, 1, LAB5_DEVICE_NAME);
	if (ret)
		return ret;

	cdev_init(&lab5_cdev, &lab5_fops);
	lab5_cdev.owner = THIS_MODULE;

	ret = cdev_add(&lab5_cdev, lab5_dev, 1);
	if (ret) {
		unregister_chrdev_region(lab5_dev, 1);
		return ret;
	}

	pr_info("lab5_char: initialized with major=%d minor=%d\n",
		MAJOR(lab5_dev), MINOR(lab5_dev));
	return 0;
}

static void __exit lab5_char_exit(void)
{
	cdev_del(&lab5_cdev);
	unregister_chrdev_region(lab5_dev, 1);
	pr_info("lab5_char: exiting\n");
}

module_init(lab5_char_init);
module_exit(lab5_char_exit);

MODULE_AUTHOR("Lab5");
MODULE_DESCRIPTION("Simple char device for LKCamp device driver tutorial");
MODULE_LICENSE("GPL");
