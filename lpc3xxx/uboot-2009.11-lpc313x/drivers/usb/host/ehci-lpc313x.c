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

#include <asm/arch/hardware.h>
#include <common.h>
#include <usb.h>

#include "ehci.h"
#include "ehci-core.h"

/* board specific externa function*/
extern int lpc31xx_enable_vbus(int en);

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(void)
{
	uint32_t cmd;
	int ret = 0;

	hccr = (struct ehci_hccr *)(USBOTG_BASE + 0x100);
	hcor = (struct ehci_hcor *)((uint32_t) hccr
			+ HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	printf("LPC31xx init hccr %x and hcor %x hc_length %d\n",
			(uint32_t)hccr, (uint32_t)hcor,
			(uint32_t)HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	/* now enable VBUS power */
	ret = lpc31xx_enable_vbus(1);
	/* reset the controller */
	ehci_writel(USBOTG_BASE + 0x140, 0x2);
	/* wait for reset to complete */
	do {
		cmd = ehci_readl(USBOTG_BASE + 0x140);
	} while (cmd & 0x2);


	//cmd = ehci_readl(USBOTG_BASE + 0x1A8);
	//cmd |= 0x03;
	ehci_writel(USBOTG_BASE + 0x1A8, 0x03);

	return ret;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(void)
{
	/* disable VBUS power */
	return lpc31xx_enable_vbus(0);
}
