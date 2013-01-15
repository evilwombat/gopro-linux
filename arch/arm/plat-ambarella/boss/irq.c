/*
 * arch/arm/plat-ambarella/generic/irq.c
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *	2007/11/29 - [Grady Chen] added VIC2, GPIO upper level VIC control
 *	2009/01/06 - [Anthony Ginger] Port to 2.6.28
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
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/device.h>

#include <asm/io.h>
#include <asm/cacheflush.h>
#include <mach/hardware.h>

#if defined(CONFIG_ARM_GIC)
#include <asm/hardware/gic.h>
#endif

#include <mach/boss.h>

extern void boss_enable_irq(int irq);
extern void boss_disable_irq(int irq);

/* ==========================================================================*/
static unsigned long ambarella_gpio_irq_bit[BITS_TO_LONGS(AMBGPIO_SIZE)];
static unsigned long ambarella_gpio_wakeup_bit[BITS_TO_LONGS(AMBGPIO_SIZE)];

/* ==========================================================================*/
#define AMBARELLA_GPIO_IRQ2GIRQ()	do { \
	girq -= GPIO_INT_VEC(0); \
	} while (0)

#if (GPIO_INSTANCES == 2)
#define AMBARELLA_GPIO_IRQ2BASE()	do { \
	gpio_base = GPIO0_BASE; \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO1_BASE; \
	} \
	} while (0)
#elif (GPIO_INSTANCES == 3)
#define AMBARELLA_GPIO_IRQ2BASE()	do { \
	gpio_base = GPIO0_BASE; \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO1_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO2_BASE; \
	} \
	} while (0)
#elif (GPIO_INSTANCES == 4)
#define AMBARELLA_GPIO_IRQ2BASE()	do { \
	gpio_base = GPIO0_BASE; \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO1_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO2_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO3_BASE; \
	} \
	} while (0)
#elif (GPIO_INSTANCES == 5)
#define AMBARELLA_GPIO_IRQ2BASE()	do { \
	gpio_base = GPIO0_BASE; \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO1_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO2_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO3_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO4_BASE; \
	} \
	} while (0)
#elif (GPIO_INSTANCES == 6)
#define AMBARELLA_GPIO_IRQ2BASE()	do { \
	gpio_base = GPIO0_BASE; \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO1_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO2_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO3_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO4_BASE; \
	} \
	if (girq >= GPIO_BANK_SIZE) { \
		girq -= GPIO_BANK_SIZE; \
		gpio_base = GPIO5_BASE; \
	} \
	} while (0)
#endif

void ambarella_gpio_ack_irq(struct irq_data *d)
{
	u32					gpio_base;
	u32					girq = d->irq;

	AMBARELLA_GPIO_IRQ2GIRQ();
	AMBARELLA_GPIO_IRQ2BASE();

	amba_writel(gpio_base + GPIO_IC_OFFSET, (0x1 << girq));
}

void inline __ambarella_gpio_disable_irq(unsigned int girq)
{
	u32					gpio_base;

	AMBARELLA_GPIO_IRQ2BASE();

	amba_clrbitsl(gpio_base + GPIO_IE_OFFSET, (0x1 << girq));
}

void ambarella_gpio_disable_irq(struct irq_data *d)
{
	u32					girq = d->irq;

	AMBARELLA_GPIO_IRQ2GIRQ();

	__ambarella_gpio_disable_irq(girq);
	__clear_bit(girq, ambarella_gpio_irq_bit);
}

void inline __ambarella_gpio_enable_irq(unsigned int girq)
{
	u32					gpio_base;

	AMBARELLA_GPIO_IRQ2BASE();

	amba_setbitsl(gpio_base + GPIO_IE_OFFSET, (0x1 << girq));
}

void ambarella_gpio_enable_irq(struct irq_data *d)
{
	u32					girq = d->irq;

	AMBARELLA_GPIO_IRQ2GIRQ();

	__ambarella_gpio_enable_irq(girq);
	__set_bit(girq, ambarella_gpio_irq_bit);
}

void ambarella_gpio_mask_ack_irq(struct irq_data *d)
{
	u32					gpio_base;
	u32					girq = d->irq;

	AMBARELLA_GPIO_IRQ2GIRQ();
	AMBARELLA_GPIO_IRQ2BASE();

	amba_clrbitsl(gpio_base + GPIO_IE_OFFSET, (0x1 << girq));
	amba_writel(gpio_base + GPIO_IC_OFFSET, (0x1 << girq));
}

