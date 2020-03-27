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

#ifndef CLOCK_H
#define CLOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "clock_main.h"
#include "clock_switchbox.h"

#define CGU_DEF_DIV_BY_0	0x00000000
#define CGU_DEF_DIV_BY_2	0x00040404
#define CGU_DEF_DIV_BY_3	0x00060404
#define CGU_DEF_DIV_BY_4	0x00060604
#define CGU_DEF_DIV_BY_6	0x00070604
#define CGU_DEF_DIV_BY_32	0x0007c7c4
#define CGU_DEF_DIV_BY_38	0x0007e4a4
#define CGU_DEF_DIV_BY_40	0x0007e4e4
#define CGU_DEF_DIV_BY_256	0x1fe0ff04

/* Define dividers for domains which have HPLL1 as 
 * their base frequency. These dividers will be 
 * different for PLL_180 and PLL_270 configurations.
 */
#ifdef CONFIG_PLL_270
#define PLL_FREQUENCY           270000000
/*(NS_TO_MPMCCLK(7812, PLLCLK)/16) | _BIT(12) = (270 * 7.812)/16 | _BIT(12)*/
#define SYSREG_TEST0_VAL	0x182
/* (10 + NS_TO_MPMCCLK(20, HCLK) + NS_TO_MPMCCLK(66, HCLK)) * (PLLCLK/HCLK)
 * = (10 + (20 * 90/1000) +  (66 * 90/1000)) * 3)
 * = (10 + 1.8 + 5.94) * 3
 * = 54
 */
#define SYSREG_TEST1_VAL	0x36

#define CGU_DEF_FDIV0_VAL	CGU_DEF_DIV_BY_3
#define CGU_DEF_FDIV1_VAL	CGU_DEF_DIV_BY_0
#define CGU_DEF_FDIV2_VAL	CGU_DEF_DIV_BY_3
#define CGU_DEF_FDIV3_VAL	CGU_DEF_DIV_BY_6
#define CGU_DEF_FDIV4_VAL	CGU_DEF_DIV_BY_6
#define CGU_DEF_FDIV5_VAL	CGU_DEF_DIV_BY_3
#define CGU_DEF_FDIV6_VAL	CGU_DEF_DIV_BY_3

#define CGU_DEF_FDIV11_VAL	CGU_DEF_DIV_BY_3
#define CGU_DEF_FDIV12_VAL	CGU_DEF_DIV_BY_40
#define CGU_DEF_FDIV13_VAL	CGU_DEF_DIV_BY_0

#define CGU_DEF_FDIV15_VAL	CGU_DEF_DIV_BY_6

#define CGU_DEF_FDIV23_VAL	CGU_DEF_DIV_BY_3

#else

#define PLL_FREQUENCY           180000000
/*(NS_TO_MPMCCLK(7812, PLLCLK)/16) | _BIT(12) = (180 * 7.812)/16 | _BIT(12)*/
#define SYSREG_TEST0_VAL	0x158
/* (10 + NS_TO_MPMCCLK(20, HCLK) + NS_TO_MPMCCLK(66, HCLK)) * (PLLCLK/HCLK)
 * = (10 + (20 * 90/1000) +  (66 * 90/1000)) * 2)
 * = (10 + 1.8 + 5.94) * 2
 * = 36
 */
#define SYSREG_TEST1_VAL	0x24

#define CGU_DEF_FDIV0_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV1_VAL	CGU_DEF_DIV_BY_0
#define CGU_DEF_FDIV2_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV3_VAL	CGU_DEF_DIV_BY_4
#define CGU_DEF_FDIV4_VAL	CGU_DEF_DIV_BY_4
#define CGU_DEF_FDIV5_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV6_VAL	CGU_DEF_DIV_BY_2

#define CGU_DEF_FDIV11_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV12_VAL	CGU_DEF_DIV_BY_40
#define CGU_DEF_FDIV13_VAL	CGU_DEF_DIV_BY_0

