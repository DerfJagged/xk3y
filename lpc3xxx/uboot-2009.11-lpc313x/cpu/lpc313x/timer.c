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
#include <asm/arch/hardware.h>
#include <asm/arch/timer.h>
#include <asm/arch/clock.h>

static long timer0dev = 0;
static ulong timestamp;
static ulong lastinc;
static ulong timer0_clk_freq;

#define DIV_MS   (1000)
#define TIMER0_LOAD_VALUE (0xFFFFFFFF)

ulong timer_usec_to_val(u32 clk_id, ulong usec, ulong* pcon)
{
	u32 clkdlycnt;
	u32 freq;

	/*
	 * Determine the value to exceed before the count
	 * reaches the desired delay time
	 */
	freq = (unsigned long long)cgu_get_clk_freq(clk_id);
	if ((freq > 1000000) && (pcon != NULL)) {
		/* if timer freq is greater than 1MHz use pre-dividers */
		*pcon &= ~TM_CTRL_PS_MASK;
		if (usec > 100000) {
			/* use divide by 256 pre-divider for delay greater than 100 msec*/
			*pcon |= TM_CTRL_PS256;
			/* divide by 256 */
			freq = freq >> 8;
		}
		else if (usec > 1000) {
			/* use divide by 16 pre-divider for delay greater than 1 msec*/
			*pcon |= TM_CTRL_PS16;
			/* divide by 16 */
			freq = freq >> 4;
		}
	}

	clkdlycnt = ((freq * (u32)usec) / 1000000);

	return (ulong)clkdlycnt;
}


void timer_delay_cmn(TIMER_REGS_T* regptr, ulong usec)
{
	ulong control = 0;
	ulong clkdlycnt;

	/* Enable timer system clock */
	cgu_clk_en_dis(CGU_SB_TIMER1_PCLK_ID, 1);

	/*
	 * Determine the value to exceed before the count
	 *reaches the desired delay time
	 */
	clkdlycnt = timer_usec_to_val(CGU_SB_TIMER1_PCLK_ID, usec, &control);

	/* Reset timer */
	regptr->control &= ~TM_CTRL_ENABLE;
	regptr->load = clkdlycnt;

	/* Enable the timer in free running mode*/
	regptr->control = control | TM_CTRL_ENABLE;

	/* Loop until terminal count matches or exceeds computed delay count */
	while (regptr->value <= clkdlycnt);

	/* Disable timer system clock */
	cgu_clk_en_dis(CGU_SB_TIMER1_PCLK_ID, 0);

	/* Stop timer */
	regptr->control &= ~TM_CTRL_ENABLE;
}

int timer_init(void)
{
	TIMER_REGS_T *regptr = (TIMER_REGS_T *)TIMER_CNTR0;
	if(timer0dev == 0) {
		/* Enable timer0 system clock */
		cgu_clk_en_dis(CGU_SB_TIMER0_PCLK_ID, 1);

		timer0_clk_freq = (ulong) cgu_get_clk_freq(CGU_SB_TIMER0_PCLK_ID);

		regptr->control = TM_CTRL_PS256;
		regptr->load = TIMER0_LOAD_VALUE;

		/* Enable the timer0 */
		regptr->control |= TM_CTRL_ENABLE;

		/* Using Predivider */
		timer0_clk_freq = timer0_clk_freq >> 8;

		/* Reset Time stamps */
		reset_timer_masked();

		timer0dev = 1;
	}

	return 0;
}

void reset_timer_masked(void)
{
	/* Capture current decrementer counter value */
	TIMER_REGS_T *regptr = (TIMER_REGS_T *)TIMER_CNTR0;

	lastinc = ((TIMER0_LOAD_VALUE - regptr->value) / (timer0_clk_freq / DIV_MS));
	timestamp = 0;		/* start "advancing" time stamp from 0 */
}

ulong get_timer_masked (void)
{
	TIMER_REGS_T *regptr = (TIMER_REGS_T *)TIMER_CNTR0;

	/* Current time in ms */
	ulong now = (TIMER0_LOAD_VALUE - regptr->value) / (timer0_clk_freq / DIV_MS);

	if (now >= lastinc)	/* normal mode (non roll) */
		/* move stamp fordward with absoulte diff ticks */
		timestamp += (now - lastinc);
	else	/* we have rollover of incrementer */
		timestamp += ((TIMER0_LOAD_VALUE / (timer0_clk_freq / DIV_MS))
				- lastinc) + now;
	lastinc = now;

	return timestamp;
}

unsigned long get_timer(unsigned long base)
{
	return (get_timer_masked() - base) ;
}

void udelay(unsigned long usec)
{
	timer_delay_cmn(TIMER_CNTR1, usec);
}

void reset_cpu(ulong ignored)
{
	while (1);
	/* Never reached */
}

void reset_timer()
{
	TIMER_CNTR0->control &= ~TM_CTRL_ENABLE;
	TIMER_CNTR0->load = TIMER0_LOAD_VALUE;
	TIMER_CNTR0->control |= TM_CTRL_ENABLE;

	/* Reset Time stamps */
	reset_timer_masked();
}
