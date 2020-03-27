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
#include <spi.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/spi.h>

struct lpc313x_spi_info
{
	long spi_init;
	struct spi_slave slave;
};

struct lpc313x_spi_info *lpc313x_spi = NULL;
const extern SPI_SLAVE_CONFIG_T slavecfg;

static inline struct lpc313x_spi_info *to_lpc313x_spi(struct spi_slave *slave)
{
	return container_of(slave, struct lpc313x_spi_info, slave);
}

void spi_init(void)
{
	lpc313x_spi = (struct lpc313x_spi_info *)malloc(sizeof(struct lpc313x_spi_info));
	if(!lpc313x_spi) {
		printf("%s not able to allocate memory\n",__FUNCTION__);
		return;
	}

	memset((void*)lpc313x_spi,0x0,sizeof(struct lpc313x_spi_info));

	lpc313x_spi_init();

	lpc313x_spi->spi_init = 1;
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	lpc313x_spi->slave.bus = bus;
	lpc313x_spi->slave.cs = cs;
	
	if(lpc313x_setup_slave(&slavecfg) != 0)
		return NULL;

	return &lpc313x_spi->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct lpc313x_spi_info *lpc313xspi = to_lpc313x_spi(slave);

	free(lpc313xspi);
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	unsigned int	len;
	int		ret;
	u32		status;
	const u8	*txp = dout;
	u8		*rxp = din;
	long 		rxbytes = 0;
	unsigned int	txbytes = 0;
	unsigned int	total_txbytes = 0;

	ret = 0;
	if (bitlen == 0)
		/* Finish any previously submitted transfers */
		goto out;

	if (bitlen % 8) {
		/* Errors always terminate an ongoing transfer */
		flags |= SPI_XFER_END;
		goto out;
	}

	len = bitlen / 8;
	if (flags & SPI_XFER_BEGIN)
		spinor_cs_low();

	/* flush the FFOS */
	spi_flush_rx_fifo();

	/* write the commands or data to Flash memory */
	
	while (total_txbytes < len) {
		if(!txp) {
			udelay(10);
			txbytes = spi_write((void *)txp, ((len > 64) ? 64: len));
		}
		else {
			udelay(10);
			txbytes = spi_write((void *)&txp[total_txbytes], ((len > 64) ? 64: len));
		}
		total_txbytes += txbytes;
		/* wait till SPI is not busy */
		do {
			status = spi_get_status();
		}
		while ((status & SPI_ST_TX_EMPTY) != SPI_ST_TX_EMPTY);
		/* Read Data from Flash memory */
		if(!rxp) {
			udelay(10);
			rxbytes += spi_read(rxp, txbytes);
		}
		else {
			udelay(10);
			rxbytes += spi_read(&rxp[rxbytes], txbytes);
		}
	}
out:
	if (flags & SPI_XFER_END) {
		spinor_cs_high();
	}
	
	return 0;
}
