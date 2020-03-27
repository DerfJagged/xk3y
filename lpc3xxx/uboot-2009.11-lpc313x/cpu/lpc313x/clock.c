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
#include <asm/arch/clock.h>


static ulong  cgu_clkid2esrid(u32 clkid)
{
	ulong esrIndex = (ulong )clkid;

	switch (clkid) {
	case CGU_SB_I2SRX_BCK0_ID:
	case CGU_SB_I2SRX_BCK1_ID:
	case CGU_SB_SYSCLK_O_ID:
			/* invalid esr index. No ESR register for these clocks */
		esrIndex = CGU_INVALID_ID;
		break;

	case CGU_SB_SPI_CLK_ID:
	case CGU_SB_SPI_CLK_GATED_ID:
		esrIndex = esrIndex - 2;
		break;
	default:
		/* do nothing */
		break;
	}

	return esrIndex;
}



static void cgu_ClkId2DomainId(u32 clkid, u32* pDomainId,
		ulong * pSubdomainId)
{
	ulong esrIndex, esrReg;
	ulong fracdiv_base = CGU_INVALID_ID;

	/* 1. Get the domain ID */

	if (clkid <= CGU_SYS_LAST) {
		*pDomainId = CGU_SB_SYS_BASE_ID;
		fracdiv_base = CGU_SB_BASE0_FDIV_LOW_ID;

	}
	else if (clkid <= CGU_AHB0APB0_LAST) {
		*pDomainId = CGU_SB_AHB0_APB0_BASE_ID;
		fracdiv_base = CGU_SB_BASE1_FDIV_LOW_ID;

	}
	else if (clkid <= CGU_AHB0APB1_LAST) {
		*pDomainId = CGU_SB_AHB0_APB1_BASE_ID;
		fracdiv_base = CGU_SB_BASE2_FDIV_LOW_ID;

	}
	else if (clkid <= CGU_AHB0APB2_LAST) {
		*pDomainId = CGU_SB_AHB0_APB2_BASE_ID;
		fracdiv_base = CGU_SB_BASE3_FDIV_LOW_ID;

	}
	else if (clkid <= CGU_AHB0APB3_LAST) {
		*pDomainId = CGU_SB_AHB0_APB3_BASE_ID;
		fracdiv_base = CGU_SB_BASE4_FDIV_LOW_ID;

	}
	else if (clkid == CGU_PCM_LAST) {
		*pDomainId = CGU_SB_PCM_BASE_ID;
		fracdiv_base = CGU_SB_BASE5_FDIV_LOW_ID;

	}
	else if (clkid == CGU_UART_LAST) {
		*pDomainId = CGU_SB_UART_BASE_ID;
		fracdiv_base = CGU_SB_BASE6_FDIV_LOW_ID;

	}
	else if (clkid <= CGU_CLK1024FS_LAST) {
		*pDomainId = CGU_SB_CLK1024FS_BASE_ID;
		fracdiv_base = CGU_SB_BASE7_FDIV_LOW_ID;

	}
	else if (clkid == CGU_I2SRX_BCK0_LAST) {
		*pDomainId = CGU_SB_I2SRX_BCK0_BASE_ID;
		fracdiv_base = CGU_INVALID_ID;

	}
	else if (clkid == CGU_I2SRX_BCK1_LAST) {
		*pDomainId = CGU_SB_I2SRX_BCK1_BASE_ID;
		fracdiv_base = CGU_INVALID_ID;

	}
	else if (clkid <= CGU_SPI_LAST) {
		*pDomainId = CGU_SB_SPI_CLK_BASE_ID;
		fracdiv_base = CGU_SB_BASE10_FDIV_LOW_ID;

	}
	else {
		*pDomainId = CGU_SB_SYSCLK_O_BASE_ID;
		fracdiv_base = CGU_INVALID_ID;
	}

	*pSubdomainId = CGU_INVALID_ID;

	/* read the clocks ESR to get the fractional divider */
	esrIndex = cgu_clkid2esrid(clkid);

	if (CGU_INVALID_ID != esrIndex) {
		/* read the clocks ESR to get the fractional divider */
		esrReg = CGU_SB->clk_esr[esrIndex];

		/* A clock may not be connected to any sub-domain and it might be
		 * connected directly to domain. This is also a valid combination. So,
		 * errror should not be returned
		 */
		if (esrReg & CGU_SB_ESR_ENABLE) {
			*pSubdomainId = CGU_SB_ESR_SEL_GET(esrReg) + fracdiv_base;
		}
	}

}

