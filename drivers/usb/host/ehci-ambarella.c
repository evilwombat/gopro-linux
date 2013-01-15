/*
* linux/drivers/usb/host/ehci-ambarella.c
* driver for High speed (USB2.0) USB host controller on Ambarella processors
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

struct ehci_ambarella {
	struct ehci_hcd ehci;
	struct ambarella_uhc_controller *plat_ehci;
};

static struct ehci_ambarella *hcd_to_ehci_ambarella(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	return container_of(ehci, struct ehci_ambarella, ehci);
}

static void ambarella_start_ehc(struct ehci_ambarella *amb_ehci)
{
	if (amb_ehci && amb_ehci->plat_ehci->enable_host)
		amb_ehci->plat_ehci->enable_host(amb_ehci->plat_ehci);
}

static void ambarella_stop_ehc(struct ehci_ambarella *amb_ehci)
{
	if (amb_ehci && amb_ehci->plat_ehci->disable_host)
		amb_ehci->plat_ehci->disable_host();

}

static int ambarella_ehci_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct ehci_ambarella *amb_ehci = hcd_to_ehci_ambarella(hcd);
	int retval = 0;

	/* registers start at offset 0x0 */
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs +
		HC_LENGTH(ehci_readl(ehci, &ehci->caps->hc_capbase));
	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	/* EHCI will still detect 2 ports even though usb port1 is configured
	 * as device port, so we fake the port number manually and report
	 * it to EHCI.*/
	if (amb_ehci->plat_ehci->usb1_is_host == 0) {
		ehci->hcs_params &= ~0xf;
		ehci->hcs_params |= 0x1;
	}

	ehci->has_synopsys_hc_bug = 1;

	retval = ehci_halt(ehci);
	if (retval)
		return retval;

	/* data structure init */
	retval = ehci_init(hcd);
	if (retval)
		return retval;

	ehci->sbrn = 0x20;

	ehci_reset(ehci);
	ehci_port_power(ehci, 0);

	return retval;
}

static const struct hc_driver ehci_ambarella_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "Ambarella EHCI",
	.hcd_priv_size		= sizeof(struct ehci_ambarella),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset			= ambarella_ehci_setup,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

static int ehci_hcd_ambarella_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct ehci_ambarella *amb_ehci;
	int ret;

	if (usb_disabled())
		return -ENODEV;

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENOMEM;
	}
	hcd = usb_create_hcd(&ehci_ambarella_hc_driver, &pdev->dev, "Ambi1");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;
	hcd->regs = (void __iomem *)pdev->resource[0].start;

	amb_ehci = hcd_to_ehci_ambarella(hcd);
	amb_ehci->plat_ehci =
		(struct ambarella_uhc_controller *)pdev->dev.platform_data;
	if (amb_ehci == NULL) {
		ret = -ENODEV;
		goto amb_ehci_err;
	}

	ambarella_start_ehc(amb_ehci);

	ret = usb_add_hcd(hcd, pdev->resource[1].start, IRQF_DISABLED | IRQF_TRIGGER_HIGH);
	if (ret < 0)
		goto add_hcd_err;

	platform_set_drvdata(pdev, hcd);

	return ret;

add_hcd_err:
	ambarella_stop_ehc(amb_ehci);
amb_ehci_err:
	usb_put_hcd(hcd);
	return ret;
}

static int ehci_hcd_ambarella_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_ambarella *amb_ehci = hcd_to_ehci_ambarella(hcd);

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);
	ambarella_stop_ehc(amb_ehci);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM

/* Maybe just need to suspend/resume controller via HAL function. */
static int ehci_hcd_ambarella_drv_resume(struct device *dev)
{
	return 0;
}

static int ehci_hcd_ambarella_drv_suspend(struct device *dev)
{
	return 0;
}

static struct dev_pm_ops ambarella_ehci_pmops = {
	.suspend	= ehci_hcd_ambarella_drv_suspend,
	.resume		= ehci_hcd_ambarella_drv_resume,
};

#define AMBARELLA_EHCI_PMOPS &ambarella_ehci_pmops

#else
#define AMBARELLA_EHCI_PMOPS NULL
#endif

static struct platform_driver ehci_hcd_ambarella_driver = {
	.probe		= ehci_hcd_ambarella_drv_probe,
	.remove		= ehci_hcd_ambarella_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver = {
		.name	= "ambarella-ehci",
		.owner	= THIS_MODULE,
		.pm	= AMBARELLA_EHCI_PMOPS,
	}
};

MODULE_ALIAS("platform:ambarella-ehci");

