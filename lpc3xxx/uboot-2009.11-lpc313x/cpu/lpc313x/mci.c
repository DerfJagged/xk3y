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

#include <asm/arch/sdmmc.h>
#include <asm/arch/sysreg.h>
#include <asm/arch/ioconf.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include <asm/arch/mci.h>
#include <asm/arch/nand.h>

#ifdef CONFIG_MMC

/* give 1 atleast 1 sec for the card to respond */
#define US_TIMEOUT		1000000
/* inter-command acquire oper condition delay in msec*/
#define MS_ACQUIRE_DELAY	(10 * 1000)
/* initial OP_COND retries */
#define INIT_OP_RETRIES		10
/* set OP_COND retries */
#define SET_OP_RETRIES		200

/* global instance of the current card*/
static MCI_CARD_INFO_T g_card_info;
block_dev_desc_t mmc_dev;

static ulong prv_card_acquired(MCI_CARD_INFO_T* pdev)
{
	return (pdev->cid[0] != 0);
}

static ulong prv_get_bits(int start, int end, ulong* data)
{
	ulong v;
	ulong i = end >> 5;
	ulong j = start & 0x1f;

	if (i == (start >> 5))
		v = (data[i] >> j);
	else
		v = ((data[i] << (32 - j)) | (data[start >> 5] >> j));

	return (v & ((1 << (end - start + 1)) - 1));
}

static void prv_clear_all(void)
{
	/* reset all blocks */
	MCI->ctrl |= MCI_CTRL_FIFO_RESET;
	/* wait till resets clear */
	while (MCI->ctrl & MCI_CTRL_FIFO_RESET);

	/* Clear interrupt status */
	MCI->rintsts = 0xFFFFFFFF;
}

static int prv_send_cmd(ulong cmd, ulong arg)
{
	volatile int tmo = 50;
	volatile int delay;
	/* set command arg reg*/
	MCI->cmdarg = arg;
	MCI->cmd = MCI_CMD_START | cmd;

	/* poll untill command is accepted by the CIU */
	while (--tmo && (MCI->cmd & MCI_CMD_START)) {
		if (tmo & 1)
			delay = 50;
		else
			delay = 18000;

		while (--delay > 1);
	}

	return (tmo < 1) ? 1 : 0;
}

static void prv_set_clock(ulong speed)
{
	/* compute SD/MMC clock dividers */
	ulong mmc_clk = cgu_get_clk_freq(CGU_SB_SD_MMC_CCLK_IN_ID);
	ulong div = ((mmc_clk / speed) + 2) >> 1;

	if ((div == MCI->clkdiv) && MCI->clkena)
		return; /* requested speed is already set */

	/* disable clock */
	MCI->clkena = 0;
	MCI->clksrc = 0;

	/* inform CIU */
	prv_send_cmd(MCI_CMD_UPD_CLK | MCI_CMD_PRV_DAT_WAIT, 0);

	/* set clock to desired speed */
	MCI->clkdiv = div;
	/* inform CIU */
	prv_send_cmd(MCI_CMD_UPD_CLK | MCI_CMD_PRV_DAT_WAIT, 0);

	/* enable clock */
	MCI->clkena = MCI_CLKEN_ENABLE;
	/* inform CIU */
	prv_send_cmd(MCI_CMD_UPD_CLK | MCI_CMD_PRV_DAT_WAIT, 0);
}

static void prv_pull_response(MCI_CARD_INFO_T* pdev, int length)
{

	/* on this chip response is not a fifo so read all 4 regs */
	pdev->response[0] = MCI->resp0;
	pdev->response[1] = MCI->resp1;
	pdev->response[2] = MCI->resp2;
	pdev->response[3] = MCI->resp3;
}

#define AES_BUF	((void*)0x70000000)
#define LEN	16

