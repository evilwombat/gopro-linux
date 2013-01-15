/*
* linux/drivers/usb/host/ohci-ambarella.c
* driver for Full speed (USB1.1) USB host controller on Ambarella processors
*
* Note:
* At present, only iONE can support USB host controller
*
* History:
*	2010/08/11 - [Cao Rongrong] created file
*
* Copyright (C) 2008 by Ambarella, Inc.
* http://www.ambarella.com
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
* along with this program; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA  02111-1307, USA.
*/

#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <plat/uhc.h>

extern int usb_disabled(void);

struct ohci_ambarella {
	struct ohci_hcd ohci;
	struct ambarella_uhc_controller *plat_ohci;
};


static struct ohci_ambarella *hcd_to_ohci_ambarella(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);

	return container_of(ohci, struct ohci_ambarella, ohci);
}

static void ambarella_start_ohc(struct ohci_ambarella *amb_ohci)
{
	if (amb_ohci && amb_ohci->plat_ohci->enable_host)
		amb_ohci->plat_ohci->enable_host(amb_ohci->plat_ohci);
}

static void ambarella_stop_ohc(struct ohci_ambarella *amb_ohci)
{
	if (amb_ohci && amb_ohci->plat_ohci->enable_host)
		amb_ohci->plat_ohci->disable_host();
}

static int __devinit ohci_ambarella_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	struct ohci_ambarella *amb_ohci = hcd_to_ohci_ambarella(hcd);
	int ret;

	ohci_dbg(ohci, "ohci_ambarella_start, ohci:%p", ohci);

	/* OHCI will still detect 2 ports even though usb port1 is configured
	 * as device port, so we fake the port number manually and report
	 * it to OHCI.*/
	if (amb_ohci->plat_ohci->usb1_is_host == 0)
		ohci->num_ports = 1;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		err ("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver ohci_ambarella_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"Ambarella OHCI",
	.hcd_priv_size =	sizeof(struct ohci_ambarella),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_ambarella_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};

static int ohci_hcd_ambarella_drv_probe(struct platform_device *pdev)
{
	int ret;
	struct usb_hcd *hcd;
	struct ohci_ambarella *amb_ohci;

	if (usb_disabled())
		return -ENODEV;

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ\n");
		return -ENOMEM;
	}

	hcd = usb_create_hcd(&ohci_ambarella_hc_driver, &pdev->dev, "Ambi1");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;
	hcd->regs = (void __iomem *)pdev->resource[0].start;

	amb_ohci = hcd_to_ohci_ambarella(hcd);
	amb_ohci->plat_ohci =
		(struct ambarella_uhc_controller *)pdev->dev.platform_data;
	if (amb_ohci == NULL) {
		ret = -ENODEV;
		goto amb_ohci_err;
	}

	ambarella_start_ohc(amb_ohci);
	ohci_hcd_init(hcd_to_ohci(hcd));

	ret = usb_add_hcd(hcd, pdev->resource[1].start, IRQF_DISABLED | IRQF_TRIGGER_LOW);
	if (ret < 0)
		goto add_hcd_err;

	platform_set_drvdata(pdev, hcd);

	return ret;

add_hcd_err:
	ambarella_stop_ohc(amb_ohci);
amb_ohci_err:
	usb_put_hcd(hcd);
	return ret;
}

static int ohci_hcd_ambarella_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ohci_ambarella *amb_ohci = hcd_to_ohci_ambarella(hcd);

	usb_remove_hcd(hcd);
	ambarella_stop_ohc(amb_ohci);
	usb_put_hcd(hcd);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM

/* Maybe just need to suspend/resume controller via HAL function. */
static int ohci_hcd_ambarella_drv_suspend(struct device *dev)
{
	return 0;
}

static int ohci_hcd_ambarella_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	ohci_finish_controller_resume(hcd);

	return 0;
}

static struct dev_pm_ops ambarella_ohci_pmops = {
	.suspend	= ohci_hcd_ambarella_drv_suspend,
	.resume		= ohci_hcd_ambarella_drv_resume,
};

#define AMBARELLA_OHCI_PMOPS &ambarella_ohci_pmops

#else
#define AMBARELLA_OHCI_PMOPS NULL
#endif

static struct platform_driver ohci_hcd_ambarella_driver = {
	.probe		= ohci_hcd_ambarella_drv_probe,
	.remove		= ohci_hcd_ambarella_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver		= {
		.name	= "ambarella-ohci",
		.owner	= THIS_MODULE,
		.pm	= AMBARELLA_OHCI_PMOPS,
	},
};

MODULE_ALIAS("platform:ambarella-ohci");
