/****************************************************************************
 * arch/tricore/include/irq.h
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

/* This file should never be included directly but, rather, only indirectly
 * through nuttx/irq.h
 */

#ifndef __ARCH_TRICORE_INCLUDE_IRQ_H
#define __ARCH_TRICORE_INCLUDE_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#ifndef __ASSEMBLY__
#  include <stdbool.h>
#endif

/* Include NuttX-specific IRQ definitions */

#include <nuttx/irq.h>

/* Include chip-specific IRQ definitions (including IRQ numbers) */

#include <arch/chip/irq.h>

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

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
 * processing.  Access to g_current_regs[] must be through the macro
 * g_current_regs for portability.
 */

/* For the case of architectures with multiple CPUs, then there must be one
 * such value for each processor that can receive an interrupt.
 */

 EXTERN volatile uintptr_t *g_current_regs[CONFIG_NR_CPUS];
 #define CURRENT_REGS (g_current_regs[up_cpu_index()])

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
static inline int up_cpu_index(void)
{
#ifdef CONFIG_ARCH_HAVE_MULTICPU
  return __mfcr(TC3XX_CPU_CUS_ID);
#else
  return 0;
#endif
}

/****************************************************************************
 * Name: up_irq_enable
 *
 * Description:
 *   Enable interrupts globally.
 *
 ****************************************************************************/

void up_irq_enable(void);

/****************************************************************************
 * Inline functions
 ****************************************************************************/

noinstrument_function static inline uintptr_t up_getsp(void)
{
#ifdef CONFIG_TRICORE_TOOLCHAIN_TASKING
  return (uintptr_t)__get_sp();
#else
  return __builtin_frame_address(0);
#endif
}

/****************************************************************************
 * Name: up_irq_disable
 *
 * Description:
 *   Disable interrupts
 *
 ****************************************************************************/
noinstrument_function static inline void up_irq_disable(void)
{
  __disable();
}

/****************************************************************************
 * Name: up_irq_save
 *
 * Description:
 *   Disable interrupts and return the previous value of the mstatus register
 *
 ****************************************************************************/

noinstrument_function static inline irqstate_t up_irq_save(void)
{
  return __disable_and_save();
}

/****************************************************************************
 * Name: up_irq_restore
 *
 * Description:
 *   Restore the value of the mstatus register
 *
 ****************************************************************************/

noinstrument_function static inline void up_irq_restore(irqstate_t flags)
{
  __restore(flags);
}

#if (CONFIG_RT_FRAMEWORK == 1)
noinstrument_function static inline void up_set_contexthdl(void * const ctxhdl)
{
  irqstate_t irq_state = up_irq_save();
  volatile uint32_t pre_psw = __mfcr(TX3XX_CPU_PSW);
  __mtcr(TX3XX_CPU_PSW, pre_psw | CPU_PSW_GW_MASK);
  __dsync();
  __isync();
#if defined(__GNUC__) && defined(__TRICORE__)
  __setareg(a8,ctxhdl);
#else
  __asm volatile("mov.a  a8, %0" ::"d"(ctxhdl):"a8");
#endif
  __mtcr(TX3XX_CPU_PSW, pre_psw);
  __dsync();
  __isync();
  up_irq_restore(irq_state);
}

noinstrument_function static inline void * up_get_contexthdl()
{
  uint32_t ret;
#if defined(__GNUC__) && defined(__TRICORE__)
  __asm__ volatile ("mov.d %0, %%a8": "=d" (ret) : :"a8");
#else
   __asm volatile( "mov.d  %0, a8" :"=d"(ret)::);
#endif
  return (void *)ret;
}
#endif

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

static inline_function uintptr_t *up_current_regs(void)
{
#if defined(CONFIG_SMP) || defined(CONFIG_BMP)
  return (uintptr_t *)g_current_regs[up_cpu_index()];
#else
  return (uintptr_t *)g_current_regs[0];
#endif
}

static inline_function void up_set_current_regs(uintptr_t *regs)
{
#if defined(CONFIG_SMP) || defined(CONFIG_BMP)
  g_current_regs[up_cpu_index()] = regs;
#else
  g_current_regs[0] = regs;
#endif
}

/****************************************************************************
 * Name: up_getusrpc
 ****************************************************************************/
static inline uintptr_t * up_getusrpc(void *regs)
{
  uintptr_t *plcsa = (uintptr_t *)((regs) ? (regs) : up_current_regs());
  uintptr_t *pucsa = (uintptr_t *)tricore_csa2addr(plcsa[REG_UPCXI]);
  return (uintptr_t *)pucsa[REG_UPC];
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
static inline_function bool up_interrupt_context(void)
{
#if defined(CONFIG_SMP) || defined(CONFIG_BMP)
  irqstate_t flags = up_irq_save();
#endif

  bool ret = up_current_regs() != NULL;

#if defined(CONFIG_SMP) || defined(CONFIG_BMP)
  up_irq_restore(flags);
#endif

  return ret;
}
#endif /* __ASSEMBLY__ */

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ARCH_TRICORE_INCLUDE_IRQ_H */
