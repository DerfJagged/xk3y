/*
 * (C) Copyright 2002-2004
 * Wolfgang Denk, DENX Software Engineering, <wd@denx.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * Copyright (C) 1999 2000 2001 Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/arch/uart.h>
#include <asm/arch/clock.h>

DECLARE_GLOBAL_DATA_PTR;

static unsigned int uart_init = 0;

long uart_set_divisors(ulong baudrate)
{
	long errorStatus = _ERROR;
	UART_REGS_T *regptr = (UART_REGS_T *)UART_BASE;
	ulong uClk;
	ulong calcBaudrate = 0;
	ulong temp = 0;
	ulong mulFracDiv, dividerAddFracDiv;
	ulong diviser = 0 ;
	ulong mulFracDivOptimal = 1;
	ulong dividerAddOptimal = 0;
	ulong diviserOptimal = 0;
	ulong relativeError = 0;
	ulong relativeOptimalError = 100000;

	/* get UART block clock */
	uClk = cgu_get_clk_freq(CGU_SB_UART_U_CLK_ID);

	/* div by 16 */
	uClk = uClk >> 4;
	/* 
	 * In the Uart IP block, baud rate is calculated using FDR and DLL-DLM
	 * registers. The formula is :
	 * BaudRate= uClk * (mulFracDiv/(mulFracDiv+dividerAddFracDiv) / (16 * (DLL)
	 * It involves floating point calculations. That's the reason the
	 * formulae are adjusted with Multiply and divide method.
	 * The value of mulFracDiv and dividerAddFracDiv should comply
	 * to the following expressions:
	 * 0 < mulFracDiv <= 15, 0 <= dividerAddFracDiv <= 15
	 */
	for (mulFracDiv = 1 ; mulFracDiv <= 15 ;mulFracDiv++) {
		for (dividerAddFracDiv = 0 ; dividerAddFracDiv <= 15 ;dividerAddFracDiv++) {
			temp = (mulFracDiv * uClk) / ((mulFracDiv + dividerAddFracDiv));

			diviser = temp / baudrate;
			if ((temp % baudrate) > (baudrate / 2))
				diviser++;

			if (diviser > 2 && diviser < 65536) {
				calcBaudrate = temp / diviser;

				if (calcBaudrate <= baudrate)
					relativeError = baudrate - calcBaudrate;
				else
					relativeError = calcBaudrate - baudrate;

				if ((relativeError < relativeOptimalError)) {
					mulFracDivOptimal = mulFracDiv ;
					dividerAddOptimal = dividerAddFracDiv;
					diviserOptimal = diviser;
					relativeOptimalError = relativeError;
					if (relativeError == 0)
						break;
				}
			} /* End of if */
		} /* end of inner for loop */
		if (relativeError == 0)
			break;
	} /* end of outer for loop */

	if (relativeOptimalError < (baudrate / 30)) {
		/* Disable all UART interrupts */
		regptr->dlm_ie = 0;

		/* Set the `Divisor Latch Access Bit` and enable so the DLL/DLM access*/
		regptr->lcr |= UART_LCR_DIVLATCH_EN;
		/* Initialise the `Divisor latch LSB` and `Divisor latch MSB` registers */
		regptr->dll_fifo = UART_LOAD_DLL(diviserOptimal);
		regptr->dlm_ie = UART_LOAD_DLM(diviserOptimal);
		regptr->lcr &= ~ UART_LCR_DIVLATCH_EN;

		/* Initialise the Fractional Divider Register */
		regptr->fdr = UART_FDR_MUL_SET(mulFracDivOptimal) |
				UART_FDR_DIVADD_SET(dividerAddOptimal);

		errorStatus = _NO_ERROR;
	}
	return errorStatus;
}

