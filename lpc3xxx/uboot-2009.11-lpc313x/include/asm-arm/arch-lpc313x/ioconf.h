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

#ifndef IOCONF_H
#define IOCONF_H

#ifdef __cplusplus
extern "C"
{
#endif

/* IOCONF Function Block Register Structure */
typedef volatile struct
{
	volatile ulong pins;
	volatile ulong _d1[3];
	volatile ulong mode0;
	volatile ulong mode0_set;
	volatile ulong mode0_clear;
	volatile ulong _d2[1];
	volatile ulong mode1;
	volatile ulong mode1_set;
	volatile ulong mode1_clear;
	volatile ulong _d3[5];
} IOCONF_FUNC_REGS_T;

/* IOCONF Register Structure */
typedef volatile struct
{
	IOCONF_FUNC_REGS_T block[13];
} IOCONF_REGS_T;

typedef enum
{
	IOCONF_MUX0,
	IOCONF_MUX1,
	IOCONF_CGU,
	IOCONF_DAI0,
	IOCONF_DAI1,
	IOCONF_DAO1,
	IOCONF_EBI,
	IOCONF_GPIO,
	IOCONF_I2C1,
	IOCONF_SPI,
	IOCONF_NAND_CTRL,
	IOCONF_PWM,
	IOCONF_UART
} IOCONF_BLOCK_T;

/**********************************************************************
 * * Macro to access IOCONF registers
 * **********************************************************************/
#define IOCONF	((IOCONF_REGS_T*)IOCONF_BASE)


/* Returns current input states of the input pin */
static __inline ulong gpio_get_pin_state(IOCONF_BLOCK_T gpio_block, ulong bitnum)
{
	/* If high, return TRUE. If low, return FALSE */
	return (IOCONF->block[gpio_block].pins & _BIT(bitnum));
}

/* Sets GPIO pin as input pin */
static __inline void gpio_set_as_input(IOCONF_BLOCK_T gpio_block, ulong bitnum)
{
	IOCONF->block[gpio_block].mode1_clear = _BIT(bitnum);
	IOCONF->block[gpio_block].mode0_clear = _BIT(bitnum);
}

/* Sets GPIO output pin to high state */
static __inline void gpio_set_outpin_high(IOCONF_BLOCK_T gpio_block, ulong bitnum)
{
	IOCONF->block[gpio_block].mode1_set = _BIT(bitnum);
	IOCONF->block[gpio_block].mode0_set = _BIT(bitnum);
}

/* Sets GPIO output pin to high state */
static __inline void gpio_set_outpin_low(IOCONF_BLOCK_T gpio_block, ulong bitnum)
{
	IOCONF->block[gpio_block].mode1_set = _BIT(bitnum);
	IOCONF->block[gpio_block].mode0_clear = _BIT(bitnum);
}

/* Sets GPIO/MUX pin as driven by IP pin */
static __inline void gpio_set_as_ip_driven(IOCONF_BLOCK_T gpio_block, ulong bitnum)
{
	IOCONF->block[gpio_block].mode1_clear = _BIT(bitnum);
	IOCONF->block[gpio_block].mode0_set = _BIT(bitnum);
}

#ifdef __cplusplus
}
#endif

#endif /* IOCONFR_H */
