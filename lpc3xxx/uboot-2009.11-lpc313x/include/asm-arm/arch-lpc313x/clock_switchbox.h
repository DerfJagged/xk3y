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

#ifndef CLOCK_SWITCHBOX_H
#define CLOCK_SWITCHBOX_H

#include <common.h>
#include <asm/arch/hardware.h>

#ifdef __cplusplus
extern "C"	/* Assume C declarations for C++ */
{
#endif

#ifndef __ASSEMBLY__
/***********************************************************************
* CGU Switchbox register structure
**********************************************************************/
typedef volatile struct
{
	/* Switches controls */
	volatile ulong base_scr[12]; /* Switch control */
	volatile ulong base_fs1[12]; /* Frequency select side 1 */
	volatile ulong base_fs2[12]; /* Frequency select side 2 */
	volatile ulong base_ssr[12]; /* Switch status */
	/* Clock enable controls (positive and inverted clock pairs share
	 * control register)
	 */
	volatile ulong clk_pcr[92]; /* power control */
	volatile ulong clk_psr[92]; /* power status */
	/* enable select from fractional dividers (positive and inverted
	 * clock pairs share esr)
	 */
	volatile ulong clk_esr[89]; /* enable select */
	/* Base controls, currently only fd_run (base wide fractional
	 *  divider enable) bit.
	 */
	volatile ulong base_bcr[5]; /* Base control */
	/* Fractional divider config & ctrl */
	volatile ulong base_fdc[24]; 
	/* Fractional divider config & ctrl for dynamic fracdivs */
	volatile ulong base_dyn_fdc[7]; 
	 /* Fractional divider register for 
	  * selecting an external signal to
	  * trigger high-speed operation
	  */
	volatile ulong base_dyn_sel[7];
} CGU_SB_REGS_T;

/* Macro pointing to CGU switch box registers */
#define CGU_SB	((CGU_SB_REGS_T *)(CGU_SWITCHBOX_BASE))

#endif /*__ASSEMBLY__*/
#ifdef __cplusplus
}
#endif

#define CGU_SB_FSR_WIDTH	3
#define CGU_SB_NR_BASE		12
#define CGU_SB_NR_CLK		92
#define CGU_SB_NR_BCR		5
#define CGU_SB_NR_FRACDIV	24
#define CGU_SB_NR_DYN_FDIV	7
#define CGU_SB_NR_ESR		89


/***********************************************************************
 Register section offsets in APB address space
***********************************************************************/
#define SCR_OFFSET		0
#define FS1_OFFSET		12
#define FS2_OFFSET		24
#define SSR_OFFSET		36
#define PCR_OFFSET		48
#define PSR_OFFSET		140
#define ESR_OFFSET		232
#define BCR_OFFSET		321
#define FDC_OFFSET		326

/***********************************************************************
* Bit positions
***********************************************************************/
/* Switch Control Register */
#define CGU_SB_SCR_EN1			_BIT(0)
#define CGU_SB_SCR_EN2			_BIT(1)
#define CGU_SB_SCR_RST			_BIT(2)
#define CGU_SB_SCR_STOP			_BIT(3)
#define CGU_SB_SCR_FS_MASK		0x3

/* Switch Status Register */
#define CGU_SB_SSR_FS_GET(x)	(((x) >> 2) & 0x7)
/* Power Control Register */
#define CGU_SB_PCR_RUN			_BIT(0)
#define CGU_SB_PCR_AUTO			_BIT(1)
#define CGU_SB_PCR_WAKE_EN		_BIT(2)
#define CGU_SB_PCR_EXTEN_EN		_BIT(3)
#define CGU_SB_PCR_ENOUT_EN		_BIT(4)
/* Power Status Register */
#define CGU_SB_PSR_ACTIVE		_BIT(0)
#define CGU_SB_PSR_WAKEUP		_BIT(1)
/* Enable Select Register */
#define CGU_SB_ESR_ENABLE		_BIT(0)
#define CGU_SB_ESR_SELECT(x)		_SBF(1, (x))
#define CGU_SB_ESR_SEL_GET(x)		(((x) >> 1) & 0x7)

