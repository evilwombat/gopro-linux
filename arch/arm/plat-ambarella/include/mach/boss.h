/*
 * arch/arm/mach-boss/include/mach/boss.h
 *
 * Authors:
 *	Charles Chiou <cchiou@ambarella.com>
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

#ifndef __ASM_ARCH_BOSS_H
#define __ASM_ARCH_BOSS_H

#include <ambhw/chip.h>

#define BOSS_BOSS_MEM_SIZE		0x1000		/* 4KB */
#define BOSS_LINUX_VERSION		0x00000004	/* linux struct boss_s version */

#if (CHIP_REV == A5S)
#define BOSS_VIRT_H2G_INT_REQ_VEC	(32 + 0)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_H2G_INT_RLY_VEC	(32 + 21)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_G2H_INT_REQ_VEC	(32 + 3)	/* Virtual 'guest-to-host' irq */
#define BOSS_VIRT_G2H_INT_RLY_VEC	(32 + 22)	/* Virtual 'guest-to-host' irq */

#define BOSS_VIRT_H2G_MTX_VEC		(32 + 19)	/* Virtual 'host-to-guest' mutex irq */
#define BOSS_VIRT_G2H_MTX_VEC		(32 + 20)	/* Virtual 'guest-to-host' mutex irq */

#define BOSS_VIRT_TIMER_INT_VEC		(32 + 15)	/* Virtual 'timer' to guest irq */
#define BOSS_VIRT_GIRQ_INT_VEC		(32 + 18)	/* Virtual 'guest IRQ' irq */
#define BOSS_VIRT_RIRQ_INT_VEC		(32 + 23)	/* Virtual 'root IRQ' irq */
#elif (CHIP_REV == A7)
#define BOSS_VIRT_H2G_INT_REQ_VEC	(32 + 0)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_H2G_INT_RLY_VEC	(32 + 27)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_G2H_INT_REQ_VEC	(32 + 3)	/* Virtual 'guest-to-host' irq */
#define BOSS_VIRT_G2H_INT_RLY_VEC	(32 + 28)	/* Virtual 'guest-to-host' irq */

#define BOSS_VIRT_H2G_MTX_VEC		(32 + 29)	/* Virtual 'host-to-guest' mutex irq */
#define BOSS_VIRT_G2H_MTX_VEC		(32 + 30)	/* Virtual 'guest-to-host' mutex irq */

#define BOSS_VIRT_TIMER_INT_VEC		(32 + 15)	/* Virtual 'timer' to guest irq */
#define BOSS_VIRT_GIRQ_INT_VEC		(32 + 31)	/* Virtual 'guest IRQ' irq */
/* FIXME: BUG: VIC2_20 on A7 is temporarily borrowed from ETH2 IRQ */
#define BOSS_VIRT_RIRQ_INT_VEC		(32 + 20)	/* Virtual 'root IRQ' irq */
#elif (CHIP_REV == A7L)
#define BOSS_VIRT_H2G_INT_REQ_VEC	(32 + 0)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_H2G_INT_RLY_VEC	(32 + 25)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_G2H_INT_REQ_VEC	(32 + 3)	/* Virtual 'guest-to-host' irq */
#define BOSS_VIRT_G2H_INT_RLY_VEC	(32 + 26)	/* Virtual 'guest-to-host' irq */

#define BOSS_VIRT_H2G_MTX_VEC		(32 + 4)	/* Virtual 'host-to-guest' mutex irq */
#define BOSS_VIRT_G2H_MTX_VEC		(32 + 21)	/* Virtual 'guest-to-host' mutex irq */

#define BOSS_VIRT_TIMER_INT_VEC		(32 + 13)	/* Virtual 'timer' to guest irq */
#define BOSS_VIRT_GIRQ_INT_VEC		(32 + 22)	/* Virtual 'guest IRQ' irq */
/* FIXME: BUG: VIC2_20 on A7 is temporarily borrowed from ETH2 IRQ */
#define BOSS_VIRT_RIRQ_INT_VEC		(32 + 20)	/* Virtual 'root IRQ' irq */
#elif (CHIP_REV == I1)
#define BOSS_VIRT_H2G_INT_REQ_VEC	AXI_SOFT_IRQ(0)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_H2G_INT_RLY_VEC	AXI_SOFT_IRQ(2)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_G2H_INT_REQ_VEC	3		/* Virtual 'guest-to-host' irq */
#define BOSS_VIRT_G2H_INT_RLY_VEC	(32 + 32 + 4)	/* Virtual 'guest-to-host' irq */
#define BOSS_VIRT_H2G_MTX_VEC		AXI_SOFT_IRQ(1)	/* Virtual 'host-to-guest' irq */
#define BOSS_VIRT_G2H_MTX_VEC		(32 + 32 + 0)	/* Virtual 'guest-to-host' irq */

