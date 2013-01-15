/*
* amb_stream.c -- Ambarella Data Streaming Gadget
*
* History:
*	2009/01/01 - [Cao Rongrong] created file
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


/*
 * Ambarella Data Streaming Gadget only needs two bulk endpoints, and it
 * supports single configurations.
 *
 * Module options include:
 *   buflen=N	default N=4096, buffer size used
 *
 */

//#define DEBUG 1
// #define VERBOSE

#include <linux/utsname.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"

#include <mach/hardware.h>

#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"

#define DRIVER_VERSION		"1 Jan 2009"

static const char shortname [] = "g_amb_stream";
static const char longname [] = "Ambarella Data Streaming Gadget";


/*-------------------------------------------------------------------------*/

#ifdef DEBUG
#define AMB_DBG(dev,fmt,args...) \
	dev_printk(KERN_DEBUG , &(dev)->gadget->dev , fmt , ## args)
#else
#define AMB_DBG(dev,fmt,args...) \
	do { } while (0)
#endif /* DEBUG */

#define AMB_ERROR(dev,fmt,args...) \
	dev_printk(KERN_ERR , &(dev)->gadget->dev , fmt , ## args)

#define AMB_INFO(dev,fmt,args...) \
	dev_printk(KERN_INFO , &(dev)->gadget->dev , fmt , ## args)


/*-------------------------------------------------------------------------*/
#define AG_NOTIFY_INTERVAL		5	/* 1 << 5 == 32 msec */
#define AG_NOTIFY_MAXPACKET		8

#define SIMPLE_CMD_SIZE		32
struct amb_cmd {
	u32 signature;
	u32 command;
	u32 parameter[(SIMPLE_CMD_SIZE / sizeof(u32)) - 2];
};

struct amb_rsp {
	u32 signature;
	u32 response;
	u32 parameter0;
	u32 parameter1;
};

struct amb_ack {
	u32 signature;
	u32 acknowledge;
	u32 parameter0;
	u32 parameter1;
};

/* for bNotifyType */
#define PORT_STATUS_CHANGE		0x55
#define PORT_NOTIFY_IDLE		0xff
/* for value */
#define PORT_FREE_ALL			2
#define PORT_CONNECT			1
#define PORT_NO_CONNECT		0
/* for status */
#define DEVICE_OPENED			1
#define DEVICE_NOT_OPEN		0

struct amb_notify {
	u16	bNotifyType;
	u16	port_id;
	u16	value;
	u16	status;
};

struct amb_notify g_port = {
	.bNotifyType = PORT_NOTIFY_IDLE,
	.port_id = 0xffff,
	.value = 0,
};

/*-------------------------------------------------------------------------*/

#define AMB_DATA_STREAM_MAGIC	'u'
#define AMB_DATA_STREAM_WR_RSP	_IOW(AMB_DATA_STREAM_MAGIC, 1, struct amb_rsp *)
#define AMB_DATA_STREAM_RD_CMD	_IOR(AMB_DATA_STREAM_MAGIC, 1, struct amb_cmd *)
#define AMB_DATA_STREAM_STATUS_CHANGE	_IOW(AMB_DATA_STREAM_MAGIC, 2, struct amb_notify *)

/*-------------------------------------------------------------------------*/

#define AMB_GADGET_MAJOR		AMBA_DEV_MAJOR
#define AMB_GADGET_MINOR_START		(AMBA_DEV_MINOR_PUBLIC_START + 0)

static struct cdev ag_cdev;

/* big enough to hold our biggest descriptor */
#define USB_BUFSIZ	256

struct amb_dev {
	spinlock_t		lock;
	wait_queue_head_t	wq;
	struct mutex		mtx;

	u8			config;

	struct usb_gadget	*gadget;
	struct usb_request	*ctrl_req;
	struct usb_request	*notify_req;

	struct usb_ep		*in_ep;
	struct usb_ep		*out_ep;
	struct usb_ep		*notify_ep;
	/* ep descriptor */
	struct usb_endpoint_descriptor *in_ep_desc;
	struct usb_endpoint_descriptor *out_ep_desc;
	struct usb_endpoint_descriptor *notify_ep_desc;

	struct list_head		in_idle_list;	/* list of idle write requests */
	struct list_head		in_queue_list;	/* list of queueing write requests */
	struct list_head		out_req_list;	/* list of bulk out requests */

	int			open_count;
	int			error;
};

struct amb_dev *ag_device;

static void notify_worker(struct work_struct *work);
static DECLARE_WORK(notify_work, notify_worker);

/*-------------------------------------------------------------------------*/

static unsigned int buflen = (48*1024);
module_param (buflen, uint, S_IRUGO);
MODULE_PARM_DESC(buflen, "buffer length, default=48K");

static unsigned int qdepth = 5;
module_param (qdepth, uint, S_IRUGO);
MODULE_PARM_DESC(qdepth, "bulk transfer queue depth, default=5");

/*-------------------------------------------------------------------------*/

/*
 * DESCRIPTORS ... most are static, but strings and (full)
 * configuration descriptors are built on demand.
 */

#define STRING_MANUFACTURER		25
#define STRING_PRODUCT			42
#define STRING_SERIAL			101
#define STRING_SOURCE_SINK		250

#define DRIVER_VENDOR_ID	0x4255		/* Ambarella */
#define DRIVER_PRODUCT_ID	0x0001

static struct usb_device_descriptor ag_device_desc = {
	.bLength =		sizeof ag_device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		__constant_cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.idVendor =		__constant_cpu_to_le16 (DRIVER_VENDOR_ID),
	.idProduct =		__constant_cpu_to_le16 (DRIVER_PRODUCT_ID),
	.iManufacturer =	STRING_MANUFACTURER,
	.iProduct =		STRING_PRODUCT,
	.iSerialNumber =	STRING_SERIAL,
	.bNumConfigurations =	1,
};

static struct usb_config_descriptor amb_bulk_config = {
	.bLength =		sizeof amb_bulk_config,
	.bDescriptorType =	USB_DT_CONFIG,

	.bNumInterfaces =	1,
	.bConfigurationValue =	1,
	.iConfiguration =	STRING_SOURCE_SINK,
	.bmAttributes =		USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower =		1,	/* self-powered */
};


static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,

	.bmAttributes =		USB_OTG_SRP,
};

/* one interface in each configuration */
static struct usb_interface_descriptor amb_data_stream_intf = {
	.bLength =		sizeof amb_data_stream_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	3,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	.iInterface =		STRING_SOURCE_SINK,
};


/* two full speed bulk endpoints; their use is config-dependent */

static struct usb_endpoint_descriptor fs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(AG_NOTIFY_MAXPACKET),
	.bInterval =		1 << AG_NOTIFY_INTERVAL,
};

static const struct usb_descriptor_header *fs_amb_data_stream_function [] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	(struct usb_descriptor_header *) &amb_data_stream_intf,
	(struct usb_descriptor_header *) &fs_bulk_out_desc,
	(struct usb_descriptor_header *) &fs_bulk_in_desc,
	(struct usb_descriptor_header *) &fs_intr_in_desc,
	NULL,
};


