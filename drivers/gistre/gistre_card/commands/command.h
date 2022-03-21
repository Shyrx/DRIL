#ifndef COMMAND_H
#define COMMAND_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/regmap.h>

#include "../mfrc522_driver.h"
#include "../../mfrc522.h"

struct mfrc_dev;

enum COMMAND_TYPE {
	COMMAND_READ = 0,
	COMMAND_WRITE,
	COMMAND_DEBUG,
	COMMAND_RANDOM,
	COMMAND_NOT_FOUND,
};

enum LOG_LEVEL {
LOG_NONE = 0,
LOG_INFO = 1,
LOG_TRACE = 2,
LOG_WARN = 4,
LOG_EXTRA = 8,
LOG_ERROR = 16,
LOG_NOT_FOUND = 32,
};

const char *enum_log_to_string_message(int log_level);
const char *format_message(const char *fmt, ...);
enum LOG_LEVEL find_log_level(const char *level, int log_level);

#define LOG(message, level_required, log_level, ...) \
	do { \
		if (log_level & level_required) { \
			printk(KERN_CONT "%s%s\n", \
				   enum_log_to_string_message(level_required), \
				   format_message(message, ##__VA_ARGS__)); \
		} \
	} while (0)

struct command {
	enum COMMAND_TYPE command_type;
	char **args;
	int nb_arg;
};

struct command *command_init(enum COMMAND_TYPE type, int nb_args);
void command_free(const struct command *command);

struct command *parse_command(const char *buf, int log_level);
struct command *parse_write(const char *buffer, int log_level);
struct command *parse_read(const char *buffer, int log_level);
struct command *parse_debug(const char *buffer, int log_level);
struct command *parse_random(const char *buffer, int log_level);

int process_command(const struct command *command,
					struct mfrc522_driver_dev *mfrc522_driver_dev);
int process_write(const struct command *command, const struct regmap *regmap,
				  struct mfrc522_driver_dev *mfrc522_driver_dev);
int process_read(const struct command *command, const struct regmap *regmap,
				 struct mfrc522_driver_dev *mfrc522_driver_dev);
int process_debug(const struct command *command, const struct regmap *regmap,
				  struct mfrc522_driver_dev *mfrc522_driver_dev);
int process_random(const struct command *command, const struct regmap *regmap,
				  struct mfrc522_driver_dev *mfrc522_driver_dev);

#endif
