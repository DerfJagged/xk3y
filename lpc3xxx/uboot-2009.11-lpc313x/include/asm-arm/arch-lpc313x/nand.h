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

#ifndef NAND_H
#define NAND_H

#include <common.h>
#include <asm/arch/hardware.h>

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************************
 * NAND device structure and it defines
***********************************************************************/
/*
 * Timing information structure for the NAND interface. Although there are
 * multiple chip selects for the NAND controller, there is only 1 set of
 * timing data shared among all chip selects. All the parts should be of
 * These values are used to adjust the NAND timing to the current system
 * clock speed.
 *
 * These values are NanoSecond timings. See the LPC31xx Users Guide for
 * information on what these timings are and set the value for each timing
 * with the matching value from the NAND device data sheet.
 */
struct lpc313x_nand_timing
{
	u32 ns_trsd;
	u32 ns_tals;
	u32 ns_talh;
	u32 ns_tcls;
	u32 ns_tclh;
	u32 ns_tdrd;
	u32 ns_tebidel;
	u32 ns_tch;
	u32 ns_tcs;
	u32 ns_treh;
	u32 ns_trp;
	u32 ns_trw;
	u32 ns_twp;
};

/* NAND Flash configuration parameters structure used by bootROM */
typedef struct
{
	char tag[8];
	u8 interface_width;
	u8 reserv_1;
	u16 page_size_in_bytes;
	u16 page_size_in_32bit_words;
	u16 pages_per_block;
	u32 nbr_of_blocks;
	u8 amount_of_address_bytes;
	u8 amount_of_erase_address_bytes;
	u8 support_read_terminate;
	u8 page_increment_byte_nr;
	char device_name[40];
	u32 timing1;
	u32 timing2;
	u8 ecc_mode;
	u8 id_mask;
	u8 reserv_2[2];
} NAND_BOOT_CFG_PARAMS_T;

/* NAND Flash controller Module Register Structure */
typedef volatile struct
{
	volatile unsigned long irq_status;
	volatile unsigned long irq_mask;
	volatile unsigned long irq_status_raw;
	volatile unsigned long config;
	volatile unsigned long io_config;
	volatile unsigned long timing1;
	volatile unsigned long timing2;
	volatile unsigned long _unused1[1];
	volatile unsigned long set_cmd;
	volatile unsigned long set_addr;
	volatile unsigned long write_data;
	volatile unsigned long set_ce;
	volatile unsigned long read_data;
	volatile unsigned long check_sts;
	volatile unsigned long control_flow;
	volatile unsigned long _unused2[1];
	volatile unsigned long gpio1;
	volatile unsigned long gpio2;
	volatile unsigned long irq_status2;
	volatile unsigned long irq_mask2;
	volatile unsigned long irq_status_raw2;
	volatile unsigned long aes_key[4];
	volatile unsigned long aes_iv[4];
	volatile unsigned long aes_state;
	volatile unsigned long ecc_error_stat;
	volatile unsigned long aes_from_ahb;
} NAND_FLASH_CTRL_REGS_T;

/**********************************************************************
* Register description of irq_status
**********************************************************************/
#define NAND_IRQ_RB4_POS_EDGE		_BIT(31)
#define NAND_IRQ_RB3_POS_EDGE		_BIT(30)
#define NAND_IRQ_RB2_POS_EDGE		_BIT(29)
#define NAND_IRQ_RB1_POS_EDGE		_BIT(28)
#define NAND_IRQ_ERASED_RAM1		_BIT(27)
#define NAND_IRQ_ERASED_RAM0		_BIT(26)
#define NAND_IRQ_WR_RAM1		_BIT(25)
#define NAND_IRQ_WR_RAM0		_BIT(24)
#define NAND_IRQ_RD_RAM1		_BIT(23)
#define NAND_IRQ_RD_RAM0		_BIT(22)
#define NAND_IRQ_ECC_DEC_RAM0		_BIT(21)
#define NAND_IRQ_ECC_ENC_RAM0		_BIT(20)
#define NAND_IRQ_ECC_DEC_RAM1		_BIT(19)
#define NAND_IRQ_ECC_ENC_RAM1		_BIT(18)
#define NAND_IRQ_NOERR_RAM0		_BIT(17)
#define NAND_IRQ_ERR1_RAM0		_BIT(16)
#define NAND_IRQ_ERR2_RAM0		_BIT(15)
#define NAND_IRQ_ERR3_RAM0		_BIT(14)
#define NAND_IRQ_ERR4_RAM0		_BIT(13)
#define NAND_IRQ_ERR5_RAM0		_BIT(12)
#define NAND_IRQ_ERR_UNR_RAM0		_BIT(11)
#define NAND_IRQ_NOERR_RAM1		_BIT(10)
#define NAND_IRQ_ERR1_RAM1		_BIT(9)
#define NAND_IRQ_ERR2_RAM1		_BIT(8)
#define NAND_IRQ_ERR3_RAM1		_BIT(7)
#define NAND_IRQ_ERR4_RAM1		_BIT(6)
#define NAND_IRQ_ERR5_RAM1		_BIT(5)
#define NAND_IRQ_ERR_UNR_RAM1		_BIT(4)
#define NAND_IRQ_AES_DONE_RAM1		_BIT(1)
#define NAND_IRQ_AES_DONE_RAM0		_BIT(0)

