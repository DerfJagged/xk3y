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

#include <common.h>
#include <nand.h>
#include <spi_flash.h>
#include <mmc.h>
#include <malloc.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/sysreg.h>
#include <asm/arch/ioconf.h>
#include <asm/arch/mci.h>
#include <asm/arch/timer.h>
#include <asm/arch/i2c.h>
#include <asm/arch/nand.h>

#include "dos_partition.h"
#include "otp.h"
#include "revocount.h"
#include "anticlone.h"
#include "emu_key.h"
#include "kernel_key.h"

typedef enum
{
	LPC313X_BOOT_MODE_NAND = 0,
	LPC313X_BOOT_MODE_SPI,
	LPC313X_BOOT_MODE_DFU_USB,
	LPC313X_BOOT_MODE_SD_MMC,
	LPC313X_BOOT_MODE_RESERVED,
	LPC313X_BOOT_MODE_NOR_FLASH,
	LPC313X_BOOT_MODE_UART,
	LPC313X_BOOT_MODE_TEST,
} LPC313X_BOOT_MODE_T;

/* LPC313x macros */
#define LPC313X_BOOTMODE_MASK		0x00000007
#define UART_DOWNLOAD_TIMEOUT_END_US	10000
#define UART_DOWNLOAD_TIMEOUT_START_US	1000000
#define READ_BLOCK_SIZE			(512)

#ifdef CONFIG_EA3152
#define AD_SLAVE_ADDR			(0x18)
#define AD_REG_OTGDCLIC_RW		0x0000
#define ANALOG_1V8_RAIL			2
/* 1v4 io voltage rail adjustments */
#define PSU_VOUT3_1_80			_BIT(16)
#endif

/* Memory macros */
#define LPC313X_ISRAM_BASE_ADD		0x11029000
#define LPC313X_SDRAM_TEMP_DATA_ADDR	0x32000000

/* SPI NOR flash macros */
#define LPC313X_SPI_NOR_UBOOT_SECTOR	0x0
#define LPC313X_SPI_NOR_SECTOR_SIZE	0x40000

/* SD/MMC macros */
#define LBA_STEP_MCI			32
/* based on LBA_STEP_MCI define we will search first 32MB of the partition only */
#define MAX_IMAGE_SEARCH_NUM_OF_BLOCKS	2048

/* USB macros */
#define EVT_usb_pll_bank		3
#define EVT_usb_atx_pll_lock_bit	_BIT(25)

extern long __INITIAL_DATA_END;
extern long __initial_boot_image_size;

int is_nand_init_done = 0;
int is_spi_init_done = 0;

#ifdef CONFIG_CPU_USBDFU_BOOT
extern unsigned char stop_polling;
#endif

DECLARE_GLOBAL_DATA_PTR;

static int timer_expired(ulong timeStart, ulong timeDelta)
{
	ulong currTime = 0;

	currTime = get_timer(0);
	if (((currTime - timeStart) > timeDelta))
		return 1;

	return 0;
}

void GPIOMuxSetup(void)
{
	gpio_set_outpin_high(IOCONF_GPIO, 2);
}

#if defined(CONFIG_USB_EHCI_LPC313X) || defined(CONFIG_USB_DEVICE)
void usb_init_clocks(void)
{

	/* enable USB to AHB clock */
	cgu_clk_en_dis(CGU_SB_USB_OTG_AHB_CLK_ID, 1);
	/* enable clock to Event router */
	cgu_clk_en_dis(CGU_SB_EVENT_ROUTER_PCLK_ID, 1);

	/* reset USB block */
	cgu_soft_reset_module(USB_OTG_AHB_RST_N_SOFT);

	if(SYS_REGS->usb_atx_pll_pd_reg != 0) {
		/* enable USB OTG PLL */
		SYS_REGS->usb_atx_pll_pd_reg = 0x0;
		/* wait for PLL to lock */
		while (!(EVRT_RSR(EVT_usb_pll_bank) & EVT_usb_atx_pll_lock_bit));
	}

	return;
}
#endif

#ifdef CONFIG_EA3152
void psu_set_voltage(u32 rail, u32 volt)
{
	u32 reg_val;
	int bit_pos;

	/* read PSU register */
	i2c_read(I2C1_CTRL, AD_SLAVE_ADDR, AD_REG_OTGDCLIC_RW,
			2, &reg_val,4);

	/* check if this is to set VOUT3 rail */
	if (ANALOG_1V8_RAIL == rail) {
		reg_val |= (volt)?_BIT(16):0;
	} else {
		/* for 1v2 bitpos is 17 and for 3v3 bit pos 20 */
		bit_pos = (rail)? 17 : 20;

		if (volt > 0x7)
			volt = 0x7;

		/* zero the dcdc1 adjust bits */
		reg_val &= ~(0x7 << bit_pos);
		/* write the new adjust value */
		reg_val |= (volt << bit_pos);
	}

	i2c_write(I2C1_CTRL, AD_SLAVE_ADDR, AD_REG_OTGDCLIC_RW,
			2, reg_val,4);
	return;
}

void setup_sdram_voltage(void)
{
	unsigned int reg_val = 0;

	i2c_init(CGU_SB_I2C1_PCLK_ID,I2C1_CTRL);

	/* Set SDRAM voltage rail to 1.8V default is 1.4V */
	psu_set_voltage(ANALOG_1V8_RAIL, PSU_VOUT3_1_80);
}
#endif

