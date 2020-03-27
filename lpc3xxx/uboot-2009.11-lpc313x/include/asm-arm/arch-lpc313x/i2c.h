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


#ifndef I2C_H
#define I2C_H

#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>

#if defined(__cplusplus)
extern "C"
{
#endif

/* I2C Master Slave interface Controller Module Register Structure */
typedef volatile struct
{
	volatile ulong fifo;
	volatile ulong sts;
	volatile ulong ctl;
	volatile ulong ckh;
	volatile ulong ckl;
	volatile ulong adr;
	volatile const ulong rfl;
	volatile const ulong tfl;
	volatile const ulong rxb;
	volatile const ulong txb;
	volatile ulong txs;
	volatile const ulong stfl;
} I2C_REGS_T;

/***********************************************************************
 * I2C Tx FIFO register definitions
 **********************************************************************/
#define I2C_TXFF_STOP_CND	_BIT(9)
#define I2C_TXFF_START_CND	_BIT(8)
#define I2C_TXFF_DATA_MSK	0xFF

/***********************************************************************
 * I2C status register definitions
 **********************************************************************/
#define I2C_STS_TFES		_BIT(13)
#define I2C_STS_TFFS		_BIT(12)
#define I2C_STS_TFE		_BIT(11)
#define I2C_STS_TFF		_BIT(10)
#define I2C_STS_RFE		_BIT(9)
#define I2C_STS_RFF		_BIT(8)
#define I2C_STS_SDA		_BIT(7)
#define I2C_STS_SCL		_BIT(6)
#define I2C_STS_ACTIVE		_BIT(5)
#define I2C_STS_DRSI		_BIT(4)
#define I2C_STS_DRMI		_BIT(3)
#define I2C_STS_NAI		_BIT(2)
#define I2C_STS_AFI		_BIT(1)
#define I2C_STS_TDI		_BIT(0)

/***********************************************************************
 * I2C control register definitions
 **********************************************************************/
#define I2C_CTL_TFFSIE		_BIT(10)
#define I2C_CTL_SEVEN		_BIT(9)
#define I2C_CTL_SOFT_RESET	_BIT(8)
#define I2C_CTL_TFFIE		_BIT(7)
#define I2C_CTL_DAIE		_BIT(6)
#define I2C_CTL_RFFIE		_BIT(5)
#define I2C_CTL_DRSIE		_BIT(4)
#define I2C_CTL_DRMIE		_BIT(3)
#define I2C_CTL_NAIE		_BIT(2)
#define I2C_CTL_AFIE		_BIT(1)
#define I2C_CTL_TDIE		_BIT(0)

/* Macro pointing to I2C controller registers */
#define I2C0_CTRL	((I2C_REGS_T *)(I2C0_BASE))
#define I2C1_CTRL	((I2C_REGS_T *)(I2C1_BASE))

void i2c_init(u32 id, I2C_REGS_T *pI2cBase);
void i2c_write(I2C_REGS_T *pI2cBase, u32 slave_addr, u32 reg_addr,
		u32 addr_len, u32 reg_value,u32 data_len);

void i2c_read(I2C_REGS_T *pI2cBase, u32 slave_addr, u32 reg_addr,
		u32 addr_len, u32 *pReg_value,u32 data_len);

#if defined(__cplusplus)
}
#endif

#endif /* I2C_H */
