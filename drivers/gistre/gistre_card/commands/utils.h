#ifndef UTILS_H
#define UTILS_H

#include "command.h"

/**
 * @param buffer: the source string to copy data from
 * @return an allocated string with data from buffer
 */
char *astrcpy(const char *buffer);

/**
 * @param buffer: the source string
 * @param separator: the separator to count
 * @return the number of occurences of @separator
 */
int count_separator_occurence(const char *buffer, const char separator);

/**
 * @param command: the source command
 * @param log_level: the current enabled logs levels
 * @return the number of arguments taken by the commands
 */
int check_arg_size(const struct command *command, int log_level);

/**
 * @param regmap: the regmap API to interact with
 * @param log_level: the current enabled log levels
 * @return 0 if the operation was successful, -1 otherwise
 */
int flush_fifo(const struct regmap *regmap, int log_level);

/**
 * @param data: the regmap API to interact with
 * @param reading: if the call was made from a reading or a writing operation
 * @param log_level: the current enabled log levels
 */
void dump_trace(const unsigned int *data, bool reading, int log_level);

/**
 * @param command: the regmap API to interact with
 * @param buffer: if the call was made from a reading or a writing operation
 * @param nb_args: the current enabled log levels
 * @param command_name: the current enabled log levels
 * @return a fully initialized struct command, also modifies the @command param in place
 */
struct command *get_args(struct command *command, const char *buffer,
						 int nb_args, const char *command_name);

/**
 * @param log_levels_list: a list of logs to enable at startup, given from the module param
 * @return a fully initialized struct command, also modifies the @command param in place
 */
int process_logs_module_param(const char *log_levels_list);

#endif // UTILS_H
