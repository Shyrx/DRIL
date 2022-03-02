#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "mfrc522.h"
#include "mfrc522_emu.h"

MODULE_AUTHOR("Geoffrey Le Gourriérec");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("An emulated MFRC522 device.");

/* Helper macros *
 *****************/

/* Only a single emulated device is handled for now.
 */
#define MFRC522_MAX_NB_EMULATED_DEVS      1

#define MFRC522_I2C_DEVICE_ID    0

#define MFRC522_NAME             "mfrc522_emu"

/* Forward declarations *
 ************************/

static int mfrc522_probe(struct i2c_client *, const struct i2c_device_id *);
static int mfrc522_remove(struct i2c_client *);

/* Global objects, e.g. the I2C driver *
 ***************************************/

/* All emulated MFRC522 devices
 */
static struct mfrc522_dev *mfrc522_emu_devs[MFRC522_MAX_NB_EMULATED_DEVS];

/* We can't avoid declaring both i2c and of device IDs, see below */
static struct i2c_device_id mfrc522_i2c_ids[] = {
    { "mfrc522_emu", MFRC522_I2C_DEVICE_ID },
    { /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, mfrc522_i2c_ids);

static struct of_device_id mfrc522_of_ids[] = {
    { .compatible = "gistre,mfrc522_emu", },
    { /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mfrc522_of_ids);

static struct i2c_driver mfrc522_drv = {
    .driver = {
        .name = "mfrc522_emu",
        .owner = THIS_MODULE,
        .of_match_table = mfrc522_of_ids,
    },
    /* For kernels <= 4.10, it is still mandatory to specify the
     * i2c_device_id event if we're using device tree, contrary
     * to SPI drivers. See chapter 7 of "Linux Device Drivers
     * Development" (Packt) by John Madieu.
     */
    .id_table = mfrc522_i2c_ids,
    .probe = mfrc522_probe,
    .remove = mfrc522_remove,
};

/* Helper functions *
 ********************/

static int mfrc522_match_dev_by_name(struct device *dev, void *data) {

    const char *name = data;

    return strcmp(name, dev->of_node->name) == 0;
}

static struct mfrc522_dev **mfrc522_find_dev_slot(void) {

    size_t i = 0;

    for (; i < MFRC522_MAX_NB_EMULATED_DEVS; ++i) {
        if (mfrc522_emu_devs[i] == NULL) {
            return &mfrc522_emu_devs[i];
        }
    }
    return NULL;
}

static void mfrc522_free_dev_slot(struct mfrc522_dev *mfrc522) {

    size_t i = 0;

    for (; i < MFRC522_MAX_NB_EMULATED_DEVS; ++i) {
        if (mfrc522_emu_devs[i] == mfrc522) {
            mfrc522_emu_devs[i] = NULL;
        }
    }
}

/* I2C driver API *
 ******************/

int mfrc522_probe(struct i2c_client *i2c, const struct i2c_device_id *id) {

    struct mfrc522_dev **mfrc522;

    /* Unused */
    (void)id;

    mfrc522 = mfrc522_find_dev_slot();
    if (! mfrc522) {
        return -ENOTSUPP;
    }
    *mfrc522 = kzalloc(sizeof(**mfrc522), GFP_KERNEL);
    if (! *mfrc522) {
        return -ENOMEM;
    }
    if (mfrc522_setup(*mfrc522, i2c)) {
        kfree(*mfrc522);
        mfrc522_free_dev_slot(*mfrc522);
        return 1;
    }
    i2c_set_clientdata(i2c, *mfrc522);

    return 0;
}

int mfrc522_remove(struct i2c_client *i2c) {

    struct mfrc522_dev *mfrc522;

    mfrc522 = i2c_get_clientdata(i2c);
    mfrc522_teardown(mfrc522);
    kfree(mfrc522);
    mfrc522_free_dev_slot(mfrc522);

    return 0;
}

/* Public API *
 **************/

struct device *mfrc522_find_dev(void) {

    char *name = MFRC522_NAME;

    return bus_find_device(
        &i2c_bus_type,
        NULL,
        name,
        mfrc522_match_dev_by_name);
}

/* Initialization and Removal *
 ******************************/

__init
static int mfrc522_init(void) {

	pr_info("Hello, emulated MFRC522 !\n");

    if (i2c_add_driver(&mfrc522_drv) < 0) {
        return 1;
    }

	return 0;
}

__exit
static void mfrc522_exit(void) {

    i2c_del_driver(&mfrc522_drv);

    pr_info("Goodbye, emulated MFRC522...\n");
}

module_init(mfrc522_init);
module_exit(mfrc522_exit);