#ifdef	CONFIG_USB_GADGET_DUALSPEED

static struct usb_endpoint_descriptor hs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	/* bEndpointAddress will be copied from fs_bulk_in_desc during amb_bind() */
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor hs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor hs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(AG_NOTIFY_MAXPACKET),
	.bInterval =		AG_NOTIFY_INTERVAL + 4,
};

static struct usb_qualifier_descriptor dev_qualifier = {
	.bLength =		sizeof dev_qualifier,
	.bDescriptorType =	USB_DT_DEVICE_QUALIFIER,

	.bcdUSB =		__constant_cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.bNumConfigurations =	1,
};

static const struct usb_descriptor_header *hs_amb_data_stream_function [] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	(struct usb_descriptor_header *) &amb_data_stream_intf,
	(struct usb_descriptor_header *) &hs_bulk_in_desc,
	(struct usb_descriptor_header *) &hs_bulk_out_desc,
	(struct usb_descriptor_header *) &hs_intr_in_desc,
	NULL,
};


/* maxpacket and other transfer characteristics vary by speed. */
#define ep_desc(g,hs,fs) (((g)->speed==USB_SPEED_HIGH)?(hs):(fs))

#else

/* if there's no high speed support, maxpacket doesn't change. */
#define ep_desc(g,hs,fs) fs

#endif	/* !CONFIG_USB_GADGET_DUALSPEED */

static char manufacturer [50];
static char serial [40];

/* static strings, in UTF-8 */
static struct usb_string		strings [] = {
	{ STRING_MANUFACTURER, manufacturer, },
	{ STRING_PRODUCT, longname, },
	{ STRING_SERIAL, serial, },
	{ STRING_SOURCE_SINK, "bulk in and out data", },
	{  }			/* end of list */
};

static struct usb_gadget_strings	stringtab = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings,
};

/*-------------------------------------------------------------------------*/