int ambarella_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	u32					gpio_base;
	u32					mask;
	u32					bit;
	u32					sense;
	u32					bothedges;
	u32					event;
	u32					girq = d->irq;
	struct irq_desc				*desc = irq_to_desc(d->irq);

	AMBARELLA_GPIO_IRQ2GIRQ();
	AMBARELLA_GPIO_IRQ2BASE();

	mask = ~(0x1 << girq);
	bit = (0x1 << girq);
	sense = amba_readl(gpio_base + GPIO_IS_OFFSET);
	bothedges = amba_readl(gpio_base + GPIO_IBE_OFFSET);
	event = amba_readl(gpio_base + GPIO_IEV_OFFSET);

	pr_debug("%s: irq[%d] type[%d]\n", __func__, d->irq, type);

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		sense &= mask;
		bothedges &= mask;
		event |= bit;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		sense &= mask;
		bothedges &= mask;
		event &= mask;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		sense &= mask;
		bothedges |= bit;
		event &= mask;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		sense |= bit;
		bothedges &= mask;
		event |= bit;
		desc->handle_irq = handle_level_irq;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		sense |= bit;
		bothedges &= mask;
		event &= mask;
		desc->handle_irq = handle_level_irq;
		break;
	default:
		pr_err("%s: can't set irq type %d for irq 0x%08x@%d\n",
			__func__, type, gpio_base, d->irq);
		return -EINVAL;
	}

	amba_writel(gpio_base + GPIO_IS_OFFSET, sense);
	amba_writel(gpio_base + GPIO_IBE_OFFSET, bothedges);
	amba_writel(gpio_base + GPIO_IEV_OFFSET, event);

	ambarella_gpio_ack_irq(d);

	return 0;
}

static int ambarella_gpio_irq_set_wake(struct irq_data *d, unsigned int on)
{
	u32					girq = d->irq;

	AMBARELLA_GPIO_IRQ2GIRQ();

	pr_info("%s: irq[%d] = girq[%d] = %d\n", __func__, d->irq, girq, on);

	if (on)
		__set_bit(girq, ambarella_gpio_wakeup_bit);
	else
		__clear_bit(girq, ambarella_gpio_wakeup_bit);

	return 0;
}

static struct irq_chip ambarella_gpio_irq_chip = {
	.name		= "ambarella gpio irq",
	.irq_ack	= ambarella_gpio_ack_irq,
	.irq_mask	= ambarella_gpio_disable_irq,
	.irq_unmask	= ambarella_gpio_enable_irq,
	.irq_mask_ack	= ambarella_gpio_mask_ack_irq,
	.irq_set_type	= ambarella_gpio_irq_set_type,
#ifdef CONFIG_PM
	.irq_set_wake	= ambarella_gpio_irq_set_wake,
#endif
};

/* ==========================================================================*/
#if (VIC_INSTANCES == 1)
#define AMBARELLA_VIC_IRQ2BASE()	do { \
	irq -= VIC_INT_VEC(0); \
	} while (0)
#elif (VIC_INSTANCES == 2)
#define AMBARELLA_VIC_IRQ2BASE()	do { \
	irq -= VIC_INT_VEC(0); \
	if (irq >= NR_VIC_IRQ_SIZE) { \
		irq -= NR_VIC_IRQ_SIZE; \
		vic_base = VIC2_BASE; \
	} \
	} while (0)
#elif (VIC_INSTANCES == 3)		//FIX_IONE
#define AMBARELLA_VIC_IRQ2BASE()	do { \
	irq -= VIC_INT_VEC(0); \
	if (irq >= NR_VIC_IRQ_SIZE) { \
		irq -= NR_VIC_IRQ_SIZE; \
		vic_base = VIC2_BASE; \
	} \
	if (irq >= NR_VIC_IRQ_SIZE) { \
		irq -= NR_VIC_IRQ_SIZE; \
		vic_base = VIC3_BASE; \
	} \
	} while (0)
#endif