#define CGU_DEF_FDIV15_VAL	CGU_DEF_DIV_BY_2

#define CGU_DEF_FDIV23_VAL	CGU_DEF_DIV_BY_2

#endif
/* Define dividers for domains which don't have 
 * HPLL1 as their base frequency. These dividers
 * will be same in bot PLL_180 and PLL_270 configurations.
 */
#define CGU_DEF_FDIV7_VAL	CGU_DEF_DIV_BY_38
#define CGU_DEF_FDIV8_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV9_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV10_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV14_VAL	CGU_DEF_DIV_BY_2
#define CGU_DEF_FDIV16_VAL	CGU_DEF_DIV_BY_0
#define CGU_DEF_FDIV17_VAL	CGU_DEF_DIV_BY_256
#define CGU_DEF_FDIV18_VAL	CGU_DEF_DIV_BY_4
#define CGU_DEF_FDIV19_VAL	CGU_DEF_DIV_BY_0
#define CGU_DEF_FDIV20_VAL	CGU_DEF_DIV_BY_4
#define CGU_DEF_FDIV21_VAL	CGU_DEF_DIV_BY_32
#define CGU_DEF_FDIV22_VAL	CGU_DEF_DIV_BY_2

/* dynamic dividers */
#define CGU_DEF_DYNFDIV0_VAL    CGU_DEF_DIV_BY_256
#define CGU_DEF_DYNFDIV1_VAL    CGU_DEF_DIV_BY_0
#define CGU_DEF_DYNFDIV2_VAL    CGU_DEF_DIV_BY_0
#define CGU_DEF_DYNFDIV3_VAL    CGU_DEF_DIV_BY_0
#define CGU_DEF_DYNFDIV4_VAL    CGU_DEF_DIV_BY_0
#define CGU_DEF_DYNFDIV5_VAL    CGU_DEF_DIV_BY_0
#define CGU_DEF_DYNFDIV6_VAL    CGU_DEF_DIV_BY_0
/* dynamic divs configurations. All events. */
#define CGU_DEF_DYNFDIV0_CFG    0x0
#define CGU_DEF_DYNFDIV1_CFG    0x0
#define CGU_DEF_DYNFDIV2_CFG    0x0
#define CGU_DEF_DYNFDIV3_CFG    0x0
#define CGU_DEF_DYNFDIV4_CFG    0x0
#define CGU_DEF_DYNFDIV5_CFG    0x0
#define CGU_DEF_DYNFDIV6_CFG    0x0


#define CGU_INVALID_ID		0xFFFF

/* Following clocks are enabled after init.
CGU_DEF_CLKS_0_31 contains bits for clocks with id between 0 & 31
CGU_DEF_CLKS_32_63 contains bits for clocks with id between 32 & 63
CGU_DEF_CLKS_64_92 contains bits for clocks with id between 64 & 92
*/
#define CGU_DEF_CLKS_0_31	(_BIT(CGU_SB_APB0_CLK_ID) | _BIT(CGU_SB_APB1_CLK_ID) | \
				_BIT(CGU_SB_APB2_CLK_ID) | _BIT(CGU_SB_APB3_CLK_ID) |_BIT(CGU_SB_APB4_CLK_ID) | \
				_BIT(CGU_SB_AHB2INTC_CLK_ID) | _BIT(CGU_SB_AHB0_CLK_ID) | \
				_BIT(CGU_SB_ARM926_CORE_CLK_ID) | _BIT(CGU_SB_ARM926_BUSIF_CLK_ID) | \
				_BIT(CGU_SB_ARM926_RETIME_CLK_ID) | _BIT(CGU_SB_ISRAM0_CLK_ID) | \
				_BIT(CGU_SB_ISRAM1_CLK_ID) | _BIT(CGU_SB_ISROM_CLK_ID) | \
				_BIT(CGU_SB_INTC_CLK_ID) | _BIT(CGU_SB_AHB2APB0_ASYNC_PCLK_ID) | \
				_BIT(CGU_SB_MPMC_CFG_CLK_ID) | _BIT(CGU_SB_MPMC_CFG_CLK2_ID) | _BIT(CGU_SB_MPMC_CFG_CLK3_ID) | \
				_BIT(CGU_SB_EBI_CLK_ID) | \
				_BIT(CGU_SB_EVENT_ROUTER_PCLK_ID) | _BIT(CGU_SB_CLOCK_OUT_ID))