static int prv_pull_data(unsigned char* pv, int cnt, int enc)
{
	int j,i = 0;
	int fcnt;

	volatile ulong * pFifo = (ulong *)0x18000100;
	ulong *pBuffer = (ulong *)pv;

	if (!enc) {
		while (i < cnt) {
			fcnt = MCI_STS_GET_FCNT(MCI->status);
			for(j=0;j<fcnt;j++)
				*pBuffer++ = pFifo[j];

			i += (fcnt * 4);
		}
	} else {

		static unsigned int iv[4] = {
		#include "kernel_iv.h"
		};

		NAND_CTRL->aes_from_ahb = NAND_AES_AHB_EN;
	
		do {
			volatile ulong *aes_buf = (ulong *)AES_BUF;
			int c = 0;
			
			/* key is set in init.c */
			
			NAND_CTRL->aes_iv[0] = iv[0];
			NAND_CTRL->aes_iv[1] = iv[1];
			NAND_CTRL->aes_iv[2] = iv[2];
			NAND_CTRL->aes_iv[3] = iv[3];
		
			while (c < 512) {
				fcnt = MCI_STS_GET_FCNT(MCI->status);

				for (j = 0; j < fcnt; j++)
				{
					ulong l = pFifo[j];
					
					*aes_buf++ = l;
					
					/* last 4 longs of ciphertext are next iv */
					if ( (c/4 + j - 124) >= 0)
						iv[ c/4 + j - 124 ] = l; 
				}
				c += (fcnt * 4);
			}
 
			/* do the decryption */
			NAND_CTRL->aes_from_ahb = NAND_AES_AHB_EN|NAND_AES_AHB_DCRYPT_RAM0;
			
			while (!(NAND_CTRL->irq_status_raw & NAND_IRQ_AES_DONE_RAM0));
			
			NAND_CTRL->irq_status_raw = NAND_IRQ_AES_DONE_RAM0;

			memcpy(&pv[i], AES_BUF, 512);
			i += c;
			
		} while ( i < cnt );

		NAND_CTRL->aes_from_ahb = 0;
	}
	return i;
}

static void prv_push_data(const unsigned char* pv, ulong cb)
{
	int i = 0;
	int fcnt;

	while (i < cb) {
		fcnt = MCI_FIFO_SZ - (MCI_STS_GET_FCNT(MCI->status) << 2);
		memcpy((void*)&MCI->data, pv + i, fcnt);
		i += fcnt;
	}
}

static ulong prv_wait_for_completion(MCI_CARD_INFO_T* pdev, ulong bits)
{
	ulong status = 0;
	int tmo_count = 10;

	/* also check error conditions */
	bits |= MCI_INT_EBE | MCI_INT_SBE | MCI_INT_HLE
		| MCI_INT_RTO | MCI_INT_RCRC | MCI_INT_RESP_ERR;

	if (bits & MCI_INT_DATA_OVER)
		bits |= MCI_INT_FRUN | MCI_INT_HTO | MCI_INT_DTO
			| MCI_INT_DCRC;

	do {
		udelay(5000);
		status = MCI->rintsts;

		if (--tmo_count < 1) {
			break;
		}
	}
	while ((status  & bits) == 0);
	/* set time out flag for driver timeout also */
	status |= ((tmo_count < 1) ? MCI_INT_RTO : 0);
	return status;
}

static void prv_process_csd(MCI_CARD_INFO_T* pdev)
{
	long status = 0;
	long c_size = 0;
	long c_size_mult = 0;
	long mult = 0;

	/* compute block length based on CSD response */
	pdev->block_len = 1 << prv_get_bits(80, 83, pdev->csd);

	if ((pdev->card_type & CARD_TYPE_HC) &&
			(pdev->card_type & CARD_TYPE_SD)) {
		/*
		 * See section 5.3.3 CSD Register (CSD Version 2.0) of SD2.0 spec
		 * an explanation for the calculation of these values
		 */
		c_size = prv_get_bits(48, 63, (ulong*)pdev->csd) + 1;
		pdev->blocknr = c_size << 10; /* 512 byte blocks */
	}
	else {
		/*
		 * See section 5.3 of the 4.1 revision of the MMC specs for
		 * an explanation for the calculation of these values
		 */
		c_size = prv_get_bits(62, 73, (ulong*)pdev->csd);
		c_size_mult = prv_get_bits(47, 49, (ulong*)pdev->csd);
		mult = 1 << (c_size_mult + 2);
		pdev->blocknr = (c_size + 1) * mult;
		
		/* adjust blocknr to 512/block */
		if (pdev->block_len > MMC_SECTOR_SIZE)
			pdev->blocknr = pdev->blocknr * (pdev->block_len >> 9);

		/* get extended CSD for newer MMC cards CSD spec >= 4.0*/
		if (((pdev->card_type & CARD_TYPE_SD) == 0) &&
				(prv_get_bits(122, 125, (ulong*)pdev->csd) >= 4)) {

			ulong ext_csd[MMC_SECTOR_SIZE/4];

			/* put card in trans state */
			status = mci_execute_command(pdev, CMD_SELECT_CARD, pdev->rca << 16, 0);
			/* set block size and byte count */
			MCI->blksiz = MMC_SECTOR_SIZE;
			MCI->bytcnt = MMC_SECTOR_SIZE;
			/* send EXT_CSD command */
			status = mci_execute_command(pdev, CMD_SEND_EXT_CSD, 0,
					MCI_INT_CMD_DONE | MCI_INT_DATA_OVER |
					MCI_INT_RXDR);

			if ((status & MCI_INT_ERROR) == 0) {
				/* read 52bytes EXT-CSD data */
				prv_pull_data((unsigned char*)ext_csd, MMC_SECTOR_SIZE, 0);

				/* check EXT_CSD_VER is greater than 1.1 */
				if ((ext_csd[48] & 0xFF) > 1)
				/* bytes 212:215 represent sec count */
					pdev->blocknr = ext_csd[53];

				/* switch to 52MHz clock if card type
				 * is set to 1 or else set to 26MH
				 * */
				if ((ext_csd[49] & 0xFF) == 1) {
					/* for type 1 MMC cards high speed is 52MHz */
					pdev->speed = MMC_HIGH_BUS_MAX_CLOCK;
				}
				else {
					/* for type 0 MMC cards high speed is 26MHz */
					pdev->speed = MMC_LOW_BUS_MAX_CLOCK;
				}
			}
		}
	}

	pdev->device_size = pdev->blocknr << 9; /* blocknr * 512 */
}

