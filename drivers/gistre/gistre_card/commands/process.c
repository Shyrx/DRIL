#include <linux/fs.h>

#include "command.h"

/**
 * @param command: the struct command containing the arguments.
 * @param log_level: the current log_level (currently unused).
 * @return a positive integer if the arguments are valid.
 */
// TODO rename func/ change behavior
// TODO Move to utils
static int check_arg_size(struct command *command, int log_level)
{
    long data_size;
    if (kstrtol(*command->args, 10, &data_size) != 0) {
        // add log here ?
        return -1;
    }
    return data_size;
}
