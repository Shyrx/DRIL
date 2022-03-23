# Groupe
- antoine.sole
- thomas.crambert

Welcome to GISTRE Linux !

# How did we organize our work

The API of the driver is available in the files `mfrc522_driver.*` located in thedirectory `gistre_card`.
It exposes the syscalls to init the module, close it properly, write data to the card and read from it.
The implementation of the commands it can handle can be found in the dedicated c source file in the directory `gistre_card/commands`.

# Commands

- `mem_write:<len>:<data>`: write data into the internal FIFO of the card, destroy the existing data and always write exactly 25 bytes.
Takes two arguments:
    - `<len>`: The length of the data to write, truncated to 25 if higher. Must be a number. The FIFO will be filled with '\0' up to 25 if len is less than 25.
    - `<data>`: The data to write in the FIFO of the card. If the given data is less than the len arguments, it crashes. If there is more, it is truncated.

- `mem_read`: Read data from the internal FIFO of the card. Always read 25 bytes if there is something to read, otherwise read 0 byte. Erase existing data and flush internal FIFO of the card. The read bytes are stored in the internal buffer of the device.

- `debug:<toggle>`: depends on the value of `toggle`.
  - `on`: Enable all logs.
  - `off`: Disable all logs.
  - `status`: Show which levels of log are enabled.

- `debug:[on|off]:<level>:...:<level>`: enable or disable all given levels of debug log.
Takes a variadic number of parameters:
    - `<level>:...:<level>`: a variadic number of debug level. Possible values are `info`, `trace`, `warn`, `extra`, `error`.
If the toggle is not supported  or a debug level is not found, does nothing.

- `gen_rand_id`: Fill 10 bytes with random values and write them to the internal FIFO of the card.

# Module parameters

- `debug`: Takes a string. It corresponds to a column separated list of values which correspond to the differents debug level. These are the debug levels that will be enabled on startup. Default value is `error`.

- `nb_devices`: Takes a positive integer, set the number of devices to create for the module. Default value is 1.

- `quiet`: Takes a boolean, disable all logs at startup if set. Default value is false. Has priority over `debug`.

