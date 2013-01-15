/*
 * drivers/rtc/ambarella_rtc.c
 *
 * History:
 *	2008/04/01 - [Cao Rongrong] Support pause and resume
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>

#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <plat/rtc.h>

/* ==========================================================================*/
static unsigned long epoch = 1970;

/* ==========================================================================*/
u32 __ambrtc_tm2epoch_diff(struct rtc_time *current_tm)
{
	time_t time;
	time_t epoch_sec;
	unsigned int year;
	unsigned int mon;
	unsigned int day;
	unsigned int hour;
	unsigned int min;
	unsigned int sec;

	year = (unsigned int)current_tm->tm_year;
	mon = (unsigned int)current_tm->tm_mon;
	day = (unsigned int)current_tm->tm_mday;
	hour = (unsigned int)current_tm->tm_hour;
	min = (unsigned int)current_tm->tm_min;
	sec = (unsigned int)current_tm->tm_sec;

	epoch_sec = mktime(epoch, 1, 1, 0, 0, 0);
	time = mktime(year + 1900, mon + 1, day, hour, min, sec);

	return (u32)(time - epoch_sec);
}

/* ==========================================================================*/
int __ambrtc_dev_set_time(struct device *dev, struct rtc_time *tm)
{
	int					ret = 0;
	struct platform_device			*pdev;
	struct ambarella_rtc_controller		*pinfo;
	u32					tm2epoch_diff;

	if (tm == NULL) {
		ret = -EPERM;
		goto __ambrtc_dev_set_time_exit;
	}

	pr_debug("%s: %d.%d.%d - %d:%d:%d\n", __func__,
		tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	pdev = to_platform_device(dev);
	pinfo = (struct ambarella_rtc_controller *)(pdev->dev.platform_data);

	tm2epoch_diff = __ambrtc_tm2epoch_diff(tm);
	ret = pinfo->check_capacity(tm2epoch_diff);
	if (ret)
		goto __ambrtc_dev_set_time_exit;

	pr_debug("%s: 0x%x\n", __func__, tm2epoch_diff);
	pinfo->set_curt_time(tm2epoch_diff);

__ambrtc_dev_set_time_exit:
	return ret;
}

int __ambrtc_dev_get_time(struct device *dev, struct rtc_time *tm)
{
	int					ret = 0;
	struct platform_device			*pdev;
	struct ambarella_rtc_controller		*pinfo;
	u32					time;

	if (tm == NULL) {
		ret = -EPERM;
		goto __ambrtc_dev_get_time_exit;
	}

	pdev = to_platform_device(dev);
	pinfo = (struct ambarella_rtc_controller *)(pdev->dev.platform_data);

	time = pinfo->get_curt_time();
	rtc_time_to_tm(time, tm);

	pr_debug("%s: 0x%x\n", __func__, time);
	pr_debug("%s: %d.%d.%d - %d:%d:%d\n", __func__,
		tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

__ambrtc_dev_get_time_exit:
	return ret;
}

int __ambrtc_dev_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int					ret = 0;
	struct platform_device			*pdev;
	struct ambarella_rtc_controller		*pinfo;
	u32					tm2epoch_diff;

	if (alrm == NULL) {
		ret = -EPERM;
		goto __ambrtc_dev_set_alarm_exit;
	}

	pr_debug("%s %d: %d.%d.%d - %d:%d:%d\n", __func__, alrm->enabled,
		alrm->time.tm_year, alrm->time.tm_mon, alrm->time.tm_mday,
		alrm->time.tm_hour, alrm->time.tm_min, alrm->time.tm_sec);

	pdev = to_platform_device(dev);
	pinfo = (struct ambarella_rtc_controller *)(pdev->dev.platform_data);

	if (alrm->enabled) {
		tm2epoch_diff = __ambrtc_tm2epoch_diff(&(alrm->time));
		ret = pinfo->check_capacity(tm2epoch_diff);
		if (!ret)
			goto __ambrtc_dev_set_alarm_exit;
	} else
		tm2epoch_diff = 0;

	pinfo->set_alat_time(tm2epoch_diff);

__ambrtc_dev_set_alarm_exit:
	return ret;
}

int __ambrtc_dev_get_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int					ret = 0;
	struct platform_device			*pdev;
	struct ambarella_rtc_controller		*pinfo;
	u32					rtc_status;
	u32					time, alrm_time;

	if (alrm == NULL) {
		ret = -EPERM;
		goto __ambrtc_dev_get_alarm_exit;
	}

	pdev = to_platform_device(dev);
	pinfo = (struct ambarella_rtc_controller *)(pdev->dev.platform_data);

	rtc_status = pinfo->check_status();
	alrm_time = pinfo->get_alat_time();
	time = pinfo->get_curt_time();

	alrm->enabled = (alrm_time > time);
	alrm->pending = !!(rtc_status & RTC_STATUS_ALA_WK);
	rtc_time_to_tm(alrm_time, &(alrm->time));

	pr_debug("%s %d: %d.%d.%d - %d:%d:%d [%d]\n", __func__, alrm->enabled,
		alrm->time.tm_year, alrm->time.tm_mon, alrm->time.tm_mday,
		alrm->time.tm_hour, alrm->time.tm_min, alrm->time.tm_sec,
		alrm->pending);

__ambrtc_dev_get_alarm_exit:
	return ret;
}

