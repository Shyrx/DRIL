#include "commands.h"

#include <linux/slab.h>
#include <linux/string.h>

// TODO
struct command *command_init(void) {
    return NULL;
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

// REFACTO WHEN WORKING
struct command *parse_command(const char *buf){
    // check if command is valid
    char *useless;
    size_t len_write = strlen("mem_write");
    struct command *command = NULL;
    pr_info("Parsing command: %s\n", buf);
    if (!strncmp(buf, "mem_write", len_write)) {
        command = kmalloc(sizeof(struct command), GFP_KERNEL);
        command->args = kmalloc(sizeof(char *) * 2, GFP_KERNEL); // TODO
        command->nb_arg = 2; // TODO

        // parse until next ':' and check if it is <= 25
        if (!sscanf(buf, "%ms:%ms:%ms", &useless, command->args,
                    command->args + 1)) {
            command_free(command);
        }
        int write_size = strlen(*(command->args + 1));
        if (write_size > 25) {
            *(*(command->args + 1) + 25) = '\0';
        }
        command->command_type = COMMAND_WRITE;
    }
    else {
        // TODO: get only first verb
        pr_warn("Unknown command: %s", buf);
    }
    return command;
}

ssize_t exec_command(struct command *command, struct mfrc_dev *mfrc_dev) {
    command_free(command);
    return 0;
}
