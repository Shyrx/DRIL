#include <linux/slab.h>

#include "command.h"
#include "utils.h"

#define WRITE_NB_ARG 2
#define WRITE_NAME "mem_write"

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_WRITE
 */
struct command *parse_write(const char *buffer, int log_level)
{

	if (count_separator_occurence(buffer, ':') != WRITE_NB_ARG) {
		// TODO: pass log level in args
		LOG("write: too many or not enough arguments given, expected 2", LOG_ERROR, log_level);
		return NULL;
	}
	struct command *command = command_init(COMMAND_WRITE, WRITE_NB_ARG);

	return get_args(command, buffer, WRITE_NB_ARG, WRITE_NAME);
}

/**
 * @param type: the struct command containing what is needed to perform
 * a `write` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with
 * the MFRC522 card.
 * @param mfrc522_driver_dev: a struct containing the data related to
 * the current context of the device.
 * @return the number of byte read, or a negative number if an error occured.
 */
int process_write(struct command *command, struct regmap *regmap,
				  struct mfrc522_driver_dev *mfrc522_driver_dev)
{
	LOG("write: trying to write on card", LOG_EXTRA,
		mfrc522_driver_dev->log_level);
	int data_size;

	if (data_size < 0) {
		LOG("write: check on arguments failed", LOG_ERROR,
			mfrc522_driver_dev->log_level)
			return -1;
	}
	if (data_size > INTERNAL_BUFFER_SIZE) {
		LOG("write: data too large, truncating", LOG_EXTRA,
			mfrc522_driver_dev->log_level);
		data_size = INTERNAL_BUFFER_SIZE;
	}

	if (flush_fifo(regmap, mfrc522_driver_dev->log_level) < 0)
		return -1;

	unsigned int i = 0;

	while (i < data_size) {
		int err = regmap_write(regmap, MFRC522_FIFODATAREG,
							   *(*(command->args + 1) + i));
		if (err) {
			LOG("write: failed to write on card, abordting", LOG_ERROR,
				mfrc522_driver_dev->log_level);
				return -1;
		}
		i++;
	}


	while (i < INTERNAL_BUFFER_SIZE) {
		int err = regmap_write(regmap, MFRC522_FIFODATAREG, 0);

		if (err) {
			LOG("write: failed to fill FIFO with zeroes", LOG_ERROR,
				mfrc522_driver_dev->log_level);
				return -1;
		}
		i++;

	}
	LOG("write: operation successful", LOG_EXTRA, mfrc522_driver_dev->log_level);
	return INTERNAL_BUFFER_SIZE;
}
