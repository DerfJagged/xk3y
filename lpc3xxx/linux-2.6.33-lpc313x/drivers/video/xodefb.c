/*
 * xodefb.c - fb driver for xode 128x8 mono oled dpy
 * 
 * Inspired by:
 * 
 * linux/drivers/video/hecubafb.c -- FB driver for Hecuba/Apollo controller
 *
 * Copyright (C) 2006, Jaya Kumar
 * This work was sponsored by CIS(M) Sdn Bhd
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/uaccess.h>

/* Display specific information */
#define DPY_W 128
#define DPY_H 64

#define	FBIO_WAIT_FOR_UPDATE	_IOWR('F', 0x19, unsigned long)

static struct fb_fix_screeninfo xodefb_fix = {
	.id =		"xodefb",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_MONO01,
	.xpanstep =	0,
	.ypanstep =	0,
	.ywrapstep =	0,
	.line_length =	DPY_W/8,
	.accel =	FB_ACCEL_NONE,
};

static struct fb_var_screeninfo xodefb_var = {
	.xres		= DPY_W,
	.yres		= DPY_H,
	.xres_virtual	= DPY_W,
	.yres_virtual	= DPY_H,
	.bits_per_pixel	= 1,
	.nonstd		= 1,
};

static DECLARE_WAIT_QUEUE_HEAD(wq);
static volatile int updated = 0;

static void xodefb_dpy_update(struct fb_info *info)
{
	updated = 1;
	wake_up(&wq);
}

/* this is called back from the deferred io workqueue */
static void xodefb_dpy_deferred_io(struct fb_info *info,
				struct list_head *pagelist)
{
	xodefb_dpy_update(info);
}

static void xodefb_fillrect(struct fb_info *info,
				   const struct fb_fillrect *rect)
{
	sys_fillrect(info, rect);

	xodefb_dpy_update(info);
}

static void xodefb_copyarea(struct fb_info *info,
				   const struct fb_copyarea *area)
{
	sys_copyarea(info, area);

	xodefb_dpy_update(info);
}

static void xodefb_imageblit(struct fb_info *info,
				const struct fb_image *image)
{
	sys_imageblit(info, image);

	xodefb_dpy_update(info);
}

/*
 * this is the slow path from userspace. they can seek and write to
 * the fb. it's inefficient to do anything less than a full screen draw
 */
static ssize_t xodefb_write(struct fb_info *info, const char __user *buf,
				size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	void *dst;
	int err = 0;
	unsigned long total_size;

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	total_size = info->fix.smem_len;

	if (p > total_size)
		return -EFBIG;

	if (count > total_size) {
		err = -EFBIG;
		count = total_size;
	}

	if (count + p > total_size) {
		if (!err)
			err = -ENOSPC;

		count = total_size - p;
	}

	dst = (void __force *) (info->screen_base + p);

	if (copy_from_user(dst, buf, count))
		err = -EFAULT;

	if  (!err)
		*ppos += count;

	xodefb_dpy_update(info);

	return (err) ? err : count;
}

static int xodefb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	long ms;
	
	switch (cmd)
	{
		case FBIO_WAIT_FOR_UPDATE:
			if (get_user(ms, (unsigned long __user*) arg))
				return -EFAULT;

			// return the remaining time in ms, or 0 if dpy was not updated
			if ((ms = wait_event_timeout(wq, updated, msecs_to_jiffies(ms))))
				updated = 0;

			return put_user(
				ms ? jiffies_to_msecs(ms) : 0, (unsigned long __user*) arg);

		default:
			return -ENOIOCTLCMD;
	}

	return 0;
}

static struct fb_ops xodefb_ops = {
	.owner		= THIS_MODULE,
	.fb_read        = fb_sys_read,
	.fb_write	= xodefb_write,
	.fb_fillrect	= xodefb_fillrect,
	.fb_copyarea	= xodefb_copyarea,
	.fb_imageblit	= xodefb_imageblit,
	.fb_ioctl	= xodefb_ioctl,
};

static struct fb_deferred_io xodefb_defio = {
	.delay		= HZ/20,
	.deferred_io	= xodefb_dpy_deferred_io,
};

static int __init xodefb_init(void)
{
	struct fb_info *info;
	int retval = -ENOMEM;
	int videomemorysize;
	unsigned char *videomemory;

	videomemorysize = (DPY_W*DPY_H)/8;

	if (!(videomemory = vmalloc(videomemorysize)))
		return retval;

	memset(videomemory, 0, videomemorysize);

	info = framebuffer_alloc(sizeof(struct fb_info), NULL);
	if (!info)
		goto err_fballoc;

	info->screen_base = (char __force __iomem *)videomemory;
	info->fbops = &xodefb_ops;

	info->var = xodefb_var;
	info->fix = xodefb_fix;
	info->fix.smem_len = videomemorysize;
	info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;

	info->fbdefio = &xodefb_defio;
	fb_deferred_io_init(info);

	retval = register_framebuffer(info);
	if (retval < 0)
		goto err_fbreg;

	printk(KERN_INFO
	       "fb%d: Xode frame buffer device, using %dK of video memory\n",
	       info->node, videomemorysize >> 10);

	return 0;
err_fbreg:
	framebuffer_release(info);
err_fballoc:
	vfree(videomemory);
	return retval;
}

module_init(xodefb_init);

MODULE_LICENSE("GPL");
