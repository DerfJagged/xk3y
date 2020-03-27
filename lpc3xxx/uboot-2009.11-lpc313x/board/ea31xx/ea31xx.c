/*
* (C) Copyright 2007-2008
* Stelian Pop <stelian.pop@leadtechdesign.com>
* Lead Tech Design <www.leadtechdesign.com>
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston,
* MA 02111-1307 USA
*/

#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/spi.h>
#include <asm/arch/i2c.h>
#include <net.h>
#include <netdev.h>

/* Event router macros */
#define EVRT_VBUS_BANK                  3
#define EVRT_VBUS_BIT			(1 << 23)


#define PCA9532_SLAVE_ADDR		(0xC0)
#define PCA9532_USB_VBUS_PWR_REG	0x07
#define PCA9532_USB_VBUS_PWR_VAL	0x10

DECLARE_GLOBAL_DATA_PTR;

const SPI_SLAVE_CONFIG_T slavecfg =
{
	.sid = (SPI_SLAVE_ID_T)SPI_SLAVE1_ID,
	.databits = 8,
	.words = SPI_FIFO_DEPTH,
	.mode = SPI_MODE0,
	.cs_high = 0,
	.pp_delay =0 ,
	.inter_delay = 0,
	.clk = 1000000
};

extern int is_nand_init_done;
extern int is_spi_init_done;

#ifdef CONFIG_DRIVER_DM9000
int board_eth_init(bd_t *bis)
{
	return dm9000_initialize(bis);
}
#endif

int misc_init_r(void)
{
#ifdef CONFIG_DRIVER_DM9000
	unsigned char i = 0;
	unsigned char oft = 0;
	uchar env_enetaddr[6];
	char enetvar[32] = "ethaddr";
	unsigned int rnd_num = 0;
	char buf[32] = {0};
	/*
	* If "ethaddr" environment variable is 00:00:00:00:00:00,
	* generate a random MAC address using Random Number Gen Module.
	*/

	eth_getenv_enetaddr(enetvar, env_enetaddr);
	if((env_enetaddr[0] | env_enetaddr[1] | env_enetaddr[2] |
		env_enetaddr[3] | env_enetaddr[4] | env_enetaddr[5]) == 0x0) {

			/* Enable clock for Random Number Generator Module */
			cgu_clk_en_dis(CGU_SB_RNG_PCLK_ID, 1);
			rnd_num = *((volatile u32 *)(CIC_RNG_BASE));

			env_enetaddr[0] = 0x00;
			env_enetaddr[1] = 0x08;
			for (i = 2; i < 6; i++) {
				env_enetaddr[i] = ((rnd_num >> ((i - 2) * 8)) & 0xFF);
			}

			sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x",
				env_enetaddr[0], env_enetaddr[1],
				env_enetaddr[2], env_enetaddr[3],
				env_enetaddr[4], env_enetaddr[5]);
			setenv(enetvar, buf);
	}

	/* fill device MAC address registers */
	for (i = 0, oft = 0x10; i < 6; i++, oft++) {
		*(volatile u8 *)DM9000_IO = oft;
		*(volatile u8 *)DM9000_DATA = env_enetaddr[i];
	}

	for (i = 0, oft = 0x16; i < 8; i++, oft++) {
		*(volatile u8 *)DM9000_IO = oft;
		*(volatile u8 *)DM9000_DATA = 0xff;
	}
#endif
	return 0;
}

int board_init(void)
{
#ifdef CONFIG_EA3152
	gd->bd->bi_arch_number = MACH_TYPE_EA3152;
#else
	gd->bd->bi_arch_number = MACH_TYPE_EA313X;
#endif
	/* adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_CMD_NAND
	/* Initialize NAND device */
	if(is_nand_init_done == 0) {
		printf("\nNAND:  ");
		nand_init();
		is_nand_init_done = 1;
	}

	prepare_write_nand_params_bbt(1);
#endif

#ifdef CONFIG_HARD_SPI
	/* Inialize SPI NOR flash*/
	if(is_spi_init_done == 0) {
		spi_init();
		is_spi_init_done = 1;
	}
#endif

	/* Initialize I2C and also sets correct
	* slave addresses for I2C0/I2C1 Controller
	*/
	i2c_init(CGU_SB_I2C0_PCLK_ID,I2C0_CTRL);
#ifndef CONFIG_EA3152
	i2c_init(CGU_SB_I2C1_PCLK_ID,I2C1_CTRL);
#endif

	/* for VBUS monitoring */
	EVRT_ATR(EVRT_VBUS_BANK) &= ~EVRT_VBUS_BIT;
	EVRT_APR(EVRT_VBUS_BANK) |= EVRT_VBUS_BIT;
	//lpc31xx_enable_vbus(1);
	return 0;
}

#ifdef CONFIG_USB_EHCI_LPC313X
int lpc31xx_enable_vbus(int enable)
{
	static int init = 0;
	int ret = 0;
	printf("%s USB VBUS power..\n", (enable)?"Enabling":"Disabling");
	if (enable) {
		/* VBUS is wrong state */
		if (EVRT_RSR(EVRT_VBUS_BANK) & EVRT_VBUS_BIT) {
			/* level VBUS is already high. May be device
			* cable is connected to PC.
			*/
			if (init == 0) {
				printf("VBUS level is already high. Remove device cable"
				" connected to mini-B connector and PC\n");
				ret = -1;
			}
		} else {
			/* enable VBUS power */
			i2c_write(I2C0_CTRL, PCA9532_SLAVE_ADDR, PCA9532_USB_VBUS_PWR_REG, 
				1, PCA9532_USB_VBUS_PWR_VAL, 1);
			init = 1;
			while (!(EVRT_RSR(EVRT_VBUS_BANK) & EVRT_VBUS_BIT)) {
				udelay(1000);
			}
		}
	} else {
		/* disable VBUS power */
		i2c_write(I2C0_CTRL, PCA9532_SLAVE_ADDR, PCA9532_USB_VBUS_PWR_REG, 
			1, 0x00, 1);
		init = 0;
		while (EVRT_RSR(EVRT_VBUS_BANK) & EVRT_VBUS_BIT) {
			udelay(1000);
		}

	}

	return ret;
}
#endif

/***********************************************************************/
int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_SIZE;
	return 0;
}

int board_late_init(void)
{
	extern unsigned int development_mode;
	extern unsigned int chineselanguageonly;
	extern unsigned int revocount;
	
	char rev_s[3] = { '0' + revocount/10, '0' + revocount%10, 0 };
	
	if (!development_mode)
		setenv("console", "none");
	else
		setenv("bootdelay", "2");

	setenv("chinaversion", chineselanguageonly ? "1" : "0");
	setenv("revocount", revocount > 9 ? rev_s : &rev_s[1]);

	return 0;
}