/* Base control Register */
#define CGU_SB_BCR_FD_RUN		_BIT(0)
/* Fractional Divider Configuration Register */
#define CGU_SB_FDC_RUN			_BIT(0)
#define CGU_SB_FDC_RESET		_BIT(1)
#define CGU_SB_FDC_STRETCH		_BIT(2)
#define CGU_SB_FDC_MADD(x)		_SBF( 3, ((x) & 0xFF))
#define CGU_SB_FDC_MSUB(x)		_SBF(11, ((x) & 0xFF))
#define CGU_SB_FDC17_MADD(x)		_SBF( 3, ((x) & 0x1FFF))
#define CGU_SB_FDC17_MSUB(x)		_SBF(16, ((x) & 0x1FFF))
#define CGU_SB_FDC_MADD_GET(x)		(((x) >> 3) & 0xFF)
#define CGU_SB_FDC_MSUB_GET(x)		((((x) >> 11) & 0xFF) | 0xFFFFFF00)
#define CGU_SB_FDC17_MADD_GET(x)	(((x) >> 3) & 0x1FFF)
#define CGU_SB_FDC17_MSUB_GET(x)	((((x) >> 16) & 0x1FFF) | 0xFFFFE000)
#define CGU_SB_FDC_MADD_POS		3

/* Dynamic Fractional Divider Configuration Register */
#define CGU_SB_DYN_FDC_RUN		_BIT(0)
#define CGU_SB_DYN_FDC_ALLOW		_BIT(1)
#define CGU_SB_DYN_FDC_STRETCH		_BIT(2)

/***********************************************************************
* Clock domain base id's
***********************************************************************/
#define CGU_SB_SYS_BASE_ID                0
#define CGU_SB_BASE_FIRST                 CGU_SB_SYS_BASE_ID
#define CGU_SB_AHB0_APB0_BASE_ID          1
#define CGU_SB_AHB0_APB1_BASE_ID          2
#define CGU_SB_AHB0_APB2_BASE_ID          3
#define CGU_SB_AHB0_APB3_BASE_ID          4
#define CGU_SB_PCM_BASE_ID                5
#define CGU_SB_UART_BASE_ID               6
#define CGU_SB_CLK1024FS_BASE_ID          7
#define CGU_SB_I2SRX_BCK0_BASE_ID         8
#define CGU_SB_I2SRX_BCK1_BASE_ID         9
#define CGU_SB_SPI_CLK_BASE_ID            10
#define CGU_SB_SYSCLK_O_BASE_ID           11
#define CGU_SB_BASE_LAST                  CGU_SB_SYSCLK_O_BASE_ID


