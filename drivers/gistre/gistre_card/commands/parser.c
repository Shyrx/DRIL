#include "command.h"

#include <linux/slab.h>
#include <linux/string.h>

#define WRITE_NB_ARG 2

static const char* map_command[] = {
[COMMAND_WRITE] = "mem_write",
[COMMAND_READ] = "mem_read",
[COMMAND_DEBUG] = "debug"
};

/**
 * @param buffer: source string to copy data from
 * @return an allocated string with data from buffer
 */
static char *astrcpy(const char *buffer) {
    char *new = kmalloc(sizeof(char) * strlen(buffer), GFP_KERNEL);
    strcpy(new, buffer);
    return new;
}

static int count_separator_occurence(const char *buffer, const char separator) {
    int i = 0;
    int count = 0;
    while (*(buffer + i) != '\0') {
        if (*(buffer + i) == separator)
            count++;
        i++;
    }
    return count;
}

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_WRITE
 */
struct command *parse_write(const char* buffer) {
    struct command *command = command_init(COMMAND_WRITE, WRITE_NB_ARG);
    char *new_buff = astrcpy(buffer);
    char *tok = NULL;
    char *sep = ":";
    new_buff += strlen(map_command[COMMAND_WRITE]) + 1;
    int i = 0;
    while ((tok = strsep(&new_buff, sep)) != NULL && i < WRITE_NB_ARG) {
      *(command->args + i++) = astrcpy(tok);
      pr_info("arg %d: %s\n", i, tok);
    }
    kfree(new_buff);

    // in case there are too many arguments
    if (tok != NULL)
    {
        kfree(tok); // TODO: strsep free tok ?
        command_free(command);
        return NULL;
    }
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

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_DEBUG
 */
struct command *parse_debug(const char* buffer) {
    int nb_args = count_separator_occurence(buffer, ':'); // missing -1 ?
    pr_info("debug: nb args = %d\n", nb_args);
    struct command *command = command_init(COMMAND_DEBUG, nb_args);
    char *new_buff = astrcpy(buffer);
    char *tok = NULL;
    char *sep = ":";
    new_buff += strlen(map_command[COMMAND_DEBUG]) + 1;
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

typedef struct command* (*map_parse_command)(const char *buffer);

static const map_parse_command jump_parse[] = {
[COMMAND_WRITE] = parse_write,
[COMMAND_READ] = parse_read,
[COMMAND_DEBUG] = parse_debug,
};

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
    pr_info("Command found: %s\n", map_command[command_type]);
    return jump_parse[command_type](buffer);
}
