/*
 * drivers/xode/xode.c
 *
 * XODE driver for NXP LPC313X SoC platforms
 *
 * anonymous coward
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/mm.h>

#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/page.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/xode.h>
#include <mach/gpio.h>
#include <mach/fpga.h>
#include <mach/dma.h>

#include "core.h"
#include "corestates.h"
#include "corestims.h"

/* makes no difference I can detect */
#define USE_BURST_DMA


extern CommandProtocol_t CommandProtocol;
extern unsigned int TransferLength;
extern unsigned long AbortParam;
extern State_t State;
extern int Aborted;
extern int Packeted;

//IOCTLS
#define IOCTL_XODE_SELECT_COMMAND_PROTOCOL		0x100

#define IOCTL_XODE_READY_TO_XFER				0x200
#define IOCTL_XODE_COMMAND_ABORTED				0x201
#define IOCTL_XODE_COMMAND_COMPLETE				0x202
#define IOCTL_XODE_RX_COMMAND_PACKET			0x203

#define IOCTL_XODE_PKT_DATA_IN					0x210
#define IOCTL_XODE_PKT_DATA_OUT					0x211
#define IOCTL_XODE_PKT_DATA_NONE				0x212


#define IOCTL_XODE_DO_OK						0x220
#define IOCTL_XODE_DO_CHK_STATUS				0x221
#define IOCTL_XODE_DO_HANG						0x222

#define IOCTL_XODE_GET_CRC_IN					0x400
#define IOCTL_XODE_GET_CRC_OUT					0x401
#define IOCTL_XODE_SET_ICRC_OK					0x410
#define IOCTL_XODE_SET_ICRC_ERROR				0x411


#define IOCTL_WAIT_FOR_EJECT					0x600
#define IOCTL_SET_TRAY_STATE					0x601
#define IOCTL_GET_TRAY_STATE					0x602
#define IOCTL_USERLAND_UP						0x603

#define IOCTL_GET_FPGA_VERSION					0x700

#define DMASIZE		0x20000

#define DRIVER_NAME 		"xode"

static int xodeopened = 0;	

/* really don't like this, how to get it from within fileops functions? Need pdev....
platform_device *pdev =>
struct xode_device *xdev;
xdev = xodeinfo->xode_dev;

There's only ever one of these so...... who cares? 
*/
static struct xode_device *_xdev;	

DECLARE_COMPLETION(crccomplete);

static int xode_major;
static DEFINE_IDR(xode_idr);
static struct class * xode_class;


XODE_FPGA_REGS_t * xode_reg;

/* really needed? */
unsigned char task_file[0x10];
static unsigned char command_packet[0x20];


DECLARE_COMPLETION(readcomplete);
DECLARE_COMPLETION(traystatecomplete);


struct xode_device {
	struct module		*owner;
	struct device		*dev;
	int					 minor;
	unsigned int		 fpga_irq;
	unsigned int		 eject_irq;
	int					 dma_ch;
	unsigned char		*dma_port;
	dma_setup_t 		 dma_setup;
};

/*******************************************************************
Setup subsequent transfers in 2K blocks.
*******************************************************************/
static void xode_dma_irq(int ch, dma_irq_type_t dtype, void *handle)
{

	struct xode_device *xdev = handle;

	dma_stop_channel(xdev->dma_ch);
	dma_set_irq_mask(xdev->dma_ch, 1, 1);

	STIM(LPCTransferComplete)
}

