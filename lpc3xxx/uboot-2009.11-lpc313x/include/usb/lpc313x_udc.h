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

#ifndef __USBDCORE_LPC313X_H__
#define __USBDCORE_LPC313X_H__

#include <asm/arch/hardware.h>

/* USB Module Register Structure */
typedef volatile struct
{
	/*Capability registers*/
	volatile u8 caplength;
	volatile u8 _otg1[1];
	volatile u16 hciversion;
	volatile u32 hcsparams;
	volatile u32 hccparams;
	volatile u32 _otg2[5];
	volatile u16 dciversion;
	volatile u16 _otg3[1];
	volatile u32 dccparams;
	volatile u32 _otg4a[4];
	volatile u32 usb_up_int;
	volatile u32 _otg4b[1];
	/* Operational registers */
	volatile u32 usbcmd;
	volatile u32 usbsts;
	volatile u32 usbintr;
	volatile u32 frindex;
	volatile u32 _otg5[1];
	volatile u32 periodiclistbase__deviceaddr;
	volatile u32 asynclistaddr__endpointlistaddr;
	volatile u32 ttctrl;
	volatile u32 burstsize;
	volatile u32 txfilltuning;
	volatile u32 txttfilltuning;
	volatile u32 _otg6[1];
	volatile u32 _otg7[2];
	volatile u32 endptnak;
	volatile u32 endptnaken;
	volatile u32 configflag;
	volatile u32 portsc1;
	volatile u32 _otg8[7];
	volatile u32 otgsc;
	volatile u32 usbmode;
	volatile u32 endptsetupstat;
	volatile u32 endptprime;
	volatile u32 endptflush;
	volatile u32 endptstatus;
	volatile u32 endptcomplete;
	volatile u32 endptctrl[4];
} USB_OTG_REGS_T;

/* dTD Transfer Description */
typedef volatile struct
{
	volatile u32 next_dTD;
	volatile u32 total_bytes;
	volatile u32 buffer0;
	volatile u32 buffer1;
	volatile u32 buffer2;
	volatile u32 buffer3;
	volatile u32 buffer4;
	volatile u32 reserved;
} DTD_T;

/* dQH Queue Head */
typedef volatile struct
{
	volatile u32 cap;
	volatile u32 curr_dTD;
	volatile u32 next_dTD;
	volatile u32 total_bytes;
	volatile u32 buffer0;
	volatile u32 buffer1;
	volatile u32 buffer2;
	volatile u32 buffer3;
	volatile u32 buffer4;
	volatile u32 reserved;
	volatile u32 setup[2];
	volatile u32 gap[4];
} DQH_T;

/* bit defines for USBCMD register */
#define USBCMD_RS		_BIT(0)
#define USBCMD_RST		_BIT(1)
#define USBCMD_ATDTW		_BIT(12)
#define USBCMD_SUTW		_BIT(13)

/* bit defines for USBSTS register */
#define USBSTS_UI		_BIT(0)
#define USBSTS_UEI		_BIT(1)
#define USBSTS_PCI		_BIT(2)
#define USBSTS_URI		_BIT(6)
#define USBSTS_SRI		_BIT(7)
#define USBSTS_SLI		_BIT(8)
#define USBSTS_NAKI		_BIT(16)

/* bit defines for DEVICEADDR register */
#define USBDEV_ADDR_AD		_BIT(24)
#define USBDEV_ADDR(n)		_SBF(25, ((n) & 0x7F))

/* bit defines for PRTSC1 register */
#define USBPRTS_CCS		_BIT(0)
#define USBPRTS_PE		_BIT(2)
#define USBPRTS_FPR		_BIT(6)
#define USBPRTS_SUSP		_BIT(7)
#define USBPRTS_PR		_BIT(8)
#define USBPRTS_HSP		_BIT(9)
#define USBPRTS_PLPSCD		_BIT(23)
#define USBPRTS_PFSC		_BIT(24)

