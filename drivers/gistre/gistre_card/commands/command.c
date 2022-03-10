#include "command.h"

#include <linux/slab.h>

#include "../../mfrc522.h"

/**
 * @param type: the type of the command
 * @param nb_args: the args the command takes
 * @return an allocated struct with every field correctly allocated
 */
struct command *command_init(enum COMMAND_TYPE type, int nb_args)
{
	struct command *command = kmalloc(sizeof(struct command), GFP_KERNEL);

	command->args = kmalloc_array(nb_args, sizeof(char *), GFP_KERNEL);
	command->nb_arg = nb_args;
	command->command_type = type;
	return command;
}

/**
 * @param type: the type of the command
 * @param nb_args: the args the command takes
 * @return an allocated struct with every allocated
 */
void command_free(const struct command *command)
{
	int i = 0;

	while (i < command->nb_arg) {
	kfree(*(command->args + i));
	i++;
	}
	kfree(command->args);
	kfree(command);
}

// ############################ PARSING #################################

static const char * const map_command[] = {
[COMMAND_WRITE] = "mem_write",
[COMMAND_READ] = "mem_read",
[COMMAND_DEBUG] = "debug",
[COMMAND_RANDOM] = "gen_rand_id",
};

typedef struct command* (*map_parse_command)(const char *buffer, int log_level);

static const map_parse_command jump_parse[] = {
[COMMAND_WRITE] = parse_write,
[COMMAND_READ] = parse_read,
[COMMAND_DEBUG] = parse_debug,
[COMMAND_RANDOM] = parse_random,
};

/**
 * @param buffer: the command to parse
 * @param log_level: current log enable
 * @return a struct command corresponding to the parsed command. Caution,
 * only sanity checks are done.
 */
struct command *parse_command(const char *buffer, int log_level)
{
	LOG("parse: parsing command: '%s'", LOG_INFO, log_level, buffer);
	enum COMMAND_TYPE command_type = 0;
	// kind of ugly, move into dedicated function ?
	while (command_type != COMMAND_NOT_FOUND
			&& strncmp(buffer, map_command[command_type],
					  strlen(map_command[command_type])) != 0) {
		command_type++;
	}

	if (command_type == COMMAND_NOT_FOUND)
		return NULL;

	return jump_parse[command_type](buffer, log_level);
}

// ############################ PROCESSING #################################

typedef int (*map_process_command)
(const struct command *command, const struct regmap *regmap,
struct mfrc522_driver_dev *mfrc522_driver_dev);

static const map_process_command jump_process[] = {
[COMMAND_WRITE] = process_write,
[COMMAND_READ] = process_read,
[COMMAND_DEBUG] = process_debug,
[COMMAND_RANDOM] = process_random,
};

static struct regmap *find_regmap(void)
{
	return mfrc522_get_regmap(dev_to_mfrc522(mfrc522_find_dev()));
}

int process_command(const struct command *command,
					struct mfrc522_driver_dev *mfrc522_driver_dev)
{
	return jump_process[command->command_type]
		(command, find_regmap(), mfrc522_driver_dev);
}

