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
#include <asm/byteorder.h>
#include <usbdevice.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>
#include <asm/arch/sysreg.h>
#include <usb/lpc313x_udc.h>
#include "ep0.h"

#define DQH_BASE	(EXT_SDRAM_BASE)
#define DTD_BASE	(DQH_BASE + EP_NUM_MAX * sizeof(DQH_T))

DQH_T* const ep_QH = (DQH_T*)DQH_BASE;
DTD_T* const ep_TD = (DTD_T*)DTD_BASE;
u32 ep_read_len[4];

static struct urb *ep0_urb = NULL;
static struct usb_device_instance *udc_device;
static int ep0state = EP0_IDLE;
u8 device_address = 0;

static struct usb_endpoint_instance *lpc313x_find_ep(int ep)
{
	int i;

	for (i = 0; i < udc_device->bus->max_endpoints; i++) {
		if (udc_device->bus->endpoint_array[i].endpoint_address == ep)
			return &udc_device->bus->endpoint_array[i];
	}
	return NULL;
}

static u32 get_ep_phy_addr(u32 EPNum)
{
	u32 val;

	val = (EPNum & 0x0F) << 1;
	if (EPNum & 0x80) {
		val += 1;
	}
	return (val);
}

static u32 get_ep_logical_addr(u32 endpoint_addr)
{
	return (endpoint_addr >> 1);
}

static void udc_setAddress(u32 adr)
{
        USB_OTG->periodiclistbase__deviceaddr = USBDEV_ADDR(adr);
        USB_OTG->periodiclistbase__deviceaddr |= USBDEV_ADDR_AD;
}

static void udc_progDTD(u32 Edpt, u32 ptrBuff, u32 TsfSize)
{
	DTD_T* pDTD;

	pDTD = (DTD_T*) &ep_TD [Edpt];

	/* Zero out the device transfer descriptors */
	memset((void*)pDTD, 0, sizeof(DTD_T));
	/* The next DTD pointer is INVALID */
	pDTD->next_dTD = 0x01;

	/* Length */
	pDTD->total_bytes = ((TsfSize & 0x7fff) << 16);
	pDTD->total_bytes |= TD_IOC;
	pDTD->total_bytes |= 0x80;

	pDTD->buffer0 = ptrBuff;
	pDTD->buffer1 = (ptrBuff + 0x1000) & 0xfffff000;
	pDTD->buffer2 = (ptrBuff + 0x2000) & 0xfffff000;
	pDTD->buffer3 = (ptrBuff + 0x3000) & 0xfffff000;
	pDTD->buffer4 = (ptrBuff + 0x4000) & 0xfffff000;

	ep_QH[Edpt].next_dTD = (u32)(&ep_TD[Edpt]);
	ep_QH[Edpt].total_bytes &= (~0xC0);
}

static u32 udc_write_urb(u32 EPNum, struct usb_endpoint_instance *endpoint)
{
	u32 n = USB_EP_BITPOS(EPNum);
	struct urb *urb = endpoint->tx_urb;
	u32 length = urb->actual_length;

	if (!urb)
		return -1;

	if(length == 0x0)
		udc_progDTD(get_ep_phy_addr(EPNum), NULL, length);
	else
		udc_progDTD(get_ep_phy_addr(EPNum), (u32)(urb->buffer + endpoint->sent),
				length);
	/* prime the endpoint for transmit */
	USB_OTG->endptprime |= _BIT(n);

	/* check if priming succeeded */
	while (USB_OTG->endptprime & _BIT(n));

	endpoint->last = length;

	if((EPNum & 0x7F) != 0x0) {
		usbd_tx_complete(endpoint);
	}
	else {
		endpoint->sent += length;
		endpoint->last -= length;

		if( (urb->actual_length - endpoint->sent) <= 0 ) {
			urb->actual_length = 0;
			endpoint->sent = 0;
			endpoint->last = 0;
		}
	}
	if(endpoint->tx_urb->actual_length == 0x0 && ep0state == EP0_IN_DATA) {
		ep0state = EP0_STATUS_OUT;
	}
	else if(endpoint->tx_urb->actual_length == 0x0 && ep0state == EP0_OUT_DATA) {
		ep0state = EP0_IDLE;
	}

	return 0;
}