void cgu_reset_all_clks(void)
{
	u32 domainId;
	ulong esr_id;
	ulong enable, i;

	/* switch reference clock in to FFAST */
	for (domainId = CGU_SB_BASE_FIRST; domainId < CGU_SB_NR_BASE; domainId++) 
		cgu_set_base_freq(domainId, CGU_FIN_SELECT_FFAST);
	
	/* disable all BCRs */
	for (i = 0; i < CGU_SB_NR_BCR; i++) 
		CGU_SB->base_bcr[i] = 0;
	
	/* disable all clocks except the needed ones */
	for (i = 0; i < (CGU_SYSCLK_O_LAST + 1); i++) {
		/* Clear the clocks ESR to deselect fractional divider */
		esr_id = cgu_clkid2esrid((u32)i);

		if (CGU_INVALID_ID != esr_id) {
			CGU_SB->clk_esr[esr_id] = 0;
		}

		if (i < 31) {
			enable = CGU_DEF_CLKS_0_31 & (1<<i);
		}
		else {
			if (i < 63) {
				enable = CGU_DEF_CLKS_32_63 & (1<<(i - 32));
			}
			else {
				enable = CGU_DEF_CLKS_64_92 & (1<<(i - 64));
			}
		}
		/* 
		 * set external enable for all possible clocks to conserve
		 * power. cgu_clk_set_exten() function sets CGU_SB_PCR_EXTEN_EN
		 * for allowed clocks only.
		 */
		cgu_clk_set_exten((u32)i, 1);

		/* set enable-out’s for only the following clocks */
		if ((i == CGU_SB_ARM926_BUSIF_CLK_ID) ||
				(i == CGU_SB_MPMC_CFG_CLK_ID)) {
			CGU_SB->clk_pcr[i] |= CGU_SB_PCR_ENOUT_EN;
		}
		else {
			CGU_SB->clk_pcr[i] &= ~CGU_SB_PCR_ENOUT_EN;
		}

		/* disable all clocks except the needed ones */
		if (enable) {
			CGU_SB->clk_pcr[i] |= CGU_SB_PCR_RUN;
		}
		else {
			CGU_SB->clk_pcr[i] &= ~CGU_SB_PCR_RUN;
		}
	}
	/* disable all fractional dividers */
	for (i = 0; i < CGU_SB_NR_FRACDIV; i++) {
		CGU_SB->base_fdc[i] &= ~CGU_SB_FDC_RUN;
	}
}

