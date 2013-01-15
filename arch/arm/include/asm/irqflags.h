#ifndef __ASM_ARM_IRQFLAGS_H
#define __ASM_ARM_IRQFLAGS_H

#ifdef __KERNEL__

#include <asm/ptrace.h>

/*
 * CPU interrupt mask handling.
 */
#if __LINUX_ARM_ARCH__ >= 6

#if defined(CONFIG_BOSS_SINGLE_CORE)

#include <mach/boss.h>
#include <mach/memory.h>
#include <ambhw/ambhw.h>
#include <ambhw/vic.h>

static inline unsigned long arm_save_flags(void)
{
	unsigned long flags;
	asm volatile(
		"	mrs	%0, cpsr	@ local_save_flags"
		: "=r" (flags) : : "memory", "cc");
	return flags;
}

static inline unsigned long arm_irq_save(void)
{
	unsigned long flags;

	asm volatile(
		"	mrs	%0, cpsr	@ arm_irq_save\n"
		"	cpsid	i"
		: "=r" (flags) : : "memory", "cc");
	return flags;
}

static inline void arm_irq_restore(unsigned long flags)
{
	asm volatile(
		"	msr	cpsr_c, %0	@ arm_irq_restore"
		:
		: "r" (flags)
		: "memory", "cc");
}

static inline void arm_irq_enable(void)
{
	asm volatile(
		"	cpsie i			@ arm_irq_enable"
		:
		:
		: "memory", "cc");
}

static inline void arm_irq_disable(void)
{
	asm volatile(
		"	cpsid i			@ arm_irq_disable"
		:
		:
		: "memory", "cc");
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	if (boss_obj->ready) {
		flags = boss_local_irq_save();
	} else {
		flags = arm_irq_save();
	}

	return flags;
}

static inline void arch_local_irq_enable(void)
{
	if (boss_obj->ready) {
		boss_local_irq_enable();
	} else {
		arm_irq_enable();
	}
}

static inline void arch_local_irq_disable(void)
{
	if (boss_obj->ready) {
		boss_local_irq_disable();
	} else {
		arm_irq_disable();
	}
}

#else

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	asm volatile(
		"	mrs	%0, cpsr	@ arch_local_irq_save\n"
		"	cpsid	i"
		: "=r" (flags) : : "memory", "cc");
	return flags;
}

static inline void arch_local_irq_enable(void)
{
	asm volatile(
		"	cpsie i			@ arch_local_irq_enable"
		:
		:
		: "memory", "cc");
}

static inline void arch_local_irq_disable(void)
{
	asm volatile(
		"	cpsid i			@ arch_local_irq_disable"
		:
		:
		: "memory", "cc");
}

#endif

#define local_fiq_enable()  __asm__("cpsie f	@ __stf" : : : "memory", "cc")
#define local_fiq_disable() __asm__("cpsid f	@ __clf" : : : "memory", "cc")
#else

/*
 * Save the current interrupt enable state & disable IRQs
 */
static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags, temp;

	asm volatile(
		"	mrs	%0, cpsr	@ arch_local_irq_save\n"
		"	orr	%1, %0, #128\n"
		"	msr	cpsr_c, %1"
		: "=r" (flags), "=r" (temp)
		:
		: "memory", "cc");
	return flags;
}

/*
 * Enable IRQs
 */
static inline void arch_local_irq_enable(void)
{
	unsigned long temp;
	asm volatile(
		"	mrs	%0, cpsr	@ arch_local_irq_enable\n"
		"	bic	%0, %0, #128\n"
		"	msr	cpsr_c, %0"
		: "=r" (temp)
		:
		: "memory", "cc");
}

/*
 * Disable IRQs
 */
static inline void arch_local_irq_disable(void)
{
	unsigned long temp;
	asm volatile(
		"	mrs	%0, cpsr	@ arch_local_irq_disable\n"
		"	orr	%0, %0, #128\n"
		"	msr	cpsr_c, %0"
		: "=r" (temp)
		:
		: "memory", "cc");
}

/*
 * Enable FIQs
 */
#define local_fiq_enable()					\
	({							\
		unsigned long temp;				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ stf\n"		\
"	bic	%0, %0, #64\n"					\
"	msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
	})

/*
 * Disable FIQs
 */
#define local_fiq_disable()					\
	({							\
		unsigned long temp;				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ clf\n"		\
"	orr	%0, %0, #64\n"					\
"	msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
	})

#endif

#if defined(CONFIG_BOSS_SINGLE_CORE)

/*
 * Save the current interrupt enable state.
 */
static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;

	if (boss_obj->ready) {
		flags = boss_local_save_flags();
	} else {
		asm volatile(
			"	mrs	%0, cpsr	@ local_save_flags"
			: "=r" (flags) : : "memory", "cc");
	}

	return flags;
}

/*
 * restore saved IRQ & FIQ state
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	if (boss_obj->ready) {
		boss_local_irq_restore(flags);
	} else {
		arm_irq_restore(flags);
	}
}

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return flags & PSR_I_BIT;
}

#else

/*
 * Save the current interrupt enable state.
 */
static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;
	asm volatile(
		"	mrs	%0, cpsr	@ local_save_flags"
		: "=r" (flags) : : "memory", "cc");
	return flags;
}

/*
 * restore saved IRQ & FIQ state
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	asm volatile(
		"	msr	cpsr_c, %0	@ local_irq_restore"
		:
		: "r" (flags)
		: "memory", "cc");
}

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return flags & PSR_I_BIT;
}

#endif

#endif
#endif