static u32 udc_read_urb(u32 EPNum, struct usb_endpoint_instance *endpoint)
{
	u32 bytes_read, n;
	DTD_T* pDTD;

	n = get_ep_phy_addr(EPNum);
	pDTD = (DTD_T*) &ep_TD [n];

	/* return the total bytes read */
	bytes_read = (pDTD->total_bytes >> 16) & 0x7FFF;
	bytes_read = ep_read_len[EPNum & 0x0F] - bytes_read;

	if(EPNum == 0x0) {
		ep0_urb->actual_length += bytes_read;
	}
	else {
		usbd_rcv_complete(endpoint, bytes_read, 0);
	}

	return 0;
}

static u32 udc_readReqEP(u32 EPNum, u8 *pData, u32 len) 
{
	u32 num = get_ep_phy_addr(EPNum);
	u32 n = USB_EP_BITPOS(EPNum);

	udc_progDTD(num, (u32)pData, len);
	ep_read_len[EPNum & 0x0F] = len;

	/* prime the endpoint for read */
	USB_OTG->endptprime |= _BIT(n);

	return len;
}

static void udc_setStallEP(u32 EPNum)
{
	u32 lep;

	lep = EPNum & 0x0F;
	if (EPNum & 0x80) {
		USB_OTG->endptctrl[lep] |= EPCTRL_TXS;
	} else {
		USB_OTG->endptctrl[lep] |= EPCTRL_RXS;
	}
}

static u32 udc_readSetupPkt(u32 num, u32 *pdata)
{
	u32 setup_int, cnt = 0;

	setup_int = USB_OTG->endptsetupstat;
	
	/* No setup are admitted on other endpoints than 0 */
	if (setup_int & _BIT(0)) {
		/* Clear the setup interrupt */
		USB_OTG->endptsetupstat = setup_int;

		do {
			/* Setup in a setup - must considere only the second setup */
			/*- Set the tripwire */
			USB_OTG->usbcmd |= USBCMD_SUTW;

			/* Transfer Set-up data to the gtmudsCore_Request buffer */
			pdata[0] = ep_QH[num].setup[0];
			pdata[1] = ep_QH[num].setup[1];
			cnt = 8;

		} while (!(USB_OTG->usbcmd & USBCMD_SUTW));

		/* setup in a setup - Clear the tripwire */
		USB_OTG->usbcmd &= (~USBCMD_SUTW);
	}
	return cnt;
}

static int udc_handle_dfu_req(void)
{

	struct usb_device_request *request = &ep0_urb->device_request;

	if((request->bmRequestType & 0x3f) != USB_TYPE_DFU) {
		printf("Error: not DFU Class Request\n");
		return -1;
	}

	switch(request->bRequest) {
	case USB_REQ_DFU_DNLOAD:
		break;
	case USB_REQ_DFU_DETACH:
	case USB_REQ_DFU_UPLOAD:
	case USB_REQ_DFU_GETSTATUS:
	case USB_REQ_DFU_CLRSTATUS:
	case USB_REQ_DFU_GETSTATE:
	case USB_REQ_DFU_ABORT:
		ep0_recv_setup(ep0_urb);
		break;
	default:
		break;
	}
	return 0;
}

