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
		LOG("write: too many or not enough arguments given, expected 2",
			LOG_ERROR, log_level);
		return NULL;
	}
	const struct command *command = command_init(COMMAND_WRITE, WRITE_NB_ARG);

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
int process_write(const struct command *command, const struct regmap *regmap,
				struct mfrc522_driver_dev *mfrc522_driver_dev)
{
	LOG("write: trying to write on card", LOG_EXTRA,
		mfrc522_driver_dev->log_level);
	int data_size = check_arg_size(command, mfrc522_driver_dev->log_level);
	int given_size = strlen(command->args[1]);

	if (data_size < 0) {
		LOG("write: check on arguments failed", LOG_ERROR,
			mfrc522_driver_dev->log_level);
			return -1;
	}
	if (data_size > INTERNAL_BUFFER_SIZE) {
		LOG("write: data too large, truncating", LOG_EXTRA,
			mfrc522_driver_dev->log_level);
		data_size = INTERNAL_BUFFER_SIZE;
	}
	if (data_size > given_size) {
		LOG("write: asking to write %d but only %d were given",
			LOG_ERROR, mfrc522_driver_dev->log_level, data_size, given_size);
		return -1;
	}

	if (flush_fifo(regmap, mfrc522_driver_dev->log_level) < 0)
		return -1;

	unsigned int i = 0;
	unsigned int data_wrote[25];

	memset(data_wrote, 0, 25);
	while (i < data_size) {
		int err = regmap_write(regmap, MFRC522_FIFODATAREG, *(*(command->args + 1) + i));
		if (err) {
			LOG("write: failed to write on card, aborting",
				LOG_ERROR, mfrc522_driver_dev->log_level);
			return -1;
		}
		data_wrote[i] = *(*(command->args + 1) + i);
		i++;
	}
	LOG("write: finished to write user content",
		LOG_EXTRA, mfrc522_driver_dev->log_level);

	while (i < INTERNAL_BUFFER_SIZE) {
		int err = regmap_write(regmap, MFRC522_FIFODATAREG, 0);

		if (err) {
			LOG("write: failed to fill FIFO with zeroes",
				LOG_ERROR, mfrc522_driver_dev->log_level);
			return -1;
		}
		data_wrote[i] = 0;
		i++;
	}

	// Writing from FIFO to card buffer
	if (regmap_write(regmap, MFRC522_CMDREG, MFRC522_MEM)) {
		LOG("write: failed to write FIFO to card buffer", LOG_ERROR,
			mfrc522_driver_dev->log_level);
		return -1;
	}

	LOG("write: operation successful", LOG_EXTRA, mfrc522_driver_dev->log_level);
	dump_trace(data_wrote, false, mfrc522_driver_dev->log_level);
	return INTERNAL_BUFFER_SIZE;
}
