#ifndef UTILS_H
#define UTILS_H

#include "command.h"

char *astrcpy(const char *buffer);
int count_separator_occurence(const char *buffer, const char separator);
int check_arg_size(struct command *command, int log_level);
int flush_fifo(struct regmap *regmap, int log_level);

struct command *get_args(struct command *command, const char *buffer,
						 int nb_args, const char *command_name);
#endif // UTILS_H
