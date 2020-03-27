/*
 * (C) Copyright 2010 NXP Semiconductors
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <asm/arch/sysreg.h>
#include <asm/arch/nand.h>
#include <asm/arch/clock.h>

#include <common.h>
#include <nand.h>

#ifdef CONFIG_CMD_NAND

#define OOB_FREE_OFFSET 4

const struct lpc313x_nand_timing ea313x_nanddev_timing =
{
	.ns_trsd	= 36,
	.ns_tals	= 36,
	.ns_talh	= 12,
	.ns_tcls	= 36,
	.ns_tclh	= 12,
	.ns_tdrd	= 36,
	.ns_tebidel	= 12,
	.ns_tch		= 12,
	.ns_tcs		= 48,
	.ns_treh	= 24,
	.ns_trp		= 48,
	.ns_trw		= 24,
	.ns_twp		= 36
};

/*
 * Autoplacement pattern for 2048+64 bytes large block NAND FLASH
 */
static const struct nand_ecclayout nand_hw_eccoob_64 =
{
	.eccbytes = 48,
	.eccpos = {
		4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		52, 53, 54, 55, 56, 67, 58, 59, 60, 61, 62, 63
	},
	.oobfree = {
		{0, 4},
		{16, 4},
		{32, 4},
		{48, 4}
	}
};

/*
 * Bad block descriptors for small/large/huge block FLASH
 * Hardware specific flash bbt decriptors
 */
static const uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static const uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static const struct nand_bbt_descr lpc313x_bbt_main_descr =
{
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 32,
	.len = 4,
	.veroffs = 48,
	.maxblocks = 4,
	.pattern = bbt_pattern
};

static const struct nand_bbt_descr lpc313x_bbt_mirror_descr =
{
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 32,
	.len = 4,
	.veroffs = 48,
	.maxblocks = 4,
	.pattern = mirror_pattern
};

/* Decode and encode buffer ECC status masks */
static const u32 nand_buff_dec_mask[2] =
{
	NAND_IRQ_ECC_DEC_RAM0, NAND_IRQ_ECC_DEC_RAM1
};
static const u32 nand_buff_enc_mask[2] =
{
	NAND_IRQ_ECC_ENC_RAM0, NAND_IRQ_ECC_ENC_RAM1
};
static const u32 nand_buff_wr_mask[2] =
{
	NAND_IRQ_WR_RAM0, NAND_IRQ_WR_RAM1
};

/* Decode buffer addresses */
static const void *nand_buff_addr[2] =
{
	(void *) NANDFLASH_CTRL_S0_BASE, (void *) (NANDFLASH_CTRL_S0_BASE + 0x400)
};

/*
 * Dummies bytes for bad block ( just for HARDWARE ECC: inaccurate )
 */
static const uint8_t scan_ff_pattern[] = { 0xff, 0xff };

static const struct nand_bbt_descr lpc313x_largepage_flashbased = 
{
	.options = NAND_BBT_SCAN2NDPAGE,
	.offs = 50,
	.len = 2,
	.pattern = scan_ff_pattern
};

/***********************************************************************
 * NAND driver private functions
 **********************************************************************/

/*
 * Clear NAND interrupt status
 */
static inline void lpc313x_nand_int_clear(u32 mask)
{
	NAND_CTRL->irq_status_raw = mask;
}

/*
 * Start a RAM read operation on RAM0 or RAM1
 */
static inline void lpc313x_ram_read(int bufnum)
{
	if (bufnum == 0) {
		/* Use RAM buffer 0 */
		NAND_CTRL->control_flow = NAND_CTRL_RD_RAM0;
	}
	else {
		/* Use RAM buffer 1 */
		NAND_CTRL->control_flow = NAND_CTRL_RD_RAM1;
	}
}

/*
 * Start a RAM write operation on RAM0 or RAM1
 */
static inline void lpc313x_ram_write(int bufnum)
{
	if (bufnum == 0) {
		/* Use RAM buffer 0 */
		NAND_CTRL->control_flow = NAND_CTRL_WR_RAM0;
	}
	else {
		/* Use RAM buffer 1 */
		NAND_CTRL->control_flow = NAND_CTRL_WR_RAM1;
	}
}

/*
 * MTD hardware ECC enable callback
 */
static void lpc313x_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	(void) mtd;
	(void) mode;

	/* Nothing to really do here, ECC is enabled and used by default */
}

/*
 * MTD ECC data correction callback
 */