/*******************************************************************
Setup first transfer, can send upto 2K bytes. I less than 2K only 1 transfer occurs
*******************************************************************/
void xode_dma_transfer( int type )
{
//	if(dir == DMA_IN)
//	{
//		xode_reg->fpga_dma_length = (TransferLength / 2) - 1;
//	}
	
	if(TransferLength > DMASIZE)
	{
		printk("*** > 128K LENGTH DMA XFER (%X) ***\n",TransferLength);
		return;
	}
	else if(TransferLength < 2)
	{
		printk("*** BAD LENGTH DMA XFER (%X) ***\n",TransferLength);
		return;
	}

#ifdef USE_BURST_DMA
		/* Whole Sectors only & > 1 sector or we screw up READ DVD STRUCT commands with length of 0x804*/
		if(TransferLength >= 0x1000) // IMPROVE - && (TransferLength & 0x0F == 0)
		{
			_xdev->dma_setup.cfg = DMA_CFG_TX_BURST | DMA_CFG_RD_SLV_NR(0) | DMA_CFG_WR_SLV_NR(0);
			_xdev->dma_setup.trans_length 	= (TransferLength >> 4) - 1;
		}
		else
#endif
		{
			_xdev->dma_setup.cfg = DMA_CFG_TX_HWORD | DMA_CFG_RD_SLV_NR(0) | DMA_CFG_WR_SLV_NR(0);
			_xdev->dma_setup.trans_length 	= (TransferLength >> 1) - 1;
		}


	switch(type)
	{
		case DMA_IN:
		case PIO_IN:
			xode_reg->fpga_dma_length = (TransferLength / 2) - 1;
			_xdev->dma_setup.src_address	= ISRAM0_PHYS;
			_xdev->dma_setup.dest_address	= EXT_SRAM1_PHYS;
			break;
			
		case DMA_OUT:
			_xdev->dma_setup.dest_address	= ISRAM0_PHYS;
			_xdev->dma_setup.src_address	= EXT_SRAM1_PHYS;
			break;

		case PIO_OUT:
			_xdev->dma_setup.dest_address	= ISRAM0_PHYS;
			_xdev->dma_setup.src_address	= EXT_SRAM1_PHYS+0x400;
			break;
	}

	dma_prog_channel(_xdev->dma_ch, &_xdev->dma_setup);
	/* logic is inverted, pass 0 to enable, 1 to disable */
	dma_set_irq_mask(_xdev->dma_ch, 1, 0);
	dma_start_channel(_xdev->dma_ch);
}


/* Protect idr accesses */
static DEFINE_MUTEX(minor_lock);

