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
#include <asm/arch/spi.h>
#include <asm/arch/clock.h>
#include <asm/arch/ioconf.h>

#ifdef CONFIG_HARD_SPI
#define CMD_DP		0xB9	/* deep power down */
#define CMD_RES		0xAB	/* release from deep power down */

void spinor_cs_low(void)
{
	gpio_set_outpin_low(IOCONF_SPI, 4);
}

void spinor_cs_high(void)
{
	gpio_set_outpin_high(IOCONF_SPI, 4);
}

void spi_flush_rx_fifo(void)
{
	unsigned int tmp = 0;

	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;

	while ((regptr->status & SPI_ST_RX_EMPTY) == 0) {
		tmp = regptr->fifo_data;
	}
}

void spi_flush_tx_fifo(void)
{
	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;

	while ((regptr->status & SPI_ST_TX_EMPTY) == 0) {
		regptr->tx_fifo_flush = SPI_TXFF_FLUSH;
	}
}

unsigned int spi_get_status()
{
	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;
	return regptr->status;
}

static void spinor_exit_deep_power_down(void)
{
	long rbytes = 0;
	unsigned char dummy;
	long bytes = 1;
	unsigned char command_byte = CMD_RES;
	unsigned int status = 0;;

	/* should we wait for command to txfer */
	spinor_cs_low();

	/* flush the FFOS */
	spi_flush_rx_fifo();

	/* write the commands */
	spi_write(&command_byte, bytes);

	/* wait till SPI is not busy */
	do {
		status = spi_get_status();
	}
	while ((status & SPI_ST_TX_EMPTY) != SPI_ST_TX_EMPTY);

	while (rbytes < bytes) {
		rbytes += spi_read(&dummy, 1);
	}

	spinor_cs_high();

	/* max tEDPD in Atmel AT45DB161D-SU datasheet is given as 35 usec */
	udelay(50);
}

SPI_SLAVE_ID_T spi_get_cur_slave(SPI_REGS_T *regptr)
{
	SPI_SLAVE_ID_T sid = SPI_INVALID_ID;
	switch (regptr->slave_enable) {
	case SPI_SLV_EN(SPI_SLAVE1_ID):
		sid = SPI_SLAVE1_ID;
		break;
	case SPI_SLV_EN(SPI_SLAVE2_ID):
		sid = SPI_SLAVE2_ID;
		break;
	case SPI_SLV_EN(SPI_SLAVE3_ID):
		sid = SPI_SLAVE3_ID;
		break;
	default:
		sid = SPI_INVALID_ID;
		break;
	}
	return sid;
}

static long spi_get_clock_div(ulong target_clock,
		ulong *pDivSet)
{
	ulong div, spi_clk, ps, div1;
	long retvalue = _ERROR;

	/*
	 * The SPI clock is derived from the (main system oscillator / 2),
	 * so compute the best divider from that clock
	 */
	spi_clk = cgu_get_clk_freq(CGU_SB_SPI_CLK_ID);

	/*
	 * Find closest divider to get at or under the target frequency.
	 * Use smallest prescaler possible and rely on the divider to get
	 * the closest target frequency
	*/
	div = (spi_clk + target_clock / 2) / target_clock;

	if ((div < SPI_MAX_DIVIDER) && (div > SPI_MIN_DIVIDER)) {
		ps = (((div - 1) / 512) + 1) * 2;
		div1 = ((((div + ps / 2) / ps) - 1));

		/* write the divider settings */
		*pDivSet = SPI_SLV1_CLK_PS(ps) | SPI_SLV1_CLK_DIV1((div1));

		retvalue = _NO_ERROR;
	}

	return retvalue;
}

long spi_write(void *buffer, long n_fifo)
{
	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;
	long count = 0;
	ushort *data16 = NULL;
	unsigned char *data8 = NULL;
	SPI_SLAVE_ID_T sid;
	unsigned char tmpbuf[SPI_FIFO_DEPTH]; 

	memset(&tmpbuf,0x0,SPI_FIFO_DEPTH);
	if(!buffer)
		buffer = &tmpbuf;

	/* get current active slave */
	sid = spi_get_cur_slave(regptr);

	if (sid != SPI_INVALID_ID)
	{
		if (SPI_SLV2_WD_SZ(regptr->slv_setting[sid].setting2) > 8) {
			data16 = (ushort *)buffer;
		}
		else {
			data8 = (unsigned char *)buffer;
		}
		/* restrict single to max fifo depth */
		n_fifo = (n_fifo > SPI_FIFO_DEPTH) ? SPI_FIFO_DEPTH : n_fifo;
		
		/* Loop until transmit ring buffer is full or until n_bytes expires */
		while ((n_fifo > 0) &&
				((regptr->status & SPI_ST_TX_FF) != SPI_ST_TX_FF)) {
			if (data16 == NULL) {
				regptr->fifo_data = (ulong) * data8;
				data8++;
			}
			else {
				regptr->fifo_data = (ulong) * data16;
				data16++;
			}

			/* Increment data count and decrement buffer size count */
			count++;
			n_fifo--;
		}
	}
	return count;
}