/***********************************************************************
 Clock id's (= clkid in address calculation)
***********************************************************************/
/* domain 0 = SYS_BASE */
#define CGU_SB_APB0_CLK_ID                 0
#define CGU_SYS_FIRST                      CGU_SB_APB0_CLK_ID
#define CGU_SB_APB1_CLK_ID                 1
#define CGU_SB_APB2_CLK_ID                 2
#define CGU_SB_APB3_CLK_ID                 3
#define CGU_SB_APB4_CLK_ID                 4
#define CGU_SB_AHB2INTC_CLK_ID             5
#define CGU_SB_AHB0_CLK_ID                 6
#define CGU_SB_EBI_CLK_ID                  7
#define CGU_SB_DMA_PCLK_ID                 8
#define CGU_SB_DMA_CLK_GATED_ID            9
#define CGU_SB_NANDFLASH_S0_CLK_ID         10
#define CGU_SB_NANDFLASH_ECC_CLK_ID        11
#define CGU_SB_NANDFLASH_AES_CLK_ID        12 /* valid on LPC3153 & LPC3154 only */
#define CGU_SB_NANDFLASH_NAND_CLK_ID       13
#define CGU_SB_NANDFLASH_PCLK_ID           14
#define CGU_SB_CLOCK_OUT_ID                15
#define CGU_SB_ARM926_CORE_CLK_ID          16
#define CGU_SB_ARM926_BUSIF_CLK_ID         17
#define CGU_SB_ARM926_RETIME_CLK_ID        18
#define CGU_SB_SD_MMC_HCLK_ID              19
#define CGU_SB_SD_MMC_CCLK_IN_ID           20
#define CGU_SB_USB_OTG_AHB_CLK_ID          21
#define CGU_SB_ISRAM0_CLK_ID               22
#define CGU_SB_RED_CTL_RSCLK_ID            23
#define CGU_SB_ISRAM1_CLK_ID               24
#define CGU_SB_ISROM_CLK_ID                25
#define CGU_SB_MPMC_CFG_CLK_ID             26
#define CGU_SB_MPMC_CFG_CLK2_ID            27
#define CGU_SB_MPMC_CFG_CLK3_ID            28
#define CGU_SB_INTC_CLK_ID                 29
#define CGU_SYS_LAST                       CGU_SB_INTC_CLK_ID
/* domain 1 = AHB0APB0_BASE */
#define CGU_SB_AHB2APB0_ASYNC_PCLK_ID      30
#define CGU_AHB0APB0_FIRST                 CGU_SB_AHB2APB0_ASYNC_PCLK_ID
#define CGU_SB_EVENT_ROUTER_PCLK_ID        31
#define CGU_SB_ADC_PCLK_ID                 32
#define CGU_SB_ADC_CLK_ID                  33
#define CGU_SB_WDOG_PCLK_ID                34
#define CGU_SB_IOCONF_PCLK_ID              35
#define CGU_SB_CGU_PCLK_ID                 36
#define CGU_SB_SYSCREG_PCLK_ID             37
#define CGU_SB_OTP_PCLK_ID                 38 /* valid on LPC315x series only */
#define CGU_SB_RNG_PCLK_ID                 39
#define CGU_AHB0APB0_LAST                  CGU_SB_RNG_PCLK_ID
/* domain 2 = AHB0APB1_BASE */
#define CGU_SB_AHB2APB1_ASYNC_PCLK_ID      40
#define CGU_AHB0APB1_FIRST                 CGU_SB_AHB2APB1_ASYNC_PCLK_ID
#define CGU_SB_TIMER0_PCLK_ID              41
#define CGU_SB_TIMER1_PCLK_ID              42
#define CGU_SB_TIMER2_PCLK_ID              43
#define CGU_SB_TIMER3_PCLK_ID              44
#define CGU_SB_PWM_PCLK_ID                 45
#define CGU_SB_PWM_PCLK_REGS_ID            46
#define CGU_SB_PWM_CLK_ID                  47
#define CGU_SB_I2C0_PCLK_ID                48
#define CGU_SB_I2C1_PCLK_ID                49
#define CGU_AHB0APB1_LAST                  CGU_SB_I2C1_PCLK_ID
/* domain 3 = AHB0APB2_BASE */
#define CGU_SB_AHB2APB2_ASYNC_PCLK_ID      50
#define CGU_AHB0APB2_FIRST                 CGU_SB_AHB2APB2_ASYNC_PCLK_ID
#define CGU_SB_PCM_PCLK_ID                 51
#define CGU_SB_PCM_APB_PCLK_ID             52
#define CGU_SB_UART_APB_CLK_ID             53
#define CGU_SB_LCD_PCLK_ID                 54
#define CGU_SB_LCD_CLK_ID                  55
#define CGU_SB_SPI_PCLK_ID                 56
#define CGU_SB_SPI_PCLK_GATED_ID           57
#define CGU_AHB0APB2_LAST                  CGU_SB_SPI_PCLK_GATED_ID
/* domain 4 = AHB0APB3_BASE */
#define CGU_SB_AHB2APB3_ASYNC_PCLK_ID      58
#define CGU_AHB0APB3_FIRST                 CGU_SB_AHB2APB3_ASYNC_PCLK_ID
#define CGU_SB_I2S_CFG_PCLK_ID             59
#define CGU_SB_EDGE_DET_PCLK_ID            60
#define CGU_SB_I2STX_FIFO_0_PCLK_ID        61
#define CGU_SB_I2STX_IF_0_PCLK_ID          62 
#define CGU_SB_I2STX_FIFO_1_PCLK_ID        63
#define CGU_SB_I2STX_IF_1_PCLK_ID          64
#define CGU_SB_I2SRX_FIFO_0_PCLK_ID        65
#define CGU_SB_I2SRX_IF_0_PCLK_ID          66
#define CGU_SB_I2SRX_FIFO_1_PCLK_ID        67
#define CGU_SB_I2SRX_IF_1_PCLK_ID          68
#define CGU_SB_RSVD69_ID                   69
#define CGU_SB_AHB2APB3_RSVD_ID            70
#define CGU_AHB0APB3_LAST                  CGU_SB_AHB2APB3_RSVD_ID
/* domain 5 = PCM_BASE */
#define CGU_SB_PCM_CLK_IP_ID               71
#define CGU_PCM_FIRST                      CGU_SB_PCM_CLK_IP_ID
#define CGU_PCM_LAST                       CGU_SB_PCM_CLK_IP_ID
/* domain 6 = UART_BASE */
#define CGU_SB_UART_U_CLK_ID               72 
#define CGU_UART_FIRST                     CGU_SB_UART_U_CLK_ID
#define CGU_UART_LAST                      CGU_SB_UART_U_CLK_ID
/* domain 7 = CLK1024FS_BASE */
#define CGU_SB_I2S_EDGE_DETECT_CLK_ID      73
#define CGU_CLK1024FS_FIRST                CGU_SB_I2S_EDGE_DETECT_CLK_ID
#define CGU_SB_I2STX_BCK0_N_ID             74
#define CGU_SB_I2STX_WS0_ID                75
#define CGU_SB_I2STX_CLK0_ID               76
#define CGU_SB_I2STX_BCK1_N_ID             77
#define CGU_SB_I2STX_WS1_ID                78
#define CGU_SB_CLK_256FS_ID                79
#define CGU_SB_I2SRX_BCK0_N_ID             80
#define CGU_SB_I2SRX_WS0_ID                81
#define CGU_SB_I2SRX_BCK1_N_ID             82
#define CGU_SB_I2SRX_WS1_ID                83
#define CGU_SB_RSVD84_ID                   84
#define CGU_SB_RSVD85_ID                   85
#define CGU_SB_RSVD86_ID                   86
#define CGU_CLK1024FS_LAST                 CGU_SB_RSVD86_ID
/* domain 8 = BCK0_BASE */
#define CGU_SB_I2SRX_BCK0_ID               87
#define CGU_I2SRX_BCK0_FIRST               CGU_SB_I2SRX_BCK0_ID
#define CGU_I2SRX_BCK0_LAST                CGU_SB_I2SRX_BCK0_ID

