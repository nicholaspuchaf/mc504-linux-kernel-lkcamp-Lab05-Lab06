// SPDX-License-Identifier: GPL-2.0-only
/*
 * lab06_char.c - simple character device for the LKCamp driver tutorial.
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

#define LAB06_DEVICE_NAME "lab06_char"

enum lab06_status {
	LAB06_STATUS_OFF = 0,
	LAB06_STATUS_ON = 1,
};

static dev_t lab06_dev;
static struct cdev lab06_cdev;
static DEFINE_MUTEX(lab06_lock);
static enum lab06_status lab06_status = LAB06_STATUS_OFF;

static ssize_t lab06_read(struct file *file, char __user *buf, size_t count,
			  loff_t *ppos)
{
	char value;

	if (*ppos > 0)
		return 0;

	mutex_lock(&lab06_lock);
	value = lab06_status == LAB06_STATUS_ON ? '1' : '0';
	mutex_unlock(&lab06_lock);

	if (count == 0)
		return 0;

	if (copy_to_user(buf, &value, 1))
		return -EFAULT;

	*ppos = 1;
	return 1;
}

static ssize_t lab06_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	char value;

	if (count == 0)
		return 0;

	if (copy_from_user(&value, buf, 1))
		return -EFAULT;

	mutex_lock(&lab06_lock);
	if (value == '0')
		lab06_status = LAB06_STATUS_OFF;
	else if (value == '1')
		lab06_status = LAB06_STATUS_ON;
	else {
		mutex_unlock(&lab06_lock);
		return -EINVAL;
	}
	mutex_unlock(&lab06_lock);

	return 1;
}

static const struct file_operations lab06_fops = {
	.owner = THIS_MODULE,
	.read = lab06_read,
	.write = lab06_write,
};

static int __init lab06_char_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&lab06_dev, 0, 1, LAB06_DEVICE_NAME);
	if (ret)
		return ret;

	cdev_init(&lab06_cdev, &lab06_fops);
	lab06_cdev.owner = THIS_MODULE;

	ret = cdev_add(&lab06_cdev, lab06_dev, 1);
	if (ret) {
		unregister_chrdev_region(lab06_dev, 1);
		return ret;
	}

	pr_info("lab06_char: initialized with major=%d minor=%d\n",
		MAJOR(lab06_dev), MINOR(lab06_dev));
	return 0;
}

static void __exit lab06_char_exit(void)
{
	cdev_del(&lab06_cdev);
	unregister_chrdev_region(lab06_dev, 1);
	pr_info("lab06_char: exiting\n");
}

module_init(lab06_char_init);
module_exit(lab06_char_exit);

MODULE_AUTHOR("Lab5");
MODULE_DESCRIPTION("Simple char device for LKCamp device driver tutorial");
MODULE_LICENSE("GPL");
