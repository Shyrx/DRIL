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
static struct my_dev *my_dev;

/*
 *  Init & Exit
 */

static struct file_operations lifecycle_fops = {
	.owner   = THIS_MODULE,
	/* Only use the kernel's defaults */
};

__exit
static void mfrc_exit(void) {

	dev_t dev;

	/* Unregister char device */
	cdev_del(&my_dev->cdev);
	pr_debug("Unregistered char device\n");

	/* Free my_dev structure */
	kfree(my_dev);
	pr_debug("Freed struct my_dev\n");

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
	my_dev = kmalloc(sizeof(*my_dev), GFP_KERNEL);
	if (! my_dev) {
		pr_err("Failed to allocate struct my_dev\n");
		return -ENOMEM;
	}
	else {
		pr_debug("Allocated struct my_dev\n");
	}

	/* Register char device */
	my_dev->cdev.owner = THIS_MODULE;
	cdev_init(&my_dev->cdev, &mfrc_fops);

	ret = cdev_add(&my_dev->cdev, dev, 1);
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
