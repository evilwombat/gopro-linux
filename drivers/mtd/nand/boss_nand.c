/*
 * drivers/mtd/nand/boss_nand.c
 *
 * Authors:
 *	Charles Chiou <cchiou@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Copyright (C) 2009-2010, Ambarella Inc.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>

#include <linux/aipc/i_flpart.h>
#include <linux/aipc/i_nand.h>

#define BUFFER_SIZE	4096
#define MAX_PARTITIONS	32

/*
 * MTD structure for BOSS.
 */
struct boss_nand_s
{
	struct nand_chip chip;
	struct mtd_info mtd;
	struct nand_hw_control controller;
	struct mtd_partition part[MAX_PARTITIONS];
	char part_name[MAX_PARTITIONS][32];
	unsigned int npart;

	struct device *dev;

	char *buf;
	unsigned int pos;

	unsigned int seqin_column;
	unsigned int seqin_page_addr;
};

/*
 * ECC layout for 512 page-sized devices.
 */
static struct nand_ecclayout ecclayout_512 = {
	.eccbytes = 6,
	.eccpos = { 5, 6, 7, 8, 9, 10, },
	.oobfree = {
		 {  0, 5, },
		 { 11, 5, },
	 },
};

/*
 * ECC layout for 2K page-sized devices.
 */
static struct nand_ecclayout ecclayout_2k = {
	.eccbytes = 24,
	.eccpos = {
		 0,  8,  9, 10 ,11, 12,
		16, 24, 25, 26, 27, 28,
		32, 40, 41, 42, 43, 44,
		48, 56, 57, 58, 59, 60,
	},
	.oobfree = {
		 {  1, 7, },
		 { 13, 3, },
		 { 17, 7, },
		 { 29, 3, },
		 { 33, 7, },
		 { 45, 3, },
		 { 49, 7, },
		 { 61, 3, },
	 },
};

static uint8_t amb_scan_ff_patten[] = { 0xff, 0xff, };

/*
 * Bad block scan pattern for 512 page-sized devices.
 */
static struct nand_bbt_descr bbt_descr_512 = {
	.offs = 5,
	.len = 1,
	.pattern = amb_scan_ff_patten,
};

/*
 * Bad block scan pattern for 2K page-sized devices.
 */
static struct nand_bbt_descr bbt_descr_2k = {
	.offs = 0,
	.len = 1,
	.pattern = amb_scan_ff_patten,
};

/*
 * Address conversion from page_addr.
 */
static inline void addr_conv(struct mtd_info *mtd, int page_addr,
			     unsigned int *addr_hi, unsigned int *addr)
{
	u64 addr64;

	addr64 = (u64) page_addr;
	addr64 *= mtd->writesize;
	*addr_hi = (unsigned int) (addr64 >> 32);
	*addr = (unsigned int) addr64;
}

/*
 * Hardware ecc generator.
 */
static void boss_nand_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	/* do nothing */
}

/*
 * Readback ecc from hardware....
 */
static int boss_nand_ecc_calculate(struct mtd_info *mtd,
				    const u_int8_t *dat, u_char *ecc_code)
{
	/* This is magic from rrcar's original NAND driver. */
	/* Not sure how this works... */
	ecc_code[0] = 0xff;
	ecc_code[1] = 0xff;
	ecc_code[2] = 0xff;
	ecc_code[3] = 0xff;
	ecc_code[4] = 0xff;
	ecc_code[5] = 0xff;

	return 0;
}

/*
 * Function for ecc correction
 */
static int boss_nand_ecc_correct(struct mtd_info *mtd, u_char *dat,
				 u_char *read_ecc, u_char *calc_ecc)
{
	/* do nothing */
	return 0;
}

/*
 * Function to write chip oob data.
 */
static int boss_nand_ecc_write_oob(struct mtd_info *mtd,
				   struct nand_chip *chip, int page)
{
        int i, status;

        /* Our nand controller will write the generated ECC code into spare
	 * area automatically, so we should mark the ECC code which located
	 * in the eccpos.
	 */
        for (i = 0; i < chip->ecc.total; i++)
                chip->oob_poi[chip->ecc.layout->eccpos[i]] = 0xff;

        chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);
        chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
        chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
        status = chip->waitfunc(mtd, chip);

        return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/*
 * Marking a bad block.
 */