/* ==========================================================================*/
static int ambrtc_gettime(struct device *dev, struct rtc_time *tm)
{
	int rval;

	rval = __ambrtc_dev_get_time(dev, tm);
	if (rval < 0)
		dev_err(dev,"%s: fail %d.\n", __func__, rval);

	return rval;
}

static int ambrtc_settime(struct device *dev, struct rtc_time *tm)
{
	int rval;

	rval = __ambrtc_dev_set_time(dev, tm);
	if (rval < 0)
		dev_err(dev,"%s: fail %d.\n", __func__, rval);

	return rval;
}

static int ambrtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int rval;

	rval = __ambrtc_dev_get_alarm(dev, alrm);
	if (rval < 0)
		dev_err(dev,"%s: fail %d.\n", __func__, rval);

	return rval;
}

static int ambrtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int rval;

	rval = __ambrtc_dev_set_alarm(dev, alrm);
	if (rval < 0)
		dev_err(dev,"%s: fail %d.\n", __func__, rval);

	return rval;
}

static int ambrtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case RTC_EPOCH_READ:
		if (put_user(epoch, (unsigned long __user *)arg))
			ret = -EFAULT;
		break;
	case RTC_EPOCH_SET:
		if (arg < 1900) {
			ret = -EINVAL;
			break;
		}
		epoch = arg;
		break;
	default:
		ret = -ENOIOCTLCMD;
	}

	return ret;
}

static int ambrtc_proc(struct device *dev, struct seq_file *seq)
{
	seq_printf(seq, "RTC_PWC_ALAT_REG= 0x%08x\n"
		"RTC_PWC_CURT_REG= 0x%08x\n"
		"RTC_CURT_REG\t= 0x%08x\n"
		"RTC_ALAT_REG\t= 0x%08x\n"
		"RTC_STATUS_REG\t= 0x%08x\n"
		"RTC_RESET_REG\t= 0x%08x\n",
		amba_readl(RTC_PWC_ALAT_REG),
		amba_readl(RTC_PWC_CURT_REG),
		amba_readl(RTC_CURT_REG),
		amba_readl(RTC_ALAT_REG),
		amba_readl(RTC_STATUS_REG),
		amba_readl(RTC_RESET_REG));

	return 0;
}

static const struct rtc_class_ops ambarella_rtc_ops = {
	.ioctl		= ambrtc_ioctl,
	.read_time	= ambrtc_gettime,
	.set_time	= ambrtc_settime,
	.read_alarm	= ambrtc_getalarm,
	.set_alarm	= ambrtc_setalarm,
	.proc	   	= ambrtc_proc,
};

/* ==========================================================================*/
static int __devinit ambrtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	int ret = 0;

	rtc = rtc_device_register(pdev->name,
		&pdev->dev, &ambarella_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "rtc_device_register fail.\n");
		ret = PTR_ERR(rtc);
		goto err_nores;
	}

	platform_set_drvdata(pdev, rtc);

err_nores:
	return ret;
}

static int __devexit ambrtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc = platform_get_drvdata(pdev);

	if (rtc) {
		platform_set_drvdata(pdev, NULL);
		rtc_device_unregister(rtc);
	}

	return 0;
}

#ifdef CONFIG_PM
static int ambrtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	int					errorCode = 0;

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);

	return errorCode;
}

static int ambrtc_resume(struct platform_device *pdev)
{
	int					errorCode = 0;

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

	return errorCode;
}
#endif

static struct platform_driver ambarella_rtc_driver = {
	.probe		= ambrtc_probe,
	.remove		= __devexit_p(ambrtc_remove),
#ifdef CONFIG_PM
	.suspend	= ambrtc_suspend,
	.resume		= ambrtc_resume,
#endif
	.driver		= {
		.name	= "ambarella-rtc",
		.owner	= THIS_MODULE,
	},
};

static int __init ambarella_rtc_init(void)
{
	return platform_driver_register(&ambarella_rtc_driver);
}

static void __exit ambarella_rtc_exit(void)
{
	platform_driver_unregister(&ambarella_rtc_driver);
}

module_init(ambarella_rtc_init);
module_exit(ambarella_rtc_exit);

MODULE_DESCRIPTION("Ambarella A2 RTC Driver");
MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_LICENSE("GPL");