static int prv_set_trans_state(MCI_CARD_INFO_T* pdev)
{
	ulong status;

	/* get current state of the card */
	status = mci_execute_command(pdev, CMD_SEND_STATUS, pdev->rca << 16, 0);
	if (status & MCI_INT_RTO) {
		/* unable to get the card state. So return immediatly. */
		return _ERROR;
	}
	/* check card state in response */
	status = R1_CURRENT_STATE(pdev->response[0]);
	switch (status) {
	case SDMMC_STBY_ST:
		/* put card in 'Trans' state */
		status = mci_execute_command(pdev, CMD_SELECT_CARD, pdev->rca << 16, 0);
		if (status != 0) {
			/* unable to put the card in Trans state. So return immediatly. */
			return _ERROR;
		}
		break;
	case SDMMC_TRAN_ST:
		/*do nothing */
		break;
	default:
		/* card shouldn't be in other states so return */
		return _ERROR;
	}

#if defined (USE_WIDE)
	if (pdev->card_type & CARD_TYPE_SD) {
		/* SD, 4 bit width */
		mci_execute_command(pdev, CMD_SD_SET_WIDTH, 2, 0);
		/* if positive response */
		MCI->ctype = MCI_CTYPE_4BIT;
	}
#endif

	/* set block length */
	MCI->blksiz = MMC_SECTOR_SIZE;
	status = mci_execute_command(pdev, CMD_SET_BLOCKLEN, MMC_SECTOR_SIZE, 0);

	return _NO_ERROR;
}