static int boss_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    	struct boss_nand_s *bn = (struct boss_nand_s *) mtd->priv;
	unsigned int addr_hi, addr;
	int page;
	int rval;
	char *buf = bn->buf;

        page = (int) (ofs >> bn->chip.page_shift);
        page &= bn->chip.pagemask;

        memset(bn->buf, 0x0, mtd->oobsize);
	addr_conv(mtd, page, &addr_hi, &addr);
#ifdef CONFIG_BOSS_SINGLE_CORE
	buf = __virt_to_phys(buf);
#endif

	rval = ipc_nand_write(addr_hi, addr,
			      buf,
			      mtd->oobsize,
			      1,	/* spare area */
			      0,	/* no ecc */
			      1);
	if (rval < 0)
		pr_err("ipc_nand_write() failed!\n");

        return rval;
}

/*
 * Read one byte.
 */
static uint8_t boss_nand_read_byte(struct mtd_info *mtd)
{
	struct boss_nand_s *bn = (struct boss_nand_s *) mtd->priv;
	uint8_t *data;

	if ((bn->pos + 1) > BUFFER_SIZE) {
		pr_err("%s(): buffer overflow\n", __func__);
		BUG();
	}

	data = &bn->buf[bn->pos];
	bn->pos++;

	return *data;
}

/*
 * Read one word.
 */
static u16 boss_nand_read_word(struct mtd_info *mtd)
{
	struct boss_nand_s *bn = (struct boss_nand_s *) mtd->priv;
	u16 *data;

	if ((bn->pos + 1) > BUFFER_SIZE) {
		pr_err("%s(): buffer overflow\n", __func__);
		BUG();
	}

	data = (u16 *) &bn->buf[bn->pos];
	bn->pos += 2;

	return *data;
}

/*
 * Write to chip.
 */
static void boss_nand_write_buf(struct mtd_info *mtd,
				const uint8_t *buf, int len)
{
	struct boss_nand_s *bn = (struct boss_nand_s *) mtd->priv;

	if ((bn->pos + len) > BUFFER_SIZE) {
		pr_err("%s(): buffer overflow\n", __func__);
		BUG();
	}

	memcpy(&bn->buf[bn->pos], buf, len);
	bn->pos += len;
}

/*
 * Read from chip.
 */
static void boss_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct boss_nand_s *bn = (struct boss_nand_s *) mtd->priv;

	if ((bn->pos + len) > BUFFER_SIZE) {
		pr_err("%s(): buffer overflow\n", __func__);
		BUG();
	}

	memcpy(buf, &bn->buf[bn->pos], len);
	bn->pos += len;
}

/*
 * Select chip number.
 */
static void boss_nand_select_chip(struct mtd_info *mtd, int chip)
{
	/* do nothing ... */
}

/*
 * Hardware specific funtion for controlling ALE/CLE/nCE.
 * Also used to write command and address
 */
static void boss_nand_cmd_ctrl(struct mtd_info *mtd,
			       int dat, unsigned int ctrl)
{
	/* do nothing */
}

/*
 * Hardware specific function for accesing device ready/busy line.
 */
static int boss_nand_dev_ready(struct mtd_info *mtd)
{
	struct boss_nand_s *bn = (struct boss_nand_s *) mtd->priv;
	struct nand_chip *chip = &bn->chip;

	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	return (chip->read_byte(mtd) & NAND_STATUS_READY) ? 1 : 0;
}

/*
 * Hardware specific function for wait on ready
 */
static int boss_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
	return chip->read_byte(mtd);
}

/*
 * Function for writing commands to chip.
 */