static int lpc313x_nand_correct_data(struct mtd_info *mtd, u_char *dat,
		u_char *read_ecc, u_char *calc_ecc)
{
	u32 tmp;
	int errs_corrected = 0;

	(void) mtd;
	(void) dat;
	(void) calc_ecc;

	/* Data is corrected in hardware, just verify that data is correct per HW */
	if (((NAND_CTRL->irq_status_raw) & NAND_IRQ_ERR_UNR_RAM0) &
			(read_ecc[OOB_FREE_OFFSET] != 0xFF)) {
		return -1;
	}

	/* Generate correction statistics */
	tmp = NAND_CTRL->irq_status_raw;
	if (!(tmp & (NAND_IRQ_NOERR_RAM0 | NAND_IRQ_NOERR_RAM1))) {
		if (tmp & (NAND_IRQ_ERR1_RAM0 | NAND_IRQ_ERR1_RAM1)) {
			errs_corrected = 1;
		}
		else if (tmp & (NAND_IRQ_ERR2_RAM0| NAND_IRQ_ERR2_RAM1)) {
			errs_corrected = 2;
		}
		else if (tmp & (NAND_IRQ_ERR3_RAM0| NAND_IRQ_ERR3_RAM1)) {
			errs_corrected = 3;
		}
		else if (tmp & (NAND_IRQ_ERR4_RAM0| NAND_IRQ_ERR4_RAM1)) {
			errs_corrected = 4;
		}
		else if (tmp & (NAND_IRQ_ERR5_RAM0| NAND_IRQ_ERR5_RAM1)) {
			errs_corrected = 5;
		}

		mtd->ecc_stats.corrected += errs_corrected;
	}
	else if (tmp & (NAND_IRQ_ERR_UNR_RAM0 | NAND_IRQ_ERR_UNR_RAM1)) {
		mtd->ecc_stats.failed++;
	}
	return 0;
}

/*
 * MTD calculate ECC callback
 */
static int lpc313x_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
	(void) mtd;
	(void) dat;
	(void) ecc_code;

	/* ECC is calculated automatically in hardware, nothing to do */
	return 0;
}

/*
 * Read the payload and OOB data from the device in the hardware storage format
 */
int lpc313x_nand_read_page_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
		uint8_t *buf,int page)
{
	int i, curbuf = 0, bufrdy = 0, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;

	for (i = 0; i < eccsteps; i++) {
		/* Clear all current statuses */
		lpc313x_nand_int_clear(~0);

		/* Start read into RAM0 or RAM1 */
		lpc313x_ram_read(curbuf);

		/* Polling for buffer loaded and decoded */
		while (!((NAND_CTRL->irq_status_raw) & nand_buff_dec_mask[curbuf]));

		chip->ecc.correct(mtd, p, oob, NULL);

		/* Read payload portion of the transfer */
		memcpy((void *)p, nand_buff_addr[bufrdy], eccsize);
		p += eccsize;

		/* Read OOB data portion of the transfer */
		memcpy((void *)oob, nand_buff_addr[bufrdy] + eccsize, eccbytes);
		oob += eccbytes;
	}

	return 0;
}

/*
 * Read the OOB data from the device in the hardware storage format
 */
static int lpc313x_nand_read_oob_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
		int page, int sndcmd)
{
	uint8_t *buf = chip->oob_poi;
	int length = mtd->oobsize;
	int chunk = chip->ecc.bytes + chip->ecc.prepad + chip->ecc.postpad;
	int eccsize = chip->ecc.size, eccsteps = chip->ecc.steps;
	uint8_t *bufpoi = buf;
	int i, toread, sndrnd = sndcmd, pos;

	chip->cmdfunc(mtd, NAND_CMD_READ0, chip->ecc.size, page);
	for (i = eccsteps; i > 0; i--) {
		/* Random position read needed? */
		if (sndrnd) {
			pos = eccsize + i * (eccsize + chunk);
			if (mtd->writesize > 512)
				chip->cmdfunc(mtd, NAND_CMD_RNDOUT, pos, -1);
			else
				chip->cmdfunc(mtd, NAND_CMD_READ0, pos, page);
		} else {
			sndrnd = 1;
		}

		toread = min_t(int, length, chunk);
		chip->read_buf(mtd, bufpoi, toread);
		bufpoi += toread;
		length -= toread;
	}
	if (length > 0)
		chip->read_buf(mtd, bufpoi, length);

	return 1;
}

/*
 * Write the payload and OOB data to the device in the hardware storage format
 */
static void lpc313x_nand_write_page_syndrome(struct mtd_info *mtd,
		struct nand_chip *chip, const uint8_t *buf)
{
	int i, curbuf = 0, bufrdy = 0, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	const uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;

