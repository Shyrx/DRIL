# Groupe
- antoine.sole
- thomas.crambert

Welcome to GISTRE Linux !

# Applying this patch

# How did we organize our work

The API of the driver is available in the files `mfrc522_driver.*` located in the
directory `gistre_card`. It exposes the syscalls to init the module, close it properly, write data to the card and read from it.
The implementation of the commands it can handle can be found in the dedicated c source file in the directory `gistre_card/commands`.
Currently, this driver handles the following commands:
- `mem_write:<len>:<data>`: write data to the internal FIFO of the card, destroy the existing data and always write exactly 25 bytes.
Takes two arguments:
    - `<len>`: the length of the data to write, truncated to 25 if higher
    - `<data>`: the data to write in the FIFO of the card, filled with null bytes if the size is not exactly 25.
- `mem_read`: read data from the internal FIFO of the card. Always read 25 bytes if there is something to read, otherwise read 0 byte.
Erase existing data and flush internal FIFO of the card. The read bytes are stored in the internal buffer of the module.
- `debug:<toggle>`: depending on the value of `toggle`, enable or disable all levels of debug log. Takes one argument:
    - `toggle`: if set to `on`, enable all levels of debug. Disable them if set to `off`.
- `debug:<command>:<level>:...:<level>`: depending on the value of `toggle`, enable or disable all given levels of debug log.
Takes a variadic number of parameters:
    - `<command>`: if set to `on`, enable the given levels of debug. Disable them if set to `off`.
    - `<level>:...:<level>`: a variadic number of debug level. Possible values are `info`, `trace`, `warn`, `extra`, `error`.
If the toggle is not supported  or a debug level is not found, does nothing.

# Questions

# Misc

# ROADMAP

- [x] Clean code
- [x] Proper log everywhere
- [x] Tests by hand read/write more

- [1] Write README
- [2] Apply checkpath
- [3] Documents all functions
- [4] Add debug:status
- [5] Implement random command
- [6] Logs with format string (if possible)
- [7] Add color on debug (if possible)