long mci_execute_command(MCI_CARD_INFO_T* pdev,
		ulong cmd, ulong arg, ulong wait_status)
{
	/* if APP command there are 2 stages */
	int step = (cmd & CMD_BIT_APP) ? 2 : 1;
	long status = 0;
	ulong cmd_reg = 0;

	if (!wait_status)
		wait_status = (cmd & CMD_MASK_RESP) ? MCI_INT_CMD_DONE : MCI_INT_DATA_OVER;

	/* Clear the interrupts & FIFOs*/
	if (cmd & CMD_BIT_DATA)
		prv_clear_all();

	while (step) {
		prv_set_clock((cmd & CMD_BIT_LS) ? SD_MMC_ENUM_CLOCK : pdev->speed);


		/* Clear the interrupts */
		MCI->rintsts = 0xFFFFFFFF;

		switch (step) {
		case 1:
			/* Execute command */
			cmd_reg = ((cmd & CMD_MASK_CMD) >> CMD_SHIFT_CMD)
				| ((cmd & CMD_BIT_INIT) ? MCI_CMD_INIT : 0)
				| ((cmd & CMD_BIT_DATA) ? (MCI_CMD_DAT_EXP |
					MCI_CMD_PRV_DAT_WAIT) : 0)
				| (((cmd & CMD_MASK_RESP) == CMD_RESP_R2) ? 
						MCI_CMD_RESP_LONG : 0)
				| ((cmd & CMD_MASK_RESP) ? MCI_CMD_RESP_EXP : 0)
				| ((cmd & CMD_BIT_WRITE) ? MCI_CMD_DAT_WR : 0)
				| ((cmd & CMD_BIT_STREAM) ? MCI_CMD_STRM_MODE : 0)
				| ((cmd & CMD_BIT_BUSY) ? MCI_CMD_STOP : 0)
				| ((cmd & CMD_BIT_AUTO_STOP) ? MCI_CMD_SEND_STOP : 0)
				| MCI_CMD_START;

			/* wait for previos data finsh for selct/deselct commands */
			if (((cmd & CMD_MASK_CMD) >> CMD_SHIFT_CMD) == MMC_SELECT_CARD) {
				cmd_reg |= MCI_CMD_PRV_DAT_WAIT;
			}

			/* wait for command to be accepted by CIU */
			if (prv_send_cmd(cmd_reg, arg) == 0)
				--step;

			break;
		case 0:
			return 0;
		case 2:
			/* APP prefix */
			cmd_reg = MMC_APP_CMD
				| MCI_CMD_RESP_EXP /* Response is status */
				| ((cmd & CMD_BIT_INIT) ? MCI_CMD_INIT : 0)
				| MCI_CMD_START;
			if (prv_send_cmd(cmd_reg, pdev->rca << 16) == 0)
				--step;
			break;
		}

		/* wait for command response*/
		status = prv_wait_for_completion(pdev, wait_status);

		/* We return an error if there is a timeout, even if we've fetched
		 * a response */
		if (status & MCI_INT_ERROR)
			return status;

		if (status & MCI_INT_CMD_DONE) {
			switch (cmd & CMD_MASK_RESP) {
			case 0:
				break;
			case CMD_RESP_R1:
			case CMD_RESP_R3:
				prv_pull_response(pdev, 48);
				break;
			case CMD_RESP_R2:
				prv_pull_response(pdev, 136);
				break;
			}
		}
	}
	return 0;
}


void mci_acquire(MCI_CARD_INFO_T* pdev)
{
	int status;
	int tries = 0;
	ulong ocr = OCR_VOLTAGE_RANGE_MSK;
	ulong r;
	int state = 0;
	ulong command = 0;

	/* clear card struct */
	memset(pdev, 0, sizeof(MCI_CARD_INFO_T));

	/* clear card type */
	MCI->ctype = 0;

	/* we could call board specific card detect routine here */

	/* set high speed for the card as 20MHz */
	pdev->speed = MMC_MAX_CLOCK;

	status = mci_execute_command(pdev, CMD_IDLE, 0, MCI_INT_CMD_DONE);

	while (state < 100) {
		switch (state) {
		case 0:
			/* Setup for SD */
			/* check if it is SDHC card */
			status = mci_execute_command(pdev, CMD_SD_SEND_IF_COND, SD_SEND_IF_ARG, 0);
			if (!(status & MCI_INT_RTO)) {
					/* check response has same echo pattern */
				if ((pdev->response[0] & SD_SEND_IF_ECHO_MSK) == SD_SEND_IF_RESP)
					/* it is SD 2.0 card so indicate we are SDHC capable*/
					ocr |= OCR_HC_CCS;
			}
				++state;
			command = CMD_SD_OP_COND;
			tries = INIT_OP_RETRIES;
			/* assume SD card */
			pdev->card_type |= CARD_TYPE_SD;
			/* for SD cards high speed is 25MHz */
			pdev->speed = SD_MAX_CLOCK;
				break;
		case 10:
			/* Setup for MMC */
			/* start fresh for MMC crds */
			pdev->card_type &= ~CARD_TYPE_SD;
			status = mci_execute_command(pdev, CMD_IDLE, 0, MCI_INT_CMD_DONE);
			command = CMD_MMC_OP_COND;
			tries = INIT_OP_RETRIES;
			ocr |= OCR_HC_CCS;
			++state;
			/* for MMC cards high speed is 20MHz */
			pdev->speed = MMC_MAX_CLOCK;
			break;
		case 1:
		case 11:
			status = mci_execute_command(pdev, command, 0, 0);
			if (status & MCI_INT_RTO)
				state += 9; /* Mode unavailable */
			else
				++state;
			break;
		case 2:
		case 12:
			ocr = pdev->response[0] | (ocr & OCR_HC_CCS);
			if (ocr & OCR_ALL_READY)
				++state;
			else
				state += 2;
			break;
		/* Initial wait for OCR clear */
		case 3:
		case 13:
			while ((ocr & OCR_ALL_READY) && --tries > 0) {
				udelay(MS_ACQUIRE_DELAY);
				status = mci_execute_command(pdev, command, 0, 0);
				ocr = pdev->response[0] | (ocr & OCR_HC_CCS);
			}
			if (ocr & OCR_ALL_READY)
				state += 7;
			else
				++state;
			break;

		case 14:
			/* for MMC cards set high capacity bit */
			ocr |= OCR_HC_CCS;
		case 4:
			/* Assign OCR */
			tries = SET_OP_RETRIES;
			ocr &= OCR_VOLTAGE_RANGE_MSK | OCR_HC_CCS; /* Mask for the bits we care about */
			do {
				udelay(MS_ACQUIRE_DELAY);
				status = mci_execute_command(pdev, command, ocr, 0);
				r = pdev->response[0];
			}
			while (!(r & OCR_ALL_READY) && --tries > 0);
			if (r & OCR_ALL_READY) {
				/* is it high capacity card */
				pdev->card_type |= (r & OCR_HC_CCS);
				++state;
			}
			else
				state += 6;
			break;
		case 5:
		case 15:
			/* CID polling */
			status = mci_execute_command(pdev, CMD_ALL_SEND_CID, 0, 0);
			memcpy(pdev->cid, pdev->response, 16);
			++state;
			break;
		case 6:
			/* RCA send, for SD get RCA */
			status = mci_execute_command(pdev, CMD_SD_SEND_RCA, 0, 0);
			pdev->rca = (pdev->response[0]) >> 16;
			++state;
			break;
		case 16:
			/* RCA assignment for MMC set to 1 */
			pdev->rca = 1;
			status = mci_execute_command(pdev, CMD_MMC_SET_RCA, pdev->rca << 16, 0);
			++state;
			break;
		case 7:
		case 17:
			status = mci_execute_command(pdev, CMD_SEND_CSD, pdev->rca << 16, 0);
			memcpy(pdev->csd, pdev->response, 16);
			state = 100;
			break;
		default:
			/* break from while loop */
			state += 100; 
			break;
		}
	}

	if (prv_card_acquired(pdev)) {
		/* change delay gates per card type */
		if (pdev->card_type & CARD_TYPE_SD)
			SYS_REGS->mci_delaymodes = SYS_REG_SD_CARD_DELAY;
		else
			SYS_REGS->mci_delaymodes = SYS_REG_MMC_CARD_DELAY;

		/*
		 * now compute card size, block size and no. of blocks
		 * based on CSD response recived.
		 */
		prv_process_csd(pdev);
	}
}

