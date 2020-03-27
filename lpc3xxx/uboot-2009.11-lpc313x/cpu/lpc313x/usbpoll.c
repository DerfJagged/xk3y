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
#include <config.h>

#ifdef CONFIG_USB_DFU
ulong dfu_load_addr = 0x32000000;
extern unsigned char stop_polling;
extern unsigned char xfer_error;

int do_usbpoll (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 i = 500;
	int ret = 0;

	if(argc < 2)
		goto usage;
	stop_polling = 0;
	xfer_error = 0;

	dfu_load_addr = (ulong)simple_strtoul(argv[1], NULL, 16);

	while(!stop_polling && (!xfer_error)) {
		usbtty_poll();
	}
	/* do usbtty_poll() for the DFU Class status request to complete */
	while(i && (!xfer_error)) {
		usbtty_poll();
		i--;
	}
	return 0;
usage:
	cmd_usage(cmdtp);
	return 0;
}
#else
int do_usbpoll (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return 0;
}
#endif

U_BOOT_CMD(
		usbpoll, 2, 1, do_usbpoll,
		"Poll USB bus Transfering data using DFU Protocol",
		"usbpoll addr\n"
		""
	  );