long cgu_init_clks(u32 fidv_val[], u32 fdiv_clks[])
{
	
	u32 domain_id = 0, fd_id = 0, clk_id = 0, esr_id=0;
	u32 clk_start = 0, fdiv_start = 0;
	u32 clk_last = CGU_SYS_LAST, fdiv_last = CGU_SB_BASE0_FDIV_HIGH_ID;
	u32 bcr_id;

	/* reset all clocks and connect them to FFAST */
	cgu_reset_all_clks();

	/* now init all fractional dividers */
	while ( domain_id < CGU_SB_NR_BASE) {

		if (domain_id == CGU_SB_SYS_BASE_ID) {
			clk_last = CGU_SYS_LAST;
			fdiv_last = CGU_SB_BASE0_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_AHB0_APB0_BASE_ID) {
			clk_last = CGU_AHB0APB0_LAST;
			fdiv_last = CGU_SB_BASE1_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_AHB0_APB1_BASE_ID) {
			clk_last = CGU_AHB0APB1_LAST;
			fdiv_last = CGU_SB_BASE2_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_AHB0_APB2_BASE_ID) {
			clk_last = CGU_AHB0APB2_LAST;
			fdiv_last = CGU_SB_BASE3_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_AHB0_APB3_BASE_ID) {
			clk_last = CGU_AHB0APB3_LAST;
			fdiv_last = CGU_SB_BASE4_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_PCM_BASE_ID) {
			clk_last = CGU_PCM_LAST;
			fdiv_last = CGU_SB_BASE5_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_UART_BASE_ID) {
			clk_last = CGU_UART_LAST;
			fdiv_last = CGU_SB_BASE6_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_CLK1024FS_BASE_ID) {
			clk_last = CGU_CLK1024FS_LAST;
			fdiv_last = CGU_SB_BASE7_FDIV_HIGH_ID;

		} else if (domain_id == CGU_SB_SPI_CLK_BASE_ID) {
			clk_last = CGU_SPI_LAST;
			fdiv_last = CGU_SB_BASE10_FDIV_HIGH_ID;

		} else
			clk_id++;
	
		fd_id = fdiv_start;
		while (fd_id <= fdiv_last) {
			/* enable frac divider only if it has valid settings. Or else it may be unused*/
			if (fidv_val[fd_id] != 0) {
				clk_id = clk_start;
				/* select frac div for each clock in this sub domain*/
				while (clk_id <= clk_last) {
					if (fdiv_clks[fd_id] & (1 << (clk_id - clk_start))) {
						/* Get esr_id. No need to check return id since 
						 * if we are here we will have esr_id for clk_id
						 */
						esr_id = cgu_clkid2esrid(clk_id); 
						/* finally configure the clock*/
						CGU_SB->clk_esr[esr_id] = CGU_SB_ESR_SELECT((fd_id - fdiv_start) ) |
										CGU_SB_ESR_ENABLE;
					}
					clk_id++;
				}
				/* enable frac divider */
				CGU_SB->base_fdc[fd_id] = fidv_val[fd_id] | CGU_SB_FDC_RUN;
			}
			clk_id = clk_last + 1;
			fd_id++;
		}
		clk_start = clk_id;
		fdiv_start = fd_id;
		domain_id++;
	}

	/* enable dynamic divider for SYS_BASE for dynamic clock scaling */
	for (fd_id = 0; fd_id < CGU_SB_NR_DYN_FDIV; fd_id++) {
		CGU_SB->base_dyn_fdc[fd_id] = fidv_val[CGU_SB_NR_FRACDIV + fd_id];
		CGU_SB->base_dyn_sel[fd_id] = fdiv_clks[CGU_SB_NR_FRACDIV + fd_id];
	}

	for (bcr_id = 0; bcr_id < CGU_SB_NR_BCR; bcr_id++) {
		/* enable BCR for domain */
		CGU_SB->base_bcr[bcr_id] = CGU_SB_BCR_FD_RUN;
	}
	/* select input for domain. All have FFAST so just domains which need PLL clocks.*/
	cgu_set_base_freq(CGU_SB_SYS_BASE_ID, CGU_FIN_SELECT_HPPLL1);
	cgu_set_base_freq(CGU_SB_AHB0_APB2_BASE_ID, CGU_FIN_SELECT_HPPLL1);
	cgu_set_base_freq(CGU_SB_PCM_BASE_ID, CGU_FIN_SELECT_HPPLL1);
	cgu_set_base_freq(CGU_SB_SPI_CLK_BASE_ID, CGU_FIN_SELECT_HPPLL1);
	cgu_set_base_freq(CGU_SB_CLK1024FS_BASE_ID, CGU_FIN_SELECT_HPPLL0);
	cgu_set_base_freq(CGU_SB_I2SRX_BCK0_BASE_ID, CGU_FIN_SELECT_XT_I2SRX_BCK0);
	cgu_set_base_freq(CGU_SB_I2SRX_BCK1_BASE_ID, CGU_FIN_SELECT_XT_I2SRX_BCK1);


	return 0;
}

