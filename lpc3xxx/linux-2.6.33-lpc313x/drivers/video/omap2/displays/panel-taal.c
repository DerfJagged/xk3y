/*
 * Taal DSI command mode panel
 *
 * Copyright (C) 2009 Nokia Corporation
 * Author: Tomi Valkeinen <tomi.valkeinen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*#define DEBUG*/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/completion.h>
#include <linux/workqueue.h>

#include <plat/display.h>

/* DSI Virtual channel. Hardcoded for now. */
#define TCH 0

#define DCS_READ_NUM_ERRORS	0x05
#define DCS_READ_POWER_MODE	0x0a
#define DCS_READ_MADCTL		0x0b
#define DCS_READ_PIXEL_FORMAT	0x0c
#define DCS_RDDSDR		0x0f
#define DCS_SLEEP_IN		0x10
#define DCS_SLEEP_OUT		0x11
#define DCS_DISPLAY_OFF		0x28
#define DCS_DISPLAY_ON		0x29
#define DCS_COLUMN_ADDR		0x2a
#define DCS_PAGE_ADDR		0x2b
#define DCS_MEMORY_WRITE	0x2c
#define DCS_TEAR_OFF		0x34
#define DCS_TEAR_ON		0x35
#define DCS_MEM_ACC_CTRL	0x36
#define DCS_PIXEL_FORMAT	0x3a
#define DCS_BRIGHTNESS		0x51
#define DCS_CTRL_DISPLAY	0x53
#define DCS_WRITE_CABC		0x55
#define DCS_READ_CABC		0x56
#define DCS_GET_ID1		0xda
#define DCS_GET_ID2		0xdb
#define DCS_GET_ID3		0xdc

/* #define TAAL_USE_ESD_CHECK */
#define TAAL_ESD_CHECK_PERIOD	msecs_to_jiffies(5000)

struct taal_data {
	struct backlight_device *bldev;

	unsigned long	hw_guard_end;	/* next value of jiffies when we can
					 * issue the next sleep in/out command
					 */
	unsigned long	hw_guard_wait;	/* max guard time in jiffies */

	struct omap_dss_device *dssdev;

	bool enabled;
	u8 rotate;
	bool mirror;

	bool te_enabled;
	bool use_ext_te;
	struct completion te_completion;

	bool use_dsi_bl;

	bool cabc_broken;
	unsigned cabc_mode;

	bool intro_printed;

	struct workqueue_struct *esd_wq;
	struct delayed_work esd_work;
};

static void taal_esd_work(struct work_struct *work);

static void hw_guard_start(struct taal_data *td, int guard_msec)
{
	td->hw_guard_wait = msecs_to_jiffies(guard_msec);
	td->hw_guard_end = jiffies + td->hw_guard_wait;
}

static void hw_guard_wait(struct taal_data *td)
{
	unsigned long wait = td->hw_guard_end - jiffies;

	if ((long)wait > 0 && wait <= td->hw_guard_wait) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(wait);
	}
}

static int taal_dcs_read_1(u8 dcs_cmd, u8 *data)
{
	int r;
	u8 buf[1];

	r = dsi_vc_dcs_read(TCH, dcs_cmd, buf, 1);

	if (r < 0)
		return r;

	*data = buf[0];

	return 0;
}

static int taal_dcs_write_0(u8 dcs_cmd)
{
	return dsi_vc_dcs_write(TCH, &dcs_cmd, 1);
}

static int taal_dcs_write_1(u8 dcs_cmd, u8 param)
{
	u8 buf[2];
	buf[0] = dcs_cmd;
	buf[1] = param;
	return dsi_vc_dcs_write(TCH, buf, 2);
}

static int taal_sleep_in(struct taal_data *td)

{
	u8 cmd;
	int r;

	hw_guard_wait(td);

	cmd = DCS_SLEEP_IN;
	r = dsi_vc_dcs_write_nosync(TCH, &cmd, 1);
	if (r)
		return r;

	hw_guard_start(td, 120);

	msleep(5);

	return 0;
}

static int taal_sleep_out(struct taal_data *td)
{
	int r;

	hw_guard_wait(td);

	r = taal_dcs_write_0(DCS_SLEEP_OUT);
	if (r)
		return r;

	hw_guard_start(td, 120);

	msleep(5);

	return 0;
}