#if !defined(CONFIG_ARM_GIC)
static void ambarella_ack_irq(struct irq_data *d)
{
	u32					vic_base = VIC_BASE;
	u32					irq = d->irq;
	unsigned long flags;

	AMBARELLA_VIC_IRQ2BASE();

	local_irq_save(flags);
	amba_writel(vic_base + VIC_EDGE_CLR_OFFSET, 0x1 << irq);
	amba_writel(vic_base + VIC_SOFTEN_CLR_OFFSET, 0x1 << irq);
	local_irq_restore(flags);
}

static void ambarella_disable_irq(struct irq_data *d)
{
	u32					vic_base = VIC_BASE;
	u32					irq = d->irq;
	unsigned long flags;

	boss_disable_irq(irq);

	AMBARELLA_VIC_IRQ2BASE();

	local_irq_save(flags);
	amba_writel(vic_base + VIC_INTEN_CLR_OFFSET, 0x1 << irq);
	local_irq_restore(flags);
}

static void ambarella_enable_irq(struct irq_data *d)
{
	u32					vic_base = VIC_BASE;
	u32					irq = d->irq;
	unsigned long flags;

	boss_enable_irq(irq);

	AMBARELLA_VIC_IRQ2BASE();

	local_irq_save(flags);
	if (boss && !boss->guest_irq_mask) {
		amba_writel(vic_base + VIC_INTEN_OFFSET, 0x1 << irq);
	}
	local_irq_restore(flags);
}

static void ambarella_mask_ack_irq(struct irq_data *d)
{
	u32					vic_base = VIC_BASE;
	u32					irq = d->irq;
	unsigned long flags;

	AMBARELLA_VIC_IRQ2BASE();

	local_irq_save(flags);
	amba_writel(vic_base + VIC_INTEN_CLR_OFFSET, 0x1 << irq);
	amba_writel(vic_base + VIC_EDGE_CLR_OFFSET, 0x1 << irq);
	amba_writel(vic_base + VIC_SOFTEN_CLR_OFFSET, 0x1 << irq);
	local_irq_restore(flags);
}

static int ambarella_irq_set_type(struct irq_data *d, unsigned int type)
{
	u32					vic_base = VIC_BASE;
	u32					mask;
	u32					bit;
	u32					sense;
	u32					bothedges;
	u32					event;
	struct irq_desc				*desc = irq_to_desc(d->irq);
	u32					irq = d->irq;
	unsigned long flags;
	u32 irqtype;

	pr_debug("%s: irq[%d] type[%d] desc[%p]\n", __func__, irq, type, desc);

	AMBARELLA_VIC_IRQ2BASE();

	local_irq_save(flags);
	mask = ~(0x1 << irq);
	bit = (0x1 << irq);
	sense = amba_readl(vic_base + VIC_SENSE_OFFSET);
	bothedges = amba_readl(vic_base + VIC_BOTHEDGE_OFFSET);
	event = amba_readl(vic_base + VIC_EVENT_OFFSET);
	irqtype = amba_readl(vic_base + VIC_INT_SEL_OFFSET);

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		sense &= mask;
		bothedges &= mask;
		event |= bit;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		sense &= mask;
		bothedges &= mask;
		event &= mask;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		sense &= mask;
		bothedges |= bit;
		event &= mask;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		sense |= bit;
		bothedges &= mask;
		event |= bit;
		desc->handle_irq = handle_level_irq;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		sense |= bit;
		bothedges &= mask;
		event &= mask;
		desc->handle_irq = handle_level_irq;
		break;
	default:
		pr_err("%s: can't set irq type %d for irq 0x%08x@%d\n",
			__func__, type, vic_base, irq);
		return -EINVAL;
	}

	if (irqtype & bit) {
		pr_err("%s: irq[%d] is FIQ mode[0x%08x]\n", __func__, irq, irqtype);
	}
	irqtype &= mask;

	amba_writel(vic_base + VIC_INT_SEL_OFFSET, irqtype);
	amba_writel(vic_base + VIC_SENSE_OFFSET, sense);
	amba_writel(vic_base + VIC_BOTHEDGE_OFFSET, bothedges);
	amba_writel(vic_base + VIC_EVENT_OFFSET, event);

	local_irq_restore(flags);
	ambarella_ack_irq(d);

	return 0;
}

