#include "commands.h"

#include <linux/slab.h>
#include <linux/string.h>

/**
 * @param buffer: source string to copy data from
 * @return an allocated string with data from buffer
 */
static char* astrcpy(const char *buffer) {
    char *new = kmalloc(sizeof(char) * strlen(buffer), GFP_KERNEL);
    strcpy(new, buffer);
    return new;
}

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


static const char* map_command[] = {
[COMMAND_WRITE] = "mem_write",
[COMMAND_READ] = "mem_read",
};

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_WRITE
 */
struct command *parse_write(const char* buffer) {
    struct command *command = command_init(COMMAND_WRITE, 3);
    char *new_buff = astrcpy(buffer);
    char *tok = NULL;
    char *sep = ":";
    new_buff += strlen(map_command[COMMAND_WRITE]) + 1;
    int i = 0;
    while ((tok = strsep(&new_buff, sep)) != NULL) {
      *(command->args + i++) = astrcpy(tok);
      pr_info("arg %d: %s\n", i, tok);
    }

    kfree(new_buff);
    return command;
}

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_READ
 */
struct command *parse_read(const char*buffer) {
    struct command *command = command_init(COMMAND_READ, 0);
    return command;
}

typedef struct command* (*map_parse_command)(const char *buffer);

static const map_parse_command jump_parse[] = {
[COMMAND_WRITE] = parse_write,
[COMMAND_READ] = parse_read,
};

// REFACTO WHEN WORKING
struct command *parse_command(const char *buffer){
    pr_info("Parsing command: %s\n", buffer);
    enum COMMAND_TYPE command_type = 0;
    // kind of ugly, move into dedicated function ?
    while (command_type != COMMAND_NOT_FOUND
           && strncmp(buffer, map_command[command_type], strlen(map_command[command_type])) != 0) {
        command_type++;
    }

    if (command_type == COMMAND_NOT_FOUND) {
        pr_err("command not found: '%s'\n", buffer);
        return NULL;
    }

    return jump_parse[command_type](buffer);
}

// TODO
ssize_t exec_command(struct command *command, struct mfrc_dev *mfrc_dev) {
    command_free(command);
    return 0;
}
