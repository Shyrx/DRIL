#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>

#include "mfrc522.h"
#include "mfrc522_emu.h"
#include "mfrc522_emu_internals.h"

/* Wrappers over mfrc522_emu_internals.h *
 *****************************************/

#define MFRC522_REG_RD(mfrc522, reg) \
    mfrc522_reg_rd(&(mfrc522)->internals, reg)
#define MFRC522_REG_WR(mfrc522, reg, val) \
    mfrc522_reg_wr(&(mfrc522)->internals, reg, val)
#define MFRC522_REG_UPDATE(mfrc522, reg, val, mask) \
    mfrc522_reg_update(&(mfrc522)->internals, reg, val, mask)

#define MFRC522_FIFO_CUR_SIZE(mfrc522) \
    mfrc522_fifo_cur_size(&(mfrc522)->internals)
#define MFRC522_FIFO_RD(mfrc522) \
    mfrc522_fifo_rd(&(mfrc522)->internals)
#define MFRC522_FIFO_WR(mfrc522, val) \
    mfrc522_fifo_wr(&(mfrc522)->internals, val)
#define MFRC522_FIFO_RESET(mfrc522) \
    mfrc522_fifo_reset(&(mfrc522)->internals)

#define MFRC522_MEM_RD(mfrc522, i) \
    mfrc522_mem_rd(&(mfrc522)->internals, i)
#define MFRC522_MEM_WR(mfrc522, i, val) \
    mfrc522_mem_wr(&(mfrc522)->internals, i, val)

/* Forward declarations *
 ************************/

static bool mfrc522_readable_reg(struct device *dev, unsigned int reg);
static bool mfrc522_writeable_reg(struct device *dev, unsigned int reg);
static int mfrc522_reg_read(void *context, unsigned int reg, unsigned int *val);
static int mfrc522_reg_write(void *context, unsigned int reg, unsigned int val);

/* Global objects, e.g. the regmap configuration *
 *************************************************/

/* Register map configuration for the emulated MFRC522 device.
 * Same one for any instance of such a device.
 */
static struct regmap_config mfrc522_regmap_conf = {
    .reg_bits = 8,
    .reg_stride = 1,
    .val_bits = 8,
    .writeable_reg = mfrc522_writeable_reg,
    .readable_reg  = mfrc522_readable_reg,
    .reg_read  = mfrc522_reg_read,
    .reg_write = mfrc522_reg_write,
    .max_register = MFRC522_MAX_REG,
};

/* Emulated MFRC522 command set *
 ********************************/

static int mfrc522_mem(struct mfrc522_dev *mfrc522) {

    unsigned int i;

    if (MFRC522_FIFO_CUR_SIZE(mfrc522) > 0) {
        /* Write 25 bytes from FIFO to internal memory buffer */
        for (i = 0; i < MFRC522_INTERNAL_MEM_LEN; ++i) {
            MFRC522_MEM_WR(mfrc522, i, MFRC522_FIFO_RD(mfrc522));
        }
    }
    else {
        /* Read 25 bytes from internal memory buffer to FIFO */
        for (i = 0; i < MFRC522_INTERNAL_MEM_LEN; ++i) {
            MFRC522_FIFO_WR(mfrc522, MFRC522_MEM_RD(mfrc522, i));
        }
    }
    return 0;
}

static int mfrc522_gen_rand_id(struct mfrc522_dev *mfrc522) {

    const size_t nb_bytes = 10;
    u8 buf[nb_bytes];
    size_t i;

    get_random_bytes(buf, nb_bytes);

    /* As specified in the datasheet, this "overwrites the 10 bytes
     * in the internal 25-byte buffer". */
    for (i = 0; i < nb_bytes; ++i) {
        MFRC522_MEM_WR(mfrc522, i, buf[i]);
    }

    return 0;
}

static int mfrc522_softreset(struct mfrc522_dev *mfrc522) {

    mfrc522_regs_init(mfrc522->internals.regs);
    return 0;
}

/* High-level command set helpers *
 **********************************/

static bool mfrc522_cmd_can_autofinish(unsigned int cmd) {

    switch (cmd) {
    case MFRC522_IDLE:
    case MFRC522_MEM:
    case MFRC522_GENERATERANDOMID:
    case MFRC522_NOCMDCHANGE:
    case MFRC522_SOFTRESET:
        return true;
    default:
        return false;
    }
}

