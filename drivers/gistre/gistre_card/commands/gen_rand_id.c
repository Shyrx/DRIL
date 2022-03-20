#include "command.h"

#include <linux/slab.h>
#include <linux/random.h>

#include "utils.h"

#define BYTES_TO_GENERATE 10
// mem_write:10:
#define WRITE_COMMAND_LENGTH 13

struct command *parse_random(const char *buffer, int log_level)
{
	int nb_args = count_separator_occurence(buffer, ':');

	if (nb_args != 0) {
		LOG("random: expected no arguments, but some were given",
			LOG_ERROR, log_level);
		return NULL;
	}

	return command_init(COMMAND_RANDOM, 0);
}

int process_random(const struct command *command, const struct regmap *regmap,
				   struct mfrc522_driver_dev *mfrc522_driver_dev)
{
	LOG("random: Starting to generate random numbers", LOG_EXTRA, mfrc522_driver_dev->log_level);
	char *rand = kmalloc_array(WRITE_COMMAND_LENGTH + BYTES_TO_GENERATE, sizeof(char), GFP_KERNEL);

	if (rand == NULL) {
		LOG("random: failed to allocate array for random number", LOG_ERROR, mfrc522_driver_dev->log_level);
		return -1;
	}

	memcpy(rand, "mem_write:10:", WRITE_COMMAND_LENGTH);
	get_random_bytes(rand + WRITE_COMMAND_LENGTH, BYTES_TO_GENERATE);
	LOG("random: generated bytes are: '%s'", LOG_EXTRA, mfrc522_driver_dev->log_level, rand + WRITE_COMMAND_LENGTH);

	struct command *command_write = parse_write(rand, mfrc522_driver_dev->log_level);

	int res = process_write(command_write, regmap, mfrc522_driver_dev);

	command_free(command_write);
	return res;
}