/* bit defines for USBMODE register */
#define USBMODE_CM_IDLE		_SBF(0, 0x0)
#define USBMODE_CM_DEV		_SBF(0, 0x2)
#define USBMODE_CM_HOST		_SBF(0, 0x3)
#define USBMODE_SLOM		_BIT(3)
#define USBMODE_SDIS		_BIT(4)

/* bit defines for EP registers*/
#define USB_EP_BITPOS(n) (((n) & 0x80)? (0x10 | ((n) & 0x7)) : ((n) & 0x7))

/* bit defines EPcontrol registers*/
#define EPCTRL_RXS		_BIT(0)
#define EPCTRL_RX_TYPE(n)	_SBF(2,((n) & 0x3))
#define EPCTRL_RX_CTL		_SBF(2,0)
#define EPCTRL_RX_ISO		_SBF(2,1)
#define EPCTRL_RX_BLK		_SBF(2,2)
#define EPCTRL_RXI		_BIT(5)
#define EPCTRL_RXR		_BIT(6)
#define EPCTRL_RXE		_BIT(7)
#define EPCTRL_TXS		_BIT(16)
#define EPCTRL_TX_TYPE(n) _SBF(18,((n) & 0x3))
#define EPCTRL_TX_CTL		_SBF(18,0)
#define EPCTRL_TX_ISO		_SBF(18,1)
#define EPCTRL_TX_BLK		_SBF(18,2)
#define EPCTRL_TX_INT		_SBF(18,3)
#define EPCTRL_TXI		_BIT(21)
#define EPCTRL_TXR		_BIT(22)
#define EPCTRL_TXE		_BIT(23)

/* dQH field and bit defines */
/* Temp fixed on max, should be taken out of table */
#define QH_MAX_CTRL_PAYLOAD	0x03ff
#define QH_MAX_PKT_LEN_POS	16
#define QH_MAXP(n)		_SBF(16,((n) & 0x3FF))
#define QH_IOS			_BIT(15)
#define QH_ZLT			_BIT(29)

/* dTD field and bit defines */
#define TD_NEXT_TERMINATE	_BIT(0)
#define TD_IOC			_BIT(15)

/* Total physical enpoints*/
#define EP_NUM_MAX		8

/**********************************************************************
 * * Macro to access USB_OTG registers
 * **********************************************************************/
#define USB_OTG	((USB_OTG_REGS_T *)(USBOTG_BASE + 0x100))

/* LPC313X Endpoint parameters */
#define EP0_MAX_PACKET_SIZE	64
#define UDC_OUT_ENDPOINT	2
#define UDC_OUT_PACKET_SIZE	64
#define UDC_IN_ENDPOINT		3
#define UDC_IN_PACKET_SIZE	64
#define UDC_INT_ENDPOINT	5
#define UDC_INT_PACKET_SIZE	64
#define UDC_BULK_PACKET_SIZE	512

/* UDC Endpoints events */
#define USB_EVT_SETUP		1
#define USB_EVT_OUT_COMPLETE	2
#define USB_EVT_IN_COMPLETE	3
#define USB_EVT_OUT_NAK		4
#define USB_EVT_IN_NAK		5
#define USB_EVT_OUT_STALL	6
#define USB_EVT_IN_STALL	7

/* States for Endpoint0 during Control X'fer */
#define EP0_IDLE		0
#define EP0_IN_DATA		1
#define EP0_OUT_DATA		2
#define EP0_STATUS_OUT		3
#define EP0_STATUS_IN		4

void udc_irq (void);
/* Flow control */
void udc_set_nak(int epid);
void udc_unset_nak (int epid);

/* Higher level functions for abstracting away from specific device */
int udc_endpoint_write(struct usb_endpoint_instance *endpoint);

int udc_init (void);

void udc_enable(struct usb_device_instance *device);
void udc_disable(void);

void udc_connect(void);
void udc_disconnect(void);

void udc_startup_events(struct usb_device_instance *device);
void udc_setup_ep(struct usb_device_instance *device, unsigned int ep,
			struct usb_endpoint_instance *endpoint);

#endif