#define CGU_DEF_CLKS_32_63	(_BIT(CGU_SB_IOCONF_PCLK_ID - 32) | _BIT(CGU_SB_CGU_PCLK_ID - 32) | \
				_BIT(CGU_SB_SYSCREG_PCLK_ID - 32) | _BIT(CGU_SB_OTP_PCLK_ID - 32) | \
				_BIT(CGU_SB_TIMER0_PCLK_ID -32) | \
				_BIT(CGU_SB_AHB2APB1_ASYNC_PCLK_ID - 32) | _BIT(CGU_SB_AHB2APB2_ASYNC_PCLK_ID - 32) | \
				_BIT(CGU_SB_AHB2APB3_ASYNC_PCLK_ID - 32) | _BIT(CGU_SB_EDGE_DET_PCLK_ID- 32))

#define CGU_DEF_CLKS_64_92	(0)

/*
 * Following macros are used to define clocks
 * belonging to different sub-domains
 * with-in each domain.
 */
#define D0_BIT(clkid)	(1 << (clkid))
#define D1_BIT(clkid)	(1 << ((clkid) - CGU_AHB0APB0_FIRST))
#define D2_BIT(clkid)	(1 << ((clkid) - CGU_AHB0APB1_FIRST))
#define D3_BIT(clkid)	(1 << ((clkid) - CGU_AHB0APB2_FIRST))
#define D4_BIT(clkid)	(1 << ((clkid) - CGU_AHB0APB3_FIRST))
#define D5_BIT(clkid)	(1 << ((clkid) - CGU_PCM_FIRST))
#define D6_BIT(clkid)	(1 << ((clkid) - CGU_UART_FIRST))
#define D7_BIT(clkid)	(1 << ((clkid) - CGU_CLK1024FS_FIRST))
/* 8 & 9 have one clk per domain so no macros */
#define D10_BIT(clkid)	(1 << ((clkid) - CGU_SPI_FIRST))


/*
 * Group the clocks 0 - 29 belonging to SYS_BASE domain into 7 different sub-domains using
 * following macro. Clocks not defined in the macros will be sourced with SYS_BASE_CLK.
 */
/* define clocks belonging to subdomain DOMAIN0_DIV0 */
#define CGU_DEF_DOMAIN0_DIV0	(D0_BIT(CGU_SB_APB0_CLK_ID) | D0_BIT(CGU_SB_APB1_CLK_ID) | \
				D0_BIT(CGU_SB_APB2_CLK_ID) | D0_BIT(CGU_SB_APB3_CLK_ID) | \
				D0_BIT(CGU_SB_APB4_CLK_ID) | D0_BIT(CGU_SB_AHB2INTC_CLK_ID) | \
				D0_BIT(CGU_SB_AHB0_CLK_ID) | D0_BIT(CGU_SB_DMA_PCLK_ID) | \
				D0_BIT(CGU_SB_DMA_CLK_GATED_ID) | D0_BIT(CGU_SB_NANDFLASH_S0_CLK_ID) | \
				D0_BIT(CGU_SB_NANDFLASH_PCLK_ID) | D0_BIT(CGU_SB_ARM926_BUSIF_CLK_ID) | \
				D0_BIT(CGU_SB_SD_MMC_HCLK_ID) | D0_BIT(CGU_SB_USB_OTG_AHB_CLK_ID) | \
				D0_BIT(CGU_SB_ISRAM0_CLK_ID) | D0_BIT(CGU_SB_ISRAM1_CLK_ID) | \
				D0_BIT(CGU_SB_ISROM_CLK_ID) | D0_BIT(CGU_SB_MPMC_CFG_CLK_ID) | \
				D0_BIT(CGU_SB_MPMC_CFG_CLK2_ID) | D0_BIT(CGU_SB_INTC_CLK_ID))