	for (i = 0; i < eccsteps; i++) {
		/* Clear all current statuses */
		lpc313x_nand_int_clear(~0);

		/* Copy payload and OOB data to the buffer */
		memcpy((void *) nand_buff_addr[bufrdy], p, eccsize);
		memcpy((void *) nand_buff_addr[bufrdy] + eccsize, oob, OOB_FREE_OFFSET);
		p += eccsize;
		oob += eccbytes;
		while(!((NAND_CTRL->irq_status_raw) & nand_buff_enc_mask[bufrdy]));

		lpc313x_ram_write(curbuf);

		/* Polling for buffer loaded and decoded */
		while (!((NAND_CTRL->irq_status_raw) & nand_buff_wr_mask[curbuf]));
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->write_buf(mtd, oob, i);
}

/*
 * Write the OOB data to the device in the hardware storage format
 */
static int lpc313x_nand_write_oob_syndrome(struct mtd_info *mtd,
		struct nand_chip *chip, int page)
{
	int chunk = chip->ecc.bytes + chip->ecc.prepad + chip->ecc.postpad;
	int eccsize = chip->ecc.size, length = mtd->oobsize;
	int i, len, pos, status = 0, sndcmd = 0, steps = chip->ecc.steps;
	const uint8_t *bufpoi = chip->oob_poi;
	int tmpStatus;

	pos = eccsize;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, pos, page);
	for (i = 0; i < steps; i++) {
		if (sndcmd) {
			if (mtd->writesize <= 512) {
				uint32_t fill = 0xFFFFFFFF;

				len = eccsize;
				while (len > 0) {
					int num = min_t(int, len, 4);
					chip->write_buf(mtd, (uint8_t *)&fill,
							num);
					len -= num;
				}
			} else {
				pos = eccsize + i * (eccsize + chunk);
				chip->cmdfunc(mtd, NAND_CMD_RNDIN, pos, -1);
			}
		} else {
			sndcmd = 1;
		}

		len = min_t(int, length, chunk);
		chip->write_buf(mtd, bufpoi, len);
		bufpoi += len;
		length -= len;
	}
	if (length > 0)
		chip->write_buf(mtd, bufpoi, length);

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);

	tmpStatus = status & NAND_STATUS_FAIL ? -1 : 0;

	return tmpStatus;
}

/*
 * Returns NAND busy(0)/ready(!0) status callback
 */
static int nand_lpc313x_dev_ready(struct mtd_info *mtd)
{
	unsigned long tmp;
	tmp = NAND_CTRL->check_sts;
	return (int)(tmp & NAND_CHK_STS_RB1_LVL); 
}

/*
 * nand_lpc313x_select_chip - [DEFAULT] control CE line
 * Asserts and deasserts chip selects (callback)
 */
static void nand_lpc313x_select_chip (struct mtd_info *mtd, int chipnr)
{
	switch (chipnr) {
	case -1:
		NAND_CTRL->set_ce = NAND_SETCE_WP | NAND_SETCE_CV_MASK;
		break;
	case 0:
		NAND_CTRL->set_ce = NAND_SETCE_WP | NAND_SETCE_CV(0);
		break;
	default:
		BUG();
	}
}

/*
 * hardwarespecific function for accesing control-lines
 */
static void nand_lpc313x_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)

{
	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE) {
		NAND_CTRL->set_cmd  = (u32 )cmd; 
	}
	else if (ctrl & NAND_ALE) {
		NAND_CTRL->set_addr  = (u32 )cmd; 
	}
}

/*
 * Setup NAND interface timing
 */