/* add a request to the tail of a list */
void req_put(struct amb_dev *dev,
		struct list_head *head, struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* get a request and remove it from the head of a list */
struct usb_request *req_get(struct amb_dev *dev, struct list_head *head)
{
	struct usb_request *req;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = NULL;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

/* get a request and move it from the head of "from" list
  * to the head of "to" list
  */
struct usb_request *req_move(struct amb_dev *dev,
		struct list_head *from, struct list_head *to)
{
	struct usb_request *req;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(from)) {
		req = NULL;
	} else {
		req = list_first_entry(from, struct usb_request, list);
		list_move_tail(&req->list, to);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static int amb_send(u8 *buf, u32 len)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int rval = 0;

	if (buf == NULL || len > buflen)
		return -EFAULT;

	if(wait_event_interruptible(dev->wq,
		(req = req_move(dev, &dev->in_idle_list, &dev->in_queue_list))
		|| dev->error)){
		rval = -EINTR;
		goto exit;
	}

	if(dev->error) {
		AMB_ERROR(dev, "%s: device error", __func__);
		spin_lock_irq(&dev->lock);
		if (req)
			list_move_tail(&req->list, &dev->in_idle_list);
		spin_unlock_irq(&dev->lock);
		rval = -EIO;
		goto exit;
	}

	memcpy(req->buf, buf, len);

	req->length = len;
	rval = usb_ep_queue(dev->in_ep, req, GFP_ATOMIC);
	if (rval != 0) {
		AMB_ERROR(dev, "%s: cannot queue bulk in request, "
			"rval=%d\n", __func__, rval);
		spin_lock_irq(&dev->lock);
		list_move_tail(&req->list, &dev->in_idle_list);
		spin_unlock_irq(&dev->lock);
		goto exit;
	}

exit:
	return rval;
}

static int amb_recv(u8 *buf, u32 len)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int rval = 0;

	if (buf == NULL || len > buflen)
		return -EFAULT;

	if(wait_event_interruptible(dev->wq,
		(req = req_get(dev, &dev->out_req_list)) || dev->error)){
		return -EINTR;
	}

	if (req) {
		memcpy(buf, req->buf, req->actual);

		req->length = buflen;
		if ((rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC))) {
			AMB_ERROR(dev, "%s: can't queue bulk out request, "
				"rval = %d\n", __func__, rval);
		}
	}

	if(dev->error) {
		AMB_ERROR(dev, "%s: device error", __func__);
		rval = -EIO;
	}

	return rval;
}


/*-------------------------------------------------------------------------*/
static long ag_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval = 0;
	struct amb_dev *dev = ag_device;
	struct amb_cmd _cmd;
	struct amb_rsp rsp;
	struct amb_notify notify;

	AMB_DBG(dev, "%s: Enter\n", __func__);

	mutex_lock(&dev->mtx);

	switch (cmd) {
	case AMB_DATA_STREAM_WR_RSP:

		if(copy_from_user(&rsp, (struct amb_rsp __user *)arg, sizeof(rsp))){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		rval = amb_send((u8 *)&rsp, sizeof(rsp));
		break;

	case AMB_DATA_STREAM_RD_CMD:

		rval = amb_recv((u8 *)&_cmd, sizeof(_cmd));
		if(rval < 0)
			break;

		if(copy_to_user((struct amb_cmd __user *)arg, &_cmd, sizeof(_cmd))){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		break;

	case AMB_DATA_STREAM_STATUS_CHANGE:
		if(copy_from_user(&notify, (struct amb_notify __user *)arg, sizeof(notify))){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		spin_lock_irq(&dev->lock);
		g_port.bNotifyType = notify.bNotifyType;
		g_port.port_id = notify.port_id;
		g_port.value = notify.value;
		spin_unlock_irq(&dev->lock);
		break;

	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&dev->mtx);

	AMB_DBG(dev, "%s: Exit\n", __func__);

	return rval;
}
static int ag_open(struct inode *inode, struct file *filp)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int rval = 0;

	mutex_lock(&dev->mtx);

	/* gadget have not been configured */
	if(dev->config == 0){
		rval = -ENOTCONN;
		goto exit;
	}

	if(dev->open_count > 0){
		rval = -EBUSY;
		goto exit;
	}

	dev->open_count++;
	dev->error = 0;

	/* throw away the data received on last connection */
	while ((req = req_get(dev, &dev->out_req_list))) {
		/* re-use the req again */
		req->length = buflen;
		if ((rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC))) {
			AMB_ERROR(dev, "%s: can't queue bulk out request, "
				"rval = %d\n", __func__, rval);
			break;
		}
	}
exit:
	mutex_unlock(&dev->mtx);
	return rval;
}

static int ag_release(struct inode *inode, struct file *filp)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req;
	int rval = 0;

	mutex_lock(&dev->mtx);

	/* dequeue the bulk-in request */
	while ((req = req_move(dev, &dev->in_queue_list, &dev->in_idle_list))) {
		rval = usb_ep_dequeue(dev->in_ep, req);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot dequeue bulk in request, "
				"rval=%d\n", __func__, rval);
			break;
		}
	}

	dev->open_count--;

	spin_lock_irq(&dev->lock);
	g_port.bNotifyType = PORT_STATUS_CHANGE;
	g_port.port_id = 0xffff;
	g_port.value = PORT_FREE_ALL;
	g_port.status = DEVICE_NOT_OPEN;
	spin_unlock_irq(&dev->lock);

	mutex_unlock(&dev->mtx);

	return rval;
}

static int ag_read(struct file *file, char __user *buf,
	size_t count, loff_t *ppos)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int len = 0, rval = 0;

	mutex_lock(&dev->mtx);

	while(count > 0){
		if(wait_event_interruptible(dev->wq,
			(req = req_get(dev, &dev->out_req_list)) || dev->error)){
			mutex_unlock(&dev->mtx);
			return -EINTR;
		}

		if(dev->error){
			AMB_ERROR(dev, "%s: device error", __func__);
			if (req) {
				req->length = buflen;
				rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC);
				if (rval)
					AMB_ERROR(dev, "%s: "
						"can't queue bulk out request, "
						"rval = %d\n", __func__, rval);
			}
			mutex_unlock(&dev->mtx);
			return -EIO;
		}

		if(copy_to_user(buf + len, req->buf, req->actual)){
			pr_err("len = %d, actual = %d\n", len, req->actual);
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}
		len += req->actual;
		count -= req->actual;

		req->length = buflen;
		if ((rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC))) {
			AMB_ERROR(dev, "%s: can't queue bulk out request, "
				"rval = %d\n", __func__, rval);
			len = rval;
			break;
		}

		if (len % buflen != 0)
			break;
	}

	mutex_unlock(&dev->mtx);

	return len;
}

