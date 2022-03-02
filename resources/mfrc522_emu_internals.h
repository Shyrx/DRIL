#ifndef _MFRC522_EMU_INTERNALS_H
#define _MFRC522_EMU_INTERNALS_H

#include <linux/compiler-gcc.h>

/* Internal memory details
 */
#define MFRC522_FIFO_LEN    64
#define MFRC522_MAX_REG     0x40
#define MFRC522_INTERNAL_MEM_LEN     25

/* Device definition *
 *********************/

struct mfrc522_internals {
    /* Read/write FIFO, interfaced with FifoDataReg */
    u8 fifo[MFRC522_FIFO_LEN];
    u8 fifo_wr_ptr;
    u8 fifo_rd_ptr;

    /* Internal memory, interfaced with the FIFO, and used to
     * perform actual data processing. */
    u8 mem[MFRC522_INTERNAL_MEM_LEN];

    /* Don't bother trying to scrap some bytes, go from zero to
     * whatever address the last register is at. */
    u8 regs[MFRC522_MAX_REG];
};
void mfrc522_internals_init(struct mfrc522_internals *internals);

/* Register map manipulation *
 *****************************/

void mfrc522_regs_init(u8 *regs);
u8 mfrc522_reg_rd(struct mfrc522_internals *internals, u8 reg);
void mfrc522_reg_wr(struct mfrc522_internals *internals, u8 reg, u8 val);
void mfrc522_reg_update(
    struct mfrc522_internals *internals,
    u8 reg,
    u8 val,
    u8 mask);

/* FIFO manipulation *
 *********************/

unsigned int mfrc522_fifo_cur_size(const struct mfrc522_internals *internals);
/* Return the next byte from the FIFO, or zero if it is empty.
 */
u8 mfrc522_fifo_rd(struct mfrc522_internals *internals);
/* Please note that writing to the FIFO is destructive: even
 * if data had not been read yet, we overwrite it.
 */
void mfrc522_fifo_wr(struct mfrc522_internals *internals, u8 val);
void mfrc522_fifo_reset(struct mfrc522_internals *internals);

/* Internal buffer manipulation *
 ********************************/

u8 mfrc522_mem_rd(struct mfrc522_internals *internals, unsigned int i);
void mfrc522_mem_wr(
    struct mfrc522_internals *internals,
    unsigned int i,
    u8 val);

#endif /* _MFRC522_EMU_INTERNALS_H */