static int taal_get_id(u8 *id1, u8 *id2, u8 *id3)
{
	int r;

	r = taal_dcs_read_1(DCS_GET_ID1, id1);
	if (r)
		return r;
	r = taal_dcs_read_1(DCS_GET_ID2, id2);
	if (r)
		return r;
	r = taal_dcs_read_1(DCS_GET_ID3, id3);
	if (r)
		return r;

	return 0;
}

static int taal_set_addr_mode(u8 rotate, bool mirror)
{
	int r;
	u8 mode;
	int b5, b6, b7;

	r = taal_dcs_read_1(DCS_READ_MADCTL, &mode);
	if (r)
		return r;

	switch (rotate) {
	default:
	case 0:
		b7 = 0;
		b6 = 0;
		b5 = 0;
		break;
	case 1:
		b7 = 0;
		b6 = 1;
		b5 = 1;
		break;
	case 2:
		b7 = 1;
		b6 = 1;
		b5 = 0;
		break;
	case 3:
		b7 = 1;
		b6 = 0;
		b5 = 1;
		break;
	}

	if (mirror)
		b6 = !b6;

	mode &= ~((1<<7) | (1<<6) | (1<<5));
	mode |= (b7 << 7) | (b6 << 6) | (b5 << 5);

	return taal_dcs_write_1(DCS_MEM_ACC_CTRL, mode);
}

static int taal_set_update_window(u16 x, u16 y, u16 w, u16 h)
{
	int r;
	u16 x1 = x;
	u16 x2 = x + w - 1;
	u16 y1 = y;
	u16 y2 = y + h - 1;

	u8 buf[5];
	buf[0] = DCS_COLUMN_ADDR;
	buf[1] = (x1 >> 8) & 0xff;
	buf[2] = (x1 >> 0) & 0xff;
	buf[3] = (x2 >> 8) & 0xff;
	buf[4] = (x2 >> 0) & 0xff;

	r = dsi_vc_dcs_write_nosync(TCH, buf, sizeof(buf));
	if (r)
		return r;

	buf[0] = DCS_PAGE_ADDR;
	buf[1] = (y1 >> 8) & 0xff;
	buf[2] = (y1 >> 0) & 0xff;
	buf[3] = (y2 >> 8) & 0xff;
	buf[4] = (y2 >> 0) & 0xff;

	r = dsi_vc_dcs_write_nosync(TCH, buf, sizeof(buf));
	if (r)
		return r;

	dsi_vc_send_bta_sync(TCH);

	return r;
}

static int taal_bl_update_status(struct backlight_device *dev)
{
	struct omap_dss_device *dssdev = dev_get_drvdata(&dev->dev);
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	int r;
	int level;

	if (dev->props.fb_blank == FB_BLANK_UNBLANK &&
			dev->props.power == FB_BLANK_UNBLANK)
		level = dev->props.brightness;
	else
		level = 0;

	dev_dbg(&dssdev->dev, "update brightness to %d\n", level);

	if (td->use_dsi_bl) {
		if (td->enabled) {
			dsi_bus_lock();
			r = taal_dcs_write_1(DCS_BRIGHTNESS, level);
			dsi_bus_unlock();
			if (r)
				return r;
		}
	} else {
		if (!dssdev->set_backlight)
			return -EINVAL;

		r = dssdev->set_backlight(dssdev, level);
		if (r)
			return r;
	}

	return 0;
}

static int taal_bl_get_intensity(struct backlight_device *dev)
{
	if (dev->props.fb_blank == FB_BLANK_UNBLANK &&
			dev->props.power == FB_BLANK_UNBLANK)
		return dev->props.brightness;

	return 0;
}

static struct backlight_ops taal_bl_ops = {
	.get_brightness = taal_bl_get_intensity,
	.update_status  = taal_bl_update_status,
};

static void taal_get_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	*timings = dssdev->panel.timings;
}

static void taal_get_resolution(struct omap_dss_device *dssdev,
		u16 *xres, u16 *yres)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);

	if (td->rotate == 0 || td->rotate == 2) {
		*xres = dssdev->panel.timings.x_res;
		*yres = dssdev->panel.timings.y_res;
	} else {
		*yres = dssdev->panel.timings.x_res;
		*xres = dssdev->panel.timings.y_res;
	}
}

