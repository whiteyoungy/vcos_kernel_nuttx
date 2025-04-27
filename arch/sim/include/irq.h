/****************************************************************************
 * arch/sim/include/irq.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* This file should never be included directly but, rather,
 * only indirectly through nuttx/irq.h
 */

#ifndef __ARCH_SIM_INCLUDE_IRQ_H
#define __ARCH_SIM_INCLUDE_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <arch/setjmp.h>
#include <sys/types.h>
#ifndef __ASSEMBLY__
#  include <stdbool.h>
#endif
#if defined(CONFIG_RT_FRAMEWORK ) && (CONFIG_RT_FRAMEWORK == 1)
#include <arch/chip/irq.h>
#endif
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* When Use CONFIG_SIM_IRQ_MANAGE. Sim will use hashtable save irqs .To sim
 * other arch. NR_IRQS can set be 1024 or bigger.
 */

#if defined(CONFIG_SIM_IRQ_MANAGE)
#define NR_IRQS 1024
#else
#define NR_IRQS 64
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifndef __ASSEMBLY__

/* This struct defines the way the registers are stored */

struct xcptcontext
{
  jmp_buf regs;

#if defined(CONFIG_RT_FRAMEWORK ) && (CONFIG_RT_FRAMEWORK == 1)
  uintptr_t *contexthdl;
  uint8_t task_started;
#endif
};

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* g_current_regs[] holds a references to the current interrupt level
 * register storage structure.  If is non-NULL only during interrupt
 * processing.  Access to g_current_regs[] must be through the
 * [get/set]_current_regs for portability.
 */

/* For the case of architectures with multiple CPUs, then there must be one
 * such value for each processor that can receive an interrupt.
 */
#ifdef CONFIG_BMP
EXTERN volatile xcpt_reg_t *g_current_regs[CONFIG_BMP_NCPUS];
#else
EXTERN volatile xcpt_reg_t *g_current_regs[CONFIG_SMP_NCPUS];
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: up_cpu_index
 *
 * Description:
 *   Return the real core number regardless CONFIG_SMP setting
 *
 ****************************************************************************/

#ifdef CONFIG_ARCH_HAVE_MULTICPU
int up_cpu_index(void) noinstrument_function;
#endif /* CONFIG_ARCH_HAVE_MULTICPU */

/* Name: up_irq_save, up_irq_restore, and friends.
 *
 * NOTE: These functions should never be called from application code and,
 * as a general rule unless you really know what you are doing, this
 * function should not be called directly from operation system code either:
 * Typically, the wrapper functions, enter_critical_section() and
 * leave_critical section(), are probably what you really want.
 */

irqstate_t up_irq_flags(void);
irqstate_t up_irq_save(void);
void up_irq_disable(void);
void up_irq_restore(irqstate_t flags);
void up_irq_enable(void);

/****************************************************************************
 * Inline functions
 ****************************************************************************/

noinstrument_function
static inline_function xcpt_reg_t *up_current_regs(void)
{
#if defined CONFIG_SMP || defined CONFIG_BMP
  return (xcpt_reg_t *)g_current_regs[up_cpu_index()];
#else
  return (xcpt_reg_t *)g_current_regs[0];
#endif
}

static inline_function void up_set_current_regs(xcpt_reg_t *regs)
{
  #if defined CONFIG_SMP || defined CONFIG_BMP
  g_current_regs[up_cpu_index()] = regs;
#else
  g_current_regs[0] = regs;
#endif
}

/* Return the current value of the stack pointer */

static inline uintptr_t up_getsp(void)
{
#ifdef _MSC_VER
  uintptr_t tmp;
  return (uintptr_t)&tmp;
#else
  return (uintptr_t)__builtin_frame_address(0);
#endif
}

/****************************************************************************
 * Name: up_interrupt_context
 *
 * Description:
 *   Return true is we are currently executing in the interrupt
 *   handler context.
 *
 ****************************************************************************/

noinstrument_function
static inline bool up_interrupt_context(void)
{
#if defined CONFIG_SMP || defined CONFIG_BMP
  irqstate_t flags = up_irq_save();
#endif

  bool ret = up_current_regs() != NULL;

#if defined CONFIG_SMP || defined CONFIG_BMP
  up_irq_restore(flags);
#endif

  return ret;
}

/****************************************************************************
 * Name: up_getusrpc
 *
 * Description:
 *   Get the PC value, The interrupted context PC register cannot be
 *   correctly obtained in sim It will return the PC of the interrupt
 *   handler function, normally it will return sim_doirq
 *
 ****************************************************************************/

#define up_getusrpc(regs) \
    (((xcpt_reg_t *)((regs) ? (regs) : up_current_regs()))[JB_PC])

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* !__ASSEMBLY__ */
#endif /* __ARCH_SIM_INCLUDE_IRQ_H */