/* define clocks belonging to subdomain DOMAIN0_DIV1 */
#define CGU_DEF_DOMAIN0_DIV1	(D0_BIT(CGU_SB_ARM926_CORE_CLK_ID))

/* define clocks belonging to subdomain DOMAIN0_DIV2 */
#define CGU_DEF_DOMAIN0_DIV2	(D0_BIT(CGU_SB_NANDFLASH_AES_CLK_ID) | \
				D0_BIT(CGU_SB_NANDFLASH_NAND_CLK_ID))

/* define clocks belonging to subdomain DOMAIN0_DIV3 */
#define CGU_DEF_DOMAIN0_DIV3	(D0_BIT(CGU_SB_NANDFLASH_ECC_CLK_ID))

/* define clocks belonging to subdomain DOMAIN0_DIV4 */
#define CGU_DEF_DOMAIN0_DIV4	(D0_BIT(CGU_SB_SD_MMC_CCLK_IN_ID))
/* define clocks belonging to subdomain DOMAIN0_DIV5 */
#define CGU_DEF_DOMAIN0_DIV5	(D0_BIT(CGU_SB_CLOCK_OUT_ID))
/* define clocks belonging to subdomain DOMAIN0_DIV6 */
#define CGU_DEF_DOMAIN0_DIV6	(D0_BIT(CGU_SB_EBI_CLK_ID))

/*
 * Group the clocks 30 - 39 belonging to AHB_APB0_BASE domain into 2 different sub-domains using
 * following macro. Clocks not defined in the macros will be sourced with AHB_APB0_BASE_CLK.
 */
/* define clocks belonging to subdomain DOMAIN1_DIV7 */
#define CGU_DEF_DOMAIN1_DIV7	(D1_BIT(CGU_SB_ADC_CLK_ID))
/* define clocks belonging to subdomain DOMAIN1_DIV8 */
#define CGU_DEF_DOMAIN1_DIV8	(D1_BIT(CGU_SB_AHB2APB0_ASYNC_PCLK_ID) | \
				D1_BIT(CGU_SB_EVENT_ROUTER_PCLK_ID) | D1_BIT(CGU_SB_ADC_PCLK_ID) | \
				D1_BIT(CGU_SB_WDOG_PCLK_ID) | D1_BIT(CGU_SB_IOCONF_PCLK_ID) | \
				D1_BIT(CGU_SB_CGU_PCLK_ID) | D1_BIT(CGU_SB_SYSCREG_PCLK_ID) | \
				D1_BIT(CGU_SB_RNG_PCLK_ID) | D1_BIT(CGU_SB_OTP_PCLK_ID))

/*
 * Group the clocks 40 - 49 belonging to AHB_APB1_BASE domain into 2 different sub-domains using
 * following macro. Clocks not defined in the macros will be sourced with AHB_APB1_BASE_CLK.
 */
/* define clocks belonging to subdomain DOMAIN2_DIV9 */
#define CGU_DEF_DOMAIN2_DIV9	(D2_BIT(CGU_SB_AHB2APB1_ASYNC_PCLK_ID) | \
				D2_BIT(CGU_SB_TIMER0_PCLK_ID) | D2_BIT(CGU_SB_TIMER1_PCLK_ID) | \
				D2_BIT(CGU_SB_TIMER2_PCLK_ID) | D2_BIT(CGU_SB_TIMER3_PCLK_ID) | \
				D2_BIT(CGU_SB_PWM_PCLK_ID) | D2_BIT(CGU_SB_PWM_PCLK_REGS_ID) | \
				D2_BIT(CGU_SB_I2C0_PCLK_ID) | D2_BIT(CGU_SB_I2C1_PCLK_ID))

