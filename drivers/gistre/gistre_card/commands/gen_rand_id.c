#include "command.h"

#include <linux/slab.h>
#include <linux/random.h>

#include "utils.h"

#define BYTES_TO_GENERATE 10
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
	LOG("random: Starting to generate random numbers",
			LOG_EXTRA,
			mfrc522_driver_dev->log_level);

	if (regmap_write(regmap, MFRC522_CMDREG, MFRC522_GENERATERANDOMID)) {
		LOG("random: Failed to generated random number", LOG_ERROR,
			mfrc522_driver_dev->log_level);
	}
	return 0;
}
