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

#ifndef MCI_H
#define MCI_H

#include <common.h>
#include <asm/arch/hardware.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct _mci_card_struct
{
	ulong response[4];	/* Most recent response */
	ulong cid[4];		/* CID of acquired card */
	ulong csd[4];		/* CSD of acquired card */
	ulong card_type;
	ulong rca;		/* Relative address assigned to card */
	ulong speed;
	ulong block_len;
	ulong device_size;
	ulong blocknr;
};
typedef struct _mci_card_struct MCI_CARD_INFO_T;

/* Memory Card Interface (MCI) Module Register Structure */
typedef volatile struct
{
	volatile ulong ctrl;	/* control register (R/W) */
	volatile ulong pwren;	/* power-enable register (R/W) */
	volatile ulong clkdiv;	/* clock-divider register (R/W) */
	volatile ulong clksrc;	/* clock-source register (R/W) */
	volatile ulong clkena;	/* clock enable register (R/W) */
	volatile ulong tmout;	/* time-out register (R/W) */
	volatile ulong ctype;	/* card-type register (R/W) */
	volatile ulong blksiz;	/* blck-size register (R/W) */
	volatile ulong bytcnt;	/* byte-count register (R/W) */
	volatile ulong intmsk;	/* interrupt-mask register (R/W) */
	volatile ulong cmdarg;	/* command-argument register (R/W) */
	volatile ulong cmd;	/* command register (R/W) */
	volatile ulong resp0;	/* response-0 register (R) */
	volatile ulong resp1;	/* response-1 register (R) */
	volatile ulong resp2;	/* response-2 register (R) */
	volatile ulong resp3;	/* response-3 register (R) */
	volatile ulong mintsts;	/* masked interrupt-status register (R) */
	volatile ulong rintsts;	/* raw interrupt-status register (R/W) */
	volatile ulong status;	/* status; mainly for debug register (R) */
	volatile ulong fifoth;	/* FIFO treshold register (R/W) */
	volatile ulong cdetec;	/* card-detect register (R) */
	volatile ulong wrtprt;	/* write-protect register (R) */
	volatile ulong gpio;	/* GPIO register (R/W) */
	volatile ulong tcbcnt;	/* transferred CIU card byte count register (R) */
	volatile ulong tbbcnt;	/* transferred host/DMA to/from BIU-FIFO byte count register (R) */
	volatile ulong debnce;	/* card detect debounce register (R/W) */
	volatile ulong usrid;	/* user ID register (R/W) */
	volatile ulong verid;	/* synopsys version ID register (R) */
	volatile ulong hcon;	/* hardware configuration register (R) */
	volatile ulong reserved[23];	/* Data FIFO (R) */
	volatile ulong data;	/* Data FIFO (R) */
} MCI_REGS_T;

#ifdef __cplusplus
}
#endif
/***********************************************************************
* useful defines
***********************************************************************/
#define SD_MMC_ENUM_CLOCK	400000
#define MMC_MAX_CLOCK		20000000
#define MMC_LOW_BUS_MAX_CLOCK	26000000
#define MMC_HIGH_BUS_MAX_CLOCK	52000000
#define SD_MAX_CLOCK		25000000
#define MCI_FIFO_SZ		32
#define SYS_REG_SD_CARD_DELAY	0x1B
#define SYS_REG_MMC_CARD_DELAY	0x16

/***********************************************************************
* Control register defines
***********************************************************************/
#define MCI_CTRL_CEATA_INT_EN	_BIT(11)
#define MCI_CTRL_SEND_AS_CCSD	_BIT(10)
#define MCI_CTRL_SEND_CCSD	_BIT(9)
#define MCI_CTRL_ABRT_READ_DATA	_BIT(8)
#define MCI_CTRL_SEND_IRQ_RESP	_BIT(7)
#define MCI_CTRL_READ_WAIT	_BIT(6)
#define MCI_CTRL_DMA_ENABLE	_BIT(5)
#define MCI_CTRL_INT_ENABLE	_BIT(4)
#define MCI_CTRL_DMA_RESET	_BIT(2)
#define MCI_CTRL_FIFO_RESET	_BIT(1)
#define MCI_CTRL_RESET		_BIT(0)