/* define clocks belonging to subdomain DOMAIN2_DIV10 */
#define CGU_DEF_DOMAIN2_DIV10	(D2_BIT(CGU_SB_PWM_CLK_ID))

/*
 * Group the clocks 50 - 57 belonging to AHB_APB2_BASE domain into 3 different sub-domains using
 * following macro. Clocks not defined in the macros will be sourced with AHB_APB2_BASE_CLK.
 */
/* define clocks belonging to subdomain DOMAIN3_DIV11 */
#define CGU_DEF_DOMAIN3_DIV11	(D3_BIT(CGU_SB_AHB2APB2_ASYNC_PCLK_ID) | \
				D3_BIT(CGU_SB_PCM_PCLK_ID) | D3_BIT(CGU_SB_PCM_APB_PCLK_ID) | \
				D3_BIT(CGU_SB_UART_APB_CLK_ID) | D3_BIT(CGU_SB_LCD_PCLK_ID) | \
				D3_BIT(CGU_SB_SPI_PCLK_ID) | D3_BIT(CGU_SB_SPI_PCLK_GATED_ID))

/* define clocks belonging to subdomain DOMAIN3_DIV12 */
#define CGU_DEF_DOMAIN3_DIV12	(D3_BIT(CGU_SB_LCD_CLK_ID))
/* Currently no clocks are connected to this subdomain */
#define CGU_DEF_DOMAIN3_DIV13	(0)

/*
 * Group the clocks 58 - 70 belonging to AHB_APB3_BASE domain into a sub-domains using
 * following macro. Clocks not defined in the macros will be sourced wih AHB_APB3_BASE_CLK.
 */
#define CGU_DEF_DOMAIN4_DIV14	(D4_BIT(CGU_SB_AHB2APB3_ASYNC_PCLK_ID) | \
				D4_BIT(CGU_SB_I2S_CFG_PCLK_ID) | D4_BIT(CGU_SB_EDGE_DET_PCLK_ID) | \
				D4_BIT(CGU_SB_I2STX_FIFO_0_PCLK_ID) | D4_BIT(CGU_SB_I2STX_IF_0_PCLK_ID) | \
				D4_BIT(CGU_SB_I2STX_FIFO_1_PCLK_ID) | D4_BIT(CGU_SB_I2STX_IF_1_PCLK_ID) | \
				D4_BIT(CGU_SB_I2SRX_FIFO_0_PCLK_ID) | D4_BIT(CGU_SB_I2SRX_IF_0_PCLK_ID) | \
				D4_BIT(CGU_SB_I2SRX_FIFO_1_PCLK_ID) | D4_BIT(CGU_SB_I2SRX_IF_1_PCLK_ID))

/*
 * Define whether CGU_SB_PCM_CLK_IP_ID clock uses the FDC_15 fractional divider or not. If
 * the following macro is set 0 then CGU_SB_PCM_CLK_IP_ID clock is sourced with PCM_BASE_CLK.
 */
#define CGU_DEF_DOMAIN5_DIV15	(D5_BIT(CGU_SB_PCM_CLK_IP_ID))

/*
 * Define whether CGU_SB_UART_U_CLK_ID clock uses the FDC_16 fractional divider or not. If
 * the following macro is set 0 then CGU_SB_UART_U_CLK_ID clock is sourced with UART_BASE_CLK.
 */
#define CGU_DEF_DOMAIN6_DIV16	(0)

/*
 *Group the clocks 73 - 86 belonging to CLK1024FS_BASE domain into 6 different sub-domains using
 *following macro. Clocks not defined in the macros will be sourced with CLK1024FS_BASE_CLK.
 */