/**********************************************************************
* Register description of config
**********************************************************************/
#define NAND_CFG_ECGC			_BIT(13)
#define NAND_CFG_8BIT_ECC		_BIT(12)
#define NAND_CFG_TL_528			_SBF(10, 0x0)
#define NAND_CFG_TL_516			_SBF(10, 0x2)
#define NAND_CFG_TL_512			_SBF(10, 0x3)
#define NAND_CFG_TL_MASK		_SBF(10, 0x3)
#define NAND_CFG_EO			_BIT(9)
#define NAND_CFG_DC			_BIT(8)
#define NAND_CFG_M			_BIT(7)
#define NAND_CFG_LC_0			_SBF(5, 0x0)
#define NAND_CFG_LC_1			_SBF(5, 0x1)
#define NAND_CFG_LC_2			_SBF(5, 0x2)
#define NAND_CFG_LC_MASK		_SBF(5, 0x3)
#define NAND_CFG_ES			_BIT(4)
#define NAND_CFG_DE			_BIT(3)
#define NAND_CFG_AO			_BIT(2)
#define NAND_CFG_WD			_BIT(1)
#define NAND_CFG_EC			_BIT(0)

/**********************************************************************
* Register description of io_config
**********************************************************************/
#define NAND_IO_CFG_IO_DRIVE		_BIT(24)
#define NAND_IO_CFG_DATA_DEF(n)		_SBF(8, ((n) & 0xFFFF))
#define NAND_IO_CFG_CLE_1		_SBF(6, 0x01)
#define NAND_IO_CFG_ALE_1		_SBF(4, 0x01)
#define NAND_IO_CFG_WE_1		_SBF(2, 0x01)
#define NAND_IO_CFG_RE_1		_SBF(0, 0x01)

/**********************************************************************
* Register description of timing1
**********************************************************************/
#define NAND_TIM1_TSRD(n)		_SBF(20, ((n) & 0x3))
#define NAND_TIM1_TALS(n)		_SBF(16, ((n) & 0x7))
#define NAND_TIM1_TALH(n)		_SBF(12, ((n) & 0x7))
#define NAND_TIM1_TCLS(n)		_SBF(4, ((n) & 0x7))
#define NAND_TIM1_TCLH(n)		((n) & 0x7)

/**********************************************************************
* Register description of timing2
**********************************************************************/
#define NAND_TIM2_TDRD(n)		_SBF(28, ((n) & 0x7))
#define NAND_TIM2_TEBI(n)		_SBF(24, ((n) & 0x7))
#define NAND_TIM2_TCH(n)		_SBF(20, ((n) & 0x7))
#define NAND_TIM2_TCS(n)		_SBF(16, ((n) & 0x7))
#define NAND_TIM2_TRH(n)		_SBF(12, ((n) & 0x7))
#define NAND_TIM2_TRP(n)		_SBF(8, ((n) & 0x7))
#define NAND_TIM2_TWH(n)		_SBF(4, ((n) & 0x7))
#define NAND_TIM2_TWP(n)		((n) & 0x7)

/**********************************************************************
* Register description of set_ce
**********************************************************************/
#define NAND_SETCE_OVR_EN(n)		_BIT(((n) & 0x3) + 12)
#define NAND_SETCE_OVR_V(n)		_BIT(((n) & 0x3) + 8)
#define NAND_SETCE_WP			_BIT(4)
#define NAND_SETCE_CV_MASK		0x0F
#define NAND_SETCE_CV(n)		(0x0F & ~_BIT(((n) & 0x3)))

/**********************************************************************
* Register description of check_sts
**********************************************************************/
#define NAND_CHK_STS_RB4_EDGE		_BIT(8)
#define NAND_CHK_STS_RB3_EDGE		_BIT(7)
#define NAND_CHK_STS_RB2_EDGE		_BIT(6)
#define NAND_CHK_STS_RB1_EDGE		_BIT(5)
#define NAND_CHK_STS_RB4_LVL		_BIT(4)
#define NAND_CHK_STS_RB3_LVL		_BIT(3)
#define NAND_CHK_STS_RB2_LVL		_BIT(2)
#define NAND_CHK_STS_RB1_LVL		_BIT(1)
#define NAND_CHK_STS_APB_BSY		_BIT(0)

/**********************************************************************
* Register description of aes_from_ahb
**********************************************************************/
#define NAND_CTRL_WR_RAM1		_BIT(5)
#define NAND_CTRL_WR_RAM0		_BIT(4)
#define NAND_CTRL_RD_RAM1		_BIT(1)
#define NAND_CTRL_RD_RAM0		_BIT(0)

/**********************************************************************
* Register description of control_flow
**********************************************************************/
#define NAND_AES_AHB_EN			_BIT(7)
#define NAND_AES_AHB_DCRYPT_RAM1	_BIT(1)
#define NAND_AES_AHB_DCRYPT_RAM0	_BIT(0)

/* Macro pointing to CGU switch box registers */
#define NAND_CTRL	((NAND_FLASH_CTRL_REGS_T *)(NANDFLASH_CTRL_CFG_BASE))

unsigned int crc32_compute(unsigned char *data, int length);
int prepare_write_nand_params_bbt(unsigned char isbooting);

#ifdef __cplusplus
}
#endif

#endif /* NAND_H */
