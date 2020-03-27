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

#ifndef SDMMC_H
#define SDMMC_H

#if defined (__cplusplus)
extern "C"
{
#endif

/* 
 * SD/MMC commands - this matrix shows the command, response types, and
 * supported card type for that command.
 * Command                 Number Resp  SD  MMC
 * ----------------------- ------ ----- --- ---
 * Reset (go idle)         CMD0   NA    x   x
 * Send op condition       CMD1   R3        x
 * All send CID            CMD2   R2    x   x
 * Send relative address   CMD3   R1        x
 * Send relative address   CMD3   R6    x
 * Program DSR             CMD4   NA        x
 * Select/deselect card    CMD7   R1b       x
 * Select/deselect card    CMD7   R1    x
 * Send CSD                CMD9   R2    x   x
 * Send CID                CMD10  R2    x   x
 * Read data until stop    CMD11  R1    x   x
 * Stop transmission       CMD12  R1/b  x   x
 * Send status             CMD13  R1    x   x
 * Go inactive state       CMD15  NA    x   x
 * Set block length        CMD16  R1    x   x
 * Read single block       CMD17  R1    x   x
 * Read multiple blocks    CMD18  R1    x   x
 * Write data until stop   CMD20  R1        x
 * Setblock count          CMD23  R1        x
 * Write single block      CMD24  R1    x   x
 * Write multiple blocks   CMD25  R1    x   x
 * Program CID             CMD26  R1        x
 * Program CSD             CMD27  R1    x   x
 * Set write protection    CMD28  R1b   x   x
 * Clear write protection  CMD29  R1b   x   x
 * Send write protection   CMD30  R1    x   x
 * Erase block start       CMD32  R1    x
 * Erase block end         CMD33  R1    x
 * Erase block start       CMD35  R1        x
 * Erase block end         CMD36  R1        x
 * Erase blocks            CMD38  R1b       x
 * Fast IO                 CMD39  R4        x
 * Go IRQ state            CMD40  R5        x
 * Lock/unlock             CMD42  R1b       x
 * Application command     CMD55  R1        x
 * General command         CMD56  R1b       x
 * SD card application commands - these must be preceded with
 * MMC CMD55 application specific command first
 * Set bus width           ACMD6  R1    x
 * Send SD status          ACMD13 R1    x
 * Send number WR blocks   ACMD22 R1    x
 * Set WR block erase cnt  ACMD23 R1    x
 * Send op condition       ACMD41 R3    x
 * Set clear card detect   ACMD42 R1    x
 * Send CSR                ACMD51 R1    x */
typedef enum
{
	SDMMC_IDLE,		/* Put card in idle mode */
	MMC_SENDOP_COND,	/* Send operating condition */
	SDMMC_ALL_SEND_CID,	/* All cards send CID */
	SDMMC_SRA,		/* Set relative address */
	MMC_PROGRAM_DSR,	/* Program DSR */
	SDMMC_SELECT_CARD,	/* Select card */
	SDMMC_SEND_CSD,		/* Send CSD data */
	SDMMC_SEND_CID,		/* Send CID register data (with rel.
					addr) */
	SDMMC_READ_UNTIL_STOP,	/* Read data until stop */
	SDMMC_STOP_XFER,	/* Stop current transmission */
	SDMMC_SSTAT,		/* Send status */
	SDMMC_INACTIVE,		/* Put card in inactive state */
	SDMMC_SET_BLEN,		/* Set block transfer length */
	SDMMC_READ_SINGLE,	/* Read single block */
	SDMMC_READ_MULTIPLE,	/* Read multiple blocks */
	SDMMC_WRITE_UNTIL_STOP,	/* Write data until stop */
	SDMMC_SET_BLOCK_COUNT,	/* Set block count */
	SDMMC_WRITE_SINGLE,	/* Write single block */
	SDMMC_WRITE_MULTIPLE,	/* Write multiple blocks */
	MMC_PROGRAM_CID,	/* Program CID */
	SDMMC_PROGRAM_CSD,	/* Program CSD */
	SDMMC_SET_WR_PROT,	/* Set write protection */
	SDMMC_CLEAR_WR_PROT,	/* Clear write protection */
	SDMMC_SEND_WR_PROT,	/* Send write protection */
	SD_ERASE_BLOCK_START,	/* Set starting erase block */
	SD_ERASE_BLOCK_END,	/* Set ending erase block */
	MMC_ERASE_BLOCK_START,	/* Set starting erase block */
	MMC_ERASE_BLOCK_END,	/* Set ending erase block */
	MMC_ERASE_BLOCKS,	/* Erase blocks */
	MMC_FAST_IO,		/* Fast IO */
	MMC_GO_IRQ_STATE,	/* Go into IRQ state */
	MMC_LOCK_UNLOCK,	/* Lock/unlock */
	SDMMC_APP_CMD,		/* Application specific command */
	SDMMC_GEN_CMD,		/* General purpose command */
	SDMMC_INVALID_CMD	/* Invalid SDMMC command */
} SDMMC_COMMAND_T;

/*
 * SDMMC application specific commands for SD cards only - these must
 * be preceded by the SDMMC CMD55 to work correctly
 */
typedef enum
{
	SD_SET_BUS_WIDTH,
	SD_SEND_STATUS,
	SD_SEND_WR_BLOCKS,
	SD_SET_ERASE_COUNT,
	SD_SENDOP_COND,
	SD_CLEAR_CARD_DET,
	SD_SEND_SCR,
	SD_INVALID_APP_CMD
} SD_APP_CMD_T;

/* Possible SDMMC response types */
typedef enum
{
	SDMMC_RESPONSE_R1,	/* Typical status */
	SDMMC_RESPONSE_R1B,	/* Typical status with busy */
	SDMMC_RESPONSE_R2,	/* CID/CSD registers (CMD2 and CMD10) */
	SDMMC_RESPONSE_R3,	/* OCR register (CMD1, ACMD41) */
	SDMMC_RESPONSE_R4,	/* Fast IO response word */
	SDMMC_RESPONSE_R5,	/* Go IRQ state response word */
	SDMMC_RESPONSE_R6,	/* Published RCA response */
	SDMMC_RESPONSE_NONE	/* No response expected */
} SDMMC_RESPONSE_T;

/* Possible SDMMC card state types */
typedef enum
{
	SDMMC_IDLE_ST = 0,
	SDMMC_READY_ST,
	SDMMC_IDENT_ST,
	SDMMC_STBY_ST,
	SDMMC_TRAN_ST,
	SDMMC_DATA_ST,
	SDMMC_RCV_ST,
	SDMMC_PRG_ST,
	SDMMC_DIS_ST
} SDMMC_STATE_T;

#if defined (__cplusplus)
}
#endif /*__cplusplus */