void cgu_clk_set_exten(u32 clkid, ulong enable)
{
	switch (clkid) {
	case CGU_SB_OTP_PCLK_ID:
	case CGU_SB_PCM_APB_PCLK_ID:
	case CGU_SB_EVENT_ROUTER_PCLK_ID:
	case CGU_SB_ADC_PCLK_ID:
	case CGU_SB_IOCONF_PCLK_ID:
	case CGU_SB_CGU_PCLK_ID:
	case CGU_SB_SYSCREG_PCLK_ID:
	case CGU_SB_DMA_CLK_GATED_ID:
	case CGU_SB_SPI_PCLK_GATED_ID:
	case CGU_SB_SPI_CLK_GATED_ID:
	case CGU_SB_PCM_CLK_IP_ID:
	case CGU_SB_PWM_PCLK_REGS_ID:
		if (enable) {
			CGU_SB->clk_pcr[clkid] |= CGU_SB_PCR_EXTEN_EN;
		}
		else {
			CGU_SB->clk_pcr[clkid] &= ~CGU_SB_PCR_EXTEN_EN;
		}
		break;
		/* force disable for the following clocks */
	case CGU_SB_I2C0_PCLK_ID:
	case CGU_SB_I2C1_PCLK_ID:
	case CGU_SB_WDOG_PCLK_ID:
	case CGU_SB_UART_APB_CLK_ID:
	case CGU_SB_LCD_PCLK_ID:
		CGU_SB->clk_pcr[clkid] &= ~CGU_SB_PCR_EXTEN_EN;
		break;
	default:
		break;
	}
}

void cgu_set_base_freq(u32 baseid, ulong  fin_sel)
{
	ulong baseSCR;

	/* Switch configuration register*/
	baseSCR = CGU_SB->base_scr[baseid] & ~CGU_SB_SCR_FS_MASK;
	/* If fs1 is currently enabled set refId to fs2 and enable fs2*/
	if (CGU_SB->base_ssr[baseid] & CGU_SB_SCR_EN1) {
		/* check if the selcted frequency is same as requested. If not switch.*/
		if (CGU_SB->base_fs1[baseid] != fin_sel) {
			CGU_SB->base_fs2[baseid] = fin_sel;

			/* Don't touch stop bit in SCR register*/
			CGU_SB->base_scr[baseid] = baseSCR | CGU_SB_SCR_EN2;
		}
	}
	else {
		/* check if the selcted frequency is same as requested. If not switch.*/
		if (CGU_SB->base_fs2[baseid] != fin_sel) {
			CGU_SB->base_fs1[baseid] = fin_sel;

			/* Don't touch stop bit in SCR register*/
			CGU_SB->base_scr[baseid] = baseSCR | CGU_SB_SCR_EN1;
		}
	}
}

ulong cgu_get_clk_freq(u32 clkid)
{
	ulong freq = 0;
	u32 domainId;
	ulong subDomainId;
	long n, m;
	ulong fdcVal;

	/* get domain and frac div info for the clock */
	cgu_ClkId2DomainId(clkid, &domainId, &subDomainId);

	/* get base frequency for the domain */
	if(CGU_SB_SSR_FS_GET(CGU_SB->base_ssr[domainId]) == CGU_FIN_SELECT_FFAST)
		freq = XTAL_IN;
	else if (CGU_SB_SSR_FS_GET(CGU_SB->base_ssr[domainId]) == CGU_FIN_SELECT_HPPLL1)
		freq = PLL_FREQUENCY;
	else
		freq = 0;

	/* direct connection has no fraction divider*/
	if (subDomainId == CGU_INVALID_ID) {
		return freq;
	}
	/* read frac div control register value */
	fdcVal = CGU_SB->base_fdc[subDomainId];

	/* Is the fracdiv enabled ?*/
	if (fdcVal & CGU_SB_FDC_RUN) {
		long msub, madd;

		if (subDomainId != CGU_SB_BASE7_FDIV_LOW_ID) {
			msub = CGU_SB_FDC_MSUB_GET(fdcVal);
			madd = CGU_SB_FDC_MADD_GET(fdcVal);
		}
		else {
			msub = CGU_SB_FDC17_MSUB_GET(fdcVal);
			madd = CGU_SB_FDC17_MADD_GET(fdcVal);
		}

		/* remove trailing zeros */
		while (!(msub & 1)  && !(madd & 1)) {
			madd = madd >> 1;
			msub = msub >> 1;
		}
		/* compute m and n values */
		n = - msub;
		m = madd + n;
		/* check m and n are non-zero values */
		if ((n == 0) || (m == 0)) {
			return 0;
		}
		/* calculate the frequency based on m and n values */
		freq = (freq * n) / m ;
	}

	return freq;
}

