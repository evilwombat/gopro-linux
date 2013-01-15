/*
 * drivers/tty/serial/ambarella_uart.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#if defined(CONFIG_SERIAL_AMBARELLA_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <asm/dma.h>

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

#if defined(CONFIG_PLAT_AMBARELLA_BOSS)
#include <mach/boss.h>
#endif

/* ==========================================================================*/
static void __serial_ambarella_enable_ms(struct uart_port *port)
{
	amba_setbitsl(port->membase + UART_IE_OFFSET, UART_IE_EDSSI);
}

static void __serial_ambarella_disable_ms(struct uart_port *port)
{
	amba_clrbitsl(port->membase + UART_IE_OFFSET, UART_IE_EDSSI);
}

static inline void wait_for_tx(struct uart_port *port)
{
	u32					ls;

	ls = amba_readl(port->membase + UART_LS_OFFSET);
	while ((ls & UART_LS_TEMT) != UART_LS_TEMT) {
		cpu_relax();
		ls = amba_readl(port->membase + UART_LS_OFFSET);
	}
}

static inline void wait_for_rx(struct uart_port *port)
{
	u32					ls;

	ls = amba_readl(port->membase + UART_LS_OFFSET);
	while ((ls & UART_LS_DR) != UART_LS_DR) {
		cpu_relax();
		ls = amba_readl(port->membase + UART_LS_OFFSET);
	}
}

/* ==========================================================================*/
static inline void serial_ambarella_receive_chars(struct uart_port *port,
	u32 tmo)
{
	struct tty_struct			*tty = port->state->port.tty;
	u32					ch;
	u32					flag;
	int					max_count;
	u32					ls;

	ls = amba_readl(port->membase + UART_LS_OFFSET);
	max_count = port->fifosize;

	do {
		flag = TTY_NORMAL;
		if (unlikely(ls & (UART_LS_BI | UART_LS_PE |
					UART_LS_FE | UART_LS_OE))) {
			if (ls & UART_LS_BI) {
				ls &= ~(UART_LS_FE | UART_LS_PE);
				port->icount.brk++;

				if (uart_handle_break(port))
					goto ignore_char;
			}
			if (ls & UART_LS_FE)
				port->icount.frame++;
			if (ls & UART_LS_PE)
				port->icount.parity++;
			if (ls & UART_LS_OE)
				port->icount.overrun++;

			ls &= port->read_status_mask;

			if (ls & UART_LS_BI)
				flag = TTY_BREAK;
			else if (ls & UART_LS_FE)
				flag = TTY_FRAME;
			else if (ls & UART_LS_PE)
				flag = TTY_PARITY;
			else if (ls & UART_LS_OE)
				flag = TTY_OVERRUN;

			if (ls & UART_LS_OE) {
				printk(KERN_DEBUG "%s: OVERFLOW\n", __func__);
			}
		}

		if (likely(ls & UART_LS_DR)) {
			ch = amba_readl(port->membase + UART_RB_OFFSET);
			port->icount.rx++;
			tmo = 0;

			if (uart_handle_sysrq_char(port, ch))
				goto ignore_char;

			uart_insert_char(port, ls, UART_LS_OE, ch, flag);
		} else {
			if (tmo) {
				ch = amba_readl(port->membase + UART_RB_OFFSET);
				printk(KERN_DEBUG "False TMO get %d\n", ch);
			}
		}

ignore_char:
		ls = amba_readl(port->membase + UART_LS_OFFSET);
	} while ((ls & UART_LS_DR) && (max_count-- > 0));

	spin_unlock(&port->lock);
	tty_flip_buffer_push(tty);
	spin_lock(&port->lock);
}

