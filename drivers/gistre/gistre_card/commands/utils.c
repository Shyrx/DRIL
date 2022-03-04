#include "utils.h"

#include <linux/slab.h>
#include <linux/string.h>

/**
 * @param buffer: source string to copy data from
 * @return an allocated string with data from buffer
 */
char *astrcpy(const char *buffer)
{
	char *new = kmalloc_array(strlen(buffer), sizeof(char), GFP_KERNEL);

	strcpy(new, buffer);
	return new;
}

int count_separator_occurence(const char *buffer, const char separator)
{
	int i = 0;
	int count = 0;

	while (*(buffer + i) != '\0') {
	if (*(buffer + i) == separator)
		count++;
	i++;
	}
	return count;
}

int check_arg_size(struct command *command, int log_level)
{
	long data_size;

	if (kstrtol(*command->args, 10, &data_size) != 0) {
		// add log here ?
		return -1;
	}
	return data_size;
}

int flush_fifo(struct regmap *regmap, int log_level)
{
	 LOG("flush: flushing fifo", LOG_EXTRA, log_level);
	 if (regmap_write(regmap, MFRC522_FIFOLEVELREG, MFRC522_FIFOLEVELREG_FLUSH)) {
		  LOG("flush: couldn't flush card, aborting", LOG_ERROR, log_level);
		  return -1;
	 }
	 return 0;
}

struct command *get_args(struct command *command, const char *buffer, int nb_args, const char *command_name)
{
	 char *new_buff = astrcpy(buffer);
	 char *tok = NULL;
	 char *sep = ":";
	 new_buff += strlen(command_name) + 1;
	 int i = 0;
	 while ((tok = strsep(&new_buff, sep)) != NULL)
		  // TODO: add log if LOG_EXTRA enabled to print all arguments parsed
		  *(command->args + i++) = astrcpy(tok);

	 kfree(new_buff);
	 return command;
}

void dump_trace(const unsigned int *data, bool reading, int log_level)
{
	 if (!(log_level & LOG_TRACE))
		  return;
	 LOG("Dumping trace:", LOG_TRACE, log_level);
	 printk(KERN_CONT "<%s>\n", reading ? "RD" : "WR");
	 int i = 0;

	 while (i < 5) {
		  int j = 0;
		  while (j < 5) {
			 printk(KERN_CONT "%02x%s", data[i * 5 + j], (j < 4 ? " " : ""));
			 j++;
		  }
		  printk(KERN_CONT "\n");
		  i++;
	 }
}