/* Standard MMC commands (3.1) type argument response */
/* class 1 */
#define MMC_GO_IDLE_STATE		0	/* bc */
#define MMC_SEND_OP_COND		1	/* bcr [31:0] OCR R3 */
#define MMC_ALL_SEND_CID		2	/* bcr R2 */
#define MMC_SET_RELATIVE_ADDR		3	/* ac [31:16] RCA R1 */
#define MMC_SET_DSR			4	/* bc [31:16] RCA */
#define MMC_SELECT_CARD			7	/* ac [31:16] RCA R1 */
#define MMC_SEND_EXT_CSD		8	/* bc R1 */
#define MMC_SEND_CSD			9	/* ac [31:16] RCA R2 */
#define MMC_SEND_CID			10	/* ac [31:16] RCA R2 */
#define MMC_STOP_TRANSMISSION		12	/* ac R1b */
#define MMC_SEND_STATUS			13	/* ac [31:16] RCA R1 */
#define MMC_GO_INACTIVE_STATE		15	/* ac [31:16] RCA */

/* class 2 */
#define MMC_SET_BLOCKLEN		16	/* ac [31:0] block len R1 */
#define MMC_READ_SINGLE_BLOCK		17	/* adtc [31:0] data addr R1 */
#define MMC_READ_MULTIPLE_BLOCK		18	/* adtc [31:0] data addr R1 */

