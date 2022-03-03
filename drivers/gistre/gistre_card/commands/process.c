#include <linux/regmap.h>
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

static const char* jump_debug_to_string[] = {
[DEBUG_OFF] = "off",
[DEBUG_INFO] = "on",
[DEBUG_WARN] = "warn",
[DEBUG_EXTRA] = "extra",
[DEBUG_ERROR] = "error"
};

typedef ssize_t (*map_process_command)(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev);

/**
 * @param command: the struct command containing the arguments.
 * @param log_level: the current log_level (currently unused).
 * @return a positive integer if the arguments are valid.
 */
static int check_arg_size(struct command *command, int log_level)
{
    long data_size;
    if (kstrtol(*command->args, 10, &data_size) != 0) {
        // add log here ?
        return -1;
    }
    return data_size;
}

/**
 * @param type: the struct command containing what is needed to perform a `write` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with the MFRC522 card.
 * @param mfrc_dev: a struct containing the data related to the current context
of the device.
 * @return the number of byte read, or a negative number if an error occured.
 */
static ssize_t process_write(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev)
{
    PRINT_DEBUG("write: trying to write on card", DEBUG_EXTRA, mfrc_dev->debug_level);
    int data_size;
    if ((data_size = check_arg_size(command, mfrc_dev->debug_level)) < 0) {
        PRINT_DEBUG("write: check on argument failed", DEBUG_ERROR, mfrc_dev->debug_level)
        return -1;
    }
    if (data_size > INTERNAL_BUFFER_SIZE) {
        PRINT_DEBUG("write: data too large, truncating", DEBUG_INFO, mfrc_dev->debug_level);
        data_size = INTERNAL_BUFFER_SIZE;
    }

    // to flush all the data
    unsigned int i = 0;
    int rc = 0;
    if ((rc = regmap_write(regmap, MFRC522_FIFOLEVELREG, &i)))
    {
        PRINT_DEBUG("write: couldn't flush card, aborting", DEBUG_ERROR, mfrc_dev->debug_level);
        return -1;
    }
    while (i < data_size) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, *(*(command->args + 1) + i));
        if (err)
        {
            PRINT_DEBUG("write: failed to write on card, aborting", DEBUG_ERROR, mfrc_dev->debug_level);
            pr_err("Write: Failed to write value to card\n");
            return -1;
        }
        i++;
    }
    PRINT_DEBUG("write: finished to write mandatory content", DEBUG_EXTRA, mfrc_dev->debug_level);

    while (i < INTERNAL_BUFFER_SIZE) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, 0);
        if (err)
        {
            PRINT_DEBUG("write: failed to fill FIFO", DEBUG_ERROR, mfrc_dev->debug_level)
            return -1;
        }
        i++;
    }
    PRINT_DEBUG("write: operation successful", DEBUG_INFO, mfrc_dev->debug_level);
    return INTERNAL_BUFFER_SIZE;
}

/**
 * @param type: the struct command containing what is needed to perform a `read` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with the card.
 * @param mfrc_dev: a struct containing the data related to the current context
of the device.
 * @return the number of byte read, or a negative number if an error occured.
 */
static ssize_t process_read(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev)
{
    PRINT_DEBUG("read: trying to read from card...", DEBUG_EXTRA, mfrc_dev->debug_level)
    memset(mfrc_dev->data, 0, INTERNAL_BUFFER_SIZE + 1);
    unsigned int fifo_size = 0;
    if (regmap_read(regmap, MFRC522_FIFOLEVELREG, &fifo_size))
    {
        PRINT_DEBUG("read: Failed to check fifo_size", DEBUG_ERROR, mfrc_dev->debug_level)
        return -1;
    }
    if (fifo_size == 0)
    {
        PRINT_DEBUG("read: no data to read from card", DEBUG_ERROR, mfrc_dev->debug_level)
        return INTERNAL_BUFFER_SIZE;
    }
    int i = 0;
    while (i < fifo_size)
    {
        int err = regmap_read(regmap, MFRC522_FIFODATAREG, mfrc_dev->data + i);
        if (err)
        {
            PRINT_DEBUG("read: failed to read value from card", DEBUG_ERROR, mfrc_dev->debug_level)
            return -1;
        }
        if (mfrc_dev->data + i == 0)
            break; // TODO: add log
        i++;
    }
    PRINT_DEBUG("read: operation successful", DEBUG_EXTRA, mfrc_dev->debug_level);
    mfrc_dev->contains_data = true;
    return INTERNAL_BUFFER_SIZE;
}

// refacto so that the user can decide to toggle specific mode
// something like debug:off:warn or debug:on:extra
// by default, having debug:on should enable all warnings and
// having debug:off should disable them all
/**
 * @param command: the struct command containing what is needed to perform a `debug` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with the card.
 * @param mfrc_dev: a struct containing the data related to the current context
of the device.
 * @return a negative integer if an error occured, zero otherwise.
 */
static ssize_t process_debug(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev) {
    // regmap is useless here
    int i = 0;
    int current_level = mfrc_dev->debug_level;
    while (i < command->nb_arg) {
        enum DEBUG_LEVEL debug_level = find_debug_level(*(command->args + i));
        if (debug_level == DEBUG_NOT_FOUND) {
            // TODO: add log
            PRINT_DEBUG("debug: level not found", DEBUG_WARN, mfrc_dev->debug_level)
            return -1;
        }

        if (debug_level == DEBUG_OFF) {
            PRINT_DEBUG("debug: disabling debug_mode", DEBUG_WARN, mfrc_dev->debug_level)
            mfrc_dev->debug_level = DEBUG_OFF;
            return 0;
        }
        current_level |= (debug_level ) ;
    }
    PRINT_DEBUG("debug: mode updated successfully", DEBUG_EXTRA, mfrc_dev->debug_level);
    mfrc_dev->debug_level = current_level;
    return 0;
}

static const map_process_command jump_process[] = {
[COMMAND_WRITE] = process_write,
[COMMAND_READ] = process_read,
[COMMAND_DEBUG] = process_debug
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

