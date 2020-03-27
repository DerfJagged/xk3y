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

#ifndef SPI_H
#define SPI_H

#include <common.h>
#include <asm/arch/hardware.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* SPI slave ids */
typedef enum
{
	SPI_SLAVE1_ID = 0,
	SPI_DEFAULT_ID = SPI_SLAVE1_ID,
	SPI_SLAVE2_ID,
	SPI_SLAVE3_ID,
	SPI_INVALID_ID = 0xFF
} SPI_SLAVE_ID_T;

/* SPI transfer mode */
typedef enum
{
	SPI_MODE0,
	SPI_MODE1,
	SPI_MODE2,
	SPI_MODE3,
	SSI_MODE
} SPI_TRANS_MODE_T;

/* Structure for setting up SPI parameters */
typedef struct
{
	/* slave index */
	SPI_SLAVE_ID_T sid;
	/* Number of data bits, must be between 4 and 16 */
	ulong databits;
	/* Number of words required in sequential multi-slave transfer mode */
	ulong words;
	/* Transfer mode */
	SPI_TRANS_MODE_T mode;
	/* Flag used to set cs polarity high between frames */
	ulong cs_high;
	/* Post & pre transfer delay */
	ulong pp_delay;
	/* inter transfer delay */
	ulong inter_delay;
	/* Serial clock rate */
	ulong clk;
} SPI_SLAVE_CONFIG_T;

/* SPI slave setting Register Structure */
typedef volatile struct
{
	volatile ulong setting1;
	volatile ulong setting2;
} SPI_SLV_SETTING_T;

/* Serial Peripheral Interface (SPI) Module Register Structure */
typedef volatile struct
{
	volatile ulong spi_config;
	volatile ulong slave_enable;
	volatile ulong tx_fifo_flush;
	volatile ulong fifo_data;
	volatile ulong nhp_pop;
	volatile ulong nhp_mode;
	volatile ulong dma_settings;
	volatile const ulong status;
	volatile const ulong hw_info;
	SPI_SLV_SETTING_T slv_setting[3];
	volatile const ulong _reserved1[998];
	volatile ulong int_threshold;
	volatile ulong int_clr_enable;
	volatile ulong int_set_enable;
	volatile const ulong int_status;
	volatile const ulong int_enable;
	volatile ulong int_clr_status;
	volatile ulong int_set_status;
	volatile const ulong _reserved2[3];
	volatile const ulong module_id;
} SPI_REGS_T;
/***********************************************************************
 * SPI device contants
 **********************************************************************/
#define SPI_FIFO_DEPTH		64	/* 64 words (16bit) deep */
#define SPI_NUM_SLAVES		3	/* number of slaves supported */
#define SPI_MAX_DIV2		254
#define SPI_MAX_DIVIDER		65024	/* = 254 * (255 + 1) */
#define SPI_MIN_DIVIDER		2

/***********************************************************************
 * SPI Configuration register definitions
 **********************************************************************/
#define SPI_CFG_INTER_DLY(n)		_SBF(16, ((n) & 0xFFFF))
#define SPI_CFG_INTER_DLY_GET(n)	(((n) >> 16) & 0xFFFF)
#define SPI_CFG_UPDATE_EN		_BIT(7)
#define SPI_CFG_SW_RESET		_BIT(6)
#define SPI_CFG_SLAVE_DISABLE		_BIT(4)
#define SPI_CFG_MULTI_SLAVE		_BIT(3)
#define SPI_CFG_LOOPBACK		_BIT(2)
#define SPI_CFG_SLAVE_MODE		_BIT(1)
#define SPI_CFG_ENABLE			_BIT(0)
/***********************************************************************
 * SPI slave_enable register definitions
 **********************************************************************/
#define SPI_SLV_EN(n)			_SBF(((n) << 1), 0x1)
#define SPI_SLV_SUSPEND(n)		_SBF(((n) << 1), 0x3)

/***********************************************************************
 * SPI tx_fifo_flush register definitions
 **********************************************************************/
#define SPI_TXFF_FLUSH			_BIT(1)

/***********************************************************************
 * SPI dma_settings register definitions
 **********************************************************************/
#define SPI_DMA_TX_EN			_BIT(1)
#define SPI_DMA_RX_EN			_BIT(0)

/***********************************************************************
 * SPI status register definitions
 **********************************************************************/
#define SPI_ST_SMS_BUSY			_BIT(5)
#define SPI_ST_BUSY			_BIT(4)
#define SPI_ST_RX_FF			_BIT(3)
#define SPI_ST_RX_EMPTY			_BIT(2)
#define SPI_ST_TX_FF			_BIT(1)
#define SPI_ST_TX_EMPTY			_BIT(0)

/***********************************************************************
 * SPI slv_setting registers definitions
 **********************************************************************/
#define SPI_SLV1_INTER_TX_DLY(n)	_SBF(24, ((n) & 0xFF))
#define SPI_SLV1_NUM_WORDS(n)		_SBF(16, ((n) & 0xFF))
#define SPI_SLV1_CLK_PS(n)		_SBF(8, ((n) & 0xFF))
#define SPI_SLV1_CLK_PS_GET(n)		(((n) >> 8) & 0xFF)
#define SPI_SLV1_CLK_DIV1(n)		((n) & 0xFF)
#define SPI_SLV1_CLK_DIV1_GET(n)	((n) & 0xFF)

#define SPI_SLV2_PPCS_DLY(n)		_SBF(9, ((n) & 0xFF))
#define SPI_SLV2_CS_HIGH		_BIT(8)
#define SPI_SLV2_SSI_MODE		_BIT(7)
#define SPI_SLV2_SPO			_BIT(6)
#define SPI_SLV2_SPH			_BIT(5)
#define SPI_SLV2_WD_SZ(n)		((n) & 0x1F)

/***********************************************************************
 * SPI int_threshold registers definitions
 **********************************************************************/
#define SPI_INT_TSHLD_TX		_SBF(8, ((n) & 0xFF))
#define SPI_INT_TSHLD_RX		((n) & 0xFF)

/***********************************************************************
 * SPI intterrupt registers definitions
 **********************************************************************/
#define SPI_SMS_INT			_BIT(4)
#define SPI_TX_INT			_BIT(3)
#define SPI_RX_INT			_BIT(2)
#define SPI_OVR_INT			_BIT(0)
#define SPI_ALL_INTS			(SPI_SMS_INT | SPI_TX_INT | \
					SPI_RX_INT | SPI_OVR_INT)

/*
 * SPI write function - the buffer must be aligned on a 16-bit
 * boundary if the data size is 9 bits or more
 */
long spi_write(void *buffer, long n_fifo);

/*
 * SPI read function - the buffer must be aligned on a 16-bit
 * boundary if the data size is 9 bits or more
 */
long spi_read(void *buffer, long max_fifo);

long lpc313x_spi_init(void);
long lpc313x_setup_slave(SPI_SLAVE_CONFIG_T *cfg);
void spi_flush_rx_fifo(void);
void spi_flush_tx_fifo(void);
unsigned int spi_get_status(void);
void spinor_cs_low(void);
void spinor_cs_high(void);

#ifdef __cplusplus
}
#endif

#endif /* SPI_H */
