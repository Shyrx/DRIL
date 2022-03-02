#ifndef _MFRC522_EMU_H
#define _MFRC522_EMU_H

#include <linux/spinlock.h>

#include "mfrc522_emu_internals.h"

/* Forward declarations
 */
struct regmap;
struct regmap_config;
struct i2c_client;

/* Device definition *
 *********************/

struct mfrc522_dev {
    struct regmap *regmap;

    spinlock_t cmd_lock;

    struct mfrc522_internals internals;
};

/* Device API *
 **************/

// TODO: we can make this bus-agnostic
int mfrc522_setup(struct mfrc522_dev *mfrc522, struct i2c_client *i2c);
void mfrc522_teardown(struct mfrc522_dev *mfrc522);

#endif /* _MFRC522_EMU_H */