static irqreturn_t taal_te_isr(int irq, void *data)
{
	struct omap_dss_device *dssdev = data;
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);

	complete_all(&td->te_completion);

	return IRQ_HANDLED;
}

static ssize_t taal_num_errors_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct omap_dss_device *dssdev = to_dss_device(dev);
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	u8 errors;
	int r;

	if (td->enabled) {
		dsi_bus_lock();
		r = taal_dcs_read_1(DCS_READ_NUM_ERRORS, &errors);
		dsi_bus_unlock();
	} else {
		r = -ENODEV;
	}

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", errors);
}

static ssize_t taal_hw_revision_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct omap_dss_device *dssdev = to_dss_device(dev);
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	u8 id1, id2, id3;
	int r;

	if (td->enabled) {
		dsi_bus_lock();
		r = taal_get_id(&id1, &id2, &id3);
		dsi_bus_unlock();
	} else {
		r = -ENODEV;
	}

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%02x.%02x.%02x\n", id1, id2, id3);
}

static const char *cabc_modes[] = {
	"off",		/* used also always when CABC is not supported */
	"ui",
	"still-image",
	"moving-image",
};

static ssize_t show_cabc_mode(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct omap_dss_device *dssdev = to_dss_device(dev);
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	const char *mode_str;
	int mode;
	int len;

	mode = td->cabc_mode;

	mode_str = "unknown";
	if (mode >= 0 && mode < ARRAY_SIZE(cabc_modes))
		mode_str = cabc_modes[mode];
	len = snprintf(buf, PAGE_SIZE, "%s\n", mode_str);

	return len < PAGE_SIZE - 1 ? len : PAGE_SIZE - 1;
}

static ssize_t store_cabc_mode(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct omap_dss_device *dssdev = to_dss_device(dev);
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	int i;

	for (i = 0; i < ARRAY_SIZE(cabc_modes); i++) {
		if (sysfs_streq(cabc_modes[i], buf))
			break;
	}

	if (i == ARRAY_SIZE(cabc_modes))
		return -EINVAL;

	if (td->enabled) {
		dsi_bus_lock();
		if (!td->cabc_broken)
			taal_dcs_write_1(DCS_WRITE_CABC, i);
		dsi_bus_unlock();
	}

	td->cabc_mode = i;

	return count;
}

static ssize_t show_cabc_available_modes(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	int len;
	int i;

	for (i = 0, len = 0;
	     len < PAGE_SIZE && i < ARRAY_SIZE(cabc_modes); i++)
		len += snprintf(&buf[len], PAGE_SIZE - len, "%s%s%s",
			i ? " " : "", cabc_modes[i],
			i == ARRAY_SIZE(cabc_modes) - 1 ? "\n" : "");

	return len < PAGE_SIZE ? len : PAGE_SIZE - 1;
}

static DEVICE_ATTR(num_dsi_errors, S_IRUGO, taal_num_errors_show, NULL);
static DEVICE_ATTR(hw_revision, S_IRUGO, taal_hw_revision_show, NULL);
static DEVICE_ATTR(cabc_mode, S_IRUGO | S_IWUSR,
		show_cabc_mode, store_cabc_mode);
static DEVICE_ATTR(cabc_available_modes, S_IRUGO,
		show_cabc_available_modes, NULL);

static struct attribute *taal_attrs[] = {
	&dev_attr_num_dsi_errors.attr,
	&dev_attr_hw_revision.attr,
	&dev_attr_cabc_mode.attr,
	&dev_attr_cabc_available_modes.attr,
	NULL,
};

static struct attribute_group taal_attr_group = {
	.attrs = taal_attrs,
};

