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

// TODO Docu
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