/* define clocks belonging to subdomain DOMAIN7_DIV17. This divider has 13 bit resolution
 *  *    for madd & msub values compared to other dividers which have 8 bit only.*/
#define CGU_DEF_DOMAIN7_DIV17	(D7_BIT(CGU_SB_I2S_EDGE_DETECT_CLK_ID) | \
				D7_BIT(CGU_SB_I2STX_WS0_ID) | D7_BIT(CGU_SB_I2STX_WS1_ID) | \
				D7_BIT(CGU_SB_I2SRX_WS0_ID) | D7_BIT(CGU_SB_I2SRX_WS1_ID))

/* define clocks belonging to subdomain DOMAIN7_DIV18 */
#define CGU_DEF_DOMAIN7_DIV18	(D7_BIT(CGU_SB_I2STX_BCK0_N_ID) | \
				D7_BIT(CGU_SB_I2STX_BCK1_N_ID))

/* define clocks belonging to subdomain DOMAIN7_DIV19 */
#define CGU_DEF_DOMAIN7_DIV19	(D7_BIT(CGU_SB_I2STX_CLK0_ID) | \
				D7_BIT(CGU_SB_CLK_256FS_ID))

/* define clocks belonging to subdomain DOMAIN7_DIV20 */
#define CGU_DEF_DOMAIN7_DIV20	(D7_BIT(CGU_SB_I2SRX_BCK0_N_ID) | \
				D7_BIT(CGU_SB_I2SRX_BCK1_N_ID))

/* define clocks belonging to subdomain DOMAIN7_DIV21 */
#define CGU_DEF_DOMAIN7_DIV21	(0)
/* define clocks belonging to subdomain DOMAIN7_DIV22 */
#define CGU_DEF_DOMAIN7_DIV22	(0)

/* I2SRX_BCK0_BASE and I2SRX_BCK1_BASE domains are directly connected. So, no entry exists here */

/*
 * Group the clocks 89 - 90 belonging to SPI_CLK_BASE domain into a sub-domain using
 * following macro. Clocks not defined in the macros will be sourced wih SPI_CLK_BASE_CLK.
 */
#define CGU_DEF_DOMAIN10_DIV23	(D10_BIT(CGU_SB_SPI_CLK_ID) | \
				D10_BIT(CGU_SB_SPI_CLK_GATED_ID))

#ifndef __ASSEMBLY__
/* CGU soft reset module ID enumerations */
typedef enum
{
	APB0_RST_SOFT,
	AHB2APB0_PNRES_SOFT,
	APB1_RST_SOFT,
	AHB2APB1_PNRES_SOFT,
	APB2_RESETN_SOFT,
	AHB2APB2_PNRES_SOFT,
	APB3_RESETN_SOFT,
	AHB2APB3_PNRES_SOFT,
	APB4_RESETN_SOFT,
	AHB2INTC_RESETN_SOFT,
	AHB0_RESETN_SOFT,
	EBI_RESETN_SOFT,
	PCM_PNRES_SOFT,
	PCM_RESET_N_SOFT,
	PCM_RESET_ASYNC_N_SOFT,
	TIMER0_PNRES_SOFT,
	TIMER1_PNRES_SOFT,
	TIMER2_PNRES_SOFT,
	TIMER3_PNRES_SOFT,
	ADC_PRESETN_SOFT,
	ADC_RESETN_ADC10BITS_SOFT,
	PWM_RESET_AN_SOFT,
	UART_SYS_RST_AN_SOFT,
	I2C0_PNRES_SOFT,
	I2C1_PNRES_SOFT,
	I2S_CFG_RST_N_SOFT,
	I2S_NSOF_RST_N_SOFT,
	EDGE_DET_RST_N_SOFT,
	I2STX_FIFO_0_RST_N_SOFT,
	I2STX_IF_0_RST_N_SOFT,
	I2STX_FIFO_1_RST_N_SOFT,
	I2STX_IF_1_RST_N_SOFT,
	I2SRX_FIFO_0_RST_N_SOFT,
	I2SRX_IF_0_RST_N_SOFT,
	I2SRX_FIFO_1_RST_N_SOFT,
	I2SRX_IF_1_RST_N_SOFT,

	LCD_INTERFACE_PNRES_SOFT = I2SRX_IF_1_RST_N_SOFT + 6,
	SPI_PNRES_APB_SOFT,
	SPI_PNRES_IP_SOFT,
	DMA_PNRES_SOFT,
	NANDFLASH_ECC_RESET_N_SOFT,
	NANDFLASH_AES_RESET_N_SOFT,
	NANDFLASH_NAND_RESET_N_SOFT,
	RNG_RESETN_SOFT,
	SD_MMC_PNRES_SOFT,
	SD_MMC_NRES_CCLK_IN_SOFT,
	USB_OTG_AHB_RST_N_SOFT,
	RED_CTL_RESET_N_SOFT,
	AHB_MPMC_HRESETN_SOFT,
	AHB_MPMC_REFRESH_RESETN_SOFT,
	INTC_RESETN_SOFT
} CGU_MOD_ID_T;

