#ifndef COMMANDS_H
#define COMMANDS_H

#include <linux/kernel.h>
#include <linux/module.h>
#include "mfrc.h"

struct mfrc_dev;

enum COMMAND_TYPE {
    COMMAND_READ = 0,
    COMMAND_WRITE,
};

struct command {
    enum COMMAND_TYPE command_type;
    char **args;
    int nb_arg;
};

struct command *parse_command(const char *buf);
ssize_t exec_command(struct command *command, struct mfrc_dev *mfrc_dev);

#endif
