#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/uaccess.h>

#include "command.h"
#include "../../mfrc522.h"


static const char* map_command[] = {
[COMMAND_WRITE] = "mem_write",
[COMMAND_READ] = "mem_read",
};

typedef ssize_t (*map_process_command)(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev);

static int check_arg_size(struct command *command)
{
    long data_size;
    if (kstrtol(*command->args, 10, &data_size) != 0) {
        pr_err("Couldn't parse the length of the data, was '%s'\n", *command->args);
        return -1;
    }
    return data_size;
}

static ssize_t process_write(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev)
{
    pr_info("Trying to write on card...\n");

    int data_size;
    if ((data_size = check_arg_size(command)) < 0) {
        pr_err("Write: argument check failed, aborting\n");
        return -1;
    }
    if (data_size > INTERNAL_BUFFER_SIZE) {
        pr_info("Write: data too large, truncating\n");
        data_size = INTERNAL_BUFFER_SIZE;
    }
    pr_info("Write: Data size is %d\n", data_size);
    // to flush all the data
    unsigned int i = 0;
    int rc = 0;
    if ((rc = regmap_write(regmap, MFRC522_FIFOLEVELREG, &i)))
    {
        pr_err("Write: Couldn't flush card buffer: %d\n", rc);
        return -1;
    }
    pr_info("Write: flush successful, starting to write\n");
    while (i < data_size) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, *(*(command->args + 1) + i));
        if (err)
        {
            pr_err("Write: Failed to write value to card\n");
            return -1;
        }
        i++;
    }
    pr_info("Write: finished to write mandatory content, will fill with zeroes if necessary\n");
    while (i < INTERNAL_BUFFER_SIZE) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, 0);
        if (err)
        {
            pr_err("Write: Failed to write zeroes to card\n");
            return -1;
        }
        i++;
    }
    pr_info("Write: finished and successful\n");
    return INTERNAL_BUFFER_SIZE;
}

// process_read
static ssize_t process_read(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev)
{
    pr_info("Trying to read from card...\n");
    memset(mfrc_dev->data, 0, INTERNAL_BUFFER_SIZE + 1);
    unsigned int fifo_size = 0;
    if (regmap_read(regmap, MFRC522_FIFOLEVELREG, &fifo_size))
    {
        pr_err("Read: Failed to check fifo_size\n");
        return -1;
    }
    if (fifo_size == 0)
    {
        pr_err("Read: No data to read from card\n");
        return INTERNAL_BUFFER_SIZE;
    }
    pr_info("Read: Card buffer size is %d\n", fifo_size);
    int i = 0;
    while (i < fifo_size)
    {
        int err = regmap_read(regmap, MFRC522_FIFODATAREG, mfrc_dev->data + i);
        if (err)
        {
            pr_err("Read: Failed to read value from card\n");
            return -1;
        }
        if (*(mfrc_dev->data + i) == 0)
        {
            pr_info("Read: Found a '0' at %d\n", i);
            break;
        }
        i++;
    }
    pr_info("Read: Successfully read '%d' bytes from card\n", fifo_size);
    mfrc_dev->contains_data = true;
    return INTERNAL_BUFFER_SIZE;
}

static const map_process_command jump_process[] = {
[COMMAND_WRITE] = process_write,
[COMMAND_READ] = process_read,
};

static struct regmap *find_regmap(void)
{
    return mfrc522_get_regmap(dev_to_mfrc522(mfrc522_find_dev()));
}

ssize_t process_command(struct command *command, struct mfrc_dev *mfrc_dev)
{
    // no need to check, would not reach this point if the command was unknown
    return jump_process[command->command_type](command, find_regmap(), mfrc_dev);
}

