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

#ifndef CLOCK_MAIN_H
#define CLOCK_MAIN_H

#include <common.h>
#include <asm/arch/hardware.h>

#ifdef __cplusplus
extern "C"	/* Assume C declarations for C++ */
{
#endif

#ifndef	__ASSEMBLY__
/* ----------------
* HP PLL Registers
* ----------------
*/
typedef volatile struct
{
	volatile ulong fin_select;
	volatile ulong mdec;
	volatile ulong ndec;
	volatile ulong pdec;
	volatile ulong mode;
	volatile ulong status;
	volatile ulong ack;
	volatile ulong req;
	volatile ulong inselr;
	volatile ulong inseli;
	volatile ulong inselp;
	volatile ulong selr;
	volatile ulong seli;
	volatile ulong selp;
} CGU_HP_CFG_REGS;


typedef volatile struct
{
	volatile ulong powermode;
	volatile ulong wd_bark;
	volatile ulong ffast_on;
	volatile ulong ffast_bypass;
	volatile ulong apb0_resetn_soft[56];
	CGU_HP_CFG_REGS hp[2];
} CGU_CONFIG_REGS;

/* Macro pointing to CGU configuration registers */
#define CGU_CFG	((CGU_CONFIG_REGS *)(CGU_BASE))

#endif /*__ASSEMBLY__*/

/**********************************************************************
* Register description of POWERMODE
**********************************************************************/
#define CGU_POWERMODE_MASK	0x3
#define CGU_POWERMODE_NORMAL	0x1
#define CGU_POWERMODE_WAKEUP	0x3

/**********************************************************************
* Register description of WD_BARK
**********************************************************************/
#define CGU_WD_BARK		0x1

/**********************************************************************
* Register description of FFAST_ON
**********************************************************************/
#define CGU_FFAST_ON		0x1

/**********************************************************************
* Register description of FFAST_BYPASS
**********************************************************************/
#define CGU_FFAST_BYPASS	0x1

/**********************************************************************
* Register description of soft reset registers
**********************************************************************/
#define CGU_CONFIG_SOFT_RESET	0x1

/**********************************************************************
* Register description of HPll REGISTERS
**********************************************************************/
#define CGU_HPLL0_ID		0
#define CGU_HPLL1_ID		1

/**********************************************************************
* Register description of HP_FIN_SELECT
**********************************************************************/
#define CGU_HPLL_FIN_SEL_MASK		0xf
#define CGU_FIN_SELECT_FFAST		0x0
#define CGU_FIN_SELECT_XT_I2SRX_BCK0	0x1
#define CGU_FIN_SELECT_XT_I2SRX_WS0	0x2
#define CGU_FIN_SELECT_XT_I2SRX_BCK1	0x3
#define CGU_FIN_SELECT_XT_I2SRX_WS1	0x4
#define CGU_FIN_SELECT_HPPLL0		0x5
#define CGU_FIN_SELECT_HPPLL1		0x6
#define CGU_FIN_SELECT_MAX		7

/**********************************************************************
* Register description of HP_MDEC
**********************************************************************/
#define CGU_HPLL_MDEC_MASK		0x1ffff
/**********************************************************************
* Register description of HP_NDEC
**********************************************************************/
#define CGU_HPLL_NDEC_MSK		0x3ff
/**********************************************************************
* Register description of HP_PDEC
**********************************************************************/
#define CGU_HPLL_PDEC_MSK		0x7f
/**********************************************************************
* Register description of HP_MODE
**********************************************************************/
#define CGU_HPLL_MODE_POR_VAL		0x6
#define CGU_HPLL_MODE_CLKEN		_BIT(0)
#define CGU_HPLL_MODE_SKEWEN		_BIT(1)
#define CGU_HPLL_MODE_PD		_BIT(2)
#define CGU_HPLL_MODE_DIRECTO		_BIT(3)
#define CGU_HPLL_MODE_DIRECTI		_BIT(4)
#define CGU_HPLL_MODE_FRM		_BIT(5)
#define CGU_HPLL_MODE_BANDSEL		_BIT(6)
#define CGU_HPLL_MODE_LIMUP_OFF		_BIT(7)
#define CGU_HPLL_MODE_BYPASS		_BIT(8)

/**********************************************************************
* Register description of HP1_STATUS
**********************************************************************/
#define CGU_HPLL_STATUS_FR		_BIT(1)
#define CGU_HPLL_STATUS_LOCK		_BIT(0)

/**********************************************************************
* Register description of HP_ACK & HP_REQ
**********************************************************************/
#define CGU_HPLL_ACK_P			_BIT(2)
#define CGU_HPLL_ACK_N			_BIT(1)
#define CGU_HPLL_ACK_M			_BIT(0)

/**********************************************************************
* Register description of HP1_INSELR
**********************************************************************/
#define CGU_HPLL_INSELR_MASK		0xf
/**********************************************************************
* Register description of HP1_INSELI
**********************************************************************/
#define CGU_HPLL_INSELI_MASK		0x3f
/**********************************************************************
* Register description of HP1_INSELP
**********************************************************************/
#define CGU_HPLL_INSELP_MASK		0x1f
/**********************************************************************
* Register description of HP1_SELR
**********************************************************************/
#define CGU_HPLL_SELR_MASK		0xf
/**********************************************************************
* Register description of HP1_SELI
**********************************************************************/
#define CGU_HPLL_SELI_MASK		0x3f
/**********************************************************************
* Register description of HP1_SELP
**********************************************************************/
#define CGU_HPLL_SELP_MASK		0x1f

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_MAIN_H */

