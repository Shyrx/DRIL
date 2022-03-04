#include "command.h"

#define WRITE_NB_ARG 2
#define WRITE_NAME "mem_write"

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_WRITE
 */
struct command *parse_write(const char* buffer) {
    // TODO Check nb of args
    struct command *command = command_init(COMMAND_WRITE, WRITE_NB_ARG);
    char *new_buff = astrcpy(buffer);
    char *tok = NULL;
    char *sep = ":";
    new_buff += strlen(WRITE_NAME) + 1;
    int i = 0;
    while ((tok = strsep(&new_buff, sep)) != NULL && i < WRITE_NB_ARG) {
      *(command->args + i++) = astrcpy(tok);
      pr_info("arg %d: %s\n", i, tok);
    }
    kfree(new_buff);

    // In case there are too many arguments
    if (tok != NULL)
    {
        kfree(tok);
        command_free(command);
        return NULL;
    }
    return command;
}

/**
 * @param type: the struct command containing what is needed to perform a `write` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with the MFRC522 card.
 * @param mfrc522_driver_dev: a struct containing the data related to the current context
of the device.
 * @return the number of byte read, or a negative number if an error occured.
 */
static ssize_t process_write(struct command *command, struct regmap *regmap, struct mfrc522_driver_dev *mfrc522_driver_dev)
{
    LOG("write: trying to write on card", LOG_EXTRA, mfrc522_driver_dev->log_level);
    int data_size;
    if ((data_size = check_arg_size(command, mfrc522_driver_dev->log_level)) < 0) {
        LOG("write: check on arguments failed", LOG_ERROR, mfrc522_driver_dev->log_level)
        return -1;
    }
    if (data_size > INTERNAL_BUFFER_SIZE) {
        LOG("write: data too large, truncating", LOG_EXTRA, mfrc522_driver_dev->log_level);
        data_size = INTERNAL_BUFFER_SIZE;
    }

    // Flush all the data
    if (regmap_write(regmap, MFRC522_FIFOLEVELREG, 0))
    {
        LOG("write: couldn't flush card, aborting", LOG_ERROR, mfrc522_driver_dev->log_level);
        return -1;
    }

    unsigned int i = 0;
    while (i < data_size) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, *(*(command->args + 1) + i));
        if (err)
        {
            LOG("write: failed to write on card, aborting", LOG_ERROR, mfrc522_driver_dev->log_level);
            return -1;
        }
        i++;
    }
    LOG("write: finished to write user content", LOG_EXTRA, mfrc522_driver_dev->log_level);

    while (i < INTERNAL_BUFFER_SIZE) {
        int err = regmap_write(regmap, MFRC522_FIFODATAREG, 0);
        if (err)
        {
            LOG("write: failed to fill FIFO with zeroes", LOG_ERROR, mfrc522_driver_dev->log_level)
            return -1;
        }
        i++;
    }
    LOG("write: operation successful", LOG_EXTRA, mfrc522_driver_dev->log_level);
    return INTERNAL_BUFFER_SIZE;
}
