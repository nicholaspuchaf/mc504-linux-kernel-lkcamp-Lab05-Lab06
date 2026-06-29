// SPDX-License-Identifier: GPL-2.0-only
/*
 * lab5_cmd_char.c - command-based character device for the LKCamp driver tutorial.
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define LAB5_CMD_DEVICE_NAME "lab5_cmd_char"
#define LAB5_CMD_MAX_LEN 16

enum lab5_cmd_status {
	LAB5_CMD_STATUS_OFF = 0,
	LAB5_CMD_STATUS_ON = 1,
};

static dev_t lab5_cmd_dev;
static struct cdev lab5_cmd_cdev;
static DEFINE_MUTEX(lab5_cmd_lock);
static enum lab5_cmd_status lab5_cmd_status = LAB5_CMD_STATUS_OFF;

static ssize_t lab5_cmd_read(struct file *file, char __user *buf, size_t count,
			     loff_t *ppos)
{
	char value;

	if (*ppos > 0)
		return 0;

	mutex_lock(&lab5_cmd_lock);
	value = lab5_cmd_status == LAB5_CMD_STATUS_ON ? '1' : '0';
	mutex_unlock(&lab5_cmd_lock);

	if (count == 0)
		return 0;

	if (copy_to_user(buf, &value, 1))
		return -EFAULT;

	*ppos = 1;
	return 1;
}

static ssize_t lab5_cmd_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	char cmd[LAB5_CMD_MAX_LEN];
	size_t len;

	if (count == 0)
		return 0;

	len = min(count, sizeof(cmd) - 1);
	if (copy_from_user(cmd, buf, len))
		return -EFAULT;

	cmd[len] = '\0';
	strim(cmd);

	mutex_lock(&lab5_cmd_lock);
	if (!strcmp(cmd, "0") || !strcmp(cmd, "off"))
		lab5_cmd_status = LAB5_CMD_STATUS_OFF;
	else if (!strcmp(cmd, "1") || !strcmp(cmd, "on"))
		lab5_cmd_status = LAB5_CMD_STATUS_ON;
	else if (!strcmp(cmd, "toggle"))
		lab5_cmd_status = lab5_cmd_status == LAB5_CMD_STATUS_ON ?
				  LAB5_CMD_STATUS_OFF : LAB5_CMD_STATUS_ON;
	else if (!strcmp(cmd, "reset"))
		lab5_cmd_status = LAB5_CMD_STATUS_OFF;
	else {
		mutex_unlock(&lab5_cmd_lock);
		return -EINVAL;
	}
	mutex_unlock(&lab5_cmd_lock);

	return count;
}

static const struct file_operations lab5_cmd_fops = {
	.owner = THIS_MODULE,
	.read = lab5_cmd_read,
	.write = lab5_cmd_write,
};

static int __init lab5_cmd_char_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&lab5_cmd_dev, 0, 1, LAB5_CMD_DEVICE_NAME);
	if (ret)
		return ret;

	cdev_init(&lab5_cmd_cdev, &lab5_cmd_fops);
	lab5_cmd_cdev.owner = THIS_MODULE;

	ret = cdev_add(&lab5_cmd_cdev, lab5_cmd_dev, 1);
	if (ret) {
		unregister_chrdev_region(lab5_cmd_dev, 1);
		return ret;
	}

	pr_info("lab5_cmd_char: initialized with major=%d minor=%d\n",
		MAJOR(lab5_cmd_dev), MINOR(lab5_cmd_dev));
	return 0;
}

static void __exit lab5_cmd_char_exit(void)
{
	cdev_del(&lab5_cmd_cdev);
	unregister_chrdev_region(lab5_cmd_dev, 1);
	pr_info("lab5_cmd_char: exiting\n");
}

module_init(lab5_cmd_char_init);
module_exit(lab5_cmd_char_exit);

MODULE_AUTHOR("Lab5");
MODULE_DESCRIPTION("Command-based char device for LKCamp device driver tutorial");
MODULE_LICENSE("GPL");