static void serial_ambarella_transmit_chars(struct uart_port *port)
{
	struct circ_buf				*xmit = &port->state->xmit;
	int					count;
	struct ambarella_uart_port_info		*port_info;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	if (port->x_char) {
		amba_writel(port->membase + UART_TH_OFFSET, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}

	if (uart_tx_stopped(port) || uart_circ_empty(xmit)) {
		port_info->stop_tx(port->membase);
		return;
	}

	count = port->fifosize;
	while (count-- > 0) {
		amba_writel(port->membase + UART_TH_OFFSET,
			xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	}
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
	if (uart_circ_empty(xmit))
		port_info->stop_tx(port->membase);
}

static inline void serial_ambarella_check_modem_status(struct uart_port *port)
{
	struct ambarella_uart_port_info		*port_info;
	u32					ms;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	if (port_info->get_ms) {
		ms = port_info->get_ms(port->membase);

		if (ms & UART_MS_RI)
			port->icount.rng++;
		if (ms & UART_MS_DSR)
			port->icount.dsr++;
		if (ms & UART_MS_DCTS)
			uart_handle_cts_change(port, (ms & UART_MS_CTS));
		if (ms & UART_MS_DDCD)
			uart_handle_dcd_change(port, (ms & UART_MS_DCD));

		wake_up_interruptible(&port->state->port.delta_msr_wait);
	}
}

static irqreturn_t serial_ambarella_irq(int irq, void *dev_id)
{
	struct uart_port			*port = dev_id;
	struct ambarella_uart_port_info		*port_info;
	int					rval = IRQ_HANDLED;
	u32					ii;
	unsigned long				flags;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	spin_lock_irqsave(&port->lock, flags);

	ii = amba_readl(port->membase + UART_II_OFFSET);
	switch (ii & 0x0F) {
	case UART_II_MODEM_STATUS_CHANGED:
		serial_ambarella_check_modem_status(port);
		break;

	case UART_II_THR_EMPTY:
		serial_ambarella_transmit_chars(port);
		break;

	case UART_II_RCV_STATUS:
	case UART_II_RCV_DATA_AVAIL:
		serial_ambarella_receive_chars(port, 0);
		break;
	case UART_II_CHAR_TIMEOUT:
		serial_ambarella_receive_chars(port, 1);
		break;

	case UART_II_NO_INT_PENDING:
	default:
		printk(KERN_DEBUG "%s: 0x%x\n", __func__, ii);
		rval = IRQ_NONE;
		break;
	}

	spin_unlock_irqrestore(&port->lock, flags);

	return rval;
}

/* ==========================================================================*/
static void serial_ambarella_enable_ms(struct uart_port *port)
{
	__serial_ambarella_enable_ms(port);
}

static void serial_ambarella_start_tx(struct uart_port *port)
{
	amba_setbitsl(port->membase + UART_IE_OFFSET, UART_IE_ETBEI);
}

static void serial_ambarella_stop_tx(struct uart_port *port)
{
	struct ambarella_uart_port_info		*port_info;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	port_info->stop_tx(port->membase);
}

static void serial_ambarella_stop_rx(struct uart_port *port)
{
	amba_clrbitsl(port->membase + UART_IE_OFFSET, UART_IE_ERBFI);
}

static unsigned int serial_ambarella_tx_empty(struct uart_port *port)
{
	unsigned long				flags;
	unsigned int				lsr;

	spin_lock_irqsave(&port->lock, flags);
	lsr = amba_readl(port->membase + UART_LS_OFFSET);
	spin_unlock_irqrestore(&port->lock, flags);

	return ((lsr & (UART_LS_TEMT | UART_LS_THRE)) ==
		(UART_LS_TEMT | UART_LS_THRE)) ? TIOCSER_TEMT : 0;
}

static unsigned int serial_ambarella_get_mctrl(struct uart_port *port)
{
	unsigned int				mctrl = 0;
	struct ambarella_uart_port_info		*port_info;
	u32					ms;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	if (port_info->get_ms) {
		ms = port_info->get_ms(port->membase);

		if (ms & UART_MS_CTS)
			mctrl |= TIOCM_CTS;
		if (ms & UART_MS_DSR)
			mctrl |= TIOCM_DSR;
		if (ms & UART_MS_RI)
			mctrl |= TIOCM_RI;
		if (ms & UART_MS_DCD)
			mctrl |= TIOCM_CD;
	}

	return mctrl;
}

static void serial_ambarella_set_mctrl(struct uart_port *port,
	unsigned int mctrl)
{
	struct ambarella_uart_port_info		*port_info;
	u32					mcr;
	u32					mcr_new = 0;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	if (port_info->get_ms) {
		mcr = amba_readl(port->membase + UART_MC_OFFSET);

		if (mctrl & TIOCM_DTR)
			mcr_new |= UART_MC_DTR;
		if (mctrl & TIOCM_RTS)
			mcr_new |= UART_MC_RTS;
		if (mctrl & TIOCM_OUT1)
			mcr_new |= UART_MC_OUT1;
		if (mctrl & TIOCM_OUT2)
			mcr_new |= UART_MC_OUT2;
		if (mctrl & TIOCM_LOOP)
			mcr_new |= UART_MC_LB;

		mcr_new |= port_info->mcr;
		if (mcr_new != mcr) {
			if ((mcr & UART_MC_AFCE) == UART_MC_AFCE) {
				mcr &= ~UART_MC_AFCE;
				amba_writel(port->membase + UART_MC_OFFSET,
					mcr);
			}
			amba_writel(port->membase + UART_MC_OFFSET, mcr_new);
		}
	}
}

static void serial_ambarella_break_ctl(struct uart_port *port, int break_state)
{
	unsigned long				flags;

	spin_lock_irqsave(&port->lock, flags);
	if (break_state != 0)
		amba_setbitsl(port->membase + UART_LC_OFFSET, UART_LC_BRK);
	else
		amba_clrbitsl(port->membase + UART_LC_OFFSET, UART_LC_BRK);
	spin_unlock_irqrestore(&port->lock, flags);
}

static int serial_ambarella_startup(struct uart_port *port)
{
	int					retval = 0;
	struct ambarella_uart_port_info		*port_info;
	unsigned long				flags;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	spin_lock_irqsave(&port->lock, flags);
	amba_setbitsl(port->membase + UART_SRR_OFFSET, 0x00);
	amba_writel(port->membase + UART_IE_OFFSET, port_info->ier |
		UART_IE_PTIME);
	amba_writel(port->membase + UART_FC_OFFSET, (port_info->fcr |
		UART_FC_XMITR | UART_FC_RCVRR));
#if defined(CONFIG_PLAT_AMBARELLA_BOSS)
	boss_set_irq_owner(port->irq, BOSS_IRQ_OWNER_LINUX, 0);	
#endif
	/*retval = request_irq(port->irq, serial_ambarella_irq,
		IRQF_TRIGGER_HIGH, dev_name(port->dev), port); */
	amba_writel(port->membase + UART_IE_OFFSET, port_info->ier);
	spin_unlock_irqrestore(&port->lock, flags);
	retval = request_irq(port->irq, serial_ambarella_irq,
		IRQF_TRIGGER_HIGH, dev_name(port->dev), port);

	return retval;
}

static void serial_ambarella_shutdown(struct uart_port *port)
{
	unsigned long				flags;

	free_irq(port->irq, port);

	spin_lock_irqsave(&port->lock, flags);
	amba_clrbitsl(port->membase + UART_LC_OFFSET, UART_LC_BRK);
	amba_setbitsl(port->membase + UART_SRR_OFFSET, 0x01);
	spin_unlock_irqrestore(&port->lock, flags);
}

static void serial_ambarella_set_termios(struct uart_port *port,
	struct ktermios *termios, struct ktermios *old)
{
	struct ambarella_uart_port_info		*port_info;
	unsigned int				baud, quot;
	u32					lc = 0x0;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);

	port->uartclk = port_info->get_pll();
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lc |= UART_LC_CLS_5_BITS;
		break;
	case CS6:
		lc |= UART_LC_CLS_6_BITS;
		break;
	case CS7:
		lc |= UART_LC_CLS_7_BITS;
		break;
	case CS8:
	default:
		lc |= UART_LC_CLS_8_BITS;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		lc |= UART_LC_STOP_2BIT;
	else
		lc |= UART_LC_STOP_1BIT;

	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & PARODD)
			lc |= (UART_LC_EPS | UART_LC_ODD_PARITY);
		else
			lc |= (UART_LC_EPS | UART_LC_EVEN_PARITY);
	}

	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk / 16);
	quot = uart_get_divisor(port, baud);

	disable_irq(port->irq);
	uart_update_timeout(port, termios->c_cflag, baud);

	port->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= UART_LSR_BI;

	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= UART_LSR_BI;
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= UART_LSR_OE;
	}
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= UART_LSR_DR;

	if ((termios->c_cflag & CRTSCTS) == 0)
		port_info->mcr &= ~UART_MC_AFCE;
	else
		port_info->mcr |= UART_MC_AFCE;

	amba_writel(port->membase + UART_LC_OFFSET, UART_LC_DLAB);
	amba_writel(port->membase + UART_DLL_OFFSET, quot & 0xff);
	amba_writel(port->membase + UART_DLH_OFFSET, (quot >> 8) & 0xff);
	amba_writel(port->membase + UART_LC_OFFSET, lc);
	if (UART_ENABLE_MS(port, termios->c_cflag))
		__serial_ambarella_enable_ms(port);
	else
		__serial_ambarella_disable_ms(port);
	serial_ambarella_set_mctrl(port, port->mctrl);

	enable_irq(port->irq);
}

