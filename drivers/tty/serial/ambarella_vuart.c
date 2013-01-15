/*
 * drivers/tty/serial/ambarella_vuart.c
 *
 * Author: Eric Chen <pzchen@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
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
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <asm/dma.h>

#include <linux/slab.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/circ_buf.h>
#include <linux/serial_reg.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>

#include <mach/hardware.h>
#include <plat/uart.h>
#include <linux/aipc/lk_vserial.h>

#define MAX_VSERIAL_AMBARELLA_PORTS 1

#define DEBUG_AMBA_VSERIAL 1
#if DEBUG_AMBA_VSERIAL
#define dbg printk
#else
#define dbg(...)
#endif

struct amba_vserial {
	struct uart_port port;
};
static struct amba_vserial *amba_vport = NULL;

int amba_vserial_report_msg(struct amba_vserial_msg *msg)
{
	struct tty_struct *tty;
	int len;
	char *str;
	unsigned long flags;

	if(msg == NULL || amba_vport == NULL)
		return -1;
#if 0
	printk("%s got message:(%p,%d)\n",__func__,msg->base_addr,msg->size);
#else
	str = (char *)msg->base_addr;
	len = msg->size;

	spin_lock_irqsave(&amba_vport->port.lock, flags);
	tty = amba_vport->port.state->port.tty;
	if(tty) {
		int max_count = amba_vport->port.fifosize;
		int i;
		for(i = 0; i < len && max_count > 0; i++, max_count--) {
			amba_vport->port.icount.rx++;
			if(!uart_handle_sysrq_char(&amba_vport->port, str[i])) {
				uart_insert_char(&amba_vport->port, UART_LS_DR, UART_LS_OE, str[i], TTY_NORMAL);
			}
		}
		tty_flip_buffer_push(tty);
	}
	spin_unlock_irqrestore(&amba_vport->port.lock, flags);
#endif
	return 0;
}
EXPORT_SYMBOL(amba_vserial_report_msg);

static unsigned int vserial_ambarella_tx_empty(struct uart_port *port)
{
	return TIOCSER_TEMT;
}

static unsigned int vserial_ambarella_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void vserial_ambarella_set_mctrl(struct uart_port *port,
	unsigned int mctrl)
{
	return;
}

static void vserial_ambarella_break_ctl(struct uart_port *port, int break_state)
{
}

static int vserial_ambarella_startup(struct uart_port *port)
{
	return 0;
}

static void vserial_ambarella_shutdown(struct uart_port *port)
{
}

static void vserial_ambarella_set_termios(struct uart_port *port,
	struct ktermios *termios, struct ktermios *old)
{
}

static void vserial_ambarella_pm(struct uart_port *port,
	unsigned int state, unsigned int oldstate)
{
}

static void vserial_ambarella_release_port(struct uart_port *port)
{
}

static int vserial_ambarella_request_port(struct uart_port *port)
{
	return 0;
}

static void vserial_ambarella_config_port(struct uart_port *port, int flags)
{
}

static int vserial_ambarella_verify_port(struct uart_port *port,
					struct serial_struct *ser)
{
	return 0;
}

static const char *vserial_ambarella_type(struct uart_port *port)
{
	return "amba gps virtual uart";
}

static void vserial_ambarella_start_tx(struct uart_port *port)
{
}

static void vserial_ambarella_stop_tx(struct uart_port *port)
{
}

static void vserial_ambarella_stop_rx(struct uart_port *port)
{
}

static void vserial_ambarella_enable_ms(struct uart_port *port)
{
}

#ifdef CONFIG_CONSOLE_POLL
static void vserial_ambarella_poll_put_char(struct uart_port *port,
	unsigned char chr)
{
}

static int vserial_ambarella_poll_get_char(struct uart_port *port)
{
	return 0;
}
#endif

struct uart_ops vserial_ambarella_pops = {
	.tx_empty	= vserial_ambarella_tx_empty,
	.set_mctrl	= vserial_ambarella_set_mctrl,
	.get_mctrl	= vserial_ambarella_get_mctrl,
	.stop_tx	= vserial_ambarella_stop_tx,
	.start_tx	= vserial_ambarella_start_tx,
	.stop_rx	= vserial_ambarella_stop_rx,
	.enable_ms	= vserial_ambarella_enable_ms,
	.break_ctl	= vserial_ambarella_break_ctl,
	.startup	= vserial_ambarella_startup,
	.shutdown	= vserial_ambarella_shutdown,
	.set_termios	= vserial_ambarella_set_termios,
	.pm		= vserial_ambarella_pm,
	.type		= vserial_ambarella_type,
	.release_port	= vserial_ambarella_release_port,
	.request_port	= vserial_ambarella_request_port,
	.config_port	= vserial_ambarella_config_port,
	.verify_port	= vserial_ambarella_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_put_char	= vserial_ambarella_poll_put_char,
	.poll_get_char	= vserial_ambarella_poll_get_char,
#endif
};

static struct uart_driver vserial_ambarella_reg = {
	.owner		= THIS_MODULE,
	.driver_name	= "ambarella-vuart",
	.dev_name	= "ttyV",
	.nr		= MAX_VSERIAL_AMBARELLA_PORTS,
	.cons		= NULL,
};

struct platform_device virtual_gps_uart = {
	.name		= "ambarella-vuart",
	.id		= 0,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= NULL,
	},
};
EXPORT_SYMBOL(virtual_gps_uart);

static int __devinit vserial_ambarella_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct amba_vserial *up;

	printk("ambarella-vuart probe!\n");

	if(amba_vport!=NULL){
		printk(" %s already exists!\n", __func__);
		return 0;
	}

	up = kzalloc(sizeof(struct amba_vserial), GFP_KERNEL);
	if (up == NULL) {
		ret = -ENOMEM;
		goto serial_ambarella_probe_exit;
	}

	up->port.fifosize = 256;
	up->port.type = PORT_UART00;
	up->port.ops = &vserial_ambarella_pops;
	up->port.dev = &pdev->dev;
	up->port.line = pdev->id;

	ret = uart_add_one_port(&vserial_ambarella_reg, &up->port);
	if (ret) {
		printk("uart_add_one_port %d fail %d!\n",
			pdev->id, ret);
		goto serial_ambarella_probe_exit;
	}

	platform_set_drvdata(pdev, up);
	amba_vport = up;

	printk("ambarella-vuart probe successfully done!");
	return 0;

serial_ambarella_probe_exit:
	printk("ambarella-vuart probe Fail!");
	return ret;
}

static int __devexit vserial_ambarella_remove(struct platform_device *pdev)
{
	int ret				= 0;
	struct amba_vserial *up = platform_get_drvdata(pdev);

	if( up != NULL && amba_vport == up) {
		platform_set_drvdata(pdev, NULL);
		ret = uart_remove_one_port(&vserial_ambarella_reg, &up->port);
		kfree(up);
		amba_vport = NULL;
	}

	return ret;
}

static struct platform_driver vserial_ambarella_driver = {
	.probe	= vserial_ambarella_probe,
	.remove = __devexit_p(vserial_ambarella_remove),
	.driver	= {
		.name = "ambarella-vuart",
	},
};

static int __init vserial_ambarella_init(void)
{
	int ret = 0;

	ret = uart_register_driver(&vserial_ambarella_reg);
	if (ret != 0)
		return ret;
	ret = platform_driver_register(&vserial_ambarella_driver);
	if (ret != 0)
		uart_unregister_driver(&vserial_ambarella_reg);
	return ret;
}


static void __exit vserial_ambarella_exit(void)
{
	platform_driver_unregister(&vserial_ambarella_driver);
	uart_unregister_driver(&vserial_ambarella_reg);
}

module_init(vserial_ambarella_init);
module_exit(vserial_ambarella_exit);
MODULE_AUTHOR("Eric Chen");
MODULE_DESCRIPTION("Ambarella boss virtual uart");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ambarella-vuart");