static int ambarella_irq_set_wake(struct irq_data *d, unsigned int on)
{
	pr_info("%s: set irq %d = %d\n", __func__, d->irq, on);

	return 0;
}

static struct irq_chip ambarella_irq_chip = {
	.name		= "ambarella onchip irq",
	.irq_ack	= ambarella_ack_irq,
	.irq_mask	= ambarella_disable_irq,
	.irq_unmask	= ambarella_enable_irq,
	.irq_mask_ack	= ambarella_mask_ack_irq,
	.irq_set_type	= ambarella_irq_set_type,
#ifdef CONFIG_PM
	.irq_set_wake	= ambarella_irq_set_wake,
#endif
};
#endif

void ambarella_swvic_set(u32 irq)
{
	u32					vic_base = VIC_BASE;

	AMBARELLA_VIC_IRQ2BASE();

	amba_writel(vic_base + VIC_SOFTEN_OFFSET, 0x1 << irq);
}
EXPORT_SYMBOL(ambarella_swvic_set);

void ambarella_swvic_clr(u32 irq)
{
	u32					vic_base = VIC_BASE;

	AMBARELLA_VIC_IRQ2BASE();

	amba_writel(vic_base + VIC_SOFTEN_CLR_OFFSET, 0x1 << irq);
}
EXPORT_SYMBOL(ambarella_swvic_clr);

/* ==========================================================================*/
static inline u32 ambarella_irq_stat2nr(u32 stat)
{
	u32					tmp;
	u32					nr;

	__asm__ __volatile__
		("rsbs	%[tmp], %[stat], #0" :
		[tmp] "=r" (tmp) : [stat] "r" (stat));
	__asm__ __volatile__
		("and	%[nr], %[tmp], %[stat]" :
		[nr] "=r" (nr) : [tmp] "r" (tmp), [stat] "r" (stat));
	__asm__ __volatile__
		("clzcc	%[nr], %[nr]" :
		[nr] "+r" (nr));
	__asm__ __volatile__
		("rsc	%[nr], %[nr], #32" :
		[nr] "+r" (nr));

	return nr;
}

static void ambarella_gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	u32					nr = 0;
	struct irq_chip				*gpio_chip;

	gpio_chip = get_irq_desc_chip(desc);
	if (gpio_chip->irq_mask_ack) {
		gpio_chip->irq_mask_ack(&desc->irq_data);
	} else {
		gpio_chip->irq_mask(&desc->irq_data);
		if (gpio_chip->irq_ack)
			gpio_chip->irq_ack(&desc->irq_data);
	}
	desc->status |= IRQ_MASKED;

	switch (irq) {
	case GPIO0_IRQ:
		nr = ambarella_irq_stat2nr(amba_readl(GPIO0_MIS_REG));
		if (nr < 32)
			generic_handle_irq(GPIO_INT_VEC(0) + nr);
		break;

	case GPIO1_IRQ:
		nr = ambarella_irq_stat2nr(amba_readl(GPIO1_MIS_REG));
		if (nr < 32) {
			generic_handle_irq(GPIO_INT_VEC(GPIO_BANK_SIZE) + nr);
			break;
		}

#if (GPIO_INSTANCES >= 3)
#if (GPIO2_IRQ != GPIO1_IRQ)
		break;
	case GPIO2_IRQ:
#endif
		nr = ambarella_irq_stat2nr(amba_readl(GPIO2_MIS_REG));
		if (nr < 32)
			generic_handle_irq(
				GPIO_INT_VEC(GPIO_BANK_SIZE * 2) + nr);
#endif
		break;

#if (GPIO_INSTANCES >= 4)
	case GPIO3_IRQ:
		nr = ambarella_irq_stat2nr(amba_readl(GPIO3_MIS_REG));
		if (nr < 32)
			generic_handle_irq(
				GPIO_INT_VEC(GPIO_BANK_SIZE * 3) + nr);
		break;
#endif

#if (GPIO_INSTANCES >= 5)
	case GPIO4_IRQ:
		nr = ambarella_irq_stat2nr(amba_readl(GPIO4_MIS_REG));
		if (nr < 32)
			generic_handle_irq(
				GPIO_INT_VEC(GPIO_BANK_SIZE * 4) + nr);
		break;
#endif

#if (GPIO_INSTANCES >= 6)
	case GPIO5_IRQ:
		nr = ambarella_irq_stat2nr(amba_readl(GPIO5_MIS_REG));
		if (nr < 32)
			generic_handle_irq(
				GPIO_INT_VEC(GPIO_BANK_SIZE * 5) + nr);
		break;
#endif

	default:
		pr_err("%s: Can't support %d\n", __func__, irq);
		break;
	}

	if (gpio_chip->irq_unmask) {
		gpio_chip->irq_unmask(&desc->irq_data);
		desc->status &= ~IRQ_MASKED;
	}
}

