#include "mfrc522_driver.h"

#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#include "commands/command.h"

#define MAX_DEVICES 1

static int major;
static struct mfrc522_driver_dev *mfrc522_driver_devs[MAX_DEVICES];

int mfrc522_driver_open(struct inode *inode, struct file *file)
{
	struct mfrc522_driver_dev *mfrc522_driver_dev;
	unsigned int i_major = imajor(inode);
	unsigned int i_minor = iminor(inode);

	if (i_major != major) {
		LOG("open: invalid major, got %d but expected %d, exiting",
			LOG_ERROR, LOG_ERROR, i_major, major);
		return -EINVAL;
	}

	LOG("open: major '%u', minor '%u'\n", LOG_EXTRA,
		mfrc522_driver_devs[i_minor]->log_level, i_major, i_minor);

	mfrc522_driver_dev = container_of(inode->i_cdev, struct mfrc522_driver_dev, cdev);
	if (file->private_data != mfrc522_driver_dev) {
		file->private_data = mfrc522_driver_dev;
		return 0;
	} else
		return -EBUSY;
}

int mfrc522_driver_release(struct inode *inode,
	struct file *file /* unused */) {
	unsigned int i_major = imajor(inode);
	unsigned int i_minor = iminor(inode);

	if (i_major != major) {
		LOG("release: invalid major, got %d but expected %d, exiting",
			LOG_ERROR, LOG_ERROR, i_major, major);
		return -EINVAL;
	}

	LOG("release: major '%u', minor '%u'\n", LOG_EXTRA,
		mfrc522_driver_devs[i_minor]->log_level, i_major, i_minor);

	return 0;
}

ssize_t mfrc522_driver_read(struct file *file, char __user *buf,
	size_t len, loff_t *off /* unused */) {
	struct mfrc522_driver_dev *mfrc522_driver_dev;
	struct mfrc522_driver_data *driver_data;
	char data[INTERNAL_BUFFER_SIZE + 1];
	int i = 0;

	mfrc522_driver_dev = file->private_data;
	driver_data = mfrc522_driver_dev->dev->driver_data;

	// check if data exists
	if (!mfrc522_driver_dev->contains_data) {
		LOG("read: no data to read from internal buffer",
			LOG_WARN, mfrc522_driver_dev->log_level);
		return 0;
	}

	// Copying our internal buffer (int *) into a string (char *)
	memset(data, 0, INTERNAL_BUFFER_SIZE + 1);
	while (i < INTERNAL_BUFFER_SIZE) {
		data[i] = mfrc522_driver_dev->data[i];
		i++;
	}

	// Flush internal buffer
	if (copy_to_user(buf, data, INTERNAL_BUFFER_SIZE + 1)) {
		LOG("read: failed to copy data to user",
			LOG_ERROR, mfrc522_driver_dev->log_level);
		return -EFAULT;
	}

	// Reset internal buffer
	memset(mfrc522_driver_dev->data, 0, INTERNAL_BUFFER_SIZE + 1);
	mfrc522_driver_dev->contains_data = false;
	driver_data->bytes_read += 25;

	return INTERNAL_BUFFER_SIZE;
}

ssize_t mfrc522_driver_write(struct file *file, const char __user *user_buf,
	size_t len, loff_t *off /* unused */) {
	struct mfrc522_driver_dev *mfrc522_driver_dev;
	struct mfrc522_driver_data *driver_data;
	char buff[MAX_ACCEPTED_COMMAND_SIZE + 1];
	struct command *command;
	int res;

	mfrc522_driver_dev = file->private_data;
	driver_data = mfrc522_driver_dev->dev->driver_data;

	memset(buff, 0, MAX_ACCEPTED_COMMAND_SIZE + 1);

	if (copy_from_user(buff, user_buf, MAX_ACCEPTED_COMMAND_SIZE)) {
		LOG("write: failed to copy data from user",
			LOG_ERROR, mfrc522_driver_dev->log_level);
		return -EFAULT;
	}

	command = parse_command(buff, mfrc522_driver_dev->log_level);

	if (command == NULL)
		return -EFAULT;

	res = process_command(command, mfrc522_driver_dev);

	if ( res < 0) {
		command_free(command);
		return -EFAULT;
	}
	if (command->command_type == COMMAND_RANDOM
		|| command->command_type == COMMAND_WRITE)
		driver_data->bytes_written += 25;

	command_free(command);
	return len;
}