static void serial_ambarella_pm(struct uart_port *port,
	unsigned int state, unsigned int oldstate)
{
}

static void serial_ambarella_release_port(struct uart_port *port)
{
}

static int serial_ambarella_request_port(struct uart_port *port)
{
	return 0;
}

static void serial_ambarella_config_port(struct uart_port *port, int flags)
{
}

static int serial_ambarella_verify_port(struct uart_port *port,
					struct serial_struct *ser)
{
	int					retval = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_UART00)
		retval = -EINVAL;
	if (port->irq != ser->irq)
		retval = -EINVAL;
	if (ser->io_type != SERIAL_IO_MEM)
		retval = -EINVAL;

	return retval;
}

static const char *serial_ambarella_type(struct uart_port *port)
{
	return "ambuart";
}

#ifdef CONFIG_CONSOLE_POLL
static void serial_ambarella_poll_put_char(struct uart_port *port,
	unsigned char chr)
{
	struct ambarella_uart_port_info		*port_info;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);
	if (!port->suspended) {
		wait_for_tx(port);
		amba_writel(port->membase + UART_TH_OFFSET, chr);
	}
}

static int serial_ambarella_poll_get_char(struct uart_port *port)
{
	struct ambarella_uart_port_info		*port_info;

	port_info = (struct ambarella_uart_port_info *)(port->private_data);
	if (!port->suspended) {
		wait_for_rx(port);
		return amba_readl(port->membase + UART_RB_OFFSET);
	}
	return 0;
}
#endif