static void bin2hex(char *out, char *in, int len)
{
	static char const tohex[] = "0123456789ABCDEF";
	
	while (len--)
	{
		*out++ = tohex[ *in >> 4 ];
		*out++ = tohex[ *in & 0xf ];

		in++;
	}
	*out = 0;
}

unsigned int bootmode; 
unsigned int development_mode;
unsigned int chineselanguageonly;
unsigned int revocount;

extern unsigned char SRAM_EMUKEY[];

int lpc313x_init(void)
{
	long uartid = 0;
	int i, rc = 0;

	char const serial[ 1 + 2*sizeof(OTP.serial) ];
	
	gd = (gd_t*)(_armboot_start - CONFIG_SYS_MALLOC_LEN - sizeof(gd_t));

	mem_malloc_init (_armboot_start - CONFIG_SYS_MALLOC_LEN,
			CONFIG_SYS_MALLOC_LEN);

	GPIOMuxSetup();

	/* Initialize Timer0 */
	timer_init();

	/* Initialize UART0 */
	gd->baudrate = CONFIG_BAUDRATE;
	gd->flags = 0;
	uartid = serial_init();
	gd->have_console = 1;

#if defined(CONFIG_USB_EHCI_LPC313X) || defined(CONFIG_USB_DEVICE)
	usb_init_clocks();
#endif

#ifdef CONFIG_CPU_NAND_BOOT
	/* Initialize NAND device */
	printf("\nNAND:  ");
	nand_init();
	is_nand_init_done = 1;
#endif

#ifdef CONFIG_CPU_SPI_BOOT
	/* Inialize SPI NOR flah*/
	spi_init();
	is_spi_init_done = 1;
#endif

#ifdef CONFIG_CPU_MMC_BOOT
	/* Initialize MMC */
	mmc_legacy_init(0);
#endif

#ifdef CONFIG_CPU_USBDFU_BOOT
	drv_usbtty_init();
#endif
	
	/* Read Boot mode pins */
	IOCONF->block[IOCONF_GPIO].mode0_clear = LPC313X_BOOTMODE_MASK;
	IOCONF->block[IOCONF_GPIO].mode1_clear = LPC313X_BOOTMODE_MASK;

	udelay(1000);

	bootmode = IOCONF->block[IOCONF_GPIO].pins;
	bootmode = ((bootmode & 0x3) << 1) | ((bootmode & 0x4) >> 2);

	otp_init();
	
	/* find the most signficant bit set */
	revocount = 32 - __builtin_clz(OTP.revocount);

	bin2hex(serial, OTP.serial, sizeof(OTP.serial));

	printf(	"\n\n"

		"Boot mode    : %s\n" 
		"Chip type    : %s\n"
		"Device class : %s\n"
		"China Flag   : %s\n"
		"Serial#      : %s\n"
		"H/W version  : %d\n"
		"Revocation#  : %d\n",

		bootmode == LPC313X_BOOT_MODE_UART ? "UART" : "SD",

		OTP.chip_id == 0x0B ? "LPC 3143" :
			OTP.chip_id == 0x0E ? "LPC 3141" : "(unknown)",

		OTP.devclass == OTP_DEVCLASS_DEVELOPMENT ? "DEVELOPMENT" :
			OTP.devclass == OTP_DEVCLASS_PRODUCTION ? "PRODUCTION" : "INVALID" ,

		OTP.chineselanguageonly == OTP_CHINESE_LANG_ONLY ? "CHINA" : "NON-CHINA",

		serial,
		OTP.hwversion,
		revocount);

#ifdef FACTORY
	factory_program_otp();
	printf("\nFUSES PROGRAMMED\n");
	while (1);
#else
	
	/* init the AES engine */
	cgu_clk_en_dis(CGU_SB_NANDFLASH_S0_CLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_NAND_CLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_PCLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_AES_CLK_ID, 1);
   	
	cgu_soft_reset_module(NANDFLASH_NAND_RESET_N_SOFT);
	cgu_soft_reset_module(NANDFLASH_AES_RESET_N_SOFT);

#ifdef ANTICLONE
	if (check_anticlone(OTP.serial, OTP.acdata))
	{
		printf("\nUNAUTHORIZED HARDWARE DETECTED\n");
		while (1);
	}
#endif

	/*
	 * The read protect on the OTP results in bits coming back
	 * as zeroes. This means zero values cannot be trusted, and 
	 * all meaningful values should contain at least a 1 bit.
	 */
	if ((OTP.revocount == 0 || OTP.devclass == 0))
	{
		printf("\nOTP corrupt or not programmed yet...\n");
		while (1);
	}
	
	if (revocount > MAX_REVO_COUNT)
	{
		printf("\nPlease upgrade your bootloader...\n");
		while (OTP.devclass != OTP_DEVCLASS_DEVELOPMENT);
	}
#ifdef REVOCATION
	else if (revocount < MAX_REVO_COUNT)
	{
		printf("\nUpdating revocation count to 0x%x\n", MAX_REVO_COUNT);
		otp_update_revocount();
	}
#endif

	/* write protect all rows */
	OTP.wprot = 0xFFFF | (1 << 31);
	
	development_mode = (OTP.devclass == OTP_DEVCLASS_DEVELOPMENT);

	chineselanguageonly = (OTP.chineselanguageonly == OTP_CHINESE_LANG_ONLY);

	/* put the EMU key at the top of SRAM */
	memcpy(SRAM_EMUKEY, emukey, sizeof(emukey));
	
	for (i = 0; i < 4; i++) {
		NAND_CTRL->aes_key[i] = kernelkey[i];
		kernelkey[i] = 0;
	}

	start_armboot();
#endif

	return 0;
}
