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

#include <asm/arch/i2c.h>
#include <asm/arch/clock.h>

void i2c_init(u32 id, I2C_REGS_T *pI2cBase)
{
	u32 srcclk;

#ifdef CONFIG_EA3152
	if(pI2cBase == I2C1_CTRL) {
		cgu_clk_en_dis(CGU_SB_SYSCLK_O_ID, 1);
		cgu_clk_en_dis(CGU_SB_CLK_256FS_ID, 1);
	}
#endif
	cgu_clk_en_dis(id, 1);

	/* Get the I2C0/I2C1 controller base clock rate */
	srcclk = cgu_get_clk_freq(id);

	pI2cBase->ckh = (srcclk/100000) - 2;
	pI2cBase->ckl = (srcclk/100000) - 2;

	/* Issue soft reset of the block */
	pI2cBase->ctl = I2C_CTL_SOFT_RESET;
	udelay(10000);

	/*
	 *Change Controller's Slave address as on LPC313x platform
	 * UDA1380 codec has the same Slave Address which is 0x1A
	 */
	pI2cBase->adr = 0x06E;
}

void i2c_write(I2C_REGS_T *pI2cBase, u32 slave_addr, u32 reg_addr,
		u32 addr_len, u32 reg_value,u32 data_len)
{
	u32 status = 0;
	int i = 0;

	/* Write Device (Slave) Address with START condition */
	pI2cBase->fifo = (slave_addr | I2C_TXFF_START_CND);

	/* Write Device Register Address */
	for(i = (addr_len -1); i >= 0; i--)
		pI2cBase->fifo = ((reg_addr >> (i * 8)) & 0xFF);

	/* Write Device Register Data */
	for(i = (data_len -1); i >= 0; i--) {
		/* Generate STOP condition after last data */
		if(i == 0)
			pI2cBase->fifo = (((reg_value >> (i * 8)) & 0xFF) |
					I2C_TXFF_STOP_CND);
		else
			pI2cBase->fifo = ((reg_value >> (i * 8)) & 0xFF);
	}

	/* Wait untill NAK or X'fer Done is gernerated */
	while ((status & (I2C_STS_NAI | I2C_STS_TDI)) == 0) {
		status = pI2cBase->sts;
	}

	if (status & I2C_STS_TDI) {
		pI2cBase->sts = I2C_STS_TDI;
	}
}

void i2c_read(I2C_REGS_T *pI2cBase, u32 slave_addr, u32 reg_addr,
		u32 addr_len, u32 *pReg_value,u32 data_len)
{
	int i = 0;
	unsigned int status = 0;

	*pReg_value = 0;

	/* Write Device (Slave) Address with START condition and Write mode*/
	pI2cBase->fifo = (slave_addr | I2C_TXFF_START_CND);

	/* Write Device Register Address */
	for(i = (addr_len -1); i >= 0; i--)
		pI2cBase->fifo = ((reg_addr >> (i * 8)) & 0xFF);

	/* Write Device (Slave) Address with Re-START condition and Read mode*/
	pI2cBase->fifo = ((slave_addr | 0x1) | I2C_TXFF_START_CND);

	/* Write Dummy Data to get data in RX FIFO*/
	for(i = (data_len -1); i >= 0; i--) {
		/* Generate STOP condition after last data */
		if(i == 0)
			pI2cBase->fifo = (0xFF | I2C_TXFF_STOP_CND);
		else
			pI2cBase->fifo = 0xFF;
	}

	/* Wait untill NAK or X'fer Done is gernerated */
	while ((status & (I2C_STS_NAI | I2C_STS_TDI)) == 0) {
		status = pI2cBase->sts;
	}

	/* If X'fer is done successfully, read data from RX FIFO */
	if (status & I2C_STS_TDI) {

		pI2cBase->sts = I2C_STS_TDI;

		*pReg_value = (((pI2cBase->fifo) & 0xFF) << 24);
		*pReg_value |= (((pI2cBase->fifo) & 0xFF) << 16);
		*pReg_value |= (((pI2cBase->fifo) & 0xFF) << 8);
		*pReg_value |= ((pI2cBase->fifo) & 0xFF);
	}
}