/* class 3 */
#define MMC_WRITE_DAT_UNTIL_STOP	20	/* adtc [31:0] data addr R1 */

/* class 4 */
#define MMC_SET_BLOCK_COUNT		23	/* adtc [31:0] data addr R1 */
#define MMC_WRITE_BLOCK			24	/* adtc [31:0] / data addr R1 */
#define MMC_WRITE_MULTIPLE_BLOCK	25	/* adtc R1 */
#define MMC_PROGRAM_CID			26	/* adtc R1 */
#define MMC_PROGRAM_CSD			27	/* adtc R1 */

/* class 6 */
#define MMC_SET_WRITE_PROT		28	/* ac [31:0] data addr R1b */
#define MMC_CLR_WRITE_PROT		29	/* ac [31:0] data addr R1b */
#define MMC_SEND_WRITE_PROT		30	/* adtc [31:0] wpdata addr R1 */

/* class 5 */
#define MMC_ERASE_GROUP_START		35	/* ac [31:0] data addr R1 */
#define MMC_ERASE_GROUP_END		36	/* ac [31:0] data addr R1 */
#define MMC_ERASE			37	/* ac R1b */

/* class 9 */
#define MMC_FAST_IO			39	/* ac <Complex> R4 */
#define MMC_GO_IRQ_STATE		40	/* bcr R5 */

/* class 7 */
#define MMC_LOCK_UNLOCK			42	/* adtc R1b */

/* class 8 */
#define MMC_APP_CMD			55	/* ac [31:16] RCA R1 */
#define MMC_GEN_CMD			56	/* adtc [0] RD/WR R1b */

/* SD commands type argument response */
/* class 8 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR		3	/* ac R6 */
#define SD_CMD8				8	/* bcr [31:0] OCR R3 */

/* Application commands */
#define SD_APP_SET_BUS_WIDTH		6	/* ac [1:0] bus width R1 */
#define SD_APP_OP_COND			41	/* bcr [31:0] OCR R1 (R4) */
#define SD_APP_SEND_SCR			51	 /* adtc R1 */

/*
  MMC status in R1
  Type
    e : error bit
    s : status bit
    r : detected and set for the actual command response
    x : detected and set during command execution. the host must poll
        the card by sending status command in order to read these bits.
  Clear condition
    a : according to the card state
    b : always related to the previous command. Reception of
        a valid command will clear it (with a delay of one command)
    c : clear by read
 */

#define R1_OUT_OF_RANGE			_BIT(31)	/* er, c */
#define R1_ADDRESS_ERROR		_BIT(30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR		_BIT(29)	/* er, c */
#define R1_ERASE_SEQ_ERROR		_BIT(28)	/* er, c */
#define R1_ERASE_PARAM			_BIT(27)	/* ex, c */
#define R1_WP_VIOLATION			_BIT(26)	/* erx, c */
#define R1_CARD_IS_LOCKED		_BIT(25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED		_BIT(24)	/* erx, c */
#define R1_COM_CRC_ERROR		_BIT(23)	/* er, b */
#define R1_ILLEGAL_COMMAND		_BIT(22)	/* er, b */
#define R1_CARD_ECC_FAILED		_BIT(21)	/* ex, c */
#define R1_CC_ERROR			_BIT(20)	/* erx, c */
#define R1_ERROR			_BIT(19)	/* erx, c */
#define R1_UNDERRUN			_BIT(18)	/* ex, c */
#define R1_OVERRUN			_BIT(17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE		_BIT(16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP		_BIT(15)	/* sx, c */
#define R1_CARD_ECC_DISABLED		_BIT(14)	/* sx, a */
#define R1_ERASE_RESET			_BIT(13)	/* sr, c */
#define R1_STATUS(x)			(x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)		((x & 0x00001E00) >> 9) /* sx, b (4 bits) */
#define R1_READY_FOR_DATA		_BIT(8) /* sx, a */
#define R1_APP_CMD			_BIT(5) /* sr, c */