/*
 * Class attributes
 */

static ssize_t avg_bits_read_show(struct class *class, struct class_attribute *attr, char *buf) {
	// TODO
	return 0;
}
CLASS_ATTR_RO(avg_bits_read);

static ssize_t avg_bits_written_show(struct class *class, struct class_attribute *attr, char *buf) {
	// TODO
	return 0;
}
CLASS_ATTR_RO(avg_bits_written);

static struct class_attribute mfrc522_driver_class_attrs[] = {
__ATTR(avg_bits_read, S_IRUGO, avg_bits_read_show, NULL),
__ATTR(avg_bits_written, S_IRUGO, avg_bits_written_show, NULL),
__ATTR_NULL,
};

static struct class mfrc522_driver_class = {
    .name = "mfrc522_driver",
    .owner = THIS_MODULE,
    .class_attrs = (struct class_attribute *)mfrc522_driver_class_attrs,
};

/*
 * Device-specific attributes start here.
 */

static ssize_t bits_read_show(struct device *dev,
	struct device_attribute *attr, char *buf) {
	int ret;
	struct mfrc522_driver_data *dd;

	dd = (struct mfrc522_driver_data *) dev->driver_data;
	ret = snprintf(buf, 8 /* 32-bit number + \n */, "%u\n", dd->bytes_read);
	if (ret < 0) {
	        pr_err("Failed to show nb_reads\n");
	}
	return ret;
}
/* Generates dev_attr_nb_reads */
DEVICE_ATTR_RO(bits_read);

static ssize_t bits_written_show(struct device *dev,
	struct device_attribute *attr, char *buf) {
	int ret;
	struct mfrc522_driver_data *dd;

	dd = (struct mfrc522_driver_data *) dev->driver_data;
	ret = snprintf(buf, 8 /* 32-bit number + \n */, "%u\n", dd->bytes_written);
	if (ret < 0) {
	        pr_err("Failed to show nb_writes\n");
	}
	return ret;
}
/* Generates dev_attr_nb_writes */
DEVICE_ATTR_RO(bits_written);

static struct attribute *mfrc522_driver_attrs[] = {
	&dev_attr_bits_read.attr,
	&dev_attr_bits_written.attr,
	NULL,
};

static const struct attribute_group mfrc522_driver_group = {
	.attrs = mfrc522_driver_attrs,
	/* is_visible() == NULL <==> always visible */
};

static const struct attribute_group *mfrc522_driver_groups[] = {
	&mfrc522_driver_group,
	NULL
};

/*
 *  Init & Exit
 */

static const struct file_operations mfrc522_driver_fops = {
	.owner	= THIS_MODULE,
	.read	= mfrc522_driver_read,
	.write	= mfrc522_driver_write,
	.open	= mfrc522_driver_open,
	.release = mfrc522_driver_release
	/* Only use the kernel's defaults */
};

static void mfrc522_driver_destroy_sysfs(struct mfrc522_driver_dev **mfrc522_driver_dev,
	size_t nb_devices) {

	size_t i;

	for (i = 0; i < nb_devices; ++i) {
	        kfree(mfrc522_driver_dev[i]->dev->driver_data);
	        device_destroy(&mfrc522_driver_class, MKDEV(major, i));
	}
	class_unregister(&mfrc522_driver_class);
}

static void mfrc522_driver_delete_devices(size_t count) {
	size_t i;

	for (i = 0; i < count; i++) {
		cdev_del(&(mfrc522_driver_devs[i])->cdev);
		kfree(mfrc522_driver_devs[i]);
	}
}

__exit
static void mfrc522_driver_exit(void)
{
	int i;
	int log_level = 0;

	for (i = 0; i < MAX_DEVICES; i++)
		log_level |= mfrc522_driver_devs[i]->log_level;

	mfrc522_driver_destroy_sysfs(mfrc522_driver_devs, MAX_DEVICES);
	mfrc522_driver_delete_devices(MAX_DEVICES);

	unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);
	LOG("Released major %d", LOG_EXTRA, log_level, major);

	LOG("Stopping driver support for MFRC_522 card", LOG_INFO, LOG_INFO);
}

