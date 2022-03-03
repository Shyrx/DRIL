#include "command.h"

/**
 * @param type: the type of the command
 * @param nb_args: the args the command takes
 * @return an allocated struct with every allocated
 */
struct command *command_init(enum COMMAND_TYPE type, int nb_args) {
    struct command *command = kmalloc(sizeof(struct command), GFP_KERNEL);
    command->args = kmalloc(sizeof(char *) * nb_args, GFP_KERNEL);
    command->nb_arg = nb_args;
    command->command_type = type;
    return command;
}

/**
 * @param type: the type of the command
 * @param nb_args: the args the command takes
 * @return an allocated struct with every allocated
 */
void command_free(struct command *command) {
    int i = 0;
    while (i < command->nb_arg) {
        kfree(*(command->args + i));
        i++;
    }
    kfree(command->args);
    kfree(command);
}
