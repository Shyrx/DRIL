#include <linux/slab.h>

#include "command.h"
#include "utils.h"

#define DEBUG_NAME "debug"
#define ENABLE_ALL_LOGS \
	(LOG_ERROR | LOG_WARN | LOG_EXTRA | LOG_TRACE | LOG_INFO)

static const char * const jump_debug_to_string[] = {
[LOG_INFO] = "info",
[LOG_TRACE] = "trace",
[LOG_WARN] = "warn",
[LOG_EXTRA] = "extra",
[LOG_ERROR] = "error"
};

enum DEBUG_OPE {
	DEBUG_UNKOWN = -1,
	DEBUG_ON = 0,
	DEBUG_OFF = 1,
	DEBUG_STATUS = 2,
};

/**
 * @param log_level: the log level to get the string representation
 * @return the string representation of the given log_level
 */
const char *enum_log_to_string_message(int log_level)
{
	switch (log_level) {
	case LOG_INFO: return "[INFO] ";
	case LOG_TRACE: return "[TRACE] ";
	case LOG_WARN: return "[WARNING] ";
	case LOG_EXTRA: return "[DEBUG] ";
	case LOG_ERROR: return "[ERROR] ";
	default: return "";
	}
}

// Can't use LOG in this function as it is used in LOG
// FIXME Docu
const char *format_message(const char *fmt, ...)
{
	va_list ap;
	int size = 100;
	char *result;
	int n;

	result = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (!result)
		return "Error allocating size";

	while (1) {
		va_start(ap, fmt);
		n = vsnprintf(result, size, fmt, ap);
		va_end(ap);

		if (n < 0)
				return "Error formatting string";

		if (n < size)
			return result;

		// There was not enough size in the buffer so we extend it
		size += 100;
		char *tmp;

		tmp = kmalloc_array(size, sizeof(char), GFP_KERNEL);
		if (!tmp) {
			kfree(result);
			return "Error allocating size";
		}

		memcpy(tmp, result, size - 100);
		kfree(result);
		result = tmp;
	}

}

/**
 * @param buffer: the buffer containing the data to process
 * @return an allocated struct of kind COMMAND_DEBUG
 */
struct command *parse_debug(const char *buffer, int log_level)
{
	int nb_args = count_separator_occurence(buffer, ':');

	if (nb_args == 0) {
		LOG("debug: expected at least one argument, but none were given",
			LOG_ERROR, log_level);
		return NULL;
	}
	struct command *command = command_init(COMMAND_DEBUG, nb_args);

	return get_args(command, buffer, nb_args, DEBUG_NAME);
}

static void print_enabled_log_levels(int log_level)
{
	char enabled_logs[200];
	int i = 1;
	int current_size = 0;

	memset(enabled_logs, 0, 200);
	while (i < LOG_NOT_FOUND) {
		if (i & log_level) {
			const char *level_to_string = jump_debug_to_string[i];
			current_size +=
				snprintf(&enabled_logs[current_size],
						 current_size + strlen(level_to_string) + 1, "%s%s",
						 current_size == 0 ? "" : ", ", level_to_string);
		}
		i <<= 1;
	}
	char to_print[250];

	memset(to_print, 0, 200);
	snprintf(to_print, current_size + strlen("Enabled levels of debug: ") + 1,
			 "Enabled levels of debug: %s", enabled_logs);
	LOG(to_print, LOG_EXTRA, LOG_EXTRA);
}

/**
 * @param buffer: the buffer containing the command
 * @return an enum with value corresponding to the kind of the debug operation to perform
 */
static enum DEBUG_OPE get_debug_op(char *buffer, int log_level)
{
	if (strcmp(buffer, "on") == 0) {
		LOG("debug: enabling log levels...", LOG_EXTRA, log_level);
		return DEBUG_ON;
	}
	if (strcmp(buffer, "off") == 0) {
		LOG("debug: disabling log levels...", LOG_EXTRA, log_level);
		return DEBUG_OFF;
	}
	if (strcmp(buffer, "status") == 0)
		return DEBUG_STATUS;

	LOG("debug: first argument should be 'on', 'off' or 'status', but was something else",
		LOG_ERROR, log_level);
	return DEBUG_UNKOWN;
}

static enum LOG_LEVEL find_log_level(char *level, int log_level)
{
	int i = 1;
	// TODO ugly, should be changed
	while (i < LOG_NOT_FOUND
			&& strcmp(level, jump_debug_to_string[i]) != 0)
		i <<= 1;

	if (i == LOG_NOT_FOUND)
		LOG("debug: unidentified debug level", LOG_ERROR, log_level);

	return i;
}

/**
 * @param command: the struct command containing what is needed to perform
 * a `debug` call, need not to be checked beforehand.
 * @param regmap: a struct containing the API used to communicate with the card.
 * @param mfrc_dev: a struct containing the data related to the current context
of the device.
 * @return a negative integer if an error occured, zero otherwise.
 */

int process_debug(const struct command *command, const struct regmap *regmap,
				struct mfrc522_driver_dev *mfrc522_driver_dev)
{
	int current_level = mfrc522_driver_dev->log_level;
	enum DEBUG_OPE debug_op = get_debug_op(command->args[0], mfrc522_driver_dev->log_level);

	switch (debug_op) {
	case DEBUG_ON:
		if (command->nb_arg > 1)
			break;
		mfrc522_driver_dev->log_level = ENABLE_ALL_LOGS;
		LOG("debug: enabling all logs", LOG_INFO, mfrc522_driver_dev->log_level);
		return 0;
	case DEBUG_OFF:
		if (command->nb_arg > 1)
			break;
		LOG("debug: disabling all logs", LOG_INFO, mfrc522_driver_dev->log_level);
		mfrc522_driver_dev->log_level = 0;
		return 0;
	case DEBUG_STATUS:
		print_enabled_log_levels(mfrc522_driver_dev->log_level);
		return 0;
	default:
		return -1;
	}

	int i = 1;

	while (i < command->nb_arg) {
		enum LOG_LEVEL log_level = find_log_level(*(command->args + i), mfrc522_driver_dev->log_level);

		if (log_level == LOG_NOT_FOUND)
			return -1;

		if (debug_op == DEBUG_ON)
			current_level |= log_level;
		else
			current_level &= ~log_level;
		i++;
	}
	LOG("debug: log mode updated successfully", LOG_EXTRA, mfrc522_driver_dev->log_level);
	mfrc522_driver_dev->log_level = current_level;
	return 0;
}