static int taal_probe(struct omap_dss_device *dssdev)
{
	struct taal_data *td;
	struct backlight_device *bldev;
	int r;

	const struct omap_video_timings taal_panel_timings = {
		.x_res		= 864,
		.y_res		= 480,
	};

	dev_dbg(&dssdev->dev, "probe\n");

	dssdev->panel.config = OMAP_DSS_LCD_TFT;
	dssdev->panel.timings = taal_panel_timings;
	dssdev->ctrl.pixel_size = 24;

	td = kzalloc(sizeof(*td), GFP_KERNEL);
	if (!td) {
		r = -ENOMEM;
		goto err0;
	}
	td->dssdev = dssdev;

	td->esd_wq = create_singlethread_workqueue("taal_esd");
	if (td->esd_wq == NULL) {
		dev_err(&dssdev->dev, "can't create ESD workqueue\n");
		r = -ENOMEM;
		goto err2;
	}
	INIT_DELAYED_WORK_DEFERRABLE(&td->esd_work, taal_esd_work);

	dev_set_drvdata(&dssdev->dev, td);

	dssdev->get_timings = taal_get_timings;
	dssdev->get_resolution = taal_get_resolution;

	/* if no platform set_backlight() defined, presume DSI backlight
	 * control */
	if (!dssdev->set_backlight)
		td->use_dsi_bl = true;

	bldev = backlight_device_register("taal", &dssdev->dev, dssdev,
			&taal_bl_ops);
	if (IS_ERR(bldev)) {
		r = PTR_ERR(bldev);
		goto err1;
	}

	td->bldev = bldev;

	bldev->props.fb_blank = FB_BLANK_UNBLANK;
	bldev->props.power = FB_BLANK_UNBLANK;
	if (td->use_dsi_bl) {
		bldev->props.max_brightness = 255;
		bldev->props.brightness = 255;
	} else {
		bldev->props.max_brightness = 127;
		bldev->props.brightness = 127;
	}

	taal_bl_update_status(bldev);

	if (dssdev->phy.dsi.ext_te) {
		int gpio = dssdev->phy.dsi.ext_te_gpio;

		r = gpio_request(gpio, "taal irq");
		if (r) {
			dev_err(&dssdev->dev, "GPIO request failed\n");
			goto err3;
		}

		gpio_direction_input(gpio);

		r = request_irq(gpio_to_irq(gpio), taal_te_isr,
				IRQF_DISABLED | IRQF_TRIGGER_RISING,
				"taal vsync", dssdev);

		if (r) {
			dev_err(&dssdev->dev, "IRQ request failed\n");
			gpio_free(gpio);
			goto err3;
		}

		init_completion(&td->te_completion);

		td->use_ext_te = true;
	}

	r = sysfs_create_group(&dssdev->dev.kobj, &taal_attr_group);
	if (r) {
		dev_err(&dssdev->dev, "failed to create sysfs files\n");
		goto err4;
	}

	return 0;
err4:
	if (td->use_ext_te) {
		int gpio = dssdev->phy.dsi.ext_te_gpio;
		free_irq(gpio_to_irq(gpio), dssdev);
		gpio_free(gpio);
	}
err3:
	backlight_device_unregister(bldev);
err2:
	cancel_delayed_work_sync(&td->esd_work);
	destroy_workqueue(td->esd_wq);
err1:
	kfree(td);
err0:
	return r;
}

static void taal_remove(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	struct backlight_device *bldev;

	dev_dbg(&dssdev->dev, "remove\n");

	sysfs_remove_group(&dssdev->dev.kobj, &taal_attr_group);

	if (td->use_ext_te) {
		int gpio = dssdev->phy.dsi.ext_te_gpio;
		free_irq(gpio_to_irq(gpio), dssdev);
		gpio_free(gpio);
	}

	bldev = td->bldev;
	bldev->props.power = FB_BLANK_POWERDOWN;
	taal_bl_update_status(bldev);
	backlight_device_unregister(bldev);

	cancel_delayed_work_sync(&td->esd_work);
	destroy_workqueue(td->esd_wq);

	kfree(td);
}