/*******************************************************************
*******************************************************************/
static int xode_open(struct inode *inode, struct file *filep)
{
	xodeopened++;
	return 0;
}
/*******************************************************************
*******************************************************************/
static int xode_release(struct inode *inode, struct file *filep)
{
	xodeopened--;
	return 0;
}
/*******************************************************************
*******************************************************************/
static ssize_t xode_read(struct file *filep, char __user *buf,
			size_t count, loff_t *ppos)
{
	wait_for_completion(&readcomplete);
	init_completion(&readcomplete);

	switch(State)
	{

		
		/* Read the data out part of a packet command*/
		case State_DPD4OUT:
		case State_DP4OUT:
//		case State_DI1:
			/* no need to actually copy anything, its already in ISRAM */
			if(Aborted)
			{
				Aborted = 0;
				printk("Aborted read data out\n");
				return -EAGAIN;
			}
			break;
	
		/* Read AT Command (Task File) */
		case State_DIA:
			if(Aborted)
			{
				Aborted = 0;
				printk("Aborted read ATA Command\n");
				return -EAGAIN;
			}
			if(copy_to_user(buf, task_file, 16) < 0) 
				return -EAGAIN;
			if(Packeted)
			{
				Packeted = 0;
				STIM(PacketOpt);
				return 0;
			}
			break;

		/* Read the Command Packet */
		case State_DPD2:
		case State_DP2:
			if(Aborted)
			{
				Aborted = 0;
				printk("Aborted read cmd packet\n");
				return -EAGAIN;
			}
			if(copy_to_user(buf, command_packet, 0x20) < 0) 
				return -EAGAIN;
			break;


		default:
			printk("*** READ COMPLETE IN BAD STATE!!! 0x%X %d\n",State,Aborted);
			Aborted = 0;
			return -EAGAIN;
			break;


	}
	return count;
}
/*******************************************************************

*******************************************************************/
static ssize_t xode_write(struct file *filep, const char __user *buf,
			size_t count, loff_t *ppos)
{
//	unsigned char tmp[12];

//	copy_from_user(tmp, buf, 12);
	
	return count;
}
/*******************************************************************
*******************************************************************/
static int xode_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long param)
{
	int retval = 0;
	switch(cmd)
	{
		case IOCTL_XODE_SELECT_COMMAND_PROTOCOL:	
			CommandProtocol = (CommandProtocol_t)param;
			STIM(CommandProtocolSelected)
			break;

		case IOCTL_XODE_READY_TO_XFER:
			TransferLength = param;
			STIM(ReadyToTransfer)
			break;

		case IOCTL_XODE_PKT_DATA_IN:
			TransferLength = param;
			STIM(DMADataIn)
			break;

		case IOCTL_XODE_PKT_DATA_OUT:
			TransferLength = param;
			STIM(DMADataOut)
			break;

		case IOCTL_XODE_PKT_DATA_NONE:
			STIM(DMADataNone)
			break;

		case IOCTL_XODE_COMMAND_ABORTED:
			AbortParam = param;
			STIM(CommandAborted)
			break;
		case IOCTL_XODE_COMMAND_COMPLETE:
			STIM(CommandComplete)
			break;
		case IOCTL_XODE_RX_COMMAND_PACKET:
			STIM(ReadyToRxCommandPacket)
			break;

		case IOCTL_XODE_DO_OK:
			STIM(DataOutOk)
			break;
		case IOCTL_XODE_DO_HANG:
			STIM(DataOutHang)
			break;
			
		case IOCTL_XODE_DO_CHK_STATUS:
			STIM(DataOutChk)
			break;
			
		case IOCTL_XODE_SET_ICRC_OK:
			STIM(CRCOk)
			break;
		
		case IOCTL_XODE_SET_ICRC_ERROR:
			STIM(CRCFail)
			break;


		case IOCTL_XODE_GET_CRC_IN:
			wait_for_completion(&crccomplete);
			init_completion(&crccomplete);
			if(Aborted)
			{
				printk("CRC IN Aborted\n");
				Aborted = 0;
				retval = 0xFFFFFFFF;
			}
			else
			{
				/* 	Make the pass/fail decision here, pass the
					result upto userland. Waiting for the LPC DMA 
					irq to complete first keeps everything in sync	*/
				retval = xode_reg->fpga_crc_status & 0x01;
				if(retval)
					STIM(CRCOk)
				else
					STIM(CRCFail)	
			}
			break;

		case IOCTL_XODE_GET_CRC_OUT:
			wait_for_completion(&crccomplete);
			init_completion(&crccomplete);
			if(Aborted)
			{
				printk("CRC OUT Aborted\n");
				Aborted = 0;
				retval = 0xFFFFFFFF;
			}
			else
				retval = (param == xode_reg->fpga_crc) ? 1 : 0;
			break;

		case IOCTL_WAIT_FOR_EJECT:
			wait_for_completion(&traystatecomplete);
			init_completion(&traystatecomplete);
			retval = (xode_reg->fpga_alt_cause & 0x100) >> 8;
			break;
			
		case IOCTL_SET_TRAY_STATE:
			CoreSetTrayState(param);
			break;

		case IOCTL_GET_TRAY_STATE:
			retval = (xode_reg->fpga_alt_cause & 0x100) >> 8;
			break;

		case IOCTL_GET_FPGA_VERSION:
			retval = xode_reg->fpga_id;
			break;
			
		case IOCTL_USERLAND_UP:
			enable_irq(_xdev->fpga_irq);
			CoreUserLandUp();
			break;
		default:
			break;
	}
	return retval;
}
/*******************************************************************
*******************************************************************/
static int xode_mmap(struct file *file, struct vm_area_struct * vma)
{
	/* offset is always 0, map in internal RAM - 0 wait states :) */
	vma->vm_pgoff = io_p2v(ISRAM0_PHYS) >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_pfn_range(vma, 
						vma->vm_start, 
						ISRAM0_PHYS >> PAGE_SHIFT,
			     		vma->vm_end - vma->vm_start, 
			     		vma->vm_page_prot)<0)
	{
		printk("mmap failed\n");	     			
		return -EAGAIN;
	}
	return 0;
}
/*******************************************************************
*******************************************************************/
static const struct file_operations xode_fops = {
	.owner		= THIS_MODULE,
	.open		= xode_open,
	.release	= xode_release,
	.read		= xode_read,
	.write		= xode_write,
	.ioctl		= xode_ioctl,
	.mmap		= xode_mmap,
};
#if 0
/*******************************************************************
*******************************************************************/
static irqreturn_t eject_interrupt(int irq, void *dev_id)
{
	struct xode_device *xdev = (struct xode_device *) dev_id;
	int level = gpio_get_value(GPIO_EJECT);
	
	set_irq_type(xdev->eject_irq, level ? IRQ_TYPE_LEVEL_LOW : IRQ_TYPE_LEVEL_HIGH);

	complete(&traystatecomplete);

	return IRQ_HANDLED;
}
#endif
/*******************************************************************
*******************************************************************/
static irqreturn_t xode_interrupt(int irq, void *dev_id)
{
	volatile unsigned short icause;
	unsigned short c = 0;
	int i;
	unsigned long flags;
	
	local_irq_save(flags);

	do {
		i = 0;
		icause = xode_reg->fpga_status; 

		if(c==0)
			c = icause;

		if(icause & FPGA_INT_IDE_RESET)
		{
			xode_reg->fpga_status = FPGA_INT_ALL; 
			printk("*** IRQ : HW RESET (0x%04X)\n",icause);
			CoreHWReset();
			return IRQ_HANDLED;
		}

		if(icause & FPGA_INT_DEV_RESET)
		{
			xode_reg->fpga_status = FPGA_INT_ALL; 
			printk("IRQ : SW RESET (0x%04X)\n",icause);
			CoreHWReset();
			return IRQ_HANDLED;
		}

		if(icause & FPGA_INT_INTRQ_CLEARED)
		{
			xode_reg->fpga_status = FPGA_INT_INTRQ_CLEARED; 
			STIM(StatusRegisterRead)
			i++;
		}
		
		if(icause & FPGA_INT_CMD_WRITE)
		{
			xode_reg->fpga_status = FPGA_INT_CMD_WRITE; 
			memcpy(task_file, &xode_reg->data_reg, 16);
			STIM(CommandRegisterWritten)
			i++;
		}

		if(icause & FPGA_INT_PIO_XFER_DONE)
		{
			xode_reg->fpga_status = FPGA_INT_PIO_XFER_DONE; 
			STIM(PIOXferDone)
			i++;
		}

		if(icause & FPGA_INT_DACK_GRANTED)
		{
			xode_reg->fpga_status = FPGA_INT_DACK_GRANTED; 
			STIM(DACKGranted)
			i++;
		}

		if(icause & FPGA_INT_DMA_OUT_COMPLETE)
		{
			xode_reg->fpga_status = FPGA_INT_DMA_OUT_COMPLETE; 
			STIM(DMATransferOutComplete)
			i++;
		}

		if(icause & FPGA_INT_PKT_WRITTEN)
		{
			xode_reg->fpga_status = FPGA_INT_PKT_WRITTEN; 
			memcpy(command_packet, &xode_reg->shadow, 0x20);
			STIM(CommandPacketTransferComplete)
			i++;
		}
	}while(i);


	if(icause & FPGA_INT_ALT_CAUSE)
	{
		icause = xode_reg->fpga_alt_cause;
		/* 
		Eject pin, low = close, high = open
		0->1	
		*/
		if(icause & 0x01)
		{
			complete(&traystatecomplete);
			xode_reg->fpga_alt_cause |= 0x01;
		}
		/* 
		Eject pin, low = close, high = open
		1->0	
		*/
		if(icause & 0x02)
		{
			complete(&traystatecomplete);
			xode_reg->fpga_alt_cause |= 0x02;
		}

		if(icause & 0x10)
		{
			printk("*** PIO ERR ***\n");
			xode_reg->fpga_alt_cause = 0x10;
		}

		if(icause & 0xF800)
		{
			printk("*** ALT BITS %X ***\n",icause);
			xode_reg->fpga_alt_cause = icause & 0xF800;
		}
	}
	local_irq_restore(flags);

	return IRQ_HANDLED;
}
/*******************************************************************
*******************************************************************/
static int xode_major_init(void)
{
	xode_major = register_chrdev(0, "xode", &xode_fops);
	if (xode_major < 0)
		return xode_major;
	return 0;
}

