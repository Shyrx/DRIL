#include "commands.h"

#include <linux/slab.h>
#include <linux/string.h>

static char* my_strcpy(const char *buffer) {
    char *new = kmalloc(sizeof(char) * strlen(buffer), GFP_KERNEL);
    strcpy(new, buffer);
    return new;
}

// TODO
struct command *command_init(enum COMMAND_TYPE type, int nb_args) {
    struct command *command = kmalloc(sizeof(struct command), GFP_KERNEL);
    command->args = kmalloc(sizeof(char *) * nb_args, GFP_KERNEL);
    command->nb_arg = nb_args;
    command->command_type = type;
    return command;
}

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

struct command *parse_write(const char* buffer) {
    struct command *command = command_init(COMMAND_WRITE, 2);
    char *new_buff = my_strcpy(buffer);
    new_buff += strlen(map_command[COMMAND_WRITE]) + 1;
    char *tok = NULL;
    char *sep = ":";
    while ((tok = strsep(&new_buff, sep)) != NULL) {
        *(command->args + command->nb_arg++) = my_strcpy(tok);
        pr_info("arg %d: %s\n", command->nb_arg - 1, tok);
        kfree(tok);
    }

    kfree(new_buff);
    return command;
}

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

ssize_t exec_command(struct command *command, struct mfrc_dev *mfrc_dev) {
    command_free(command);
    return 0;
}
