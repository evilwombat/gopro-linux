/*
 * arch/arm/plat-ambarella/boss/boss.c
 *
 * Author: Henry Lin <hllin@ambarella.com>
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
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/aipc/aipc.h>
#include <linux/aipc/ipc_slock.h>
#include <mach/boss.h>

extern u32 ipc_tick_get(void);

#define BOSS_IRQ_IS_ENABLED(f)	((f & PSR_I_BIT) == 0)

struct boss_obj_s G_boss_obj = {
	.ready		= 0,
	.count		= 0,
	.irq_enable_count = 0,
	.irq_disable_count = 0,
	.irq_save_count = 0,
	.irq_restore_count = 0,
};

struct boss_obj_s *boss_obj = &G_boss_obj;
EXPORT_SYMBOL(boss_obj);

/*
 * Set BOSS to ready state
 */
void boss_set_ready(int ready)
{
	boss_obj->ready = ready;
	if (ready) {
		boss_obj->count = 0;
	}
}
EXPORT_SYMBOL(boss_set_ready);

#if defined(CONFIG_BOSS_SINGLE_CORE)

/*
 * BOSS: arch_local_irq_enable
 */
void boss_local_irq_enable(void)
{
	arm_irq_disable();

	boss->guest_irq_mask = 0;
	boss_obj->irq_enable_count++;
	boss_obj->irq_enable_time = ipc_tick_get();

	amba_writel(VIC_INTEN_REG, boss->vic1mask & boss->guest_vic1_en);
	amba_writel(VIC2_INTEN_REG, boss->vic2mask & boss->guest_vic2_en);

	arm_irq_enable();
}
EXPORT_SYMBOL(boss_local_irq_enable);

/*
 * BOSS: arch_local_irq_enable
 */
void boss_local_irq_disable(void)
{
	unsigned long flags;

	flags = arm_irq_save();

	amba_writel(VIC_INTEN_CLR_REG, boss->vic1mask);
	amba_writel(VIC2_INTEN_CLR_REG, boss->vic2mask);

	boss->guest_irq_mask = 1;

	boss_obj->irq_disable_count++;
	boss_obj->irq_disable_time = ipc_tick_get();

	arm_irq_restore(flags);
}
EXPORT_SYMBOL(boss_local_irq_disable);

/*
 * BOSS: arch_local_save_flags
 */
unsigned long boss_local_save_flags(void)
{
	unsigned long flags;
	asm volatile(
		"	mrs	%0, cpsr	@ local_save_flags"
		: "=r" (flags) : : "memory", "cc");

	if (boss->guest_irq_mask) {
		flags |= PSR_I_BIT;
	}

	return flags;
}
EXPORT_SYMBOL(boss_local_save_flags);

/*
 * BOSS: arch_local_irq_save
 */
unsigned long boss_local_irq_save(void)
{
	unsigned long flags;

	flags = arm_irq_save();
	if (boss_obj->count == 0) {
		boss_obj->irq_start_time = ipc_tick_get();
	}
	boss_obj->count++;
	boss_obj->irq_save_count++;
	boss_obj->irq_save_time = ipc_tick_get();
	arm_irq_restore(flags);

	flags = arch_local_save_flags();

	if (BOSS_IRQ_IS_ENABLED(flags)) {
		boss_local_irq_disable();
	}

	return flags;
}
EXPORT_SYMBOL(boss_local_irq_save);

/*
 * BOSS: arch_local_irq_restore
 */
void boss_local_irq_restore(unsigned long flags)
{
	if (boss_obj->count == 0) {
		arm_irq_restore(flags);
		return;
	}

	if (BOSS_IRQ_IS_ENABLED(flags)) {
		boss_local_irq_enable();
	}

	flags = arm_irq_save();
	boss_obj->irq_restore_count++;
	boss_obj->irq_restore_time = ipc_tick_get();
	arm_irq_restore(flags);
}
EXPORT_SYMBOL(boss_local_irq_restore);

#endif	/* CONFIG_BOSS_SINGLE_CORE */

/*
 * Called when ipc reports ready status
 */
void boss_on_ipc_report_ready(int ready)
{
	if (ready) {
		boss->log_buf_ptr = ipc_virt_to_phys (boss_log_buf_ptr);
		boss->log_buf_len_ptr = ipc_virt_to_phys (boss_log_buf_len_ptr);
		boss->log_buf_last_ptr = ipc_virt_to_phys (boss_log_buf_last_ptr);
	}
}
EXPORT_SYMBOL(boss_on_ipc_report_ready);

/*
 * Get the owner of a BOSS IRQ
 */
int boss_get_irq_owner(int irq)
{
	int owner = BOSS_IRQ_OWNER_UITRON;

	ipc_spin_lock(boss->lock, IPC_SLOCK_POS_BOSS_GET_IRQ_OWNER);

	if (irq < 32) {
		if (boss->vic1mask & (0x1 << irq)) {
			owner = BOSS_IRQ_OWNER_LINUX;
		}
	}
	else if (irq < 64) {
		if (boss->vic2mask & (0x1 << (irq - 32))) {
			owner = BOSS_IRQ_OWNER_LINUX;
		}
	}
	else if (irq < 96) {
		if (boss->vic3mask & (0x1 << (irq - 64))) {
			owner = BOSS_IRQ_OWNER_LINUX;
		}
	}

	ipc_spin_unlock(boss->lock, IPC_SLOCK_POS_BOSS_GET_IRQ_OWNER);

	return owner;
}
EXPORT_SYMBOL(boss_get_irq_owner);

/*
 * Set the owner of a BOSS IRQ
 */
