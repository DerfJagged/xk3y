/*
 * (C) Copyright 2007
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Configuation settings for the LPC313X board from Embedded Artists.
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

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * If we are developing, we might want to start u-boot from ram
 * so we MUST NOT initialize critical regs like mem-timing ...
 */
#undef CONFIG_SKIP_LOWLEVEL_INIT
#undef CONFIG_USE_IRQ			/* don't need them anymore */
#undef CONFIG_CMD_FLASH
#undef CFG_CLKS_IN_HZ			/* everything, incl board info, in Hz */

/*
 * SoC Configuration
 */
#define CONFIG_LPC313X		1		/* This is a ARM926EJS CPU */
#define CONFIG_ARM_THUMB	1		/* Use Thumb instruction set */
#define CONFIG_PLL_270		1		/* Use 270MHz Clk Freq */
#define XTAL_IN			12000000	/* Crystal clk freq */
#define CONFIG_SYS_HZ		(XTAL_IN / 256)	/* decrementer freq in Hz */
#define CONFIG_SYS_NO_FLASH			/* No NOR flash */
#define	CONFIG_WINBOND_RAM	1
/*#undef	CONFIG_WINBOND_RAM*/

/* LPC313x U-boot bootmode support.
 * The following defines effect which low-level boot mode
 * is supported to boot u-boot images.
 */
/*#define CONFIG_CPU_SPI_BOOT
#define CONFIG_CPU_NAND_BOOT*/
#define CONFIG_CPU_MMC_BOOT
/*#define CONFIG_CPU_USBDFU_BOOT*/

#define BOARD_LATE_INIT

/*
 * Memory Info
 */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM			0x30000000	/* SDRAM Bank #1 */
#define PHYS_SDRAM_SIZE		0x02000000	/* 32 MB SDRAM */

/*
 * U-Boot memory configuration
 */
#define CONFIG_STACKSIZE			(32 * 1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ		(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ		(4*1024)	/* FIQ stack */
#endif
#define CONFIG_SYS_MALLOC_LEN		(1 * 1024 * 1024)		/* No need for any malloc space, we steal from the end of the DRAM */
#define CONFIG_SYS_GBL_DATA_SIZE	128		/* size in bytes reserved for initial data */
#define CONFIG_SYS_MEMTEST_START	0x30000000	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END		0x31FFFFFF	/* 32MB of DRAM */

/*
 * Serial Driver Console
 */
#define CONFIG_SERIAL1			1	/* we use Serial line 1 */
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*
 * U-boot Boot configuration
 */
#define CONFIG_BOOTFILE			uImage
#define CONFIG_BOOTDELAY		0
#define CONFIG_EXTRA_ENV_SETTINGS	\
"loadaddr=0x30007fc0\0" \
"console=ttyS0,115200n8\0" \
"verify=yes\0" \
"enc=1\0" \
"maxsize=0x300000\0" \
"bootenv=setenv bootargs lpj=270000 console=$(console) langrestrict=$(chinaversion) revocount=$(revocount)\0" \
"ext2ld=ext2load mmc 0:3 $(loadaddr) $(bootfile) $(maxsize) $(enc)\0" \
"fatld=fatload mmc 0:1 $(loadaddr) $(bootfile) $(maxsize) $(enc)\0" \
"bootcmd=run bootenv; mmc init; run fatld; run ext2ld; bootm\0" \
""

/*
 * Linux interfacing
 */
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_SYS_MAXARGS		16			/* max number of command args	*/
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size	*/

/*
 * U-boot geneal configurable
 */
#define CONFIG_SYS_LONGHELP				/* undef to save memory */
#ifndef CONFIG_EA3152
#define CONFIG_SYS_PROMPT		"EA3131-NXP # "	/* Monitor Command Prompt */
#else
#define CONFIG_SYS_PROMPT		"EA3152-NXP # "	/* Monitor Command Prompt */
#endif

#define CONFIG_SYS_CBSIZE		256		/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16) 
#define CONFIG_SYS_LOAD_ADDR		11029000
#define CONFIG_ENV_OVERWRITE		/* allow to overwrite serial and ethaddr */
#undef CONFIG_AUTO_COMPLETE
#define CONFIG_CMDLINE_EDITING

/*#include <config_cmd_default.h>*/

/*
 * U-boot Generic Command line configuration.
 */
#define CONFIG_CMD_CONSOLE	/* coninfo */
#define CONFIG_CMD_ECHO		/* echo arguments */
#define CONFIG_CMD_IMI		/* iminfo */
#define CONFIG_CMD_MISC		/* Misc functions like sleep etc*/
#define CONFIG_CMD_RUN		/* run command in env variable */
#define CONFIG_CMD_XIMG		/* Load part of Multi Image */
#undef CONFIG_CMD_MEMORY	/* Use for Memory*/
#define CONFIG_MISC_INIT_R

#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS

/*
 * Flash (NAND,SPINOR) configuration
 */

#define CONFIG_ENV_IS_NOWHERE	1
#define CONFIG_ENV_SIZE			0x400		/* Total Size of Environment Sector */

/* Uncomment if you want USB host support and
 * disable USB gadget supoprt. Please note if
 * you disable USB gadget support, USB DFU boot
 * mode will not work.
 */
/*
 * USB configuration as EHCI HOST
 */

/*
 * MMC Driver configuration
 */
#define CONFIG_CMD_MMC			1
#define CONFIG_MMC				1

#define CONFIG_CMD_FAT			1
#define CONFIG_CMD_EXT2			1
#define CONFIG_DOS_PARTITION
#undef CONFIG_ISO_PARTITION

#undef ANTICLONE
#define REVOCATION

/*
 * Factory tooling mode
 */
#undef FACTORY
#undef FACTORY_DEVELOPMENT_BUILD

#undef FACTORY_CHINA_ONLY

#endif /* __CONFIG_H */