struct uart_ops serial_ambarella_pops = {
	.tx_empty	= serial_ambarella_tx_empty,
	.set_mctrl	= serial_ambarella_set_mctrl,
	.get_mctrl	= serial_ambarella_get_mctrl,
	.stop_tx	= serial_ambarella_stop_tx,
	.start_tx	= serial_ambarella_start_tx,
	.stop_rx	= serial_ambarella_stop_rx,
	.enable_ms	= serial_ambarella_enable_ms,
	.break_ctl	= serial_ambarella_break_ctl,
	.startup	= serial_ambarella_startup,
	.shutdown	= serial_ambarella_shutdown,
	.set_termios	= serial_ambarella_set_termios,
	.pm		= serial_ambarella_pm,
	.type		= serial_ambarella_type,
	.release_port	= serial_ambarella_release_port,
	.request_port	= serial_ambarella_request_port,
	.config_port	= serial_ambarella_config_port,
	.verify_port	= serial_ambarella_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_put_char	= serial_ambarella_poll_put_char,
	.poll_get_char	= serial_ambarella_poll_get_char,
#endif
};

/* ==========================================================================*/
#if defined(CONFIG_SERIAL_AMBARELLA_CONSOLE)
static struct uart_driver serial_ambarella_reg;

static void serial_ambarella_console_putchar(struct uart_port *port, int ch)
{
	wait_for_tx(port);
	amba_writel(port->membase + UART_TH_OFFSET, ch);
}