#define BOSS_VIRT_TIMER_INT_VEC		(32 + 15)	/* Virtual 'timer' to guest irq */
#define BOSS_VIRT_GIRQ_INT_VEC		(32 + 18)	/* Virtual 'guest IRQ' irq */
#define BOSS_VIRT_RIRQ_INT_VEC		(32 + 23)	/* Virtual 'root IRQ' irq */
#else
#error "Boss is not supported on this chip!"
#endif

/* Keep these in sync with data structure below */
#define BOSS_ROOT_CTX_OFFSET		0
#define BOSS_GUEST_CTX_OFFSET		4
#define BOSS_MODE_OFFSET		8
#define BOSS_GILDLE_OFFSET		12
#define BOSS_ENVTIMER_OFFSET		16
#define BOSS_CVTIMER_OFFSET		20
#define BOSS_NVTIMER_OFFSET		24
#define BOSS_IRQNO_OFFSET		28
#define BOSS_VIC1MASK_OFFSET		32
#define BOSS_VIC2MASK_OFFSET		36
#define BOSS_VIC3MASK_OFFSET		40
#define BOSS_GPIO0MASK_OFFSET		44
#define BOSS_GPIO1MASK_OFFSET		48
#define BOSS_GPIO2MASK_OFFSET		52
#define BOSS_GPIO3MASK_OFFSET		56
#define BOSS_GPIO4MASK_OFFSET		60
#define BOSS_GPIO5MASK_OFFSET		64
#define BOSS_ROOT_IRQ_MASK_OFFSET	68
#define BOSS_GUEST_IRQ_MASK_OFFSET	72
#define BOSS_ROOT_VIC1_EN_OFFSET	76
#define BOSS_ROOT_VIC2_EN_OFFSET	80
#define BOSS_ROOT_VIC3_EN_OFFSET	84
#define BOSS_GUEST_VIC1_EN_OFFSET	88
#define BOSS_GUEST_VIC2_EN_OFFSET	92
#define BOSS_GUEST_VIC3_EN_OFFSET	96

#if !defined(__ASM__)

#include <linux/aipc/aipc_struct.h>

enum {
	BOSS_STATE_UNINIT = 0,
	BOSS_STATE_INIT,
	BOSS_STATE_BOOTING,
	BOSS_STATE_READY,
	BOSS_STATE_SUSPENDED,
	BOSS_STATE_NUM
};

enum boss_device {
	BOSS_DEVICE_USB = 0,
	BOSS_DEVICE_NUM
};

enum boss_device_owner {
	BOSS_DEVICE_OWNER_UITRON = 0,
	BOSS_DEVICE_OWNER_LINUX,
	BOSS_DEVICE_OWNER_NUM
};

/*
 * CPU context.
 */
struct boss_context_s
{
	unsigned int r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
	unsigned int pc, cpsr;
	unsigned int usr_sp, usr_lr;
	unsigned int svr_sp, svr_lr, svr_spsr;
	unsigned int abt_sp, abt_lr, abt_spsr;
	unsigned int irq_sp, irq_lr, irq_spsr;
	unsigned int und_sp, und_lr, und_spsr;
	unsigned int fiq_sp, fiq_lr, fiq_spsr;
	unsigned int fiq_r8, fiq_r9, fiq_r10, fiq_r11, fiq_r12;
	unsigned int cp15_ctr;
	unsigned int cp15_ttb;
	unsigned int cp15_dac;
};

/*
 * The data structure for BOSS.
 */
struct boss_s
{
	struct boss_context_s *root_ctx;	/* Root context */
	struct boss_context_s *guest_ctx;	/* Guest context */
	unsigned int *mode;			/* BOSS mode */
	unsigned int *gidle;			/* Guest idle flag */

	/*
	 * The following 3 fields are used by BOSS to manage the
	 * vtimer interrupt to the guest OS.
	 */
	unsigned int *envtimer;		/* vtimer enabled? */
	unsigned int *cvtimer;		/* vtimer counter */
	unsigned int *nvtimer;		/* Number of vtimer delivered */
	unsigned int *irqno;		/* IRQ number computed by BOSS */