static void lpc313x_nand_setrate(struct lpc313x_nand_timing *timing)
{
	u32 tmp, timing1, timing2, srcclk;

	/* Get the NAND controller base clock rate */
	srcclk = cgu_get_clk_freq(CGU_SB_NANDFLASH_NAND_CLK_ID);

	/* Compute number of clocks for timing1 parameters */
	tmp = srcclk / (1000000000 / timing->ns_trsd);
	if (tmp > 0x3)
		tmp = 0x3;
	timing1 = NAND_TIM1_TSRD(tmp);
	tmp = srcclk / (1000000000 / timing->ns_tals);
	if (tmp > 0x7)
		tmp = 0x7;
	timing1 |= NAND_TIM1_TALS(tmp);
	tmp = srcclk / (1000000000 / timing->ns_talh);
	if (tmp > 0x7)
		tmp = 0x7;
	timing1 |= NAND_TIM1_TALH(tmp);
	tmp = srcclk / (1000000000 / timing->ns_tcls);
	if (tmp > 0x7)
		tmp = 0x7;
	timing1 |= NAND_TIM1_TCLS(tmp);
	tmp = srcclk / (1000000000 / timing->ns_tclh);
	if (tmp > 0x7)
		tmp = 0x7;
	timing1 |= NAND_TIM1_TCLH(tmp);
	NAND_CTRL->timing1 = timing1;


	/* Compute number of clocks for timing2 parameters */
	tmp = srcclk / (1000000000 / timing->ns_tdrd);
	if (tmp > 0x3)
		tmp = 0x3;
	timing2 = NAND_TIM2_TDRD(tmp);
	tmp = srcclk / (1000000000 / timing->ns_tebidel);
	if (tmp > 0x7)
		tmp = 0x7;
	timing2 |= NAND_TIM2_TEBI(tmp);
	tmp = srcclk / (1000000000 / timing->ns_tch);
	if (tmp > 0x7)
		tmp = 0x7;
	timing2 |= NAND_TIM2_TCH(tmp);
	tmp = srcclk / (1000000000 / timing->ns_tcs);
	if (tmp > 0x7)
		tmp = 0x7;
	timing2 |= NAND_TIM2_TCS(tmp);
	tmp = srcclk / (1000000000 / timing->ns_treh);
	if (tmp > 0x7)
		tmp = 0x7;
	timing2 |= NAND_TIM2_TRH(tmp);
	tmp = srcclk / (1000000000 / timing->ns_trp);
	if (tmp > 0x7)
		tmp = 0x7;
	timing2 |= NAND_TIM2_TRP(tmp);
	tmp = srcclk / (1000000000 / timing->ns_trw);
	if (tmp > 0x7)
		tmp = 0x7;
	timing2 |= NAND_TIM2_TWH(tmp);
	tmp = srcclk / (1000000000 / timing->ns_twp);
	if (tmp > 0x7)
		tmp = 0x7;
	timing2 |= NAND_TIM2_TWP(tmp);
	NAND_CTRL->timing2 = timing2;
}

static void nand_lpc313x_inithw( void )
{
	/* enable NAND clocks */
	cgu_clk_en_dis(CGU_SB_NANDFLASH_S0_CLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_ECC_CLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_NAND_CLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_PCLK_ID, 1);

	/* reset NAND controller */
	cgu_soft_reset_module(NANDFLASH_NAND_RESET_N_SOFT);
	cgu_soft_reset_module(NANDFLASH_ECC_RESET_N_SOFT);

	/* check NAND mux signals */
	SYS_REGS->mux_nand_mci_sel = 0;

	/* Disable all NAND interrupts */
	NAND_CTRL->irq_mask = ~0;
	NAND_CTRL->irq_mask2 = ~0;

	/* Setup device and controller timing */
	lpc313x_nand_setrate(&ea313x_nanddev_timing);

	/* enable the controller for 8-bit NAND and de-assert nFCE */
	NAND_CTRL->config = NAND_CTRL->config | (NAND_CFG_DC |
				NAND_CFG_ECGC | NAND_CFG_EC);
}

int board_nand_init(struct nand_chip *nand)
{
	/* Populdate device callbacks used by MTD driver */
	nand->IO_ADDR_R = (void __iomem *)&NAND_CTRL->read_data;
	nand->IO_ADDR_W = (void __iomem *)&NAND_CTRL->write_data;

	nand->select_chip = nand_lpc313x_select_chip;
	nand->chip_delay = 20;
	nand->cmd_ctrl = nand_lpc313x_hwcontrol;
	nand->dev_ready = nand_lpc313x_dev_ready;

	/* Configuration ECC related stuff */
	nand->ecc.mode = NAND_ECC_HW_SYNDROME;
	nand->ecc.read_page_raw = lpc313x_nand_read_page_syndrome;
	nand->ecc.read_page = lpc313x_nand_read_page_syndrome;
	nand->ecc.write_page = lpc313x_nand_write_page_syndrome;
	nand->ecc.write_oob = lpc313x_nand_write_oob_syndrome;
	nand->ecc.read_oob = lpc313x_nand_read_oob_syndrome;
	nand->ecc.calculate = lpc313x_nand_calculate_ecc;
	nand->ecc.correct   = lpc313x_nand_correct_data;
	nand->ecc.hwctl = lpc313x_nand_enable_hwecc;
	nand->ecc.layout = &nand_hw_eccoob_64;

	nand->options |= NAND_USE_FLASH_BBT;

	/* Configuration for Bad Block Table */
	nand->bbt_td = &lpc313x_bbt_main_descr;
	nand->bbt_md = &lpc313x_bbt_mirror_descr;
	nand->badblock_pattern = &lpc313x_largepage_flashbased;

	nand->ecc.size = 512;
	nand->ecc.bytes = 16;
	nand->ecc.prepad = 0;

	nand_lpc313x_inithw();

	return(0);
}
#endif