void boss_set_irq_owner(int irq, int owner, int update)
{
	ipc_spin_lock(boss->lock, 0);

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A7L)
	if (irq < 32) {
		if (owner == BOSS_IRQ_OWNER_UITRON) {
			BOSS_VIC_SET_UITRON(boss->vic1mask, irq);
			BOSS_VIC_SET(boss->root_vic1_en, irq);
			if (update) {
				amba_writel(VIC_INTEN_REG, 1 << irq);
			}
		} else {
			BOSS_VIC_SET_LINUX(boss->vic1mask, irq);
			BOSS_VIC_SET(boss->guest_vic1_en, irq);
			if (update) {
				if (boss->guest_irq_mask) {
					amba_writel(VIC_INTEN_CLR_REG, 1 << irq);
				} else {
					amba_writel(VIC_INTEN_REG, 1 << irq);
				}
			}
		}
	}
	else if (irq < 64) {
		if (owner == BOSS_IRQ_OWNER_UITRON) {
			BOSS_VIC_SET_UITRON(boss->vic2mask, irq - 32);
			BOSS_VIC_SET(boss->root_vic2_en, irq - 32);
			if (update) {
				amba_writel(VIC2_INTEN_REG, 1 << (irq - 32));
			}
		} else {
			BOSS_VIC_SET_LINUX(boss->vic2mask, irq - 32);
			BOSS_VIC_SET(boss->guest_vic2_en, irq - 32);
			if (update) {
				if (boss->guest_irq_mask) {
					amba_writel(VIC2_INTEN_CLR_REG, 1 << (irq - 32));
				} else {
					amba_writel(VIC2_INTEN_REG, 1 << (irq - 32));
				}
			}
		}
	}
#if (VIC_INSTANCES >= 3)
	else if (irq < 96) {
		if (owner == BOSS_IRQ_OWNER_UITRON) {
			BOSS_VIC_SET_UITRON(boss->vic3mask, irq - 64);
			BOSS_VIC_SET(boss->root_vic3_en, irq - 64);
			if (update) {
				amba_writel(VIC3_INTEN_REG, 1 << (irq - 64));
			}
		} else {
			BOSS_VIC_SET_LINUX(boss->vic3mask, irq - 64);
			BOSS_VIC_SET(boss->guest_vic3_en, irq - 64);
			if (update) {
				if (boss->guest_irq_mask) {
					amba_writel(VIC3_INTEN_CLR_REG, 1 << (irq - 64));
				} else {
					amba_writel(VIC3_INTEN_REG, 1 << (irq - 64));
				}
			}
		}

	}
#endif
#endif

#if (CHIP_REV == I1)
#if defined(CONFIG_ARM_GIC)
	if (update)
	{
		irq -= 32;	// for GIC

		if (irq < 32) {
			if (owner == BOSS_IRQ_OWNER_UITRON) {
				amba_writel(VIC_INTEN_REG , 1 << irq);
			} else {
				amba_writel(VIC_INTEN_CLR_REG , 1 << irq);
			}
		}
		else if (irq < 64) {
			if (owner == BOSS_IRQ_OWNER_UITRON) {
				amba_writel(VIC2_INTEN_REG , 1 << (irq - 32));
			} else {
				amba_writel(VIC2_INTEN_CLR_REG , 1 << (irq - 32));
			}
		}
		else if (irq < 96) {
			if (owner == BOSS_IRQ_OWNER_UITRON) {
				amba_writel(VIC3_INTEN_REG , 1 << (irq - 64));
			} else {
				amba_writel(VIC3_INTEN_CLR_REG , 1 << (irq - 64));
			}
		}
	}
#else
	#error "Only support GIC on i1 now..."
#endif
#endif

	ipc_spin_unlock(boss->lock, 0);
}
EXPORT_SYMBOL(boss_set_irq_owner);

/*
 * Set Linux as the owner of an BOSS IRQ
 */
void boss_enable_irq(int irq)
{
	ipc_spin_lock(boss->lock, IPC_SLOCK_POS_BOSS_ENABLE_IRQ);

	if (boss->vic1mask & (1 << 12)) {
		for (;;);
	}

	if (irq < 32) {
		BOSS_VIC_SET(boss->guest_vic1_en, irq);
	}
	else if (irq < 64) {
		BOSS_VIC_SET(boss->guest_vic2_en, irq - 32);
	}
#if (VIC_INSTANCES >= 3)
	else if (irq < 96) {
		BOSS_VIC_SET(boss->guest_vic3_en, irq - 64);
	}
#endif

	ipc_spin_unlock(boss->lock, IPC_SLOCK_POS_BOSS_ENABLE_IRQ);
}
EXPORT_SYMBOL(boss_enable_irq);

/*
 * Set uITRON as the owner of an BOSS IRQ
 */
void boss_disable_irq(int irq)
{
	ipc_spin_lock(boss->lock, IPC_SLOCK_POS_BOSS_DISABLE_IRQ);

	if (boss->vic1mask & (1 << 12)) {
		for (;;);
	}

	if (irq < 32) {
		BOSS_VIC_CLR(boss->guest_vic1_en, irq);
	}
	else if (irq < 64) {
		BOSS_VIC_CLR(boss->guest_vic2_en, irq - 32);
	}
#if (VIC_INSTANCES >= 3)
	else if (irq < 96) {
		BOSS_VIC_CLR(boss->guest_vic3_en, irq - 64);
	}
#endif

	ipc_spin_unlock(boss->lock, IPC_SLOCK_POS_BOSS_DISABLE_IRQ);
}
EXPORT_SYMBOL(boss_disable_irq);

int boss_get_device_owner(int device)
{
	K_ASSERT(boss != NULL);

	if (device >= BOSS_DEVICE_NUM) {
		return -1;
	}

	return ((boss->device_owner_mask >> device) & 1);
}
EXPORT_SYMBOL(boss_get_device_owner);
