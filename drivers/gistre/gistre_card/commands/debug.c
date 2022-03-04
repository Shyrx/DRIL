#include "command.h"

#include <linux/slab.h>

#include "utils.h"

#define DEBUG_NAME "debug"

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_DEBUG
 */
struct command *parse_debug(const char* buffer) 
{
	int nb_args = count_separator_occurence(buffer, ':'); // missing -1 ?

	pr_info("debug: nb args = %d\n", nb_args);
	struct command *command = command_init(COMMAND_DEBUG, nb_args);
	char *new_buff = astrcpy(buffer);
	char *tok = NULL;
	char *sep = ":";

	new_buff += strlen(DEBUG_NAME) + 1;
	int i = 0;

	while ((tok = strsep(&new_buff, sep)) != NULL && i < nb_args) {
	  *(command->args + i++) = astrcpy(tok);
	  pr_info("arg %d: %s\n", i, tok);
	}
	kfree(new_buff);

	// in case there are too many arguments
	if (tok != NULL)
	{
	pr_info("debug: too many args\n");
	kfree(tok); // TODO: strsep free tok ?
	command_free(command);
	return NULL;
	}
	return command;
}

static int set_log(char *buffer, int log_level) 
{
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
ssize_t process_debug(struct command *command, struct regmap *regmap /* unused */, struct mfrc_dev *mfrc_dev) 
{
	int current_level = mfrc_dev->log_level;
	int set = set_log(command->args[0], mfrc_dev->log_level);

	if (set == -1)
	return -1;

	int i = 1;

	while (i < command->nb_arg) {
	enum LOG_LEVEL log_level = find_log_level(*(command->args + i), mfrc_dev->log_level);

	if (log_level == LOG_NOT_FOUND) {
		return -1;
	}

	if (set) {
		current_level |= log_level;
		} else {
		current_level &= ~log_level;
	}
	i++;
	}

	LOG("debug: log mode updated successfully", LOG_EXTRA, mfrc522_driver_dev->log_level);
	mfrc522_driver_dev->log_level = current_level;
	return 0;
}
