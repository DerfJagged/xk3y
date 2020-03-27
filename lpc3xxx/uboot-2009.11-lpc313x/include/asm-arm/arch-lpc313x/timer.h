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

#ifndef TIMER_H
#define TIMER_H

#include <common.h>
#include <asm/arch/hardware.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Timer Module Register Structure */
typedef volatile struct
{
	volatile ulong load;
	volatile const ulong value;
	volatile ulong control;
	volatile ulong clear;
	volatile ulong test;
} TIMER_REGS_T;

/**********************************************************************
*  Timer Control Register (TimerCtrl) (0x08) Read/Write
**********************************************************************/
#define TM_CTRL_ENABLE		_BIT(7)
#define TM_CTRL_MODE		_BIT(6)
#define TM_CTRL_PERIODIC	_BIT(6)
#define TM_CTRL_PS1		_SBF(2, 0)
#define TM_CTRL_PS16		_SBF(2, 1)
#define TM_CTRL_PS256		_SBF(2, 2)
#define TM_CTRL_PS_MASK		_SBF(2, 0x3)

/**********************************************************************
* Macro to access TIMER registers
**********************************************************************/
#define TIMER_CNTR0		((TIMER_REGS_T*)TIMER0_BASE)
#define TIMER_CNTR1		((TIMER_REGS_T*)TIMER1_BASE)
#define TIMER_CNTR2		((TIMER_REGS_T*)TIMER2_BASE)
#define TIMER_CNTR3		((TIMER_REGS_T*)TIMER3_BASE)

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H */