static void boss_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
			      int column, int page_addr)
{
	struct boss_nand_s *bn = (struct boss_nand_s *) mtd->priv;
	int rval;
	unsigned int addr_hi;
	unsigned int addr;
	unsigned int data;
	char *buf = bn->buf;

#ifdef CONFIG_BOSS_SINGLE_CORE
	buf = __virt_to_phys(buf);
#endif

	switch (command) {
	case NAND_CMD_RESET:
		rval = ipc_nand_reset(0);
		if (rval < 0)
			pr_err("ipc_nand_reset() failed!\n");
		break;
	case NAND_CMD_READID:
		addr_conv(mtd, page_addr, &addr_hi, &addr);
		rval = ipc_nand_read_id(addr_hi, addr, &data);
		if (rval < 0)
			pr_err("ipc_nand_read_id() failed!\n");
		bn->pos = 0;
		bn->buf[0] = (u8) (data >> 24);
		bn->buf[1] = (u8) (data >> 16);
		bn->buf[2] = (u8) data >> 8;
		bn->buf[3] = (u8) data;
		break;
	case NAND_CMD_STATUS:
		bn->pos = 0;
		bn->buf[0] = NAND_STATUS_WP | NAND_STATUS_READY;
		break;
	case NAND_CMD_ERASE1:
		addr_conv(mtd, page_addr, &addr_hi, &addr);
		rval = ipc_nand_erase(addr_hi, addr, 1);
		if (rval < 0)
			pr_err("ipc_nand_erase() failed!\n");
		break;
	case NAND_CMD_ERASE2:
		break;
	case NAND_CMD_READOOB:
		addr_conv(mtd, page_addr, &addr_hi, &addr);
		bn->pos = column;
		rval = ipc_nand_read(addr_hi, addr,
				     buf,
				     mtd->oobsize,
				     1,		/* spare area */
				     0,		/* no ecc */
				     1);
		if (rval < 0)
			pr_err("ipc_nand_read() failed!\n");
		break;
	case NAND_CMD_READ0:
		addr_conv(mtd, page_addr, &addr_hi, &addr);
		/* Read the main area first */
		bn->pos = column;
		rval = ipc_nand_read(addr_hi, addr,
				     buf,
				     mtd->writesize,
				     0,		/* main area */
				     1,		/* w/ ecc */
				     1);
		if (rval < 0)
			pr_err("ipc_nand_read() failed!\n");

		/* Read the spare area next */
		rval = ipc_nand_read(addr_hi, addr,
				     buf + mtd->writesize,
				     mtd->oobsize,
				     1,		/* spare area */
				     0,		/* no ecc */
				     1);
		if (rval < 0)
			pr_err("ipc_nand_read() failed!\n");
		break;
	case NAND_CMD_SEQIN:
		bn->pos = column;
		bn->seqin_column = column;
		bn->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		addr_conv(mtd, bn->seqin_page_addr, &addr_hi, &addr);
		if (bn->seqin_column < mtd->writesize) {
			/* Write to main area */
			rval = ipc_nand_write(addr_hi, addr,
					      buf,
					      mtd->writesize,
					      0,	/* main area */
					      1,	/* w/ ecc */
					      1);
			if (rval < 0)
				pr_err("ipc_nand_write() failed!\n");
		}
		/* Write to spare area */
		rval = ipc_nand_write(addr_hi, addr,
				      buf + mtd->writesize,
				      mtd->oobsize,
				      1,	/* spare area */
				      0,	/* no ecc */
				      1);
		if (rval < 0)
			pr_err("ipc_nand_write() failed!\n");
		break;
	default:
		pr_err("%s: command = %d\n", __func__, command);
		BUG();
		break;
	}
}

/*
 * BOSS NAND virtual driver initialization.
 */
