#ifndef COMMAND_H
#define COMMAND_H

#include <linux/kernel.h>
#include <linux/module.h>

#include "../mfrc.h"

struct mfrc_dev;

enum COMMAND_TYPE {
    COMMAND_READ = 0,
    COMMAND_WRITE,
    COMMAND_DEBUG,
    COMMAND_NOT_FOUND,
};

enum DEBUG_LEVEL {
    DEBUG_NOT_FOUND = -1,
    DEBUG_OFF = 0,
    DEBUG_INFO = 1,
    DEBUG_WARN = 2,
    DEBUG_EXTRA = 4,
    DEBUG_ERROR = 8
};

inline const char *enum_debug_to_string_message(int debug_level) {
        switch(debug_level) {
    case DEBUG_INFO: return "INFO: ";
    case DEBUG_WARN: return "WARNING: ";
    case DEBUG_EXTRA: return "DEBUG: ";
    case DEBUG_ERROR: return "ERROR: ";
    default: return "";
        }
}

// TODO: check if it respects the coding style
#define PRINT_DEBUG(message, level_required, log_level)                 \
    if (log_level & level_required) {                                   \
        pr_info("%s%s\n", enum_debug_to_string_message(level_required), message); \
    }

struct command {
    enum COMMAND_TYPE command_type;
    char **args;
    int nb_arg;
};


struct command *parse_command(const char *buf);
ssize_t process_command(struct command *command, struct mfrc_dev *mfrc_dev);
struct command *command_init(enum COMMAND_TYPE type, int nb_args);
void command_free(struct command *command);

#endif