/* Create the whole file hierarchy under /sys/class/statistics/.
 * Character devices setup must have already been completed.
 */
static int mfrc522_driver_create_sysfs(struct mfrc522_driver_dev **mfrc522_drivers_dev) {

	int ret;
	struct device *dev;
	struct mfrc522_driver_data *data;
	size_t i;

	ret = class_register(&mfrc522_driver_class);
	if (ret < 0) {
	        ret = 1;
	        goto sysfs_end;
	}

	for (i = 0; i < MAX_DEVICES; ++i) {
	        /* Create device with all its attributes */
	        dev = device_create_with_groups(&mfrc522_driver_class, NULL,
	                MKDEV(major, i), NULL /* No private data */,
	                mfrc522_driver_groups, "mfrc%zu", i);
	        if (IS_ERR(dev)) {
	                ret = 1;
	                goto sysfs_cleanup;
	        }

	        /* Store access to the device. As we're the one creating it,
	         * we take advantage if this direct access to struct device.
	         * Note that on regular scenarios, this is not the case;
	         * "struct device" and "struct cdev" are disjoint. */
	        mfrc522_drivers_dev[i]->dev = dev;

	        data = kmalloc(sizeof(struct mfrc522_driver_data), GFP_KERNEL);
	        if (!data) {
	                device_destroy(&mfrc522_driver_class, MKDEV(major, i));
	                ret = 1;
	                goto sysfs_cleanup;
	        }
			data->bytes_read = 0;
			data->bytes_written = 0;
			// Stock our data to retrieve them later
	        mfrc522_drivers_dev[i]->dev->driver_data = (void *)data;
	}

	goto sysfs_end;

sysfs_cleanup:
	mfrc522_driver_destroy_sysfs(mfrc522_drivers_dev, i);
sysfs_end:
	return ret;
}


static void mfrc522_driver_init_dev(struct mfrc522_driver_dev *dev)
{
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &mfrc522_driver_fops);
	dev->dev = NULL;
}

static int mfrc522_driver_setup_dev(size_t i)
{
	int ret;

	/* Allocate our device structure */
	mfrc522_driver_devs[i] = kmalloc(sizeof(*mfrc522_driver_devs[i]), GFP_KERNEL);
	if (!mfrc522_driver_devs[i]) {
		LOG("init: failed to allocate struct mfrc522_driver_dev",
			LOG_ERROR, LOG_ERROR);
		return 1;
	}

	mfrc522_driver_init_dev(mfrc522_driver_devs[i]);

	ret = cdev_add(&(mfrc522_driver_devs[i])->cdev, MKDEV(major, i), 1);
	if (ret < 0) {
		LOG("init: failed to add device", LOG_ERROR,
			mfrc522_driver_devs[i]->log_level);
		kfree(mfrc522_driver_devs[i]);
		return 1;
	}

	return 0;
}

__init
static int mfrc522_driver_init(void)
{
	dev_t dev;
	int ret = 0;
	size_t devices_set_up = 0;
	size_t i;

	LOG("Hello, GISTRE card !", LOG_INFO, LOG_INFO);

	/* Allocate major */
	ret = alloc_chrdev_region(&dev, 0, MAX_DEVICES, "mfrc");
	if (ret < 0)
		return ret;

	major = MAJOR(dev);
	LOG("Got major %d for driver support for MRFC_522 card",
		LOG_INFO, LOG_INFO, major);

	for (i = 0; i < MAX_DEVICES; i++) {
		if (mfrc522_driver_setup_dev(i))
			goto init_cleanup;

		devices_set_up++;
	}

	if (mfrc522_driver_create_sysfs(mfrc522_driver_devs)) {
		LOG("Failed to create class", LOG_ERROR, LOG_ERROR);
		ret = -ENOMEM;
		goto init_cleanup;
	}

	LOG("init: %d devices successfully initialized",
		LOG_INFO, LOG_INFO, MAX_DEVICES);
	goto init_end;

init_cleanup:
	mfrc522_driver_delete_devices(devices_set_up);
	unregister_chrdev_region(MKDEV(major, 0), MAX_DEVICES);
init_end:
	return ret;
}

module_init(mfrc522_driver_init);
module_exit(mfrc522_driver_exit);