int mmc_legacy_init(int verbose)
{
	/* enable SD/MMC clock */
	cgu_clk_en_dis(CGU_SB_SD_MMC_HCLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_SD_MMC_CCLK_IN_ID, 1);

	/* reset SD/MMC/MCI modules through CGU */
	cgu_soft_reset_module(SD_MMC_PNRES_SOFT);
	cgu_soft_reset_module(SD_MMC_NRES_CCLK_IN_SOFT);

	/* Set IOCONF to MCI pins */
	SYS_REGS->mci_delaymodes = 0;
	SYS_REGS->mux_gpio_mci_sel = 1;
	/* set the pins as driven by IP in IOCONF */
	IOCONF->block[IOCONF_MUX0].mode1_clear = 0xF0000003;
	IOCONF->block[IOCONF_MUX0].mode0_set = 0xF0000003;

	/* set delay gates */
	SYS_REGS->mci_delaymodes = SYS_REG_SD_CARD_DELAY;

	/* reset all blocks */
	MCI->ctrl = MCI_CTRL_RESET | MCI_CTRL_FIFO_RESET
		| MCI_CTRL_DMA_RESET;
	/* wait till resets clear */
	while (MCI->ctrl & (MCI_CTRL_RESET | MCI_CTRL_FIFO_RESET |
			MCI_CTRL_DMA_RESET));

	/* Clear the interrupts for the host controller */
	MCI->rintsts = 0xFFFFFFFF;

	/* Put in max timeout */
	MCI->tmout = 0xFFFFFFFF;

	/* FIFO threshold settings */
	MCI->fifoth = (0x2 << 28) | (0xF << 16) | (0x10 << 0);

	/* disable clock to CIU */
	MCI->clkena = 0;
	MCI->clksrc = 0;

	/* clear mmc structure*/
	memset(&g_card_info, 0, sizeof(MCI_CARD_INFO_T));

	/* start card enumeration */
	mci_acquire(&g_card_info);
	if(g_card_info.cid[0] == 0) {
		return 1;
	}
	mmc_dev.if_type = IF_TYPE_MMC;
	mmc_dev.part_type = PART_TYPE_DOS;
	mmc_dev.dev = 0;
	mmc_dev.lun = 0;
	mmc_dev.type = 0;
	mmc_dev.blksz = g_card_info.block_len;
	mmc_dev.lba = g_card_info.blocknr;
	sprintf((char*)mmc_dev.vendor, "Unknown vendor");
	sprintf((char*)mmc_dev.product, "Unknown product");
	sprintf((char*)mmc_dev.revision, "N/A");
	mmc_dev.removable = 0;  /* should be true??? */
	mmc_dev.block_read = mci_read_blocks;
	mmc_dev.block_write = mci_write_blocks;

	return 0;
}

