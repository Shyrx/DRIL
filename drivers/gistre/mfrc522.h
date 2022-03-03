/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _MFRC522_H
#define _MFRC522_H

/* Device tree representation of the MFRC522 emulated device
 */
#define MFRC522_PROP_VERSION     "version"

/* Register map
 */
#define MFRC522_CMDREG    0x01
#define MFRC522_CMDREG_CMD          0x0F
#define MFRC522_CMDREG_SOFTPWRDOWN  0x10
#define MFRC522_CMDREG_RCVOFF       0x20
#define MFRC522_FIFODATAREG    0x09
#define MFRC522_FIFOLEVELREG    0x0A
#define MFRC522_FIFOLEVELREG_LEVEL  0x7F
#define MFRC522_FIFOLEVELREG_FLUSH  0x80

/* Command set
 */
#define MFRC522_IDLE                0x0
#define MFRC522_MEM                 0x1
#define MFRC522_GENERATERANDOMID    0x2
#define MFRC522_NOCMDCHANGE         0x7
#define MFRC522_SOFTRESET           0xF

/* Forward declarations
 */
struct device;
struct mfrc522_dev;

/* Find any MFRC522 device in the system, and return a pointer
 * to its core struct device object. Otherwise, return NULL.
 */
struct device *mfrc522_find_dev(void);

/* Get the complete MFRC522 device from its core struct device.
 */
struct mfrc522_dev *dev_to_mfrc522(const struct device *);

/* Get the regmap handle associated with the MFRC522 device.
 * This is the object you'll use to communicate with it.
 */
struct regmap *mfrc522_get_regmap(const struct mfrc522_dev *);

#endif /* _MFRC522_H */