static int udc_handle_ep0(u32 event)
{
	struct usb_endpoint_instance *endpoint;
	struct usb_device_request *request = &ep0_urb->device_request;
	u32 transfer_size = 0;

	endpoint = lpc313x_find_ep(0x00);

	switch(ep0state) {
	case EP0_IDLE:
		if(event == USB_EVT_SETUP) {
			udc_readSetupPkt(0x00, (u32 *)&ep0_urb->device_request);
			/* Handle setup stage without having data stage */
			if (ep0_urb->device_request.wLength == 0) {
				if (ep0_recv_setup(ep0_urb)) {
					usberr("!!!!!!!can't parse setup packet\n");
					goto stall;
				}
				/* Manually Set Address */
				if(ep0_urb->device_request.bRequest == USB_REQ_SET_ADDRESS) {
					//device_address = 0x80 | udc_device->address;
					usbd_device_event_irq(udc_device,
							DEVICE_ADDRESS_ASSIGNED,0);
					endpoint->tx_urb = ep0_urb;
					udc_write_urb(0x80,endpoint);
          printf("set_addr%x\n",udc_device->address);
					//udelay(1);
					//udc_setAddress(udc_device->address);
					ep0state = EP0_IDLE;
				}

				if ((ep0_urb->device_request.bmRequestType &
						USB_REQ_DIRECTION_MASK)
						== USB_REQ_DEVICE2HOST) {
					ep0state = EP0_STATUS_OUT;
				}
				else {
					if(ep0_urb->device_request.bRequest != USB_REQ_SET_ADDRESS) {
						endpoint->tx_urb = ep0_urb;
						udc_write_urb(0x80,endpoint);
						ep0state = EP0_IDLE;
					}
				}
			}
			/* Handle setup stage having data stage */
			else {
				/* Check direction */
				if ((ep0_urb->device_request.bmRequestType &
							USB_REQ_DIRECTION_MASK)
						== USB_REQ_HOST2DEVICE) {
					/* Handle DFU Class request sepratley */
					if ((request->bmRequestType & 0x3f) != USB_TYPE_DFU) {
						if (ep0_recv_setup(ep0_urb)) {
							usberr(".....can't parse SETUP packet\n");
							ep0state = EP0_IDLE;
							goto stall;
						}
					}
					else
						udc_handle_dfu_req();
					ep0state = EP0_OUT_DATA;
					ep0_urb->actual_length = 0;
				}
				else {
					if (ep0_recv_setup(ep0_urb)) {
						usberr("@@@@@@can't parse setup packet\n");
						ep0state = EP0_IDLE;
						goto stall;
					}
					ep0state = EP0_IN_DATA;
					endpoint->tx_urb = ep0_urb;
					udc_write_urb(0x80,endpoint);
				}
			}
		}
        /* temporary fix */
    else if (event == USB_EVT_IN_COMPLETE) 
      udc_setAddress(udc_device->address);

		break;
	case EP0_OUT_DATA:
		if(event == USB_EVT_OUT_NAK ) {
			if ((ep0_urb->device_request.bmRequestType & 
					USB_REQ_DIRECTION_MASK) == USB_REQ_HOST2DEVICE) {
				if ((request->bmRequestType & 0x3f) == USB_TYPE_DFU)
					transfer_size = le16_to_cpu (ep0_urb->device_request.wLength);
				else
					transfer_size = endpoint->rcv_transferSize;
				udc_readReqEP(0x00, ep0_urb->buffer +
						ep0_urb->actual_length,
						transfer_size);
			}
		}
		else if(event == USB_EVT_OUT_COMPLETE) {
			udc_read_urb(0x00,endpoint);
			if (ep0_recv_setup(ep0_urb)) {
				usberr("can't parse setup packet\n");
				ep0state = EP0_IDLE;
				goto stall;
			}
			ep0state = EP0_STATUS_IN;
		}
		break;
	case EP0_IN_DATA:
		if(event == USB_EVT_IN_COMPLETE) {
			endpoint->tx_urb = ep0_urb;
			udc_write_urb(0x80,endpoint);
		}
		break;
	case EP0_STATUS_IN:
		ep0_urb->actual_length = 0;
		endpoint->tx_urb = ep0_urb;
		udc_write_urb(0x80,endpoint);
		ep0state = EP0_IDLE;
		break;
	case EP0_STATUS_OUT:
		if(event == USB_EVT_OUT_NAK ) {
			/* might be zero length pkt */
			// printf("Zero Len\n");
			udc_readReqEP(0x00, ep0_urb->buffer, 0);
		}
		else if (event == USB_EVT_OUT_COMPLETE) {
			/* Read zero length packet */
			udc_read_urb(0x00,endpoint);
			ep0state = EP0_IDLE;
		}
		break;
	}
	return 0;
stall:
	udc_setStallEP(0x80);

	return -1;
}

