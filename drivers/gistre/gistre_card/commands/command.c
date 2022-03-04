#include "command.h"

#include <linux/slab.h>
#include <linux/regmap.h>

#include "../../mfrc522.h"

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

// TODO Move to debug
static const char* jump_debug_to_string[] = {
[LOG_INFO] = "info",
[LOG_TRACE] = "trace",
[LOG_WARN] = "warn",
[LOG_EXTRA] = "extra",
[LOG_ERROR] = "error"
};

const char *enum_log_to_string_message(int log_level) {
    switch(log_level) {
        case LOG_INFO: return "[INFO] ";
        case LOG_TRACE: return "[TRACE] ";
        case LOG_WARN: return "[WARNING] ";
        case LOG_EXTRA: return "[DEBUG] ";
        case LOG_ERROR: return "[ERROR] ";
        default: return "";
    }
}


// ##### PARSING #####

static const char* map_command[] = {
[COMMAND_WRITE] = "mem_write",
[COMMAND_READ] = "mem_read",
[COMMAND_DEBUG] = "debug"
};

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

// ##### PROCESSING #####

typedef ssize_t (*map_process_command)(struct command *command, struct regmap *regmap, struct mfrc_dev *mfrc_dev);

static const map_process_command jump_process[] = {
[COMMAND_WRITE] = process_write,
[COMMAND_READ] = process_read,
[COMMAND_DEBUG] = process_debug
};

static struct regmap *find_regmap(void)
{
    return mfrc522_get_regmap(dev_to_mfrc522(mfrc522_find_dev()));
}

ssize_t process_command(struct command *command, struct mfrc522_driver_dev *mfrc522_driver_dev)
{
    // no need to check, would not reach this point if the command was unknown
    return jump_process[command->command_type](command, find_regmap(), mfrc522_driver_dev);
}