static int ag_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int rval, size, len = 0;
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;

	mutex_lock(&dev->mtx);

	while(count > 0) {
		if(wait_event_interruptible(dev->wq,
			(req = req_move(dev, &dev->in_idle_list, &dev->in_queue_list))
			|| dev->error)){
			mutex_unlock(&dev->mtx);
			return -EINTR;
		}

		if(dev->error){
			AMB_ERROR(dev, "%s: device error", __func__);
			spin_lock_irq(&dev->lock);
			if (req)
				list_move_tail(&req->list, &dev->in_idle_list);
			spin_unlock_irq(&dev->lock);
			mutex_unlock(&dev->mtx);
			return -EIO;
		}

		size = count < buflen ? count : buflen;
		if(copy_from_user(req->buf, buf + len, size)){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		req->length = size;
		if ((count - size == 0) && (size % dev->in_ep->maxpacket == 0))
			req->zero = 1;

		rval = usb_ep_queue(dev->in_ep, req, GFP_ATOMIC);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot queue bulk in request, "
				"rval=%d\n", __func__, rval);
			list_move_tail(&req->list, &dev->in_idle_list);
			mutex_unlock(&dev->mtx);
			return rval;
		}

		count -= size;
		len += size;
	}

	mutex_unlock(&dev->mtx);

	AMB_DBG(dev, "%s: Exit\n", __func__);

	return len;
}

static struct file_operations ag_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ag_ioctl,
	.open = ag_open,
	.read = ag_read,
	.write = ag_write,
	.release = ag_release
};


static void amb_notify_complete (struct usb_ep *ep, struct usb_request *req);

static void notify_worker(struct work_struct *work)
{
	struct amb_dev		*dev = ag_device;
	struct usb_request	*req = NULL;
	struct amb_notify	*event = NULL;
	unsigned long flags;
	int rval = 0;

	if (dev && (req = dev->notify_req)) {
		event = req->buf;
		spin_lock_irqsave(&dev->lock, flags);
		memcpy(event, &g_port, sizeof(struct amb_notify));
		g_port.bNotifyType = PORT_NOTIFY_IDLE;
		g_port.port_id = 0xffff;
		g_port.value = 0;
		if(dev->open_count > 0)
			g_port.status = DEVICE_OPENED;
		else
			g_port.status = DEVICE_NOT_OPEN;

		req->length = AG_NOTIFY_MAXPACKET;
		req->complete = amb_notify_complete;
		req->context = dev;
		req->zero = !(req->length % dev->notify_ep->maxpacket);
		spin_unlock_irqrestore(&dev->lock, flags);

		rval = usb_ep_queue (dev->notify_ep, req, GFP_ATOMIC);
		if (rval < 0)
			AMB_DBG(dev, "status buf queue --> %d\n", rval);
	}
}

static int amb_config_buf (struct usb_gadget *gadget,
		u8 *buf, u8 type, unsigned index)
{
	int len;
	const struct usb_descriptor_header **function;

#ifdef CONFIG_USB_GADGET_DUALSPEED
	int hs = (gadget->speed == USB_SPEED_HIGH);
#endif

	/* one configuration will always be index 0 */
	if (index > 0)
		return -EINVAL;

#ifdef CONFIG_USB_GADGET_DUALSPEED
	if (type == USB_DT_OTHER_SPEED_CONFIG)
		hs = !hs;
	if (hs)
		function = hs_amb_data_stream_function;
	else
#endif
		function = fs_amb_data_stream_function;

	/* for now, don't advertise srp-only devices */
	if (!gadget->is_otg)
		function++;

	len = usb_gadget_config_buf (&amb_bulk_config,
			buf, USB_BUFSIZ, function);
	if (len < 0)
		return len;

	((struct usb_config_descriptor *) buf)->bDescriptorType = type;

	return len;
}

static struct usb_request *amb_alloc_buf_req (struct usb_ep *ep,
		unsigned length, gfp_t kmalloc_flags)
{
	struct usb_request	*req;

	req = usb_ep_alloc_request (ep, kmalloc_flags);
	if (req) {
		req->length = length;
		req->buf = kmalloc(length, kmalloc_flags);
		if (!req->buf) {
			usb_ep_free_request (ep, req);
			req = NULL;
		}
	}
	return req;
}

static void amb_free_buf_req (struct usb_ep *ep, struct usb_request *req)
{
	if(req){
		if (req->buf){
			kfree(req->buf);
			req->buf = NULL;
		}
		usb_ep_free_request (ep, req);
	}
}