/*******************************************************************
*******************************************************************/
static void xode_major_cleanup(void)
{
	unregister_chrdev(xode_major, "xode");
}

/*******************************************************************
*******************************************************************/
static int xode_get_minor(struct xode_device *xdev)
{
	int retval = -ENOMEM;
	int id;

	mutex_lock(&minor_lock);
	if (idr_pre_get(&xode_idr, GFP_KERNEL) == 0)
		goto exit;

	retval = idr_get_new(&xode_idr, xdev, &id);
	if (retval < 0) {
		if (retval == -EAGAIN)
			retval = -ENOMEM;
		goto exit;
	}
	xdev->minor = id & MAX_ID_MASK;
exit:
	mutex_unlock(&minor_lock);
	return retval;
}

/*******************************************************************
*******************************************************************/
static void xode_free_minor(struct xode_device *idev)
{
	mutex_lock(&minor_lock);
	idr_remove(&xode_idr, idev->minor);
	mutex_unlock(&minor_lock);
}

/*******************************************************************
*******************************************************************/
static int init_xode_class( void )
{
	int ret = 0;
	if (xode_class != NULL) {
		goto exit;
	}
	ret = xode_major_init();
	if (ret)
		goto exit;

	xode_class = class_create(THIS_MODULE, "xode");
	if (IS_ERR(xode_class)) {
		ret = IS_ERR(xode_class);
		printk(KERN_ERR "class_create failed for xode\n");
		goto err_class_create;
	}
	return 0;

err_class_create:
	kfree(xode_class);
	xode_class = NULL;
	xode_major_cleanup();
exit:
	return ret;
}

/*******************************************************************
*******************************************************************/
static void xode_class_destroy(void)
{
	class_destroy(xode_class);
	xode_major_cleanup();
	xode_class = NULL;
}