#define OCR_ALL_READY			_BIT(31)	/* Card Power up status bit */
#define OCR_HC_CCS			_BIT(30)	/* High capacity card */
#define OCR_VOLTAGE_RANGE_MSK		0x00ff8000

#define SD_SEND_IF_ARG			0x000001AA
#define SD_SEND_IF_ECHO_MSK		0x000000FF
#define SD_SEND_IF_RESP			0x000000AA


#define CMD_MASK_RESP			_SBF(28, 0x3)
#define CMD_RESP(r)			_SBF(28, ((r) & 0x3))
#define CMD_RESP_R0			_SBF(28, 0)
#define CMD_RESP_R1			_SBF(28, 1)
#define CMD_RESP_R2			_SBF(28, 2)
#define CMD_RESP_R3			_SBF(28, 3)
#define CMD_BIT_AUTO_STOP		_BIT(24)
#define CMD_BIT_APP			_BIT(23)
#define CMD_BIT_INIT			_BIT(22)
#define CMD_BIT_BUSY			_BIT(21)
#define CMD_BIT_LS			_BIT(20)	/* Low speed, used during acquire */
#define CMD_BIT_DATA			_BIT(19)
#define CMD_BIT_WRITE			_BIT(18)
#define CMD_BIT_STREAM			_BIT(17)
#define CMD_MASK_CMD			(0xff)
#define CMD_SHIFT_CMD			(0)

#define CMD(c,r)		( ((c) & CMD_MASK_CMD) | CMD_RESP((r)) )

#define CMD_IDLE		CMD(MMC_GO_IDLE_STATE,0) | CMD_BIT_LS | CMD_BIT_INIT
#define CMD_SD_OP_COND		CMD(SD_APP_OP_COND,1) | CMD_BIT_LS | CMD_BIT_APP
#define CMD_SD_SEND_IF_COND	CMD(SD_CMD8,1) | CMD_BIT_LS
#define CMD_MMC_OP_COND		CMD(MMC_SEND_OP_COND,3) | CMD_BIT_LS | CMD_BIT_INIT
#define CMD_ALL_SEND_CID	CMD(MMC_ALL_SEND_CID,2) | CMD_BIT_LS
#define CMD_MMC_SET_RCA		CMD(MMC_SET_RELATIVE_ADDR,1) | CMD_BIT_LS
#define CMD_SD_SEND_RCA		CMD(SD_SEND_RELATIVE_ADDR,1) | CMD_BIT_LS
#define CMD_SEND_CSD		CMD(MMC_SEND_CSD,2) | CMD_BIT_LS
#define CMD_SEND_EXT_CSD	CMD(MMC_SEND_EXT_CSD,1) | CMD_BIT_LS | CMD_BIT_DATA
#define CMD_DESELECT_CARD	CMD(MMC_SELECT_CARD,0)
#define CMD_SELECT_CARD		CMD(MMC_SELECT_CARD,1)
#define CMD_SET_BLOCKLEN	CMD(MMC_SET_BLOCKLEN,1)
#define CMD_SEND_STATUS		CMD(MMC_SEND_STATUS,1)
#define CMD_READ_SINGLE		CMD(MMC_READ_SINGLE_BLOCK,1) | CMD_BIT_DATA
#define CMD_READ_MULTIPLE	CMD(MMC_READ_MULTIPLE_BLOCK,1) | CMD_BIT_DATA | CMD_BIT_AUTO_STOP
#define CMD_SD_SET_WIDTH	CMD(SD_APP_SET_BUS_WIDTH,1)| CMD_BIT_APP
#define CMD_STOP		CMD(MMC_STOP_TRANSMISSION,1) | CMD_BIT_BUSY
#define CMD_WRITE_SINGLE	CMD(MMC_WRITE_BLOCK,1) | CMD_BIT_DATA | CMD_BIT_WRITE
#define CMD_WRITE_MULTIPLE	CMD(MMC_WRITE_MULTIPLE_BLOCK,1) | CMD_BIT_DATA | \
				CMD_BIT_WRITE | CMD_BIT_AUTO_STOP

#define MMC_SECTOR_SIZE		512

#endif /* SDMMC_H */