static void serial_ambarella_console_write(struct console *co,
	const char *s, unsigned int count)
{
	u32					ie;
	struct uart_port			*port;
	unsigned long				flags;
	int					locked = 1;

	port = (struct uart_port *)(
		ambarella_uart_ports.amba_port[co->index].port);

	if (!port->suspended) {
		local_irq_save(flags);
		if (port->sysrq) {
			locked = 0;
		} else if (oops_in_progress) {
			locked = spin_trylock(&port->lock);
		} else {
			spin_lock(&port->lock);
			locked = 1;
		}

		ie = amba_readl(port->membase + UART_IE_OFFSET);
		amba_writel(port->membase + UART_IE_OFFSET,
			ie & ~UART_IE_ETBEI);

		uart_console_write(port, s, count,
			serial_ambarella_console_putchar);

		wait_for_tx(port);
		amba_writel(port->membase + UART_IE_OFFSET, ie);

		if (locked)
			spin_unlock(&port->lock);
		local_irq_restore(flags);
	}
}

static int __init serial_ambarella_console_setup(struct console *co,
	char *options)
{
	struct uart_port			*port;
	int					baud = 115200;
	int					bits = 8;
	int					parity = 'n';
	int					flow = 'n';

	if (co->index < 0 || co->index >= ambarella_uart_ports.total_port_num)
		co->index = 0;
	port = (struct uart_port *)(
		ambarella_uart_ports.amba_port[co->index].port);
#if !defined(CONFIG_PLAT_AMBARELLA_BOSS)
	ambarella_uart_ports.amba_port[co->index].set_pll();
#endif
	port->uartclk = ambarella_uart_ports.amba_port[co->index].get_pll();
	port->ops = &serial_ambarella_pops;
	port->private_data = &(ambarella_uart_ports.amba_port[co->index]);
	port->line = co->index;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct console serial_ambarella_console = {
	.name		= "ttyS",
	.write		= serial_ambarella_console_write,
	.device		= uart_console_device,
	.setup		= serial_ambarella_console_setup,
	.flags		= CON_PRINTBUFFER | CON_ANYTIME,
	.index		= -1,
	.data		= &serial_ambarella_reg,
};

static int __init serial_ambarella_console_init(void)
{
	if (serial_ambarella_reg.nr == -1)
		serial_ambarella_reg.nr = ambarella_uart_ports.total_port_num;

	register_console(&serial_ambarella_console);

	return 0;
}

console_initcall(serial_ambarella_console_init);

#define AMBARELLA_CONSOLE	&serial_ambarella_console
#else
#define AMBARELLA_CONSOLE	NULL
#endif

/* ==========================================================================*/
static struct uart_driver serial_ambarella_reg = {
	.owner		= THIS_MODULE,
	.driver_name	= "ambarella-uart",
	.dev_name	= "ttyS",
	.major		= TTY_MAJOR,
	.minor		= 64,
	.nr		= -1,
	.cons		= AMBARELLA_CONSOLE,
};

static int __devinit serial_ambarella_probe(struct platform_device *pdev)
{
	int					retval = 0;
	struct ambarella_uart_platform_info	*pinfo;
	struct uart_port			*port;

	pinfo = (struct ambarella_uart_platform_info *)pdev->dev.platform_data;
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Can't get UART platform data!\n");
		retval = -ENXIO;
		goto serial_ambarella_probe_exit;
	}

	if ((pdev->id < 0) || (pdev->id >= pinfo->total_port_num)) {
		dev_err(&pdev->dev, "Wrong UART ID %d!\n", pdev->id);
		retval = -ENXIO;
		goto serial_ambarella_probe_exit;
	}

	port = (struct uart_port *)(pinfo->amba_port[pdev->id].port);

#if !defined(CONFIG_PLAT_AMBARELLA_BOSS)
	pinfo->amba_port[pdev->id].set_pll();
#endif
	port->uartclk = pinfo->amba_port[pdev->id].get_pll();
	port->ops = &serial_ambarella_pops;
	port->dev = &pdev->dev;
	port->private_data = &(pinfo->amba_port[pdev->id]);
	port->line = pdev->id;

	if (pinfo->registed_port_num == 0) {
		if (serial_ambarella_reg.nr != pinfo->total_port_num)
			serial_ambarella_reg.nr = pinfo->total_port_num;

		retval = uart_register_driver(&serial_ambarella_reg);
		if (retval) {
			dev_err(&pdev->dev,
				"uart_register_driver fail %d!\n",
				retval);
			goto serial_ambarella_probe_exit;
		}
	}

	retval = uart_add_one_port(&serial_ambarella_reg, port);
	if (retval) {
		dev_err(&pdev->dev,
			"uart_add_one_port %d fail %d!\n",
			pdev->id, retval);
		goto serial_ambarella_probe_unregister_driver;
	} else {
		pinfo->registed_port_num++;
	}

	goto serial_ambarella_probe_exit;

serial_ambarella_probe_unregister_driver:
	if (pinfo->registed_port_num == 0)
		uart_unregister_driver(&serial_ambarella_reg);

serial_ambarella_probe_exit:
	return retval;
}