/*******************************************************************
*******************************************************************/
static int xode_get_resources(struct platform_device *pdev, struct xode_device *xdev)
{
	struct resource * rsrc;	
	int ret;

	/* First the FPGA control registers */
	rsrc = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xode_reg = (XODE_FPGA_REGS_t*) io_p2v(rsrc->start);
	rsrc = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	rsrc = platform_get_resource(pdev, IORESOURCE_MEM, 2);


	/* grab a DMA channel */
	xdev->dma_ch = dma_request_channel("xode", xode_dma_irq, xdev);
	if (xdev->dma_ch < 0)
	{
		dev_err(&pdev->dev, "error getting xode DMA channel.\n");
		goto err_dma;
	}
	/* IRQ from FPGA */
	xode_reg->fpga_status = FPGA_INT_ALL; 
	rsrc = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	xdev->fpga_irq = rsrc->start;

	if(xdev->fpga_irq >= 0) {
		disable_irq(xdev->fpga_irq);
		ret = request_irq(	xdev->fpga_irq, 
							xode_interrupt,
				  			IRQF_DISABLED, 
				  			"xode", 
				  			xdev);
		if (ret) goto err_fpga;
	}
	return 0;

err_fpga:
	/* pointless, but whatever */
	dma_release_channel(xdev->dma_ch);
err_dma:
	return -ENXIO;
}

/*******************************************************************
*******************************************************************/
static int xode_probe(struct platform_device *pdev)
{
	struct xode_info *xodeinfo = pdev->dev.platform_data;
	int ret = -ENODEV;

	if(!xodeinfo)
		return ret;

	xodeinfo->xode_dev = NULL;

	ret = init_xode_class();
	if(ret)
		return ret;

	_xdev = kzalloc(sizeof(*_xdev), GFP_KERNEL);
	if (!_xdev) {
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	ret = xode_get_minor(_xdev);
	if (ret) {
		printk("get minor fails %d\n",ret);
		goto err_get_minor;
	}

	_xdev->dev = device_create(xode_class, &pdev->dev,
				  MKDEV(xode_major, _xdev->minor), _xdev,
				  "xode%d", _xdev->minor);
	if (IS_ERR(_xdev->dev)) {
		printk(KERN_ERR "XODE: device register failed\n");
		ret = PTR_ERR(_xdev->dev);
		goto err_device_create;
	}

	ret = xode_get_resources(pdev, _xdev);
	if (ret)
		goto err_get_resources;

	xodeinfo->xode_dev = _xdev;

	/* setup completion for xode_read sync */
	init_completion(&readcomplete);
	/* setup completion for DMA CRC calcs */
	init_completion(&crccomplete);
	/* init the state machine */
	InitCoreStateMachine();
	
	init_completion(&traystatecomplete);

	/* first time around app must get current state & not wait for interrupt */	
	complete(&traystatecomplete);


	printk("XODE installed [FPGA ID '0x%04X']\n",xode_reg->fpga_id);
	return 0;

err_get_resources:
	device_destroy(xode_class, MKDEV(xode_major, _xdev->minor));
err_device_create:
	xode_free_minor(_xdev);
err_get_minor:
	kfree(_xdev);
err_kzalloc:
	xode_class_destroy();
	return ret;
}

/*******************************************************************
*******************************************************************/
static int xode_pdrv_remove(struct platform_device *pdev)
{
	struct xode_device *xdev;
	struct xode_info *xodeinfo = platform_get_drvdata(pdev);

	if (!xodeinfo || !xodeinfo->xode_dev)
		return 0;

	printk("xode_pdrv_remove\n");

	xdev = xodeinfo->xode_dev;

	xode_free_minor(xdev);

	dev_set_drvdata(xdev->dev, NULL);
	device_destroy(xode_class, MKDEV(xode_major, xdev->minor));
	kfree(xdev);
	xode_class_destroy();
	return 0;
}

/*******************************************************************
*******************************************************************/
static struct platform_driver xode_pdrv = {
	.probe = xode_probe,
	.remove = xode_pdrv_remove,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

/*******************************************************************
*******************************************************************/
static int __init lpc313x_xode_init(void)
{
	return platform_driver_register(&xode_pdrv);
}

/*******************************************************************
*******************************************************************/
static void __exit lpc313x_xode_exit(void)
{
}

module_init(lpc313x_xode_init);
module_exit(lpc313x_xode_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anonymous Coward");
MODULE_DESCRIPTION("x360Key Driver lpc313x platforms");
MODULE_ALIAS("platform:lpc313x_xode");
