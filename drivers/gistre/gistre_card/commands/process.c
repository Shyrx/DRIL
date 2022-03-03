#include <linux/regmap.h>
#include <linux/slab.h>

#include "command.h"
#include "../../mfrc522.h"


typedef ssize_t (*map_process_command)(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev);

// process_write
static ssize_t process_write(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev)
{
    return 1;
}

// process_read
static ssize_t process_read(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev)
{
    memset(mfrc_dev->data, 0, MAX_SIZE_BUFFER + 1);
    int i = 0;
    while (i < MAX_SIZE_BUFFER)
    {
        int err = regmap_read(regmap, MFRC522_FIFODATAREG, mfrc_dev->data + i);
        if (err)
        {
            if (i > 0) {
                pr_err("Failed to read value from card");
                return -1;
            }
            pr_err("No data to read");
            return 0;
        }
    }
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