/***********************************************************************
* Clock Enable register defines
***********************************************************************/
#define MCI_CLKEN_LOW_PWR	_BIT(16)
#define MCI_CLKEN_ENABLE	_BIT(0)

/***********************************************************************
* time-out register defines
***********************************************************************/
#define MCI_TMOUT_DATA(n)	_SBF(8, (n))
#define MCI_TMOUT_DATA_MSK	0xFFFFFF00
#define MCI_TMOUT_RESP(n)	((n) & 0xFF)
#define MCI_TMOUT_RESP_MSK	0xFF

/***********************************************************************
* card-type register defines
***********************************************************************/
#define MCI_CTYPE_8BIT		_BIT(16)
#define MCI_CTYPE_4BIT		_BIT(0)

/***********************************************************************
* Interrupt status & mask register defines
***********************************************************************/
#define MCI_INT_SDIO		_BIT(16)
#define MCI_INT_EBE		_BIT(15)
#define MCI_INT_ACD		_BIT(14)
#define MCI_INT_SBE		_BIT(13)
#define MCI_INT_HLE		_BIT(12)
#define MCI_INT_FRUN		_BIT(11)
#define MCI_INT_HTO		_BIT(10)
#define MCI_INT_DTO		_BIT(9)
#define MCI_INT_RTO		_BIT(8)
#define MCI_INT_DCRC		_BIT(7)
#define MCI_INT_RCRC		_BIT(6)
#define MCI_INT_RXDR		_BIT(5)
#define MCI_INT_TXDR		_BIT(4)
#define MCI_INT_DATA_OVER	_BIT(3)
#define MCI_INT_CMD_DONE	_BIT(2)
#define MCI_INT_RESP_ERR	_BIT(1)
#define MCI_INT_CD		_BIT(0)
#define MCI_INT_ERROR		0xbfc2

/***********************************************************************
* Command register defines
***********************************************************************/
#define MCI_CMD_START		_BIT(31)
#define MCI_CMD_CCS_EXP		_BIT(23)
#define MCI_CMD_CEATA_RD	_BIT(22)
#define MCI_CMD_UPD_CLK		_BIT(21)
#define MCI_CMD_UPD_CLK		_BIT(21)

#define MCI_CMD_INIT		_BIT(15)
#define MCI_CMD_STOP		_BIT(14)
#define MCI_CMD_PRV_DAT_WAIT	_BIT(13)
#define MCI_CMD_SEND_STOP	_BIT(12)
#define MCI_CMD_STRM_MODE	_BIT(11)
#define MCI_CMD_DAT_WR		_BIT(10)
#define MCI_CMD_DAT_EXP		_BIT(9)
#define MCI_CMD_RESP_CRC	_BIT(8)
#define MCI_CMD_RESP_LONG	_BIT(7)
#define MCI_CMD_RESP_EXP	_BIT(6)
#define MCI_CMD_INDX(n)		((n) & 0x1F)

/***********************************************************************
* status register defines
***********************************************************************/
#define MCI_STS_GET_FCNT(x)	(((x)>>17) & 0x1FF)

/**********************************************************************
* Macro to access MCI registers
**********************************************************************/
#define MCI	((MCI_REGS_T*)SD_MMC_BASE)

/* card type defines */
#define CARD_TYPE_SD		(1 << 0)
#define CARD_TYPE_4BIT		(1 << 1)
#define CARD_TYPE_8BIT		(1 << 2)
#define CARD_TYPE_HC		(OCR_HC_CCS) /* high capacity card > 2GB */

/* MCI read function - read all 16 data registers */
ulong mci_read_blocks(int devid, ulong start_block,
		long blkcnt, void* buffer, int enc);

/* MCI write function - writes all 16 data registers */
ulong mci_write_blocks(int devid, ulong start_block,
		long blkcnt, void* buffer);
/* MCI command execution */
long mci_execute_command(MCI_CARD_INFO_T* pdev, ulong cmd,
		ulong arg, ulong wait_status);
#endif /* MCI_H */