static int mfrc522_start_cmd(struct mfrc522_dev *mfrc522, unsigned int cmd) {

    int ret = 0;

    switch (cmd) {
    case MFRC522_IDLE:
        /* Do nothing */
        break;
    case MFRC522_MEM:
        ret = mfrc522_mem(mfrc522);
        break;
    case MFRC522_GENERATERANDOMID:
        ret = mfrc522_gen_rand_id(mfrc522);
        break;
    case MFRC522_NOCMDCHANGE:
        /* Do nothing */
        /* TODO: When asynchronous command execution will be
         * implemented, this will have to be handled higher
         * in the call stack, because contrary to Idle, the
         * current command is untouched. */
        break;
    case MFRC522_SOFTRESET:
        ret = mfrc522_softreset(mfrc522);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    if (mfrc522_cmd_can_autofinish(cmd)) {
        MFRC522_REG_WR(mfrc522, MFRC522_CMDREG, MFRC522_IDLE);
        /* Give back the command lock that the next command
         * start might be waiting on. This happens when the
         * command currently processed here can terminate
         * automatically, in which case we serialize access
         * to MFRC522_CMDREG to ease emulation work. */
        spin_unlock(&mfrc522->cmd_lock);
    }

    return ret;
}

int mfrc522_setup(struct mfrc522_dev *mfrc522, struct i2c_client *i2c) {

    struct regmap *regmap;

    regmap = devm_regmap_init(
        &i2c->dev,
        NULL /* no bus */,
        mfrc522 /* bus context */,
        &mfrc522_regmap_conf);
    if (IS_ERR(regmap)) {
        return 1;
    }
    mfrc522->regmap = regmap;
    spin_lock_init(&mfrc522->cmd_lock);
    mfrc522_internals_init(&mfrc522->internals);

    return 0;
}

void mfrc522_teardown(struct mfrc522_dev *mfrc522) {

    regmap_exit(mfrc522->regmap);
}

/* Regmap API *
 **************/

bool mfrc522_readable_reg(struct device *dev, unsigned int reg) {

    /* Unused */
    (void)dev;

    switch (reg) {
    case MFRC522_CMDREG:
    case MFRC522_FIFODATAREG:
    case MFRC522_FIFOLEVELREG:
        return true;
    default:
        return false;
    }
}

bool mfrc522_writeable_reg(struct device *dev, unsigned int reg) {

    /* Unused */
    (void)dev;

    switch (reg) {
    case MFRC522_CMDREG:
    case MFRC522_FIFODATAREG:
    case MFRC522_FIFOLEVELREG:
        return true;
    default:
        return false;
    }
}

int mfrc522_reg_read(void *context, unsigned int reg, unsigned int *val) {

    struct mfrc522_dev *mfrc522 = (struct mfrc522_dev *)context;

    switch (reg) {
    case MFRC522_FIFODATAREG:
        MFRC522_REG_WR(mfrc522, MFRC522_FIFODATAREG, MFRC522_FIFO_RD(mfrc522));
        break;
    case MFRC522_FIFOLEVELREG:
        MFRC522_REG_WR(
            mfrc522,
            MFRC522_FIFOLEVELREG,
            MFRC522_FIFO_CUR_SIZE(mfrc522));
    default:
        break;
    }

    *val = MFRC522_REG_RD(mfrc522, reg);

    /* A register may not be fully readable. Zero out all bits
     * that are not part of the readable mask, or are indicated
     * as reserved in the datasheet. */
    switch (reg) {
    case MFRC522_CMDREG:
        *val &=
            MFRC522_CMDREG_CMD         |
            MFRC522_CMDREG_SOFTPWRDOWN |
            MFRC522_CMDREG_RCVOFF;
        break;
    case MFRC522_FIFOLEVELREG:
        *val &= MFRC522_FIFOLEVELREG_LEVEL;
        break;
    default:
        break;
    }

    return 0;
}

int mfrc522_reg_write(void *context, unsigned int reg, unsigned int val) {

    struct mfrc522_dev *mfrc522 = (struct mfrc522_dev *)context;
    unsigned int mask = 0xFF;

    /* If the command can terminate by itself, wait it out:
     * cutting a command right in the middle is needlessly
     * complicated work for now.
     */
    if (reg == MFRC522_CMDREG &&
        mfrc522_cmd_can_autofinish(MFRC522_REG_RD(mfrc522, MFRC522_CMDREG))) {
        spin_lock(&mfrc522->cmd_lock);
    }

    /* A register may not be fully writeable. Specify all bits
     * that are part of the writeable mask. This does not
     * include bits indicated as reserved in the the datasheet.
     */
    switch (reg) {
    case MFRC522_CMDREG:
        mask = 
            MFRC522_CMDREG_CMD         |
            MFRC522_CMDREG_SOFTPWRDOWN |
            MFRC522_CMDREG_RCVOFF;
        break;
    case MFRC522_FIFOLEVELREG:
        mask = MFRC522_FIFOLEVELREG_FLUSH;
        break;
    default:
        break;
    }

    MFRC522_REG_UPDATE(mfrc522, reg, val, mask);

    switch (reg) {
    case MFRC522_CMDREG:
        /* TODO: Handle SOFTPWRDOWN and RCVOFF */
        mfrc522_start_cmd(mfrc522, MFRC522_REG_RD(mfrc522, MFRC522_CMDREG));
        break;
    case MFRC522_FIFODATAREG:
        MFRC522_FIFO_WR(mfrc522, val);
        break;
    case MFRC522_FIFOLEVELREG:
        if (MFRC522_REG_RD(mfrc522, MFRC522_FIFOLEVELREG) &
            MFRC522_FIFOLEVELREG_FLUSH) {
            /* TODO: Handle ErrorReg */
            MFRC522_FIFO_RESET(mfrc522);
            /* Don't forget to reset this bit */
            MFRC522_REG_UPDATE(mfrc522, reg, 0, MFRC522_FIFOLEVELREG_FLUSH);
        }
        break;
    default:
        BUG_ON(! "Invalid register should have already been detected");
        break;
    }

    return 0;
}

/* Public API *
 **************/

struct mfrc522_dev *dev_to_mfrc522(const struct device *dev) {

    return i2c_get_clientdata(container_of(dev, struct i2c_client, dev));
}

struct regmap *mfrc522_get_regmap(const struct mfrc522_dev *mfrc522) {

    return mfrc522->regmap;
}