	/*
	 * The following 2 fields are zero-initialized by the root OS.
	 * The guest OS may modify the VIC controllers, but should set
	 * the bitmasks below to indicate to the root OS which lines that
	 * it subscribes to.
	 */
	unsigned int vic1mask;		/* VIC1 bitmask */
	unsigned int vic2mask;		/* VIC2 bitmask */
	unsigned int vic3mask;		/* VIC3 bitmask */
	unsigned int gpio0mask;		/* GPIO0 bitmask */
	unsigned int gpio1mask;		/* GPIO1 bitmask */
	unsigned int gpio2mask;		/* GPIO2 bitmask */
	unsigned int gpio3mask;		/* GPIO3 bitmask */
	unsigned int gpio4mask;		/* GPIO4 bitmask */
	unsigned int gpio5mask;		/* GPIO5 bitmask */

	/* IRQ status */
	unsigned int root_irq_mask;	/* Root IRQ status */
	unsigned int guest_irq_mask;	/* If guest IRQ is masked? */

	unsigned int root_vic1_en;	/* Root VIC1 enable */
	unsigned int root_vic2_en;	/* Root VIC2 enable */
	unsigned int root_vic3_en;	/* Root VIC3 enable */

	unsigned int guest_vic1_en;	/* Guest VIC1 enable */
	unsigned int guest_vic2_en;	/* Guest VIC2 enable */
	unsigned int guest_vic3_en;	/* Guest VIC3 enable */

	/*
	 * The following 3 variables are initialized to 0 and expected to
	 * be filled in by the guest OS and maintained thereafter to trigger
	 * log buffer updates and dumps by the root OS.
	 */
	unsigned int log_buf_ptr;	/* Guest OS log buffer address */
	unsigned int log_buf_len_ptr;	/* Guest OS log buffer length */
	unsigned int log_buf_last_ptr;	/* Index to latest updated buf. */

	unsigned int state;		/* BOSS state */
	unsigned int lock;		/* BOSS lock */

	unsigned int smem_addr;	/* Shared memory address */
	unsigned int smem_size;	/* Shared memory length */

	unsigned int ipc_log_lock;
	unsigned int ipc_log_level;
	unsigned int ipc_log_ptr;
	unsigned int ipc_log_size;
	unsigned int ipc_log_total;

	/* device ownership */
	unsigned int device_owner_mask;

	/*
	 * The following fields are used by the IPC drivers.
	 * The IPC binder on the root OS sets up the pointers and the sizes of
	 * the circular buffers so that the corresponding IPC binder on the
	 * remote OS can retrieve these when it boots up.
	 */
	struct ipc_buf_s ipc_buf;	/* IPC buffer */
};

typedef struct boss_obj_s {
	unsigned int	ready;
	unsigned int	count;
	unsigned int	irq_start_time;
	unsigned int	irq_enable_count;
	unsigned int	irq_enable_time;
	unsigned int	irq_disable_count;
	unsigned int	irq_disable_time;
	unsigned int	irq_save_count;
	unsigned int	irq_save_time;
	unsigned int	irq_restore_count;
	unsigned int	irq_restore_time;
} boss_obj_t;

/*
 * BOSS: global declaration.
 */
extern struct boss_s *boss;
extern struct boss_obj_s *boss_obj;


#define BOSS_IRQ_OWNER_UITRON	0
#define BOSS_IRQ_OWNER_LINUX	1

#define BOSS_VIC_SET_UITRON(vic, irq)	(vic &= ~(1 << (irq)))
#define BOSS_VIC_SET_LINUX(vic, irq)	(vic |= (1 << (irq)))

#define BOSS_VIC_SET(vic, irq)		(vic |= (1 << (irq)))
#define BOSS_VIC_CLR(vic, irq)		(vic &= ~(1 << (irq)))

/*
 * BOSS: API.
 */
extern void boss_set_ready(int ready);

extern unsigned long boss_local_irq_save(void);
extern void boss_local_irq_restore(unsigned long flags);
extern void boss_local_irq_enable(void);
extern void boss_local_irq_disable(void);
extern unsigned long boss_local_save_flags(void);

extern int boss_get_irq_owner(int irq);
extern void boss_set_irq_owner(int irq, int owner, int update);

extern void boss_enable_irq(int irq);
extern void boss_disable_irq(int irq);

extern int boss_get_device_owner(int device);

/* BOSS printk */
extern unsigned int boss_log_buf_ptr;
extern unsigned int boss_log_buf_len_ptr;
extern unsigned int boss_log_buf_last_ptr;

#endif  /* !__ASM__ */

#endif