static int __devinit boss_nand_probe(struct platform_device *pdev)
{
	int rval = 0;
	struct boss_nand_s *boss_nand;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct nand_hw_control *controller;
	struct ipc_flpart part;
	int i, num;

	/* Allocate memory for driver. */
	boss_nand = kzalloc(sizeof(*boss_nand), GFP_KERNEL);
	if (boss_nand == NULL) {
		rval = -ENOMEM;
		goto done;
	}
	platform_set_drvdata(pdev, boss_nand);
	boss_nand->dev = &pdev->dev;
	mtd = &boss_nand->mtd;
	chip = &boss_nand->chip;
	controller = &boss_nand->controller;

	/* Allocate memory for buffer so it aligns to page boundary */
	boss_nand->buf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (boss_nand->buf == NULL) {
		pr_err("unable to allocate buffer!\n");
		rval = -ENOMEM;
		goto done;
	}

	/* Link private data with the MTD structure */
	mtd->priv = boss_nand;
	mtd->owner = THIS_MODULE;

	/* Set up functions */
	chip->read_byte = boss_nand_read_byte;
	chip->read_word = boss_nand_read_word;
	chip->write_buf = boss_nand_write_buf;
	chip->read_buf = boss_nand_read_buf;
	chip->select_chip = boss_nand_select_chip;
	chip->cmd_ctrl = boss_nand_cmd_ctrl;
	chip->dev_ready = boss_nand_dev_ready;
	chip->waitfunc = boss_nand_waitfunc;
	chip->cmdfunc = boss_nand_cmdfunc;
	chip->options = NAND_NO_AUTOINCR;
	chip->block_markbad = boss_nand_block_markbad;

	/* Set up controller */
	chip->controller = controller;
        spin_lock_init(&controller->lock);
        init_waitqueue_head(&controller->wq);

	/* Scan for device identification */
	rval = nand_scan_ident(&boss_nand->mtd, 1, NULL);
	if (rval < 0)
		goto done;

	/* Set up ECC layout, bad block scan patterns, etc. */
	if (mtd->writesize == 512) {
		chip->ecc.layout = &ecclayout_512;
		chip->badblock_pattern = &bbt_descr_512;
	} else if (mtd->writesize == 2048) {
		chip->ecc.layout = &ecclayout_2k;
		chip->badblock_pattern = &bbt_descr_2k;
	} else {
		rval = -EIO;
		pr_err("mtd->writesize (%d) unhandled!\n", mtd->writesize);
		goto done;
	}
	chip->ecc.mode = NAND_ECC_HW;
	chip->ecc.size = 512;
	chip->ecc.bytes = 6;
	chip->ecc.hwctl = boss_nand_ecc_hwctl;
	chip->ecc.calculate = boss_nand_ecc_calculate;
	chip->ecc.correct = boss_nand_ecc_correct;
	chip->ecc.write_oob = boss_nand_ecc_write_oob;

	/* Scan for rest of device */
	rval = nand_scan_tail(&boss_nand->mtd);
	if (rval < 0)
		goto done;

	/* Set up partition info by querying over IPC */
	rval = ipc_flpart_num_parts();
	if (rval < 0) {
		pr_err("ipc_flpart_num_part(): error (%d)\n", rval);
		rval = -EIO;
		goto done;
	}
	num = rval;
	for (i = boss_nand->npart = 0; i < num; i++) {
		rval = ipc_flpart_get(i, &part);
		if (rval < 0) {
			pr_err("ipc_flpart_get(): error (%d)\n", rval);
			rval = -EIO;
			goto done;
		}

		if (part.nblk == 0)
			continue;

		strncpy(boss_nand->part_name[boss_nand->npart], part.name,
			sizeof(boss_nand->part_name[boss_nand->npart]));
		boss_nand->part[boss_nand->npart].name =
			boss_nand->part_name[boss_nand->npart];
		boss_nand->part[boss_nand->npart].offset =
			part.sblk * mtd->erasesize;
		boss_nand->part[boss_nand->npart].size =
			part.nblk * mtd->erasesize;
		boss_nand->npart++;
	}

	/* Add partitions to MTD */
	add_mtd_partitions(mtd, boss_nand->part, boss_nand->npart);
	rval = 0;

done:
	if (rval != 0) {
		if (boss_nand) {
			if (boss_nand->buf)
				kfree(boss_nand->buf);
			kfree(boss_nand);
		}
	}

	return rval;
}

/*
 * BOSS NAND virtual driver removal.
 */
static int __devexit boss_nand_remove(struct platform_device *pdev)
{
	struct boss_nand_s *boss_nand =
		(struct boss_nand_s *) platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	nand_release(&boss_nand->mtd);
	if (boss_nand->buf)
		kfree(boss_nand->buf);
	kfree(boss_nand);

	return 0;
}

/*
 * Platform device object.
 */
static struct platform_device *boss_nand_device = NULL;

/*
 * Platform driver object.
 */
static struct platform_driver boss_nand_driver = {
	.probe =	boss_nand_probe,
	.remove =	boss_nand_remove,
	.driver = {
		.name =		"boss_nand",
		.owner =	THIS_MODULE,
	},
};

/*
 * BOSS NAND virtual driver initialization.
 */
static int __init boss_nand_init(void)
{
	int rval;

	boss_nand_device = platform_device_alloc("boss_nand", -1);
	if (boss_nand_device == NULL)
		return -ENOMEM;

	rval = platform_device_add(boss_nand_device);
	if (rval) {
		platform_device_put(boss_nand_device);
		return rval;
	}

	return platform_driver_register(&boss_nand_driver);
}

/*
 * BOSS NAND virtual driver cleanup.
 */
static void __exit boss_nand_cleanup(void)
{
	platform_device_unregister(boss_nand_device);
	platform_driver_unregister(&boss_nand_driver);
}

module_init(boss_nand_init);
module_exit(boss_nand_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Charles Chiou <cchiou@ambarella.com>");
MODULE_DESCRIPTION("NAND virtual driver under BOSS");
