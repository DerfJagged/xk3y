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

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <config.h>

#define _BIT(n)		(((ulong)(1)) << (n))

#undef _SBF
/* Set bit field macro */
#define _SBF(f,v)	(((ulong)(v)) << (f))
#define _BITMASK(field_width)	(_BIT(field_width) - 1)

#define _NO_ERROR	(long)(0)
#define _ERROR		(long)(-1)

#define ISRAM_BASE			0x11028000
#define ISRAM_ESRAM0_BASE		0x11028000
#define ISRAM_ESRAM1_BASE		0x11040000
#define ISROM_BASE			0x12000000
#define EVENT_ROUTER_BASE		0x13000000
#define ADC_BASE			0x13002000
#define WDOG_BASE			0x13002400
#define SYSCREG_BASE			0x13002800
#define IOCONF_BASE			0x13003000
#define CGU_SWITCHBOX_BASE		0x13004000
#define CGU_BASE			0x13004c00
#define CIC_RNG_BASE			0x13006000
#define TIMER0_BASE			0x13008000
#define TIMER1_BASE			0x13008400
#define TIMER2_BASE			0x13008800
#define TIMER3_BASE			0x13008c00
#define PWM_BASE			0x13009000
#define I2C0_BASE			0x1300a000
#define I2C1_BASE			0x1300a400
#define IPINT_BASE			0x15000000
#define LCD_INTERFACE_BASE		0x15000400
#define UART_BASE			0x15001000
#define SPI_BASE			0x15002000
#define I2S_SYS_CONFIG			0x16000000
#define I2S_TX0_BASE			0x16000080
#define I2S_TX1_BASE			0x16000100
#define I2S_RX0_BASE			0x16000180
#define I2S_RX1_BASE			0x16000200
#define DMA_BASE			0x17000000
#define NANDFLASH_CTRL_CFG_BASE		0x17000800
#define MPMC_CFG_BASE			0x17008000
#define SD_MMC_BASE			0x18000000
#define USBOTG_BASE			0x19000000
#define EXT_SRAM0_0_BASE		0x20000000
#define EXT_SRAM0_1_BASE		0x20020000
#define EXT_SDRAM_BASE			0x30000000
#define AHB2MMIO_BASE			0x60000000
#define INTC_BASE			0x60000000
#define NANDFLASH_CTRL_S0_BASE		0x70000000

#ifndef __ASSEMBLY__
#define __REG(x)	(*((volatile u32 *)(x)))
#else
#define __REG(x)	(x)
#endif

/***********************************************************************
 * Event router register definitions
 **********************************************************************/
#define EVRT_INT_PEND(bank)  __REG (EVENT_ROUTER_BASE + 0xC00 + ((bank) << 2))
#define EVRT_INT_CLR(bank)   __REG (EVENT_ROUTER_BASE + 0xC20 + ((bank) << 2))
#define EVRT_INT_SET(bank)   __REG (EVENT_ROUTER_BASE + 0xC40 + ((bank) << 2))
#define EVRT_MASK(bank)      __REG (EVENT_ROUTER_BASE + 0xC60 + ((bank) << 2))
#define EVRT_MASK_CLR(bank)  __REG (EVENT_ROUTER_BASE + 0xC80 + ((bank) << 2))
#define EVRT_MASK_SET(bank)  __REG (EVENT_ROUTER_BASE + 0xCA0 + ((bank) << 2))
#define EVRT_APR(bank)       __REG (EVENT_ROUTER_BASE + 0xCC0 + ((bank) << 2))
#define EVRT_ATR(bank)       __REG (EVENT_ROUTER_BASE + 0xCE0 + ((bank) << 2))
#define EVRT_RSR(bank)       __REG (EVENT_ROUTER_BASE + 0xD20 + ((bank) << 2))
#define EVRT_OUT_PEND(vec,bank)     __REG (EVENT_ROUTER_BASE + 0x1000 + ((vec) << 5) + ((bank) << 2))
#define EVRT_OUT_MASK(vec,bank)     __REG (EVENT_ROUTER_BASE + 0x1400 + ((vec) << 5) + ((bank) << 2))
#define EVRT_OUT_MASK_CLR(vec,bank) __REG (EVENT_ROUTER_BASE + 0x1800 + ((vec) << 5) + ((bank) << 2))
#define EVRT_OUT_MASK_SET(vec,bank) __REG (EVENT_ROUTER_BASE + 0x1C00 + ((vec) << 5) + ((bank) << 2))

#endif /* ASM_ARCH_HARDWARE_H */