static int taal_enable(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	u8 id1, id2, id3;
	int r;

	dev_dbg(&dssdev->dev, "enable\n");

	if (dssdev->platform_enable) {
		r = dssdev->platform_enable(dssdev);
		if (r)
			return r;
	}

	/* it seems we have to wait a bit until taal is ready */
	msleep(5);

	r = taal_sleep_out(td);
	if (r)
		goto err;

	r = taal_get_id(&id1, &id2, &id3);
	if (r)
		goto err;

	/* on early revisions CABC is broken */
	if (id2 == 0x00 || id2 == 0xff || id2 == 0x81)
		td->cabc_broken = true;

	taal_dcs_write_1(DCS_BRIGHTNESS, 0xff);
	taal_dcs_write_1(DCS_CTRL_DISPLAY, (1<<2) | (1<<5)); /* BL | BCTRL */

	taal_dcs_write_1(DCS_PIXEL_FORMAT, 0x7); /* 24bit/pixel */

	taal_set_addr_mode(td->rotate, td->mirror);
	if (!td->cabc_broken)
		taal_dcs_write_1(DCS_WRITE_CABC, td->cabc_mode);

	taal_dcs_write_0(DCS_DISPLAY_ON);

#ifdef TAAL_USE_ESD_CHECK
	queue_delayed_work(td->esd_wq, &td->esd_work, TAAL_ESD_CHECK_PERIOD);
#endif

	td->enabled = 1;

	if (!td->intro_printed) {
		dev_info(&dssdev->dev, "revision %02x.%02x.%02x\n",
				id1, id2, id3);
		if (td->cabc_broken)
			dev_info(&dssdev->dev,
					"old Taal version, CABC disabled\n");
		td->intro_printed = true;
	}

	return 0;
err:
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);

	return r;
}

static void taal_disable(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);

	dev_dbg(&dssdev->dev, "disable\n");

	cancel_delayed_work(&td->esd_work);

	taal_dcs_write_0(DCS_DISPLAY_OFF);
	taal_sleep_in(td);

	/* wait a bit so that the message goes through */
	msleep(10);

	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);

	td->enabled = 0;
}

static int taal_suspend(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	struct backlight_device *bldev = td->bldev;

	bldev->props.power = FB_BLANK_POWERDOWN;
	taal_bl_update_status(bldev);

	return 0;
}

static int taal_resume(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	struct backlight_device *bldev = td->bldev;

	bldev->props.power = FB_BLANK_UNBLANK;
	taal_bl_update_status(bldev);

	return 0;
}

static void taal_setup_update(struct omap_dss_device *dssdev,
				    u16 x, u16 y, u16 w, u16 h)
{
	taal_set_update_window(x, y, w, h);
}

static int taal_enable_te(struct omap_dss_device *dssdev, bool enable)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	int r;

	td->te_enabled = enable;

	if (enable)
		r = taal_dcs_write_1(DCS_TEAR_ON, 0);
	else
		r = taal_dcs_write_0(DCS_TEAR_OFF);

	return r;
}

static int taal_wait_te(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	long wait = msecs_to_jiffies(500);

	if (!td->use_ext_te || !td->te_enabled)
		return 0;

	INIT_COMPLETION(td->te_completion);
	wait = wait_for_completion_timeout(&td->te_completion, wait);
	if (wait == 0) {
		dev_err(&dssdev->dev, "timeout waiting TE\n");
		return -ETIME;
	}

	return 0;
}

static int taal_rotate(struct omap_dss_device *dssdev, u8 rotate)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	int r;

	dev_dbg(&dssdev->dev, "rotate %d\n", rotate);

	if (td->enabled) {
		r = taal_set_addr_mode(rotate, td->mirror);

		if (r)
			return r;
	}

	td->rotate = rotate;

	return 0;
}

static u8 taal_get_rotate(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	return td->rotate;
}

static int taal_mirror(struct omap_dss_device *dssdev, bool enable)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	int r;

	dev_dbg(&dssdev->dev, "mirror %d\n", enable);

	if (td->enabled) {
		r = taal_set_addr_mode(td->rotate, enable);

		if (r)
			return r;
	}

	td->mirror = enable;

	return 0;
}

static bool taal_get_mirror(struct omap_dss_device *dssdev)
{
	struct taal_data *td = dev_get_drvdata(&dssdev->dev);
	return td->mirror;
}

static int taal_run_test(struct omap_dss_device *dssdev, int test_num)
{
	u8 id1, id2, id3;
	int r;

	r = taal_dcs_read_1(DCS_GET_ID1, &id1);
	if (r)
		return r;
	r = taal_dcs_read_1(DCS_GET_ID2, &id2);
	if (r)
		return r;
	r = taal_dcs_read_1(DCS_GET_ID3, &id3);
	if (r)
		return r;

	return 0;
}