typedef struct
{
	ulong fin_select;
	ulong ndec;
	ulong mdec;
	ulong pdec;
	ulong selr;
	ulong seli;
	ulong selp;
	ulong mode;
	ulong freq;
} CGU_HPLL_SETUP_T;

typedef struct
{
	u32 id;
	ulong fin_sel;
	ulong clk_min;
	ulong clk_cnt;
	ulong fdiv_min;
	ulong fdiv_cnt;
} CGU_DOMAIN_CFG_T;


/* Reset all clocks to be sourced from FFAST. */
void cgu_reset_all_clks(void);

/* Initialize all clocks at startup using the defaults structure */
long cgu_init_clks(u32 fidv_val[], u32 fdiv_clks[]);

/* Change the base frequency for the requested domain */
void cgu_set_base_freq(u32 baseid, ulong fin_sel);

/* Return the current frequecy of the requested clock*/
ulong cgu_get_clk_freq(u32 clkid);

/* Configure the selected HPLL */
void cgu_hpll_config(u32 pllid, CGU_HPLL_SETUP_T *pllinfo);

/* Get selected HPLL ulong */
ulong cgu_hpll_status (u32 id);

/* Issue a software reset to the requested module */
void cgu_soft_reset_module(CGU_MOD_ID_T mod);

/* enable / disable external enabling of the requested clock in CGU */
void cgu_clk_set_exten(u32 clkid, ulong enable);

static __inline ulong cgu_get_watchdog_bark(void)
{
	return ((CGU_CFG->wd_bark & CGU_WD_BARK) == CGU_WD_BARK);
}

static __inline ulong cgu_get_ffast_on(void)
{
	return ((CGU_CFG->ffast_on & CGU_FFAST_ON) == CGU_FFAST_ON);
}

static __inline void cgu_set_ffast_on(ulong enable)
{
	if (enable) {
		CGU_CFG->ffast_on = CGU_FFAST_ON;
	}
	else {
		CGU_CFG->ffast_on = 0;
	}
}

static __inline void cgu_set_ffast_bypass(ulong enable)
{
	if (enable) {
		CGU_CFG->ffast_bypass = CGU_FFAST_BYPASS;
	}
	else {
		CGU_CFG->ffast_bypass = 0;
	}
}

/* enable / disable the requested clock in CGU */
static __inline void cgu_clk_en_dis(u32 clkid, ulong enable)
{
	if (enable) {
		CGU_SB->clk_pcr[clkid] |= CGU_SB_PCR_RUN;
	}
	else {
		CGU_SB->clk_pcr[clkid] &= ~CGU_SB_PCR_RUN;
	}

}

#endif /*__ASSEMBLY__*/
#ifdef __cplusplus
}
#endif

#endif /* CLOCK_H */
