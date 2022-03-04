#include "command.h"

#define READ_NB_ARG 0

#include "utils.h"

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_READ
 */
struct command *parse_read(const char *buffer) {
    // TODO: check arg number
    struct command *command = command_init(COMMAND_READ, 0);
    return command;
}

/**
 * @param type: the struct command containing what is needed to perform a `read` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with the card.
 * @param mfrc522_driver_dev: a struct containing the data related to the current context
of the device.
 * @return the number of byte read, or a negative number if an error occured.
 */
int process_read(struct command *command, struct regmap *regmap, struct mfrc522_driver_dev *mfrc522_driver_dev)
{
    // TODO Check number arg
    LOG("read: trying to read from card...", LOG_EXTRA, mfrc522_driver_dev->log_level)
    memset(mfrc522_driver_dev->data, 0, INTERNAL_BUFFER_SIZE + 1);
    unsigned int fifo_size = 0;
    if (regmap_read(regmap, MFRC522_FIFOLEVELREG, &fifo_size))
    {
        LOG("read: Failed to check fifo_size", LOG_ERROR, mfrc522_driver_dev->log_level)
        return -1;
    }
    if (fifo_size == 0)
    {
        LOG("read: no data to read from card", LOG_WARN, mfrc522_driver_dev->log_level)
        return INTERNAL_BUFFER_SIZE;
    }
    pr_info("Read: Card buffer size is %d\n", fifo_size);
    int i = 0;
    while (i < fifo_size)
    {
        int err = regmap_read(regmap, MFRC522_FIFODATAREG, mfrc522_driver_dev->data + i);
        if (err)
        {
            LOG("read: failed to read value from card", LOG_ERROR, mfrc522_driver_dev->log_level)
            return -1;
        }
        i++;
    }
    if (flush_fifo(regmap, mfrc522_driver_dev->log_level) < 0) {
        return -1;
    }
    LOG("read: operation successful", LOG_EXTRA, mfrc522_driver_dev->log_level);
    mfrc522_driver_dev->contains_data = true;
    return INTERNAL_BUFFER_SIZE;
}