long spi_read(void *buffer, long max_fifo)
{
	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;
	long count = 0;
	ushort *data16 = NULL;
	unsigned char *data8 = NULL;
	SPI_SLAVE_ID_T sid;
	unsigned char tmpbuf[SPI_FIFO_DEPTH];

	memset(&tmpbuf,0x0,SPI_FIFO_DEPTH);
	if(!buffer)
		buffer = &tmpbuf;

	/* get current active slave */
	sid = spi_get_cur_slave(regptr);

	if (sid != SPI_INVALID_ID) {
		/* determine sample width */
		if (SPI_SLV2_WD_SZ(regptr->slv_setting[sid].setting2) > 8) {
			data16 = (ushort *)buffer;
		}
		else {
			data8 = (unsigned char *)buffer;
		}
		/* Loop until transmit ring buffer is full or until n_bytes expires */
		while ((max_fifo > 0) &&
				((regptr->status & SPI_ST_RX_EMPTY) != SPI_ST_RX_EMPTY)) {
			if (data16 == NULL) {
				*data8 = (unsigned char)regptr->fifo_data;
				data8++;
			}
			else {
				*data16 = (ushort)regptr->fifo_data;
				data16++;
			}

			/* Increment data count and decrement buffer size count */
			count++;
			max_fifo--;
		}
	}
	return count;
}

static long spi_slave_configure(SPI_SLAVE_CONFIG_T *pSlaveCfg)
{
	ulong set1, set2;
	long setup = _ERROR;
	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;

	/* get clock divider setting */
	if (spi_get_clock_div(pSlaveCfg->clk, &set1) == _NO_ERROR) {
		/* set no of words for SMS mode and inter frame delay timing */
		set1 |= SPI_SLV1_NUM_WORDS(pSlaveCfg->words) |
			SPI_SLV1_INTER_TX_DLY(pSlaveCfg->inter_delay);

		/* set data width & pre-post delay timing */
		set2 = SPI_SLV2_WD_SZ(pSlaveCfg->databits - 1) |
			SPI_SLV2_PPCS_DLY(pSlaveCfg->pp_delay);

		/* set the transfer mode */
		switch (pSlaveCfg->mode) {
		case SSI_MODE:
			set2 = SPI_SLV2_SSI_MODE;
			break;
		case SPI_MODE1:
			set2 |= SPI_SLV2_SPH;
			break;
		case SPI_MODE2:
			set2 |= SPI_SLV2_SPO;
			break;
		case SPI_MODE3:
			set2 |= SPI_SLV2_SPO | SPI_SLV2_SPH;
			break;
		default:
			break;
		}
		/* check is CS high is needed */
		if (pSlaveCfg->cs_high)
			set2 |= SPI_SLV2_CS_HIGH;

		/* finally write to the register */
		regptr->slv_setting[pSlaveCfg->sid].setting1 = set1;
		regptr->slv_setting[pSlaveCfg->sid].setting2 = set2;

		setup = _NO_ERROR;
	}
	return setup;
}

long lpc313x_setup_slave(SPI_SLAVE_CONFIG_T *slavecfg)
{
	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;
	/* de-select the device by pulling the CS high. */
	gpio_set_outpin_high(IOCONF_SPI, 4);

	/* Reset SPI block */
	regptr->spi_config = SPI_CFG_SW_RESET;

	/* Configure SPI slave */
	if(spi_slave_configure(slavecfg) != 0) {
		printf("failed to configure spi slave\n");
		return -1;
	}

	/* enable the last configured slave only */
	regptr->slave_enable = SPI_SLV_EN(0);
	/* inform the SPI block about changes in slave enable */
	regptr->spi_config |= SPI_CFG_UPDATE_EN;

	/* make sure SPI module is enabled */
	regptr->spi_config |= SPI_CFG_ENABLE;

	spinor_exit_deep_power_down();
	return 0;
}

long lpc313x_spi_init(void)
{
	SPI_REGS_T *regptr = (SPI_REGS_T *)SPI_BASE;

	/* Enable SPI clock */
	cgu_clk_en_dis(CGU_SB_SPI_PCLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_SPI_PCLK_GATED_ID, 1);
	cgu_clk_en_dis(CGU_SB_SPI_CLK_ID, 1);
	cgu_clk_en_dis(CGU_SB_SPI_CLK_GATED_ID, 1);

	/* set master mode & also set inter slave delay */
	regptr->spi_config |= SPI_CFG_INTER_DLY(1);

	/* Empty FIFO */
	regptr->tx_fifo_flush = SPI_TXFF_FLUSH;

	/* Clear latched interrupts */
	regptr->int_clr_enable = SPI_ALL_INTS;
	regptr->int_clr_status = SPI_ALL_INTS;

	/* Enable interrupts */
	regptr->int_set_enable = (SPI_OVR_INT |
			SPI_RX_INT | SPI_TX_INT);

	return 0;
}
#endif
