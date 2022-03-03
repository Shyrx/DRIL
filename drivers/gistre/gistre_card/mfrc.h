#ifndef MFRC_H
#define MFRC_H

#define MAX_ACCEPTED_COMMAND_SIZE 50
#define INTERNAL_BUFFER_SIZE 25

// TO CLEAN
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/slab.h>

MODULE_AUTHOR("antoine.sole, thomas.crambert");
MODULE_LICENSE("GPL v2");

struct mfrc_dev {
	struct cdev cdev;
    bool contains_data;
    unsigned int debug_level;
    unsigned int data[INTERNAL_BUFFER_SIZE + 1]; // + 1 to be null terminated
};

int mfrc_open(struct inode *inode, struct file *file);
int mfrc_release(struct inode *inode /* unused */,
                 struct file *file /* unused */);
ssize_t mfrc_read(struct file *file, char __user *buf,
                  size_t len, loff_t *off /* unused */);
ssize_t mfrc_write(struct file *file, const char __user *buf,
                   size_t len, loff_t *off /* unused */);

#endif
