#include "commands.h"

#include <linux/slab.h>
#include <linux/string.h>

struct command *parse_command(char *buf){
    // check if command is valid
    size_t len_write = strlen("mem_write");
    if (!strncmp(buf, "mem_write", len_write)) {
        // parse until next ':' and check if it is <= 25
        
        // get len next bytes
    }
    else if (!strncmp(buf, "mem_read", len_write)) {
        
    }
    else {
        // TODO: get only first verb
        pr_warn("Unknown command: %s", buf);
        return NULL;
    }
    

    // allocate memory
    struct command *command = kmalloc(sizeof(struct command));
    
    return NULL;
}
ssize_t exec_command(struct command *command, struct mfrc_dev *mfrc_dev) {
    return 0;
}
