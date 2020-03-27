#ifndef _DFU_H
#define _DFU_H

/* USB Device Firmware Update Implementation for u-boot
 * (C) 2007 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * based on: USB Device Firmware Update Implementation for OpenPCD
 * (C) 2006 by Harald Welte <hwelte@hmw-consulting.de>
 *
 * This ought to be compliant to the USB DFU Spec 1.0 as available from
 * http://www.usb.org/developers/devclass_docs/usbdfu10.pdf
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <asm/types.h>
#include <usbdescriptors.h>
#include <usb_dfu_descriptors.h>
#include <config.h>

/* USB DFU functional descriptor */
#define DFU_FUNC_DESC  {						\
	.bLength		= USB_DT_DFU_SIZE,			\
	.bDescriptorType	= USB_DT_DFU,				\
	.bmAttributes		= USB_DFU_CAN_DOWNLOAD | USB_DFU_CAN_UPLOAD | USB_DFU_MANIFEST_TOL, \
	.wDetachTimeOut		= 0xff00,				\
	.wTransferSize		= CONFIG_USBD_DFU_XFER_SIZE,		\
	.bcdDFUVersion		= 0x0100,				\
}

/* USB Interface descriptor in Runtime mode */
#define DFU_RT_IF_DESC	{						\
	.bLength		= USB_DT_INTERFACE_SIZE,		\
	.bDescriptorType	= USB_DT_INTERFACE,			\
	.bInterfaceNumber	= CONFIG_USBD_DFU_INTERFACE,		\
	.bAlternateSetting	= 0x00,					\
	.bNumEndpoints		= 0x00,					\
	.bInterfaceClass	= 0xfe,					\
	.bInterfaceSubClass	= 0x01,					\
	.bInterfaceProtocol	= 0x01,					\
	.iInterface		= DFU_STR_CONFIG,			\
}

#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

#ifndef DFU_NUM_ALTERNATES
#define DFU_NUM_ALTERNATES	1
#endif

#define DFU_STR_MANUFACTURER	STR_MANUFACTURER
#define DFU_STR_PRODUCT		STR_PRODUCT
#define DFU_STR_SERIAL		STR_SERIAL
#define DFU_STR_CONFIG		(STR_COUNT)
#define DFU_STR_ALT(n)		(STR_COUNT+(n)+1)
#define DFU_STR_COUNT		DFU_STR_ALT(DFU_NUM_ALTERNATES)

#define CONFIG_DFU_CFG_STR	"USB Device Firmware Upgrade"
#define CONFIG_DFU_ALT0_STR	"RAM memory"

struct _dfu_desc {
	struct usb_configuration_descriptor ucfg;
	struct usb_interface_descriptor uif[DFU_NUM_ALTERNATES];
	struct usb_dfu_func_descriptor func_dfu;
};

int dfu_init_instance(struct usb_device_instance *dev);

#define DFU_EP0_NONE		0
#define DFU_EP0_UNHANDLED	1
#define DFU_EP0_STALL		2
#define DFU_EP0_ZLP		3
#define DFU_EP0_DATA		4

extern volatile enum dfu_state *system_dfu_state; /* for 3rd parties */

int dfu_ep0_handler(struct urb *urb);

void dfu_event(struct usb_device_instance *device,
	       usb_device_event_t event, int data);

#endif /* _DFU_H */