void cgu_hpll_config(u32 pllid, CGU_HPLL_SETUP_T *pllinfo)
{
	CGU_HP_CFG_REGS* hppll;
	ulong switched_domains = 0;
	u32 domainId;

	/**********************************************************
	 * switch domains connected to HPLL to FFAST automatically
	 ***********************************************************/
	for (domainId = CGU_SB_BASE_FIRST; domainId < CGU_SB_NR_BASE;
			 domainId++) {
		if (CGU_SB_SSR_FS_GET(CGU_SB->base_ssr[domainId]) ==
				(CGU_FIN_SELECT_HPPLL0 + pllid)) {
			/* switch reference clock in to FFAST */
			cgu_set_base_freq(domainId, CGU_FIN_SELECT_FFAST);
			/* store the domain id to switch back to HPLL */
			switched_domains |= (1<<domainId);
		}
	}

	/* get PLL regs */
	hppll = &CGU_CFG->hp[pllid];

	/* disable clock, disable skew enable, power down pll,
	 * (dis/en)able post divider, (dis/en)able pre-divider,
	 * disable free running mode, disable bandsel,
	 * enable up limmiter, disable bypass
	 */
	hppll->mode = CGU_HPLL_MODE_PD;

	/* Select fin */
	hppll->fin_select = pllinfo->fin_select;

	/* M divider */
	hppll->mdec = pllinfo->mdec & CGU_HPLL_MDEC_MASK;

	/* N divider */
	hppll->ndec = pllinfo->ndec & CGU_HPLL_NDEC_MSK;

	/* P divider */
	hppll->pdec = pllinfo->pdec & CGU_HPLL_PDEC_MSK;

	/* Set bandwidth */
	hppll->selr = pllinfo->selr;
	hppll->seli = pllinfo->seli;
	hppll->selp = pllinfo->selp;

	/* Power up pll */
	hppll->mode = (pllinfo->mode & ~CGU_HPLL_MODE_PD) | CGU_HPLL_MODE_CLKEN;

	/* wait for PLL to lock */
	while ((hppll->status & CGU_HPLL_STATUS_LOCK) == 0);

	 /* 
	  * switch domains back to HPLL
	  */
	for (domainId = CGU_SB_BASE_FIRST; domainId < CGU_SB_NR_BASE; domainId++) {
		if (switched_domains & (1<<domainId)) {
			/* switch reference clock in to HPLL */
			cgu_set_base_freq(domainId, CGU_FIN_SELECT_HPPLL0 + pllid);
		}
	}

}

void cgu_soft_reset_module(CGU_MOD_ID_T modId)
{
	volatile ulong i;
	volatile ulong * pmod_rst_reg = &(CGU_CFG->apb0_resetn_soft);

	/* All the reset registers are continously mapped with an address difference of 4 */
	pmod_rst_reg += modId;

	/* clear and set the register */
	*pmod_rst_reg = 0;
	/* introduce some delay */
	for (i = 0;i < 1000;i++);

	*pmod_rst_reg = CGU_CONFIG_SOFT_RESET;
}


void init_clocks(u32 fidv_val[], u32 fdiv_clks[])
{
	CGU_HPLL_SETUP_T pllinfo;

#ifdef CONFIG_PLL_270
	/* settings for 270MHz */
	pllinfo.fin_select = CGU_FIN_SELECT_FFAST;
	pllinfo.ndec = 514;
	pllinfo.mdec = 19660;
	pllinfo.pdec = 98;
	pllinfo.selr = 0;
	pllinfo.seli = 48;
	pllinfo.selp = 23;
	pllinfo.mode = 0;

#else
	/* settings for 180MHz */
	pllinfo.fin_select = CGU_FIN_SELECT_FFAST;
	pllinfo.ndec = 770;
	pllinfo.mdec = 8191;
	pllinfo.pdec = 98;
	pllinfo.selr = 0;
	pllinfo.seli = 16;
	pllinfo.selp = 8;
	pllinfo.mode = 0;
#endif
	cgu_reset_all_clks();
	cgu_hpll_config(CGU_HPLL1_ID, &pllinfo);
	cgu_init_clks(fidv_val, fdiv_clks);
}
