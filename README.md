# Groupe
- antoine.sole
- thomas.crambert

Welcome to GISTRE Linux !

# Applying this patch

# How did we organize our work

The API of the driver is available in the files `mfrc522_driver.*` located in thedirectory `gistre_card`.
It exposes the syscalls to init the module, close it properly, write data to the card and read from it.
The implementation of the commands it can handle can be found in the dedicated c source file in the directory `gistre_card/commands`.

Currently, this driver handles the following commands:
- `mem_write:<len>:<data>`: write data into the internal FIFO of the card, destroy the existing data and always write exactly 25 bytes.
Takes two arguments:
    - `<len>`: the length of the data to write, truncated to 25 if higher
    - `<data>`: the data to write in the FIFO of the card, filled with null bytes if the size is not exactly 25.
- `mem_read`: read data from the internal FIFO of the card. Always read 25 bytes if there is something to read, otherwise read 0 byte.
Erase existing data and flush internal FIFO of the card. The read bytes are stored in the internal buffer of the module.
- `debug:<toggle>`: depending on the value of `toggle`, enable or disable all levels of debug log. Takes one argument:
    - `toggle`: if set to `on`, enable all levels of debug. Disable them if set to `off`. Print the current enabled levels if it set to `debug`.
- `debug:<command>:<level>:...:<level>`: depending on the value of `toggle`, enable or disable all given levels of debug log.
Takes a variadic number of parameters:
    - `<command>`: if set to `on`, enable the given levels of debug. Disable them if set to `off`.
    - `<level>:...:<level>`: a variadic number of debug level. Possible values are `info`, `trace`, `warn`, `extra`, `error`.
If the toggle is not supported  or a debug level is not found, does nothing.

# Questions

- Should there be no warning when running checkpath ?
- To what extand should this driver check the arguments given in input ?
- How do we toggle on the pr_debug logs on dmesg when running qemu ?
- How can we give option parameters to our module (level of debugs for example) ?
- Can we rename the `debug` command to `log` to make more sense with our code and logic ?

# Misc
