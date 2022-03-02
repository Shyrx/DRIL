#ifndef COMMANDS_H
#define COMMANDS_H

#include <linux/kernel.h>
#include <linux/module.h>
#include "mfrc.h"

struct mfrc_dev;

struct command {
    char *name;
    char **args;
};

struct command *parse_command(char *buf);
ssize_t exec_command(struct command *command, struct mfrc_dev *mfrc_dev);

#endif