/* ==========================================================================*/
#if defined(CONFIG_PLAT_AMBARELLA_A5S_BOSS) || \
    defined(CONFIG_PLAT_AMBARELLA_A7_BOSS) || \
    defined(CONFIG_PLAT_AMBARELLA_A7L_BOSS)

void __init ambarella_init_irq_a5s_a7_a7l(void)
{
	u32					i;

	/*
	 * We should set up the BOSS vectors so that it knows how to properly
	 * jump to Linux vectors.
	 */
	unsigned long vectors = CONFIG_VECTORS_BASE;
	unsigned int backed, linst;
	unsigned int off;

	/* Swap entries in backup and the one installed by early_tap_init() */
	for (off = 0; off < 0x20; off += 4) {
		linst = amba_readl((void *) vectors + off);
		backed = amba_readl((void *) vectors + off + 0x20);

		/* Need to modify the address field by subtracting 0x20 */
		/* Just assume that the instructions installed in Linux */
		/* vector that need modification are either LDR or B */
		if ((linst & 0x0f000000) == 0x0a000000) {	/* B */
			linst -= 0x8;
		}
		if ((linst & 0x0ff00000) == 0x05900000) {	/* LDR */
			linst -= 0x20;
		}

		amba_writel((void *) vectors + off, backed);
		amba_writel((void *) vectors + off + 0x20, linst);
	}
	flush_icache_range(vectors, vectors + PAGE_SIZE);

	/* Install addresses for our vector to BOSS */
	for (off = 0; off < 0x20; off += 4) {
		amba_writel((void *) vectors + off + 0x1020,
			    vectors + off + 0x20);
	}
	clean_dcache_area((void *) vectors + 0x1000, PAGE_SIZE);

	for (i = 0; i <  NR_VIC_IRQ_SIZE; i++) {
		set_irq_chip(VIC_INT_VEC(i), &ambarella_irq_chip);
		set_irq_handler(VIC_INT_VEC(i), handle_level_irq);
		set_irq_flags(VIC_INT_VEC(i), IRQF_VALID);
	}
#if (VIC_INSTANCES >= 2)
	for (i = 0; i <  NR_VIC_IRQ_SIZE; i++) {
		set_irq_chip(VIC2_INT_VEC(i), &ambarella_irq_chip);
		set_irq_handler(VIC2_INT_VEC(i), handle_level_irq);
		set_irq_flags(VIC2_INT_VEC(i), IRQF_VALID);
	}
#endif
#if (VIC_INSTANCES >= 3)
	for (i = 0; i <  NR_VIC_IRQ_SIZE; i++) {
		set_irq_chip(VIC3_INT_VEC(i), &ambarella_irq_chip);
		set_irq_handler(VIC3_INT_VEC(i), handle_level_irq);
		set_irq_flags(VIC3_INT_VEC(i), IRQF_VALID);
	}
#endif
#if defined(CONFIG_AMBARELLA_IPC)
       /* Setup SD CD GPIO IRQs */
       set_irq_chip(GPIO_INT_VEC(67), &ambarella_gpio_irq_chip);
       set_irq_handler(GPIO_INT_VEC(67), handle_level_irq);
       __clear_bit(67, ambarella_gpio_wakeup_bit);
       set_irq_flags(GPIO_INT_VEC(67), IRQF_VALID);

       /* Setup SDIO CD GPIO IRQs */
       set_irq_chip(GPIO_INT_VEC(75), &ambarella_gpio_irq_chip);
       set_irq_handler(GPIO_INT_VEC(75), handle_level_irq);
       __clear_bit(75, ambarella_gpio_wakeup_bit);
       set_irq_flags(GPIO_INT_VEC(75), IRQF_VALID);

       set_irq_type(GPIO2_IRQ, IRQ_TYPE_LEVEL_HIGH);
       set_irq_chained_handler(GPIO2_IRQ, ambarella_gpio_irq_handler);
#endif
	boss_set_ready(1);

}
#endif