static void amb_bulk_in_complete (struct usb_ep *ep, struct usb_request *req)
{
	struct amb_dev	*dev = ep->driver_data;
	int		status = req->status;
	unsigned long	flags;
	int 		rval = 0;

	spin_lock_irqsave(&dev->lock, flags);
	switch (status) {
	case 0: 			/* normal completion? */
		dev->error = 0;
		list_move_tail(&req->list, &dev->in_idle_list);
		break;
	/* this endpoint is normally active while we're configured */
	case -ECONNRESET:		/* request dequeued */
		dev->error = 1;
		usb_ep_fifo_flush(ep);
		break;
	case -ESHUTDOWN:		/* disconnect from host */
		dev->error = 1;
		amb_free_buf_req (ep, req);
		break;
	default:
		AMB_ERROR(dev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
		dev->error = 1;
		/* queue request again */
		rval = usb_ep_queue(dev->in_ep, req, GFP_ATOMIC);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot queue bulk in request, "
				"rval=%d\n", __func__, rval);
		}
		break;
	}
	wake_up_interruptible(&dev->wq);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void amb_bulk_out_complete (struct usb_ep *ep, struct usb_request *req)
{
	struct amb_dev	*dev = ep->driver_data;
	int		status = req->status;
	unsigned long	flags;
	int 		rval = 0;

	spin_lock_irqsave(&dev->lock, flags);
	switch (status) {
	case 0: 			/* normal completion */
		dev->error = 0;
		list_add_tail(&req->list, &dev->out_req_list);
		break;
	/* this endpoint is normally active while we're configured */
	case -ECONNRESET:		/* request dequeued */
		dev->error = 1;
		usb_ep_fifo_flush(ep);
		break;
	case -ESHUTDOWN:		/* disconnect from host */
		dev->error = 1;
		amb_free_buf_req (ep, req);
		break;
	default:
		AMB_ERROR(dev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
		dev->error = 1;
		/* queue request again */
		rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot queue bulk in request, "
				"rval=%d\n", __func__, rval);
		}
		break;
	}
	wake_up_interruptible(&dev->wq);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void amb_notify_complete (struct usb_ep *ep, struct usb_request *req)
{
	int				status = req->status;
	struct amb_dev			*dev = ep->driver_data;
	unsigned long 			flags;

	spin_lock_irqsave(&dev->lock, flags);
	req->context = NULL;

	switch (status) {
	case 0: 			/* normal completion */
		/* issue the second notification if host reads the previous one */
		schedule_work(&notify_work);
		break;

	/* this endpoint is normally active while we're configured */
	case -ECONNRESET:		/* request dequeued */
		usb_ep_fifo_flush(ep);
		break;
	case -ESHUTDOWN:		/* disconnect from host */
		amb_free_buf_req (ep, req);
		break;
	default:
		AMB_ERROR(dev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
		break;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void amb_start_notify (struct amb_dev *dev)
{
	struct usb_request *req = dev->notify_req;
	struct amb_notify *event;
	int value;

	/* flush old status
	 * FIXME iff req->context != null just dequeue it
	 */
	usb_ep_disable (dev->notify_ep);
	usb_ep_enable (dev->notify_ep, dev->notify_ep_desc);

	event = req->buf;
	event->bNotifyType = __constant_cpu_to_le16(PORT_NOTIFY_IDLE);
	event->port_id = __constant_cpu_to_le16 (0);
	event->value = __constant_cpu_to_le32 (0);
	event->status = DEVICE_NOT_OPEN;

	req->length = AG_NOTIFY_MAXPACKET;
	req->complete = amb_notify_complete;
	req->context = dev;
	req->zero = !(req->length % dev->notify_ep->maxpacket);

	value = usb_ep_queue (dev->notify_ep, req, GFP_ATOMIC);
	if (value < 0)
		AMB_DBG (dev, "status buf queue --> %d\n", value);
}

static void amb_reset_config (struct amb_dev *dev)
{
	struct usb_request *req;
	unsigned long flags;

	if (dev == NULL) {
		pr_err("amb_reset_config: NULL device pointer\n");
		return;
	}

	if (dev->config == 0)
		return;
	dev->config = 0;

	spin_lock_irqsave(&dev->lock, flags);

	/* free write requests on the free list */
	while(!list_empty(&dev->in_idle_list)) {
		req = list_entry(dev->in_idle_list.next,
				struct usb_request, list);
		list_del_init(&req->list);
		req->length = buflen;
		amb_free_buf_req(dev->in_ep, req);
	}
	while(!list_empty(&dev->in_queue_list)) {
		req = list_entry(dev->in_queue_list.prev,
				struct usb_request, list);
		list_del_init(&req->list);
		req->length = buflen;
		amb_free_buf_req(dev->in_ep, req);
	}

	while(!list_empty(&dev->out_req_list)) {
		req = list_entry(dev->out_req_list.prev,
				struct usb_request, list);
		list_del_init(&req->list);
		req->length = buflen;
		amb_free_buf_req(dev->out_ep, req);
	}

	spin_unlock_irqrestore(&dev->lock, flags);
	/* just disable endpoints, forcing completion of pending i/o.
	 * all our completion handlers free their requests in this case.
	 */
	/* Disable the endpoints */
	if (dev->notify_ep)
		usb_ep_disable(dev->notify_ep);
	usb_ep_disable(dev->in_ep);
	usb_ep_disable(dev->out_ep);
}

static int amb_set_stream_config (struct amb_dev *dev, gfp_t gfp_flags)
{
	int			result = 0, i;
	struct usb_gadget	*gadget = dev->gadget;
	struct usb_endpoint_descriptor	*d;
	struct usb_ep *ep;
	struct usb_request *req;

	/* one endpoint writes data in (to the host) */
	d = ep_desc (gadget, &hs_bulk_in_desc, &fs_bulk_in_desc);
	result = usb_ep_enable(dev->in_ep, d);
	if (result == 0) {
		dev->in_ep_desc = d;
	} else {
		AMB_ERROR(dev, "%s: can't enable %s, result=%d\n",
			__func__, dev->in_ep->name, result);
		goto exit;
	}

	/* one endpoint reads data out (from the host) */
	d = ep_desc (gadget, &hs_bulk_out_desc, &fs_bulk_out_desc);
	result = usb_ep_enable(dev->out_ep, d);
	if (result == 0) {
		dev->out_ep_desc = d;
	} else {
		AMB_ERROR(dev, "%s: can't enable %s, result=%d\n",
			__func__, dev->out_ep->name, result);
		usb_ep_disable(dev->in_ep);
		goto exit;
	}

	/* one endpoint report status  (to the host) */
	d = ep_desc (gadget, &hs_intr_in_desc, &fs_intr_in_desc);
	result = usb_ep_enable(dev->notify_ep, d);
	if (result == 0) {
		dev->notify_ep_desc = d;
	} else {
		AMB_ERROR(dev, "%s: can't enable %s, result=%d\n",
			__func__, dev->notify_ep->name, result);
		usb_ep_disable(dev->out_ep);
		usb_ep_disable(dev->in_ep);
		goto exit;
	}

	/* allocate and queue read requests */
	ep = dev->out_ep;
	for (i = 0; i < qdepth && result == 0; i++) {
		req = amb_alloc_buf_req(ep, buflen, GFP_ATOMIC);
		if (req) {
			req->complete = amb_bulk_out_complete;
			result = usb_ep_queue(ep, req, GFP_ATOMIC);
			if (result < 0) {
				AMB_ERROR(dev, "%s: can't queue bulk out request, "
					"rval = %d\n", __func__, result);
			}
		} else {
			AMB_ERROR(dev, "%s: can't allocate bulk in requests\n", __func__);
			result = -ENOMEM;
			goto exit;
		}
	}

	/* allocate write requests, and put on free list */
	ep = dev->in_ep;
	for (i = 0; i < qdepth; i++) {
		req = amb_alloc_buf_req(ep, buflen, GFP_ATOMIC);
		if (req) {
			req->complete = amb_bulk_in_complete;
			req_put(dev, &dev->in_idle_list, req);
		} else {
			AMB_ERROR(dev, "%s: can't allocate bulk in requests\n", __func__);
			result = -ENOMEM;
			goto exit;
		}
	}

	if (dev->notify_ep) {
		dev->notify_req = amb_alloc_buf_req(dev->notify_ep,
			AG_NOTIFY_MAXPACKET, GFP_ATOMIC);
		amb_start_notify(dev);
	}

exit:
	/* caller is responsible for cleanup on error */
	return result;
}


static int amb_set_config (struct amb_dev *dev, unsigned number, gfp_t gfp_flags)
{
	int			result = 0;
	struct usb_gadget	*gadget = dev->gadget;

	if (number == dev->config)
		return 0;

	amb_reset_config (dev);

	switch (number) {
	case 1:
		result = amb_set_stream_config(dev, gfp_flags);
		break;
	default:
		result = -EINVAL;
		/* FALL THROUGH */
	case 0:
		return result;
	}

	if (!result && (!dev->in_ep || !dev->out_ep))
		result = -ENODEV;
	if (result)
		amb_reset_config (dev);
	else {
		char *speed;

		switch (gadget->speed) {
		case USB_SPEED_LOW:	speed = "low"; break;
		case USB_SPEED_FULL:	speed = "full"; break;
		case USB_SPEED_HIGH:	speed = "high"; break;
		default: 		speed = "?"; break;
		}

		dev->config = number;
		AMB_INFO(dev, "%s speed config #%d: \n", speed, number);

		AMB_INFO(dev, "bulk_out address = 0x%02x, bulk_in address = 0x%02x,"
			" intr_in address = 0x%02x\n",
			dev->out_ep_desc->bEndpointAddress,
			dev->in_ep_desc->bEndpointAddress,
			dev->notify_ep_desc->bEndpointAddress);
	}
	return result;
}

static void amb_setup_complete (struct usb_ep *ep, struct usb_request *req)
{
	struct amb_dev *dev;

	dev = ep->driver_data;

	if (req->status || req->actual != req->length) {
		AMB_DBG(dev, "setup complete --> %d, %d/%d\n",
			req->status, req->actual, req->length);
	}
}

static int amb_setup (struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct amb_dev		*dev = get_gadget_data (gadget);
	struct usb_request	*req = dev->ctrl_req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	req->zero = 0;
	switch (ctrl->bRequest) {

	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;

		switch (w_value >> 8) {
		case USB_DT_DEVICE:
			value = min (w_length, (u16) sizeof ag_device_desc);
			memcpy (req->buf, &ag_device_desc, value);
			break;
#ifdef CONFIG_USB_GADGET_DUALSPEED
		case USB_DT_DEVICE_QUALIFIER:
			if (!gadget->is_dualspeed)
				break;
			value = min (w_length, (u16) sizeof dev_qualifier);
			memcpy (req->buf, &dev_qualifier, value);
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
			if (!gadget->is_dualspeed)
				break;
			/* fall through */
#endif /* CONFIG_USB_GADGET_DUALSPEED */
		case USB_DT_CONFIG:
			value = amb_config_buf (gadget, req->buf,
					w_value >> 8,
					w_value & 0xff);
			if (value >= 0)
				value = min (w_length, (u16) value);
			break;

		case USB_DT_STRING:
			/* wIndex == language code.
			 * this driver only handles one language, you can
			 * add string tables for other languages, using
			 * any UTF-8 characters
			 */
			value = usb_gadget_get_string (&stringtab,
					w_value & 0xff, req->buf);
			if (value >= 0)
				value = min (w_length, (u16) value);
			break;
		}
		break;

	/* currently two configs, two speeds */
	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != 0)
			goto unknown;
		value = amb_set_config (dev, w_value, GFP_ATOMIC);
		break;

	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		*(u8 *)req->buf = dev->config;
		value = min (w_length, (u16) 1);
		break;

	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != USB_RECIP_INTERFACE)
			goto unknown;
		if (dev->config && w_index == 0 && w_value == 0) {
			u8		config = dev->config;

			/* resets interface configuration, forgets about
			 * previous transaction state (queued bufs, etc)
			 * and re-inits endpoint state (toggle etc)
			 * no response queued, just zero status == success.
			 * if we had more than one interface we couldn't
			 * use this "reset the config" shortcut.
			 */
			amb_set_config (dev, config, GFP_ATOMIC);
			value = 0;
		}
		break;
	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE))
			goto unknown;
		if (!dev->config)
			break;
		if (w_index != 0) {
			value = -EDOM;
			break;
		}
		*(u8 *)req->buf = 0;
		value = min (w_length, (u16) 1);
		break;

	default:
unknown:
		AMB_DBG(dev, "unknown control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer before status phase? */
	if (value >= 0) {
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue (gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			AMB_DBG(dev, "ep_queue --> %d\n", value);
			req->status = 0;
			amb_setup_complete (gadget->ep0, req);
		}
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static void amb_disconnect (struct usb_gadget *gadget)
{
	struct amb_dev		*dev = get_gadget_data (gadget);

	amb_reset_config (dev);

	/* a more significant application might have some non-usb
	 * activities to quiesce here, saving resources like power
	 * or pushing the notification up a network stack.
	 */

	/* next we may get setup() calls to enumerate new connections;
	 * or an unbind() during shutdown (including removing module).
	 */
}

static void __exit amb_unbind (struct usb_gadget *gadget)
{
	struct amb_dev		*dev = get_gadget_data (gadget);

	AMB_DBG(dev, "unbind\n");

	/* we've already been disconnected ... no i/o is active */
	if (dev->ctrl_req) {
		dev->ctrl_req->length = USB_BUFSIZ;
		amb_free_buf_req (gadget->ep0, dev->ctrl_req);
		dev->ctrl_req = NULL;
	}

	kfree (dev);
	set_gadget_data (gadget, NULL);
	ag_device = NULL;
}

static int __ref amb_bind (struct usb_gadget *gadget)
{
	struct amb_dev		*dev;
	struct usb_ep		*ep;
	int			gcnum;

	gcnum = usb_gadget_controller_number (gadget);
	if (gcnum >= 0)
		ag_device_desc.bcdDevice = cpu_to_le16 (0x0200 + gcnum);
	else {
		pr_warn("%s: controller '%s' not recognized\n",
			shortname, gadget->name);
		ag_device_desc.bcdDevice = __constant_cpu_to_le16 (0x9999);
	}

	/* ok, we made sense of the hardware ... */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init (&dev->lock);
	mutex_init(&dev->mtx);
	init_waitqueue_head(&dev->wq);
	INIT_LIST_HEAD(&dev->in_idle_list);
	INIT_LIST_HEAD(&dev->in_queue_list);
	INIT_LIST_HEAD(&dev->out_req_list);

	dev->gadget = gadget;
	dev->config = 0;
	dev->error = 0;
	ag_device = dev;
	set_gadget_data (gadget, dev);

	/* configure the endpoints */
	usb_ep_autoconfig_reset (gadget);

	ep = usb_ep_autoconfig (gadget, &fs_bulk_in_desc);
	if (!ep) {
		AMB_ERROR(dev, "amb_bind: cannot get bulk-in endpoint\n");
		return -ENODEV;
	}
	ep->driver_data = dev;	/* claim the endpoint */
	dev->in_ep = ep;

	ep = usb_ep_autoconfig (gadget, &fs_bulk_out_desc);
	if (!ep) {
		AMB_ERROR(dev, "amb_bind: cannot get bulk-out endpoint\n");
		return -ENODEV;
	}
	ep->driver_data = dev;	/* claim the endpoint */
	dev->out_ep = ep;

	ep = usb_ep_autoconfig(gadget, &fs_intr_in_desc);
	if (!ep) {
		AMB_ERROR(dev, "amb_bind: cannot use notify endpoint\n");
		return -ENODEV;
	}
	dev->notify_ep = ep;
	ep->driver_data = dev;	/* claim the endpoint */

	/* preallocate control response and buffer */
	dev->ctrl_req = amb_alloc_buf_req (gadget->ep0, USB_BUFSIZ, GFP_KERNEL);
	if (dev->ctrl_req == NULL) {
		AMB_ERROR(dev, "%s: No memory\n", __func__);
		amb_unbind (gadget);
		return -ENOMEM;
	}
	dev->ctrl_req->complete = amb_setup_complete;
	gadget->ep0->driver_data = dev;

	ag_device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;

#ifdef CONFIG_USB_GADGET_DUALSPEED
	/* assume ep0 uses the same value for both speeds ... */
	dev_qualifier.bMaxPacketSize0 = ag_device_desc.bMaxPacketSize0;

	/* and that all endpoints are dual-speed */
	hs_bulk_in_desc.bEndpointAddress = fs_bulk_in_desc.bEndpointAddress;
	hs_bulk_out_desc.bEndpointAddress = fs_bulk_out_desc.bEndpointAddress;
	hs_intr_in_desc.bEndpointAddress = fs_intr_in_desc.bEndpointAddress;
#endif
	if (gadget->is_otg) {
		otg_descriptor.bmAttributes |= USB_OTG_HNP,
		amb_bulk_config.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	usb_gadget_set_selfpowered (gadget);

	gadget->ep0->driver_data = dev;

	strlcpy (serial, "123456789ABC", sizeof serial);
	snprintf (manufacturer, sizeof manufacturer, "%s %s with %s",
		init_utsname()->sysname, init_utsname()->release,
		gadget->name);

	AMB_INFO(dev, "%s, version: " DRIVER_VERSION "\n", longname);

	return 0;
}

static void amb_suspend (struct usb_gadget *gadget)
{
	struct amb_dev		*dev = NULL;

	if (gadget->speed == USB_SPEED_UNKNOWN)
		return;

	dev = get_gadget_data (gadget);
	AMB_DBG(dev, "suspend\n");
}

static void amb_resume (struct usb_gadget *gadget)
{
	struct amb_dev		*dev = NULL;

	dev = get_gadget_data (gadget);
	AMB_DBG(dev, "resume\n");
}

static struct usb_gadget_driver amb_gadget_driver = {
#ifdef CONFIG_USB_GADGET_DUALSPEED
	.speed		= USB_SPEED_HIGH,
#else
	.speed		= USB_SPEED_FULL,
#endif
	.function	= (char *) longname,
	.unbind		= __exit_p(amb_unbind),

	.setup		= amb_setup,
	.disconnect	= amb_disconnect,

	.suspend	= amb_suspend,
	.resume		= amb_resume,

	.driver 	= {
		.name		= (char *) shortname,
		.owner		= THIS_MODULE,
	},
};

static int __init amb_gadget_init(void)
{
	int rval = 0;
	dev_t dev_id;

	rval = usb_gadget_probe_driver(&amb_gadget_driver, amb_bind);
	if (rval) {
		pr_err("amb_gadget_init: cannot register gadget driver, "
			"rval=%d\n", rval);
		goto out0;
	}

	if (buflen >= 65536) {
		pr_err("amb_gadget_init: buflen is too large\n");
		rval = -EINVAL;
		goto out1;
	}

	dev_id = MKDEV(AMB_GADGET_MAJOR, AMB_GADGET_MINOR_START);
	rval = register_chrdev_region(dev_id, 1, "amb_gadget");
	if(rval < 0){
		pr_err("amb_gadget_init: register devcie number error!\n");
		goto out1;
	}

	cdev_init(&ag_cdev, &ag_fops);
	ag_cdev.owner = THIS_MODULE;
	rval = cdev_add(&ag_cdev, dev_id, 1);
	if (rval) {
		pr_err("amb_gadget_init: cdev_add failed\n");
		unregister_chrdev_region(dev_id, 1);
		goto out1;
	}

out1:
	if(rval)
		usb_gadget_unregister_driver(&amb_gadget_driver);
out0:
	return rval;
}
module_init (amb_gadget_init);

static void __exit amb_gadget_exit (void)
{
	dev_t dev_id;

	usb_gadget_unregister_driver (&amb_gadget_driver);

	dev_id = MKDEV(AMB_GADGET_MAJOR, AMB_GADGET_MINOR_START);
	unregister_chrdev_region(dev_id, 1);
	cdev_del(&ag_cdev);
}
module_exit (amb_gadget_exit);


MODULE_AUTHOR ("Cao Rongrong <rrcao@ambarella.com>");
MODULE_LICENSE ("GPL");