void udc_handle_ep(u32 num, u32 event)
{

	struct usb_endpoint_instance *endpoint;
	u32 epnum = get_ep_phy_addr(num);
	endpoint = lpc313x_find_ep(epnum);

	if((num & 0x7F) == 0x0) {
	  udc_handle_ep0(event);
		return;
	}

	switch (event) {
	case USB_EVT_OUT_NAK:
		udc_readReqEP(num, endpoint->rcv_urb->buffer +
				endpoint->rcv_urb->actual_length,
				endpoint->rcv_packetSize);
		break;
	case USB_EVT_OUT_COMPLETE:
		udc_read_urb(num,endpoint);
		break;
	case USB_EVT_IN_COMPLETE:
		udc_write_urb(num,endpoint);
		break;
	}
}

/*
 * USB Public functions
 */
void udc_reset(void)
{
	u32 i = 0;

	/* disable all EPs */
	USB_OTG->endptctrl[1] &= ~(EPCTRL_RXE | EPCTRL_TXE);
	USB_OTG->endptctrl[2] &= ~(EPCTRL_RXE | EPCTRL_TXE);
	USB_OTG->endptctrl[3] &= ~(EPCTRL_RXE | EPCTRL_TXE);

	/* Clear all pending interrupts */
	USB_OTG->endptnak = 0xFFFFFFFF;
	USB_OTG->endptnaken = 0;
	USB_OTG->usbsts = 0xFFFFFFFF;
	USB_OTG->endptsetupstat = USB_OTG->endptsetupstat;
	USB_OTG->endptcomplete = USB_OTG->endptcomplete;
   
	/* Wait until all bits are 0 */
	while (USB_OTG->endptprime) {}
	USB_OTG->endptflush = 0xFFFFFFFF;
	while (USB_OTG->endptflush); /* Wait until all bits are 0 */

	/* Set the interrupt Threshold control interval to 0 */
	USB_OTG->usbcmd &= ~0x00FF0000;

	/* Zero out the Endpoint queue heads */
	memset((void*)ep_QH, 0, EP_NUM_MAX * sizeof(DQH_T));
	/* Zero out the device transfer descriptors */
	memset((void*)ep_TD, 0, EP_NUM_MAX * sizeof(DTD_T));
	memset((void*)ep_read_len, 0, sizeof(ep_read_len));
	/* Configure the Endpoint List Address */
	/* make sure it in on 64 byte boundary !!! */
	/* init list address */
	USB_OTG->asynclistaddr__endpointlistaddr = (u32)ep_QH;
	/* Initialize device queue heads for non ISO endpoint only */
	for (i = 0; i < EP_NUM_MAX; i++) {
		ep_QH[i].next_dTD = (u32)&ep_TD[i];
	}
	/* Enable interrupts */
	USB_OTG->usbintr = USBSTS_UI
		| USBSTS_UEI
		| USBSTS_PCI
		| USBSTS_URI
		| USBSTS_SLI
		| USBSTS_NAKI;
}