#ifdef CONFIG_PLAT_AMBARELLA_I1_BOSS
void __init ambarella_init_irq_i1(void)
{
#if !defined(CONFIG_ARM_GIC)
	u32					i;
#endif

#if defined(CONFIG_ARM_GIC)
	gic_init(0, LOCAL_TIMER_IRQ, __io(AMBARELLA_VA_GIC_DIST_BASE),
		__io(AMBARELLA_VA_GIC_CPU_BASE));

	irq_set_affinity(CODING_ORC0_IRQ, cpumask_of(0));
	irq_set_affinity(CODING_ORC1_IRQ, cpumask_of(0));
	irq_set_affinity(CODING_ORC2_IRQ, cpumask_of(0));
	irq_set_affinity(CODING_ORC3_IRQ, cpumask_of(0));
	irq_set_affinity(VOUT_IRQ, cpumask_of(0));
	irq_set_affinity(ORC_VOUT0_IRQ, cpumask_of(0));
	irq_set_affinity(VIN_IRQ, cpumask_of(0));
	irq_set_affinity(IDSP_SENSOR_VSYNC_IRQ, cpumask_of(0));
	irq_set_affinity(IDSP_LAST_PIXEL_IRQ, cpumask_of(0));
	irq_set_affinity(IDSP_VSYNC_IRQ, cpumask_of(0));
	irq_set_affinity(IDSP_SENSOR_VSYNC_IRQ, cpumask_of(0));
#else
	/* Set VIC sense and event type for each entry */
	amba_writel(VIC_SENSE_REG, 0x00000000);
	amba_writel(VIC_BOTHEDGE_REG, 0x00000000);
	amba_writel(VIC_EVENT_REG, 0x00000000);

#if (VIC_INSTANCES >= 2)
	amba_writel(VIC2_SENSE_REG, 0x00000000);
	amba_writel(VIC2_BOTHEDGE_REG, 0x00000000);
	amba_writel(VIC2_EVENT_REG, 0x00000000);
#endif

#if (VIC_INSTANCES >= 3)
	amba_writel(VIC3_SENSE_REG, 0x00000000);
	amba_writel(VIC3_BOTHEDGE_REG, 0x00000000);
	amba_writel(VIC3_EVENT_REG, 0x00000000);
#endif

	/* Disable all IRQ */
	amba_writel(VIC_INT_SEL_REG, 0x00000000);
	amba_writel(VIC_INTEN_REG, 0x00000000);
	amba_writel(VIC_INTEN_CLR_REG, 0xffffffff);
	amba_writel(VIC_EDGE_CLR_REG, 0xffffffff);

#if (VIC_INSTANCES >= 2)
	amba_writel(VIC2_INT_SEL_REG, 0x00000000);
	amba_writel(VIC2_INTEN_REG, 0x00000000);
	amba_writel(VIC2_INTEN_CLR_REG, 0xffffffff);
	amba_writel(VIC2_EDGE_CLR_REG, 0xffffffff);
#endif

#if (VIC_INSTANCES >= 3)
	amba_writel(VIC3_INT_SEL_REG, 0x00000000);
	amba_writel(VIC3_INTEN_REG, 0x00000000);
	amba_writel(VIC3_INTEN_CLR_REG, 0xffffffff);
	amba_writel(VIC3_EDGE_CLR_REG, 0xffffffff);
#endif

	for (i = 0; i <  NR_VIC_IRQ_SIZE; i++) {
		set_irq_chip(VIC_INT_VEC(i), &ambarella_irq_chip);
		set_irq_handler(VIC_INT_VEC(i), handle_level_irq);
		set_irq_flags(VIC_INT_VEC(i), IRQF_VALID);
	}
#if (VIC_INSTANCES >= 2)
	for (i = 0; i <  NR_VIC_IRQ_SIZE; i++) {
		set_irq_chip(VIC2_INT_VEC(i), &ambarella_irq_chip);
		set_irq_handler(VIC2_INT_VEC(i), handle_level_irq);
		set_irq_flags(VIC2_INT_VEC(i), IRQF_VALID);
	}
#endif
#if (VIC_INSTANCES >= 3)
	for (i = 0; i <  NR_VIC_IRQ_SIZE; i++) {
		set_irq_chip(VIC3_INT_VEC(i), &ambarella_irq_chip);
		set_irq_handler(VIC3_INT_VEC(i), handle_level_irq);
		set_irq_flags(VIC3_INT_VEC(i), IRQF_VALID);
	}
#endif
#endif
#if defined(CONFIG_SUPPORT_ANDROID_TT)
	/* Setup GPIO IRQs */
	{
	int j = 0;
	for (j = 0; j < AMBGPIO_SIZE; j++) {
		set_irq_chip(GPIO_INT_VEC(j), &ambarella_gpio_irq_chip);
		set_irq_handler(GPIO_INT_VEC(j), handle_level_irq);
		__clear_bit(j, ambarella_gpio_wakeup_bit);
		set_irq_flags(GPIO_INT_VEC(j), IRQF_VALID);
	}
	}
	set_irq_type(GPIO0_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO0_IRQ, ambarella_gpio_irq_handler);
	set_irq_type(GPIO1_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO1_IRQ, ambarella_gpio_irq_handler);
#if (GPIO_INSTANCES >= 3)
#if (GPIO2_IRQ != GPIO1_IRQ)
	set_irq_type(GPIO2_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO2_IRQ, ambarella_gpio_irq_handler);
#endif
#endif
#if (GPIO_INSTANCES >= 4)
	set_irq_type(GPIO3_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO3_IRQ, ambarella_gpio_irq_handler);
#endif
#if (GPIO_INSTANCES >= 5)
	set_irq_type(GPIO4_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO4_IRQ, ambarella_gpio_irq_handler);
#endif
#if (GPIO_INSTANCES >= 6)
	set_irq_type(GPIO5_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO5_IRQ, ambarella_gpio_irq_handler);
#endif
#else
	/* Setup SD CD GPIO IRQs */
	set_irq_chip(GPIO_INT_VEC(67), &ambarella_gpio_irq_chip);
	set_irq_handler(GPIO_INT_VEC(67), handle_level_irq);
	__clear_bit(67, ambarella_gpio_wakeup_bit);
	set_irq_flags(GPIO_INT_VEC(67), IRQF_VALID);

	/* Setup SDIO CD GPIO IRQs */
	set_irq_chip(GPIO_INT_VEC(75), &ambarella_gpio_irq_chip);
	set_irq_handler(GPIO_INT_VEC(75), handle_level_irq);
	__clear_bit(75, ambarella_gpio_wakeup_bit);
	set_irq_flags(GPIO_INT_VEC(75), IRQF_VALID);

	set_irq_type(GPIO2_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO2_IRQ, ambarella_gpio_irq_handler);

	/* Setup SDXC CD GPIO IRQs */
	set_irq_chip(GPIO_INT_VEC(129), &ambarella_gpio_irq_chip);
	set_irq_handler(GPIO_INT_VEC(129), handle_level_irq);
	__clear_bit(129, ambarella_gpio_wakeup_bit);
	set_irq_flags(GPIO_INT_VEC(129), IRQF_VALID);

	set_irq_type(GPIO5_IRQ, IRQ_TYPE_LEVEL_HIGH);
	set_irq_chained_handler(GPIO5_IRQ, ambarella_gpio_irq_handler);
#endif
}
#endif

void __init ambarella_init_irq(void)
{
#if defined(CONFIG_PLAT_AMBARELLA_A5S_BOSS) || \
    defined CONFIG_PLAT_AMBARELLA_A7_BOSS || \
    defined CONFIG_PLAT_AMBARELLA_A7L_BOSS
	ambarella_init_irq_a5s_a7_a7l();
#endif
#ifdef CONFIG_PLAT_AMBARELLA_I1_BOSS
	ambarella_init_irq_i1();
#endif
}

/* ==========================================================================*/
u32 ambarella_irq_suspend(u32 level)
{
#if defined(CONFIG_ARM_GIC)
	gic_suspend(level);
#endif
	return 0;
}

u32 ambarella_irq_resume(u32 level)
{
#if defined(CONFIG_ARM_GIC)
	gic_resume(level);
#endif
	return 0;
}

