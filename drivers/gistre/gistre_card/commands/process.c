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
[LOG_INFO] = "info",
[LOG_TRACE] = "trace",
[LOG_WARN] = "warn",
[LOG_EXTRA] = "extra",
[LOG_ERROR] = "error"
};

const char *enum_log_to_string_message(int log_level) {
    switch(log_level) {
        case LOG_INFO: return "[INFO] ";
        case LOG_TRACE: return "[TRACE] ";
        case LOG_WARN: return "[WARNING] ";
        case LOG_EXTRA: return "[DEBUG] ";
        case LOG_ERROR: return "[ERROR] ";
        default: return "";
    }
}

typedef ssize_t (*map_process_command)(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev);

/**
 * @param command: the struct command containing the arguments.
 * @param log_level: the current log_level (currently unused).
 * @return a positive integer if the arguments are valid.
 */
// TODO rename func/ change behavior
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
    LOG("write: trying to write on card", LOG_EXTRA, mfrc_dev->debug_level);
    int data_size;
    if ((data_size = check_arg_size(command, mfrc_dev->debug_level)) < 0) {
        LOG("write: check on arguments failed", LOG_ERROR, mfrc_dev->debug_level)
        return -1;
    }
    if (data_size > INTERNAL_BUFFER_SIZE) {
        LOG("write: data too large, truncating", LOG_EXTRA, mfrc_dev->debug_level);
        data_size = INTERNAL_BUFFER_SIZE;
    }

    // to flush all the data
    if (regmap_write(regmap, MFRC522_FIFOLEVELREG, 0))
    {
        LOG("write: couldn't flush card, aborting", LOG_ERROR, mfrc_dev->debug_level);
        return -1;
    }

    unsigned int i = 0;
    while (i < data_size) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, *(*(command->args + 1) + i));
        if (err)
        {
            LOG("write: failed to write on card, aborting", LOG_ERROR, mfrc_dev->debug_level);
            return -1;
        }
        i++;
    }
    LOG("write: finished to write user content", LOG_EXTRA, mfrc_dev->debug_level);

    while (i < INTERNAL_BUFFER_SIZE) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, 0);
        if (err)
        {
            LOG("write: failed to fill FIFO with zeroes", LOG_ERROR, mfrc_dev->debug_level)
            return -1;
        }
        i++;
    }
    LOG("write: operation successful", LOG_EXTRA, mfrc_dev->debug_level);
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
    LOG("read: trying to read from card...", LOG_EXTRA, mfrc_dev->debug_level)
    memset(mfrc_dev->data, 0, INTERNAL_BUFFER_SIZE + 1);
    unsigned int fifo_size = 0;
    if (regmap_read(regmap, MFRC522_FIFOLEVELREG, &fifo_size))
    {
        LOG("read: Failed to check fifo_size", LOG_ERROR, mfrc_dev->debug_level)
        return -1;
    }
    if (fifo_size == 0)
    {
        LOG("read: no data to read from card", LOG_WARN, mfrc_dev->debug_level)
        return INTERNAL_BUFFER_SIZE;
    }
    pr_info("Read: Card buffer size is %d\n", fifo_size);
    int i = 0;
    while (i < fifo_size)
    {
        int err = regmap_read(regmap, MFRC522_FIFODATAREG, mfrc_dev->data + i);
        pr_info("Read: read '%c-%d'\n", *(mfrc_dev->data + i), *(mfrc_dev->data + i));
        if (err)
        {
            LOG("read: failed to read value from card", LOG_ERROR, mfrc_dev->debug_level)
            return -1;
        }
        if (*(mfrc_dev->data + i) == 0)
        {
            LOG("read: null byte received", LOG_WARN, mfrc_dev->debug_level);
            break;
        }
        i++;
    }
    LOG("read: operation successful", LOG_EXTRA, mfrc_dev->debug_level);
    mfrc_dev->contains_data = true;
    return INTERNAL_BUFFER_SIZE;
}

static int set_log(char *buffer, int log_level) {
    if (strcmp(buffer, "on") == 0)
    {
        LOG("debug: enabling log levels...", LOG_EXTRA, log_level);
        return 1;
    }
    if (strcmp(buffer, "off") == 0)
    {
        LOG("debug: disabling log levels...", LOG_EXTRA, log_level);
        return 0;
    }
    LOG("debug: first argument should be 'on' or 'off', was something else", LOG_ERROR, log_level);
    return -1;
}

static enum LOG_LEVEL find_log_level(char *level, int log_level)
{
    int i = 1;
    // TODO ugly, should be changed
    while (i < LOG_NOT_FOUND && strcmp(level, jump_debug_to_string[i]) != 0) {
        i *= 2;
    }

    if (i == LOG_NOT_FOUND) {
        LOG("debug: unidentified debug level", LOG_ERROR, log_level);
    }

    return i;
}

/**
 * @param command: the struct command containing what is needed to perform a `debug` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with the card.
 * @param mfrc_dev: a struct containing the data related to the current context
of the device.
 * @return a negative integer if an error occured, zero otherwise.
 */
static ssize_t process_debug(struct command *command, struct regmap *regmap /* unused */, struct mfrc_dev *mfrc_dev) {
    int current_level = mfrc_dev->debug_level;
    int set = set_log(command->args[0], mfrc_dev->debug_level);
    if (set == -1)
        return -1;

    int i = 1;
    while (i < command->nb_arg) {
        enum LOG_LEVEL log_level = find_log_level(*(command->args + i), mfrc_dev->debug_level);

        if (log_level == LOG_NOT_FOUND) {
            return -1;
        }

        if (set) {
            current_level |= log_level;
        }
        else {
            current_level &= ~log_level;
        }
        i++;
    }

    LOG("debug: log mode updated successfully", LOG_EXTRA, mfrc_dev->debug_level);
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