static int taal_memory_read(struct omap_dss_device *dssdev,
		void *buf, size_t size,
		u16 x, u16 y, u16 w, u16 h)
{
	int r;
	int first = 1;
	int plen;
	unsigned buf_used = 0;

	if (size < w * h * 3)
		return -ENOMEM;

	size = min(w * h * 3,
			dssdev->panel.timings.x_res *
			dssdev->panel.timings.y_res * 3);

	/* plen 1 or 2 goes into short packet. until checksum error is fixed,
	 * use short packets. plen 32 works, but bigger packets seem to cause
	 * an error. */
	if (size % 2)
		plen = 1;
	else
		plen = 2;

	taal_setup_update(dssdev, x, y, w, h);

	r = dsi_vc_set_max_rx_packet_size(TCH, plen);
	if (r)
		return r;

	while (buf_used < size) {
		u8 dcs_cmd = first ? 0x2e : 0x3e;
		first = 0;

		r = dsi_vc_dcs_read(TCH, dcs_cmd,
				buf + buf_used, size - buf_used);

		if (r < 0) {
			dev_err(&dssdev->dev, "read error\n");
			goto err;
		}

		buf_used += r;

		if (r < plen) {
			dev_err(&dssdev->dev, "short read\n");
			break;
		}

		if (signal_pending(current)) {
			dev_err(&dssdev->dev, "signal pending, "
					"aborting memory read\n");
			r = -ERESTARTSYS;
			goto err;
		}
	}

	r = buf_used;

err:
	dsi_vc_set_max_rx_packet_size(TCH, 1);

	return r;
}

static void taal_esd_work(struct work_struct *work)
{
	struct taal_data *td = container_of(work, struct taal_data,
			esd_work.work);
	struct omap_dss_device *dssdev = td->dssdev;
	u8 state1, state2;
	int r;

	if (!td->enabled)
		return;

	dsi_bus_lock();

	r = taal_dcs_read_1(DCS_RDDSDR, &state1);
	if (r) {
		dev_err(&dssdev->dev, "failed to read Taal status\n");
		goto err;
	}

	/* Run self diagnostics */
	r = taal_sleep_out(td);
	if (r) {
		dev_err(&dssdev->dev, "failed to run Taal self-diagnostics\n");
		goto err;
	}

	r = taal_dcs_read_1(DCS_RDDSDR, &state2);
	if (r) {
		dev_err(&dssdev->dev, "failed to read Taal status\n");
		goto err;
	}

	/* Each sleep out command will trigger a self diagnostic and flip
	 * Bit6 if the test passes.
	 */
	if (!((state1 ^ state2) & (1 << 6))) {
		dev_err(&dssdev->dev, "LCD self diagnostics failed\n");
		goto err;
	}
	/* Self-diagnostics result is also shown on TE GPIO line. We need
	 * to re-enable TE after self diagnostics */
	if (td->use_ext_te && td->te_enabled)
		taal_enable_te(dssdev, true);

	dsi_bus_unlock();

	queue_delayed_work(td->esd_wq, &td->esd_work, TAAL_ESD_CHECK_PERIOD);

	return;
err:
	dev_err(&dssdev->dev, "performing LCD reset\n");

	taal_disable(dssdev);
	taal_enable(dssdev);

	dsi_bus_unlock();

	queue_delayed_work(td->esd_wq, &td->esd_work, TAAL_ESD_CHECK_PERIOD);
}

static struct omap_dss_driver taal_driver = {
	.probe		= taal_probe,
	.remove		= taal_remove,

	.enable		= taal_enable,
	.disable	= taal_disable,
	.suspend	= taal_suspend,
	.resume		= taal_resume,

	.setup_update	= taal_setup_update,
	.enable_te	= taal_enable_te,
	.wait_for_te	= taal_wait_te,
	.set_rotate	= taal_rotate,
	.get_rotate	= taal_get_rotate,
	.set_mirror	= taal_mirror,
	.get_mirror	= taal_get_mirror,
	.run_test	= taal_run_test,
	.memory_read	= taal_memory_read,

	.driver         = {
		.name   = "taal",
		.owner  = THIS_MODULE,
	},
};

static int __init taal_init(void)
{
	omap_dss_register_driver(&taal_driver);

	return 0;
}

static void __exit taal_exit(void)
{
	omap_dss_unregister_driver(&taal_driver);
}

module_init(taal_init);
module_exit(taal_exit);

MODULE_AUTHOR("Tomi Valkeinen <tomi.valkeinen@nokia.com>");
MODULE_DESCRIPTION("Taal Driver");
MODULE_LICENSE("GPL");
