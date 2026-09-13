// SPDX-License-Identifier: BSD-2-Clause
/*
 * FreeLinX Reference Out-of-Tree Driver Module (flx_dummy)
 *
 * Copyright (c) 2026 FreeLinX Project
 * Author: FreeLinX Core Team
 *
 * A clean reference character driver demonstrating out-of-tree module compilation
 * using Clang/LLVM for FreeLinX's non-GNU kernel architecture.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DRIVER_NAME "flx_dummy"
#define BUF_SIZE    1024

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("FreeLinX Project");
MODULE_DESCRIPTION("FreeLinX Reference Out-of-Tree Character Driver");
MODULE_VERSION("1.0.0");

static char *driver_buffer;
static size_t driver_msg_len;
static DEFINE_MUTEX(dummy_mutex);

static int dummy_open(struct inode *inode, struct file *file)
{
	pr_info("flx_dummy: device opened by PID %d\n", current->pid);
	return 0;
}

static int dummy_release(struct inode *inode, struct file *file)
{
	pr_info("flx_dummy: device closed\n");
	return 0;
}

static ssize_t dummy_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	ssize_t ret = 0;

	mutex_lock(&dummy_mutex);
	if (*ppos >= driver_msg_len) {
		mutex_unlock(&dummy_mutex);
		return 0;
	}

	if (count > driver_msg_len - *ppos)
		count = driver_msg_len - *ppos;

	if (copy_to_user(buf, driver_buffer + *ppos, count)) {
		mutex_unlock(&dummy_mutex);
		return -EFAULT;
	}

	*ppos += count;
	ret = count;
	mutex_unlock(&dummy_mutex);

	return ret;
}

static ssize_t dummy_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	mutex_lock(&dummy_mutex);
	if (count > BUF_SIZE - 1)
		count = BUF_SIZE - 1;

	if (copy_from_user(driver_buffer, buf, count)) {
		mutex_unlock(&dummy_mutex);
		return -EFAULT;
	}

	driver_buffer[count] = '\0';
	driver_msg_len = count;
	*ppos = 0;
	mutex_unlock(&dummy_mutex);

	pr_info("flx_dummy: received %zu bytes: '%s'\n", count, driver_buffer);
	return count;
}

static const struct file_operations dummy_fops = {
	.owner   = THIS_MODULE,
	.open    = dummy_open,
	.release = dummy_release,
	.read    = dummy_read,
	.write   = dummy_write,
};

static struct miscdevice dummy_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DRIVER_NAME,
	.fops  = &dummy_fops,
	.mode  = 0666,
};

static int __init flx_dummy_init(void)
{
	int ret;

	driver_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
	if (!driver_buffer)
		return -ENOMEM;

	snprintf(driver_buffer, BUF_SIZE, "FreeLinX Non-GNU Driver Subsystem Ready!\n");
	driver_msg_len = strlen(driver_buffer);

	ret = misc_register(&dummy_misc);
	if (ret) {
		pr_err("flx_dummy: failed to register misc device (%d)\n", ret);
		kfree(driver_buffer);
		return ret;
	}

	pr_info("flx_dummy: initialized device /dev/%s\n", DRIVER_NAME);
	return 0;
}

static void __exit flx_dummy_exit(void)
{
	misc_deregister(&dummy_misc);
	kfree(driver_buffer);
	pr_info("flx_dummy: unloaded\n");
}

module_init(flx_dummy_init);
module_exit(flx_dummy_exit);