static int __devexit serial_ambarella_remove(struct platform_device *pdev)
{
	int					retval = 0;
	struct ambarella_uart_platform_info	*pinfo;
	struct uart_port			*port;

	pinfo = (struct ambarella_uart_platform_info *)pdev->dev.platform_data;

	if (pinfo) {
		port = (struct uart_port *)(pinfo->amba_port[pdev->id].port);
		retval = uart_remove_one_port(&serial_ambarella_reg, port);
		if (retval) {
			dev_err(&pdev->dev,
				"uart_remove_one_port %d fail %d!\n",
				pdev->id, retval);
		} else {
			pinfo->registed_port_num--;
		}

		if (pinfo->registed_port_num == 0)
			uart_unregister_driver(&serial_ambarella_reg);

		dev_notice(&pdev->dev,
			"Remove Ambarella Media Processor UART.\n");
	}

	return retval;
}

#ifdef CONFIG_PM
static int serial_ambarella_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int					retval = 0;
	struct ambarella_uart_platform_info	*pinfo;
	struct uart_port			*port;

	pinfo = (struct ambarella_uart_platform_info *)pdev->dev.platform_data;
	if (pinfo) {
		port = (struct uart_port *)(pinfo->amba_port[pdev->id].port);
		retval = uart_suspend_port(&serial_ambarella_reg, port);
	}
	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, retval, state.event);

	return retval;
}

static int serial_ambarella_resume(struct platform_device *pdev)
{
	int					retval = 0;
	struct ambarella_uart_platform_info	*pinfo;
	struct uart_port			*port;

	pinfo = (struct ambarella_uart_platform_info *)pdev->dev.platform_data;
	if (pinfo) {
		port = (struct uart_port *)(pinfo->amba_port[pdev->id].port);
		retval = uart_resume_port(&serial_ambarella_reg, port);
	}
	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, retval);

	return retval;
}
#endif

static struct platform_driver serial_ambarella_driver = {
	.probe		= serial_ambarella_probe,
	.remove		= __devexit_p(serial_ambarella_remove),
#ifdef CONFIG_PM
	.suspend	= serial_ambarella_suspend,
	.resume		= serial_ambarella_resume,
#endif
	.driver		= {
		.name	= "ambarella-uart",
	},
};

int __init serial_ambarella_init(void)
{
	return platform_driver_register(&serial_ambarella_driver);
}

void __exit serial_ambarella_exit(void)
{
	platform_driver_unregister(&serial_ambarella_driver);
}

module_init(serial_ambarella_init);
module_exit(serial_ambarella_exit);

MODULE_AUTHOR("Anthony Ginger");
MODULE_DESCRIPTION("Ambarella Media Processor UART driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ambarella-uart");

