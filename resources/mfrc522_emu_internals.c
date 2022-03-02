#include <linux/string.h>

#include "mfrc522.h"
#include "mfrc522_emu_internals.h"

void mfrc522_internals_init(struct mfrc522_internals *internals) {

    memset(internals->fifo, 0, MFRC522_FIFO_LEN);
    internals->fifo_wr_ptr = 0;
    internals->fifo_rd_ptr = 0;

    memset(internals->mem, 0, MFRC522_INTERNAL_MEM_LEN);

    mfrc522_regs_init(internals->regs);
}

void mfrc522_regs_init(u8 *regs) {

    /* Not all registers are handled for now. By default, their
     * value will have zero, which is perfectly fine if we don't
     * touch them anyway. */
    memset(regs, 0, MFRC522_MAX_REG);

    regs[MFRC522_CMDREG] = 0x20;
    /* FIFODATAREG is OK */
    /* FIFOLEVELREG is OK */
}

u8 mfrc522_reg_rd(struct mfrc522_internals *internals, u8 reg) {

    return internals->regs[reg];
}

void mfrc522_reg_wr(struct mfrc522_internals *internals, u8 reg, u8 val) {

    internals->regs[reg] = val;
}

void mfrc522_reg_update(
    struct mfrc522_internals *internals,
    u8 reg,
    u8 val,
    u8 mask) {

    u8 tmp = internals->regs[reg];
    tmp &= ~(mask);
    tmp |= val & (mask);
    internals->regs[reg] = tmp;
}

unsigned int mfrc522_fifo_cur_size(const struct mfrc522_internals *internals) {

    return internals->fifo_wr_ptr > internals->fifo_rd_ptr ?
     (internals->fifo_wr_ptr - internals->fifo_rd_ptr) :
     (internals->fifo_rd_ptr - internals->fifo_wr_ptr);
}

u8 mfrc522_fifo_rd(struct mfrc522_internals *internals) {

    if (mfrc522_fifo_cur_size(internals) == 0) {
        return 0;
    }
    else {
        u8 val = internals->fifo[internals->fifo_rd_ptr++];
        if (internals->fifo_rd_ptr == MFRC522_FIFO_LEN) {
            internals->fifo_rd_ptr = 0;
        }
        return val;
    }
}

void mfrc522_fifo_wr(struct mfrc522_internals *internals, u8 val) {

    internals->fifo[internals->fifo_wr_ptr++] = val;
    if (internals->fifo_wr_ptr == MFRC522_FIFO_LEN) {
        internals->fifo_wr_ptr = 0;
    }
}

void mfrc522_fifo_reset(struct mfrc522_internals *internals) {

    internals->fifo_wr_ptr = 0;
    internals->fifo_rd_ptr = 0;
}

u8 mfrc522_mem_rd(struct mfrc522_internals *internals, unsigned int i) {

    return internals->mem[i];
}

void mfrc522_mem_wr(
    struct mfrc522_internals *internals,
    unsigned int i,
    u8 val) {

    internals->mem[i] = val;
}