static void udc_init_endpoints(void)
{
	u32 i = 0;

	for (i = 1; i < udc_device->bus->max_endpoints; i++) {
		udc_setup_ep (udc_device, i, &udc_device->bus->endpoint_array[i]);
	}

}
void udc_irq(void)
{
	u32 disr, val, n;
	struct usb_endpoint_instance *ep0 = udc_device->bus->endpoint_array;

	disr = USB_OTG->usbsts; /* Device Interrupt Status */
	USB_OTG->usbsts = disr;

	/* Device Status Interrupt (Reset, Connect change, Suspend/Resume) */
	if (disr & USBSTS_URI) { 
		printf("USB get RESET interrupt\n");
		udc_reset();
		udc_setup_ep(udc_device,0,ep0);
		usbd_device_event_irq (udc_device, DEVICE_RESET, 0);
		udc_init_endpoints();
		goto isr_end;
	}

	if (disr & USBSTS_SLI) {
		printf("USB get suspend interrupt\n");
		goto isr_end;
	}

	if (disr & USBSTS_PCI) {
		goto isr_end;
	}

	/* handle completion interrupts */
	val = USB_OTG->endptcomplete;
	if (val) {
		USB_OTG->endptnak = val;
		for (n = 0; n < EP_NUM_MAX / 2; n++) {
			if (val & _BIT(n)) {
				udc_handle_ep(n,USB_EVT_OUT_COMPLETE);
				USB_OTG->endptcomplete = _BIT(n);
			}
			if (val & _BIT(n + 16)) {
				ep_TD [(n << 1) + 1 ].total_bytes &= 0xC0;
				udc_handle_ep(n | 0x80,USB_EVT_IN_COMPLETE);
				USB_OTG->endptcomplete = _BIT(n + 16);
			}
		}
	}

	/* handle setup status interrupts */
	val = USB_OTG->endptsetupstat;
	/* Only EP0 will have setup packets so call EP0 handler */
	if (val) {

		/* Clear the endpoint complete CTRL OUT & IN when */
		/* a Setup is received */
		USB_OTG->endptcomplete = 0x00010001;
		/* enable NAK inetrrupts */
		USB_OTG->endptnaken |= 0x00010001;
		udc_handle_ep0(USB_EVT_SETUP);
	}

	if (disr & USBSTS_NAKI) {
		val = USB_OTG->endptnak;
		val &= USB_OTG->endptnaken;
		/* handle NAK interrupts */
		if (val) {
			for (n = 0; n < EP_NUM_MAX/2; n++) {

				if (val & _BIT(n)) {
					udc_handle_ep(n,USB_EVT_OUT_NAK);
				}
				if (val & _BIT(n + 16)) {
					udc_handle_ep(n | 0x80,USB_EVT_IN_NAK);
				}
			}
			USB_OTG->endptnak = val;
		}
	}
isr_end:
	return;
}

/*
 * udc_set_nak
 *
 * Allow upper layers to signal lower layers should not accept more RX data
 */
void udc_set_nak(int ep_num)
{
	return;
}

/*
 * udc_unset_nak
 *
 * Suspend sending of NAK tokens for DATA OUT tokens on a given endpoint.
 * Switch off NAKing on this endpoint to accept more data output from host.
 */
void udc_unset_nak(int ep_num)
{
	return;
}

int udc_endpoint_write(struct usb_endpoint_instance *endpoint)
{
	return udc_write_urb(get_ep_logical_addr(endpoint->endpoint_address & 0x7F),
			endpoint);
}

/* Associate a physical endpoint with endpoint instance */
void udc_setup_ep(struct usb_device_instance *device, unsigned int id,
		struct usb_endpoint_instance *endpoint)
{
	u32 epnum, lepnum;
	u32 ep_cfg;
	u32 bitpos;

	if (!endpoint) {
		usberr("endpoint void!");
		return;
	}

	epnum = endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK;
	if (epnum >= EP_NUM_MAX) {
		usberr("unable to setup ep %d!", epnum);
		return;
	}

	if(epnum == 0 || epnum == 1) {
		/* enable ep0 IN and ep0 OUT */
		ep_QH[0].cap = QH_MAXP(EP0_MAX_PACKET_SIZE)
			| QH_IOS
			| QH_ZLT;
		ep_QH[1].cap = QH_MAXP(EP0_MAX_PACKET_SIZE)
			| QH_IOS
			| QH_ZLT;
		/* enable EP0 */
		USB_OTG->endptctrl[0] = EPCTRL_RXE | EPCTRL_RXR | EPCTRL_TXE | EPCTRL_TXR;

		return;
	}

	/* Get logical address of endpoint */
	lepnum = get_ep_logical_addr(epnum);
	ep_cfg = USB_OTG->endptctrl[lepnum];