long uart_setup_trans_mode(ulong baud_rate)
{
	UART_REGS_T *regptr = (UART_REGS_T *)UART_BASE;
	ulong tmp = 0;
	long err = _NO_ERROR;


	tmp |= UART_LCR_WLEN_8BITS;
	/* Find closest baud rate for desired clock frequency */
	err = uart_set_divisors(baud_rate);

	if (err == _NO_ERROR) {
		/* Set new UART settings */
		regptr->lcr = tmp;
	}

	return err;
}

long uart_read(void *buffer, long max_bytes)
{
	long bread = 0;
	UART_REGS_T *regptr = (UART_REGS_T *)UART_BASE;
	unsigned char *buff8 = (unsigned char *) buffer;

	/* Wait for a character from the UART */
	while ((regptr->lsr & UART_LSR_RDR) == 0);

	while ((max_bytes > 0) && ((regptr->lsr & UART_LSR_RDR) != 0)) {
		*buff8 = (unsigned char) regptr->dll_fifo;
		buff8++;
		max_bytes--;
		bread++;
	}

	return bread;
}


long uart_write(void *buffer, long n_bytes)
{
	long bwrite = 0;
	UART_REGS_T *regptr = (UART_REGS_T *)UART_BASE;
	unsigned char *buff8 = (unsigned char *) buffer;

	/* Only add data if the current FIFO level can be determined */
	while ((regptr->lsr & UART_LSR_TEMT) == 0){}

	if ((regptr->lsr & UART_LSR_TEMT) != 0) {
		/* Assuming TX fifo is 32 byte deep. */
		if (n_bytes > 32) {
			n_bytes = 32;
		}
		while (n_bytes > 0) {
			regptr->dll_fifo = (ulong) * buff8;
			buff8++;
			n_bytes--;
			bwrite++;
		}
	}

	return bwrite;
}

int uart_rx_char (unsigned char *buff)
{
	UART_REGS_T *regptr = (UART_REGS_T *)UART_BASE;

	if((regptr->lsr & UART_LSR_RDR) == 0)
		return 0;

	*buff = (unsigned char)regptr->dll_fifo;

	return 1;
}

void serial_setbrg (void)
{
}

int serial_init (void)
{
	UART_REGS_T *regptr = (UART_REGS_T *)UART_BASE;
	volatile ulong tmp;

	if(uart_init == 0) {
		/* UART is free */
		uart_init = 1;

		/* Enable UART system clock */
		cgu_clk_en_dis(CGU_SB_UART_APB_CLK_ID, 1);
		cgu_clk_en_dis(CGU_SB_UART_U_CLK_ID, 1);

		uart_setup_trans_mode(gd->baudrate);

		/* Clear FIFOs, set FIFO level, and pending interrupts */
		regptr->iir_fcr = (UART_FCR_RXFIFO_TL16 |
				UART_FCR_FIFO_EN | UART_FCR_TXFIFO_FLUSH |
				UART_FCR_RXFIFO_FLUSH);
		tmp = regptr->iir_fcr;
		tmp = regptr->lsr;

		/* Receive and RX line status interrupts enabled */
		regptr->dlm_ie = (UART_IE_RXLINE_STS |
				UART_IE_RDA | UART_IE_THRE);

	}
	return (0);
}

/*
 * Output a single byte to the serial port.
 */
void serial_putc (const char c)
{
	if (c == '\n')
		uart_write("\r\n", 2);
	else
		uart_write((void *)&c, 1);
}

/*
 *Test whether a character is in the RX buffer
 */

int serial_tstc (void)
{
	UART_REGS_T *regptr = (UART_REGS_T *)UART_BASE;

	if(uart_init == 1) {
		if ((regptr->lsr & UART_LSR_RDR) == 0) {
			return 0;
		}
		else
			return 1;	
	}
	else
		return -1;
}

/*
 * Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c.
 */
int serial_getc (void)
{
	char c = 0;
	int ret = 0;
	ret = uart_read(&c, 1);
	return c;
}

void serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}

int serial_getchar(unsigned char* buf)
{
	return uart_rx_char(buf);
}
