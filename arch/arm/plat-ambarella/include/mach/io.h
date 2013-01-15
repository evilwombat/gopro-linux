/*
 * arch/arm/plat-ambarella/include/mach/io.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#ifndef __ASM_ARCH_IO_H
#define __ASM_ARCH_IO_H

/* ==========================================================================*/
#include <mach/hardware.h>

/* ==========================================================================*/
#define IO_SPACE_LIMIT		0xffffffff

#define __io(a)			((void __iomem *)(a))
#define __mem_pci(a)		(a)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

#if defined(CONFIG_PLAT_AMBARELLA_DISABLE_8_16_ACCESS)
static inline u8 __amba_readb(const volatile void __iomem *address)
{
	u32					tmpreg;
	u32					index;
	volatile void __iomem			*base;

	index = (u32)address & 0x03;
	base = (volatile void __iomem *)((u32)address & 0xFFFFFFFC);
	AMBARELLA_REG_LOCK();
	AMBARELLA_INC_REGLOCK_COUNT();
	tmpreg = __raw_readl(base);
	switch (index) {
	case 1:
		tmpreg &= 0x0000FF00;
		tmpreg >>= 8;
		break;

	case 2:
		tmpreg &= 0x00FF0000;
		tmpreg >>= 16;
		break;

	case 3:
		tmpreg &= 0xFF000000;
		tmpreg >>= 24;
		break;

	case 0:
	default:
		tmpreg &= 0x000000FF;
		break;
	}
	AMBARELLA_REG_UNLOCK();

	return (u8)tmpreg;
}

static inline void __amba_writeb(const volatile void __iomem *address, u8 value)
{
	u32					tmpreg;
	u32					index;
	volatile void __iomem			*base;

	index = (u32)address & 0x03;
	base = (volatile void __iomem *)((u32)address & 0xFFFFFFFC);
	AMBARELLA_REG_LOCK();
	AMBARELLA_INC_REGLOCK_COUNT();
	tmpreg = __raw_readl(base);
	switch (index) {
	case 1:
		tmpreg &= 0xFFFF00FF;
		tmpreg |= ((u32)value << 8);
		break;

	case 2:
		tmpreg &= 0xFF00FFFF;
		tmpreg |= ((u32)value << 16);
		break;

	case 3:
		tmpreg &= 0x00FFFFFF;
		tmpreg |= ((u32)value << 24);
		break;

	case 0:
	default:
		tmpreg &= 0xFFFFFF00;
		tmpreg |= value;
		break;
	}
	__raw_writel(tmpreg, base);
	AMBARELLA_REG_UNLOCK();
}

static inline u16 __amba_readw(const volatile void __iomem *address)
{
	u32					tmpreg;
	u32					index;
	volatile void __iomem			*base;

	index = (u32)address & 0x03;
	base = (volatile void __iomem *)((u32)address & 0xFFFFFFFC);
	AMBARELLA_REG_LOCK();
	AMBARELLA_INC_REGLOCK_COUNT();
	tmpreg = __raw_readl(base);
	switch (index) {
	case 2:
		tmpreg &= 0xFFFF0000;
		tmpreg >>= 16;
		break;

	case 0:
	default:
		tmpreg &= 0x0000FFFF;
		break;
	}
	AMBARELLA_REG_UNLOCK();

	return (u16)tmpreg;
}

static inline void __amba_writew(const volatile void __iomem *address, u16 value)
{
	u32					tmpreg;
	u32					index;
	volatile void __iomem			*base;

	index = (u32)address & 0x03;
	base = (volatile void __iomem *)((u32)address & 0xFFFFFFFC);
	AMBARELLA_REG_LOCK();
	AMBARELLA_INC_REGLOCK_COUNT();
	tmpreg = __raw_readl(base);
	switch (index) {
	case 2:
		tmpreg &= 0x0000FFFF;
		tmpreg |= ((u32)value << 16);
		break;

	case 0:
	default:
		tmpreg &= 0xFFFF0000;
		tmpreg |= value;
		break;
	}
	__raw_writel(tmpreg, base);
	AMBARELLA_REG_UNLOCK();
}

#define amba_readb(v)		__amba_readb((const volatile void __iomem *)v)
#define amba_readw(v)		__amba_readw((const volatile void __iomem *)v)
#define amba_writeb(v,d)	__amba_writeb((const volatile void __iomem *)v, d)
#define amba_writew(v,d)	__amba_writew((const volatile void __iomem *)v, d)
#else
#define amba_readb(v)		__raw_readb(v)
#define amba_readw(v)		__raw_readw(v)
#define amba_writeb(v,d)	__raw_writeb(d,v)
#define amba_writew(v,d)	__raw_writew(d,v)
#endif

