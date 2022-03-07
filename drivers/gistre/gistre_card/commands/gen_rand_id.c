#include "command.h"


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

}