	/* set EP type */
	if (endpoint->tx_attributes != USB_ENDPOINT_XFER_ISOC) {
		/* init EP capabilities */
		ep_QH[epnum].cap = QH_MAXP(endpoint->tx_packetSize)
			| QH_IOS
			| QH_ZLT ;
		/* The next DTD pointer is INVALID */
		ep_TD[epnum].next_dTD = 0x01 ;
	} else {
		/* init EP capabilities */
		ep_QH[epnum].cap  = QH_MAXP(0x400) | QH_ZLT ;
	}
	/* setup EP control register */
	if (endpoint->endpoint_address & 0x80) {
		ep_cfg &= ~0xFFFF0000;
		ep_cfg |= EPCTRL_TX_TYPE(endpoint->tx_attributes)
			| EPCTRL_TXR;
	} else {
		ep_cfg &= ~0xFFFF;
		ep_cfg |= EPCTRL_RX_TYPE(endpoint->rcv_attributes)
			| EPCTRL_RXR;
	}

	USB_OTG->endptctrl[lepnum] = ep_cfg;

	/* Enable Endpoint */
	if (lepnum & 0x80) {
		USB_OTG->endptctrl[lepnum] |= EPCTRL_TXE;
	} else {
		USB_OTG->endptctrl[lepnum] |= EPCTRL_RXE;
		/* enable NAK interrupt */
		bitpos = USB_EP_BITPOS(lepnum); 
		USB_OTG->endptnaken |= _BIT(bitpos);
	}

	/* Flush EP buffers */
	USB_OTG->endptflush = _BIT(bitpos);
	while (USB_OTG->endptflush & _BIT(bitpos));

	/* reset data toggles */
	if (lepnum & 0x80) {
		USB_OTG->endptctrl[lepnum] |= EPCTRL_TXE;
	} else {
		USB_OTG->endptctrl[lepnum] |= EPCTRL_RXE;
	}

	return;
}

/* Connect the USB device to the bus */
void udc_connect(void)
{
	USB_OTG->usbcmd |= USBCMD_RS;
}

/* Disconnect the USB device to the bus */
void udc_disconnect(void)
{
	USB_OTG->usbcmd &= ~USBCMD_RS;
}

void udc_shutdown (void)
{
	/* reset the controller */
	USB_OTG->usbcmd = USBCMD_RST;

	/* wait for reset to complete */
	while (USB_OTG->usbcmd & USBCMD_RST);

	/* reset USB block */
	cgu_soft_reset_module(USB_OTG_AHB_RST_N_SOFT);

	/* disable USB to AHB clock */
	cgu_clk_en_dis(CGU_SB_USB_OTG_AHB_CLK_ID, 0);

	/* disable USB OTG PLL */
	SYS_REGS->usb_atx_pll_pd_reg = 0x1;
}

/* Allow udc code to do any additional startup */
void udc_startup_events(struct usb_device_instance *device)
{
	/* The DEVICE_INIT event puts the USB device in the state STATE_INIT. */
	usbd_device_event_irq (device, DEVICE_INIT, 0);

	/* The DEVICE_CREATE event puts the USB device in the state
	 * STATE_ATTACHED.
	 */
	usbd_device_event_irq (device, DEVICE_CREATE, 0);

	udc_device = device;

	if (!ep0_urb)
		ep0_urb = usbd_alloc_urb(udc_device,
				udc_device->bus->endpoint_array);
	else
		usbinfo("ep0_urb %p already allocated", ep0_urb);
}

/* Initialize h/w stuff */
int udc_init(void)
{
	/* USB clock is already initialized in lpc313x_init().
	 * so not doing again here.
	 */
	/* enable USB AHB clock */
	cgu_clk_en_dis(CGU_SB_USB_OTG_AHB_CLK_ID, 1);

	/* reset the controller */
	USB_OTG->usbcmd = USBCMD_RST;
	/* wait for reset to complete */
	while (USB_OTG->usbcmd & USBCMD_RST);

	/* Program the controller to be the USB device controller */
	USB_OTG->usbmode = USBMODE_CM_DEV
		/*| USBMODE_SDIS*/
		| USBMODE_SLOM ;

	/* init UDC now */
	udc_reset();
	udc_setAddress(0);
	return 0;
}
