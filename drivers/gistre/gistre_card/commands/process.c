#include <linux/regmap.h>
#include <linux/slab.h>

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
    if (strlen(*(command->args + 1)) != data_size)
    {
        // is it the expected behavior ?
        pr_err("Given size was different from the actual, aborting\n");
        return -2;
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
    if (data_size > MAX_SIZE_BUFFER) {
        data_size = MAX_SIZE_BUFFER;
    }
    // to flush all the data
    unsigned int i = 0;
    int rc = 0;
    if ((rc = regmap_write(regmap, MFRC522_FIFOLEVELREG | MFRC522_FIFOLEVELREG_FLUSH, &i)))
    {
        pr_err("Couldn't flush card buffer: %d\n", rc);
        return -1;
    }

    while (i < data_size) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, mfrc_dev->data + i);
        if (err)
        {
            pr_err("Failed to write value to card\n");
            return -1;
        }
    }
    int zero = 0;
    while (i < MAX_SIZE_BUFFER) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, &zero);
        if (err)
        {
            pr_err("Failed to write zeroes to card\n");
            return -1;
        }
    }
    return MAX_SIZE_BUFFER;
}

// process_read
static ssize_t process_read(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev)
{
    pr_info("Trying to read from card...\n");
    memset(mfrc_dev->data, 0, MAX_SIZE_BUFFER + 1);
    unsigned int fifo_size = 0;
    if (regmap_read(regmap, MFRC522_FIFOLEVELREG, &fifo_size))
    {
        pr_err("Failed to check fifo_size\n");
        return -1;
    }
    if (fifo_size == 0)
    {
        pr_err("No data to read from card\n");
        return MAX_SIZE_BUFFER;
    }
    int i = 0;
    while (i < fifo_size)
    {
        int err = regmap_read(regmap, MFRC522_FIFODATAREG, mfrc_dev->data + i);
        if (err)
        {
            pr_err("Failed to read value from card\n");
            return -1;
        }
        if (mfrc_dev->data + i == 0)
            break;
    }
    pr_info("Successfully read '%d' bytes from card\n", fifo_size);
    return MAX_SIZE_BUFFER;
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

