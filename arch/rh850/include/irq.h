/****************************************************************************
 * arch/rh850/include/irq.h
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

#ifndef __ARCH_RH850_INCLUDE_IRQ_H
#define __ARCH_RH850_INCLUDE_IRQ_H

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
 * register storage structure.  It is non-NULL only during interrupt
 * processing.  Access to g_current_regs[] must be through the macro
 * up_current_regs() for portability.
 */

/* For the case of architectures with multiple CPUs, then there must be one
 * such value for each processor that can receive an interrupt.
 */

EXTERN volatile uintptr_t *g_current_regs[CONFIG_SMP_NCPUS];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: up_cpu_index
 *
 * Description:
 *   Return an index in the range of 0 through (CONFIG_SMP_NCPUS-1) that
 *   corresponds to the currently executing CPU.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   An integer index in the range of 0 through (CONFIG_SMP_NCPUS-1) that
 *   corresponds to the currently executing CPU.
 *
 ****************************************************************************/

#ifdef CONFIG_SMP
int up_cpu_index(void) noinstrument_function;
#else
#  define up_cpu_index() (0)
#endif

/****************************************************************************
 * Inline functions
 ****************************************************************************/

noinstrument_function static inline uintptr_t up_getsp(void)
{
    unsigned int sp = 0U;
    __asm volatile
    (
        "\tmov sp, %0\n"
        : "=r" (sp)
        :
        : "memory"
    );

    return sp;
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
    unsigned int psw = 0U;

    __asm volatile
    (
        "\tstsr sr5, %0, 0\n"
        "\tdi \n"
        : "=r" (psw)
        :
        : "memory"
    );

    return psw;
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
      __asm volatile
    (
        "ldsr %0, sr5, 0"
        :
        : "r" (flags)
        : "memory"
    );
}
/****************************************************************************
 * Name: up_irq_enable
 *
 * Description:
 *   Enable the value of the mstatus register
 *
 ****************************************************************************/

noinstrument_function static inline irqstate_t up_irq_enable(void)
{
  unsigned int psw = 0U;

    __asm volatile
    (
        "\tstsr sr5, %0, 0\n"
        "\tei \n"
        : "=r" (psw)
        :
        : "memory"
    );

  return psw;
}



/****************************************************************************
 * Inline Functions
 ****************************************************************************/

static inline_function uintptr_t *up_current_regs(void)
{
#ifdef CONFIG_SMP
  return (uintptr_t *)g_current_regs[up_cpu_index()];
#else
  return (uintptr_t *)g_current_regs[0];
#endif
}

static inline_function void up_set_current_regs(uintptr_t *regs)
{
#ifdef CONFIG_SMP
  g_current_regs[up_cpu_index()] = regs;
#else
  g_current_regs[0] = regs;
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
#ifdef CONFIG_SMP
  irqstate_t flags = up_irq_save();
#endif

  bool ret = up_current_regs() != NULL;

#ifdef CONFIG_SMP
  up_irq_restore(flags);
#endif

  return ret;
}


/****************************************************************************
 * Name: up_getusrpc
 ****************************************************************************/

#define up_getusrpc(regs) \
    (((uintptr_t *)((regs) ? (regs) : up_current_regs()))[REG_EIPC])

#endif /* __ASSEMBLY__ */

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ARCH_RH850_INCLUDE_IRQ_H */