/* domain 9 = BCK1_BASE */
#define CGU_SB_I2SRX_BCK1_ID               88
#define CGU_I2SRX_BCK1_FIRST               CGU_SB_I2SRX_BCK1_ID
#define CGU_I2SRX_BCK1_LAST                CGU_SB_I2SRX_BCK1_ID

/* domain 10 = SPI_BASE */
#define CGU_SB_SPI_CLK_ID                  89
#define CGU_SPI_FIRST                      CGU_SB_SPI_CLK_ID
#define CGU_SB_SPI_CLK_GATED_ID            90
#define CGU_SPI_LAST                       CGU_SB_SPI_CLK_GATED_ID

/* domain 11 = SYSCLKO_BASE */
#define CGU_SB_SYSCLK_O_ID                 91
#define CGU_SYSCLK_O_FIRST                 CGU_SB_SYSCLK_O_ID
#define CGU_SYSCLK_O_LAST                  CGU_SB_SYSCLK_O_ID

#define CGU_SB_INVALID_CLK_ID              -1

/*
 * NR of fractional dividers available for each base frequency,
 * their bit widths and extractions for sub elements from the
 * fractional divider configuration register
 */
#define CGU_SB_BASE0_FDIV_CNT		7
#define CGU_SB_BASE0_FDIV_LOW_ID	0
#define CGU_SB_BASE0_FDIV_HIGH_ID	6
#define CGU_SB_BASE0_FDIV0_W		8

#define CGU_SB_BASE1_FDIV_CNT		2
#define CGU_SB_BASE1_FDIV_LOW_ID	7
#define CGU_SB_BASE1_FDIV_HIGH_ID	8
#define CGU_SB_BASE1_FDIV0_W		8

#define CGU_SB_BASE2_FDIV_CNT		2
#define CGU_SB_BASE2_FDIV_LOW_ID	9
#define CGU_SB_BASE2_FDIV_HIGH_ID	10
#define CGU_SB_BASE2_FDIV0_W		8

#define CGU_SB_BASE3_FDIV_CNT		3
#define CGU_SB_BASE3_FDIV_LOW_ID	11
#define CGU_SB_BASE3_FDIV_HIGH_ID	13
#define CGU_SB_BASE3_FDIV0_W		8

#define CGU_SB_BASE4_FDIV_CNT		1
#define CGU_SB_BASE4_FDIV_LOW_ID	14
#define CGU_SB_BASE4_FDIV_HIGH_ID	14
#define CGU_SB_BASE4_FDIV0_W		8

#define CGU_SB_BASE5_FDIV_CNT		1
#define CGU_SB_BASE5_FDIV_LOW_ID	15
#define CGU_SB_BASE5_FDIV_HIGH_ID	15
#define CGU_SB_BASE5_FDIV0_W		8

#define CGU_SB_BASE6_FDIV_CNT		1
#define CGU_SB_BASE6_FDIV_LOW_ID	16
#define CGU_SB_BASE6_FDIV_HIGH_ID	16
#define CGU_SB_BASE6_FDIV0_W		8

#define CGU_SB_BASE7_FDIV_CNT		6
#define CGU_SB_BASE7_FDIV_LOW_ID	17
#define CGU_SB_BASE7_FDIV_HIGH_ID	22
#define CGU_SB_BASE7_FDIV0_W		13

#define CGU_SB_BASE8_FDIV_CNT		0
#define CGU_SB_BASE9_FDIV_CNT		0
#define CGU_SB_BASE10_FDIV_CNT		1
#define CGU_SB_BASE10_FDIV_LOW_ID	23
#define CGU_SB_BASE10_FDIV_HIGH_ID	23
#define CGU_SB_BASE10_FDIV0_W		8

#define CGU_SB_BASE11_FDIV_CNT		0



#endif /* CLOCK_SWITCHBOX_H */
