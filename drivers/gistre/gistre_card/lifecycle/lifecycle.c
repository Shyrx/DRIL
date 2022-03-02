#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

/*
 * Metadata
 */

MODULE_AUTHOR("antoine.sole, thomas.crambert");
MODULE_LICENSE("GPL v2");

/*
 * Local definitions
 */
struct my_dev {
	struct cdev cdev;
};

/* Major will always be dynamically allocated */
static int major;
static struct mfrc_dev *mfrc_dev;

/* open read write */

int mfrc_open(struct inode *inode, struct file *file) {

	unsigned int i_major = imajor(inode);
	unsigned int i_minor = iminor(inode);

	pr_info("%s()\n", __func__);

	if (i_major != major) {
		pr_err("Invalid major %d, expected %d\n", i_major, major);
		return -EINVAL;
	}

	/* Make file descriptor "remember" its mirror device object,
	 * for later use in open() and write(). */
	file->private_data = pp_dev;

	return 0;
}

int mfrc_release(struct inode *inode /* unused */,
        struct file *file /* unused */) {

	pr_debug("%s()\n", __func__);

	/* Nothing in particular to do */
	return 0;
}

ssize_t mfrc_read(struct file *file, char __user *buf,
        size_t len, loff_t *off /* unused */) {

	struct mfrc_dev *dev;
    dev = file->private_data;
    // TODO
	return 0;
}

ssize_t mfrc_write(struct file *file, const char __user *buf,
        size_t len, loff_t *off /* unused */) {

	struct pingpong_dev *dev;
    dev = file->private_data;
    // TODO
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
		pr_debug("Got major %d\n", major);
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