#if defined(CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK)
static inline u32 __amba_readl(const volatile void __iomem *address)
{
	u32					tmpval;

	AMBARELLA_REG_LOCK();
	AMBARELLA_INC_REGLOCK_COUNT();
	tmpval = __raw_readl(address);
	AMBARELLA_REG_UNLOCK();

	return tmpval;
}

static inline void __amba_writel(const volatile void __iomem *address, u32 value)
{
	AMBARELLA_REG_LOCK();
	AMBARELLA_INC_REGLOCK_COUNT();
	__raw_writel(value, address);
	AMBARELLA_REG_UNLOCK();
}

#define amba_readl(v)		__amba_readl((const volatile void __iomem *)v)
#define amba_writel(v,d)	__amba_writel((const volatile void __iomem *)v, d)
#else
#define amba_readl(v)		__raw_readl(v)
#define amba_writel(v,d)	__raw_writel(d,v)
#endif

static inline void __amba_read2w(const volatile void __iomem *address,
	u16 *value1, u16 *value2)
{
	u32					tmpreg;
	u32					index;
	volatile void __iomem			*base;

	index = (u32)address & 0x03;
	base = (volatile void __iomem *)((u32)address & 0xFFFFFFFC);
	tmpreg = amba_readl(base);

	switch (index) {
	case 0:
		*value1 = (u16)tmpreg;
		*value2 = (u16)(tmpreg >> 16);
		break;

	default:
		BUG_ON(1);
		break;
	}
}

static inline void __amba_write2w(const volatile void __iomem *address,
	u16 value1, u16 value2)
{
	u32					tmpreg;
	u32					index;
	volatile void __iomem			*base;

	index = (u32)address & 0x03;
	base = (volatile void __iomem *)((u32)address & 0xFFFFFFFC);
	tmpreg = value2;
	tmpreg <<= 16;
	tmpreg |= value1;
	switch (index) {
	case 0:
		break;

	default:
		BUG_ON(1);
		break;
	}
	amba_writel(base, tmpreg);
}

#define amba_read2w(v,pd1,pd2)	__amba_read2w(v,pd1,pd2)
#define amba_write2w(v,d1,d2)	__amba_write2w(v,d1,d2)

#define amba_setbitsb(v, mask)	amba_writeb((v),(amba_readb(v) | (mask)))
#define amba_setbitsw(v, mask)	amba_writew((v),(amba_readw(v) | (mask)))
#define amba_setbitsl(v, mask)	amba_writel((v),(amba_readl(v) | (mask)))

#define amba_clrbitsb(v, mask)	amba_writeb((v),(amba_readb(v) & ~(mask)))
#define amba_clrbitsw(v, mask)	amba_writew((v),(amba_readw(v) & ~(mask)))
#define amba_clrbitsl(v, mask)	amba_writel((v),(amba_readl(v) & ~(mask)))

#define amba_tstbitsb(v, mask)	(amba_readb(v) & (mask))
#define amba_tstbitsw(v, mask)	(amba_readw(v) & (mask))
#define amba_tstbitsl(v, mask)	(amba_readl(v) & (mask))

static inline void amba_change_bit(
	const volatile void __iomem *vaddress,
	unsigned int bit)
{
	u32					mask = 1UL << (bit & 31);
	u32					data;

	data = amba_readl(vaddress);
	data ^= mask;
	amba_writel(vaddress, data);
}
static inline int amba_test_and_set_bit(
	const volatile void __iomem *vaddress,
	unsigned int bit)
{
	u32					mask = 1UL << (bit & 31);
	u32					data;
	u32					tmp;

	data = amba_readl(vaddress);
	tmp = data | mask;
	amba_writel(vaddress, tmp);

	return data & mask;
}
static inline int amba_test_and_clear_bit(
	const volatile void __iomem *vaddress,
	unsigned int bit)
{
	u32					mask = 1UL << (bit & 31);
	u32					data;
	u32					tmp;

	data = amba_readl(vaddress);
	tmp = data & ~mask;
	amba_writel(vaddress, tmp);

	return data & mask;
}
static inline int amba_test_and_change_bit(
	const volatile void __iomem *vaddress,
	unsigned int bit)
{
	u32					mask = 1UL << (bit & 31);
	u32					data;
	u32					tmp;

	data = amba_readl(vaddress);
	tmp = data ^ mask;
	amba_writel(vaddress, tmp);

	return data & mask;
}
static inline int amba_test_and_set_mask(
	const volatile void __iomem *vaddress,
	unsigned int mask)
{
	u32					data;
	u32					tmp;

	data = amba_readl(vaddress);
	tmp = data | mask;
	amba_writel(vaddress, tmp);

	return data & mask;
}
static inline int amba_test_and_clear_mask(
	const volatile void __iomem *vaddress,
	unsigned int mask)
{
	u32					data;
	u32					tmp;

	data = amba_readl(vaddress);
	tmp = data & ~mask;
	amba_writel(vaddress, tmp);

	return data & mask;
}
#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

