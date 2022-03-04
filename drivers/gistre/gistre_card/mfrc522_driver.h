#ifndef MFRC522_DRIVER_H
#define MFRC522_DRIVER_H

#define MAX_ACCEPTED_COMMAND_SIZE 100
#define INTERNAL_BUFFER_SIZE 25

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_AUTHOR("antoine.sole, thomas.crambert");
MODULE_LICENSE("GPL v2");

struct mfrc522_driver_dev {
	struct cdev cdev;
	bool contains_data;
	unsigned int log_level;
	unsigned int data[INTERNAL_BUFFER_SIZE + 1];
};

int mfrc522_driver_open(struct inode *inode, struct file *file);
int mfrc522_driver_release(struct inode *inode /* unused */,
			   struct file *file /* unused */);
ssize_t mfrc522_driver_read(struct file *file, char __user *buf,
			    size_t len, loff_t *off /* unused */);
ssize_t mfrc522_driver_write(struct file *file, const char __user *buf,
			     size_t len, loff_t *off /* unused */);

#endif