ulong mci_read_blocks( int devid, ulong start_block,
		long blkcnt, void* buffer, int enc)
{
	MCI_CARD_INFO_T* pdev = &g_card_info;
	long end_block = start_block + blkcnt - 1; 
	long cbRead = (end_block - start_block + 1) << 9; /*(end_block - start_block) * 512 */
	long status;
	long index;

	if (!blkcnt)
		return 0;

	/* if card is not acquired return immediately */
	if ((prv_card_acquired(pdev) == 0)
			|| (end_block < start_block) /* check block index in range */
			|| (start_block < 0)
			|| (end_block > pdev->blocknr)) {
		printf("Invalid parameters or Card is not aquired\n");
		return -1;
	}
	/* put card in trans state */
	if (prv_set_trans_state(pdev) != _NO_ERROR)
		return 0;

	/* set number of bytes to read */
	MCI->bytcnt = cbRead;

	/* if high capacity card use block indexing */
	if (pdev->card_type & CARD_TYPE_HC)
		index = start_block;
	else
		index = start_block << 9;

	/* check how many blocks to read */
	if (end_block == start_block) {
		status = mci_execute_command(pdev, CMD_READ_SINGLE, index,
				MCI_INT_CMD_DONE | MCI_INT_DATA_OVER | MCI_INT_RXDR);
	}
	else {
		/* do read multiple */
		status = mci_execute_command(pdev, CMD_READ_MULTIPLE, index,
				MCI_INT_CMD_DONE | MCI_INT_DATA_OVER | MCI_INT_RXDR);
	}

	memset(buffer,0,cbRead);

	/* read data from the FIFO */
	if (status == 0)
		cbRead = prv_pull_data((unsigned char*)buffer, cbRead, enc);
	else
		cbRead = 0; /* return error if command fails */

	return cbRead >> 9;
}

ulong mci_write_blocks(int devid, ulong start_block,
		long blkcnt, void* buffer)
{
	MCI_CARD_INFO_T* pdev = &g_card_info;
	/*(end_block - start_block) * 512 */
	long end_block = start_block + blkcnt - 1; 
	long cbWrote = (end_block - start_block + 1) << 9;
	int status;
	long index;

	/* if card is not acquired return immediately */
	if ((prv_card_acquired(pdev) == 0)
			|| (end_block < start_block) /* check block index in range */
			|| (start_block < 0)
			|| (end_block > pdev->blocknr)) {
		return 0;
	}

	/* put card in trans state */
	if (prv_set_trans_state(pdev) != _NO_ERROR)
		return 0;

	/* set number of bytes to write */
	MCI->bytcnt = cbWrote;

	/* if high capacity card use block indexing */
	if (pdev->card_type & CARD_TYPE_HC)
		index = start_block;
	else
		index = start_block << 9;

	/* check how many blocks to write */
	if (end_block == start_block) {
		status = mci_execute_command(pdev, CMD_WRITE_SINGLE, index,
				MCI_INT_CMD_DONE | MCI_INT_TXDR);
	}
	else {
		/* do write multiple */
		status = mci_execute_command(pdev, CMD_WRITE_MULTIPLE, index,
				MCI_INT_CMD_DONE | MCI_INT_TXDR);
	}
	/* write data to the FIFO */
	if (status == 0) {
		prv_push_data((unsigned char*)buffer, cbWrote);
		/* wait for transfer done */
		status = prv_wait_for_completion(pdev, MCI_INT_DATA_OVER);
	}
	else {
		cbWrote = 0; /* return error if command fails */
	}

	return cbWrote >> 9;
}


unsigned int mmc_get_card_size(int dev)
{
	return (g_card_info.device_size >> 9);
}

block_dev_desc_t * mmc_get_dev(int dev)
{
	return (block_dev_desc_t *)(&mmc_dev);
}

#endif
