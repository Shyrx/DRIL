#include "mfrc.h"

// TO CLEAN
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "commands/command.h"

MODULE_AUTHOR("antoine.sole, thomas.crambert");
MODULE_LICENSE("GPL v2");

/* Major will always be dynamically allocated */
static int major;
static struct mfrc_dev *mfrc_dev;

/* open read write */

int mfrc_open(struct inode *inode, struct file *file) {

	unsigned int i_major = imajor(inode);
	unsigned int i_minor = iminor(inode);
	if (i_major != major) {
		pr_err("Invalid major %d, expected %d\n", i_major, major);
		return -EINVAL;
	}
    pr_debug("mfrc_open: major '%u', minor '%u'\n", i_major, i_minor);
    file->private_data = mfrc_dev;

	return 0;
}

int mfrc_release(struct inode *inode /* unused */,
        struct file *file /* unused */) {

    pr_debug("mfrc_release\n");

	/* Nothing in particular to do */
	return 0;
}

ssize_t mfrc_read(struct file *file, char __user *buf,
        size_t len, loff_t *off /* unused */) {
    // TODO: communicate with internal buffer of card
	struct mfrc_dev *dev;
    dev = file->private_data;

    // check if data exists
    if (!dev->contains_data) {
        pr_warn("\nNo data in internal buffer\n");
        return 0;
    }

    char data[INTERNAL_BUFFER_SIZE + 1];
    memset(data, 0, INTERNAL_BUFFER_SIZE + 1);
    int i = 0;
    while (i < INTERNAL_BUFFER_SIZE)
    {
        data[i] = dev->data[i];
        i++;
    }
    pr_info("Read: dev data is '%s'\n", data);

    // flush internal buffer
    if (copy_to_user(buf, data, INTERNAL_BUFFER_SIZE + 1)) {
        pr_err("Failed to copy data to user\n");
        return -EFAULT;
    }

    // reset data
    memset(dev->data, 0, INTERNAL_BUFFER_SIZE + 1);
    dev->contains_data = false;
    return INTERNAL_BUFFER_SIZE;
}

ssize_t mfrc_write(struct file *file, const char __user *user_buf,
        size_t len, loff_t *off /* unused */) {
	struct mfrc_dev *dev;
    dev = file->private_data;
    
    char buff[MAX_ACCEPTED_COMMAND_SIZE + 1];

    memset(buff, 0, MAX_ACCEPTED_COMMAND_SIZE + 1);

    if (copy_from_user(buff, user_buf, MAX_ACCEPTED_COMMAND_SIZE)) {
        pr_err("Failed to copy user");
        return -EFAULT;
    }

    struct command *command = parse_command(buff);
    if (command == NULL) {
        return -EFAULT;
    }
    
	if (process_command(command, dev) < 0) {
        // TODO precise error here
        pr_err("Error while processing command\n");
        return -1;
    }

    pr_info("Write: dev data is '%s'\n", dev->data);
    return len;
}

/*
 *  Init & Exit
 */

static struct file_operations mfrc_fops = {
	.owner   = THIS_MODULE,
    .read    = mfrc_read,
    .write   = mfrc_write,
    .open    = mfrc_open,
    .release = mfrc_release
	/* Only use the kernel's defaults */
};

__exit
static void mfrc_exit(void) {

	dev_t dev;

	/* Unregister char device */
	cdev_del(&mfrc_dev->cdev);
	pr_debug("Unregistered char device\n");

	/* Free mfrc_dev structure */
	kfree(mfrc_dev);
	pr_debug("Freed struct mfrc_dev\n");

	/* Release major */
	dev = MKDEV(major, 0);
	unregister_chrdev_region(dev, 1);
	pr_debug("Released major %d\n", major);

    pr_info("Stopping driver support for MFRC_522 card\n");
}

__init
static int mfrc_init(void) {
    pr_info("Hello, GISTRE card !\n");
    pr_debug("Driver, activated\n");
	dev_t dev;
	int ret;
	const char devname[] = "mfrc0";

	/* Allocate major */
	ret = alloc_chrdev_region(&dev, 0, 1, devname);
	if (ret < 0) {
		pr_err("Failed to allocate major\n");
		return 1;
	}
	else {
		major = MAJOR(dev);
		pr_info("Got major %d\n", major);
	}

	/* Allocate our device structure */
	mfrc_dev = kmalloc(sizeof(*mfrc_dev), GFP_KERNEL);
	if (! mfrc_dev) {
		pr_err("Failed to allocate struct mfrc_dev\n");
		return -ENOMEM;
	}
	else {
		pr_debug("Allocated struct mfrc_dev\n");
	}

    // Enable error logs by default
    mfrc_dev->log_level = LOG_ERROR;

	/* Register char device */
	mfrc_dev->cdev.owner = THIS_MODULE;
	cdev_init(&mfrc_dev->cdev, &mfrc_fops);

	ret = cdev_add(&mfrc_dev->cdev, dev, 1);
	if (ret < 0) {
		pr_err("Failed to register char device\n");
		return -ENOMEM;
	}
	else {
		pr_debug("Registered char device\n");
	}

	return 0;
}

module_init(mfrc_init);
module_exit(mfrc_exit);
