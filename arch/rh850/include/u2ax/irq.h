/****************************************************************************
 * arch/rh850/include/u2ax/irq.h
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

#ifndef __ARCH_RH850_INCLUDE_U2AX_IRQ_H
#define __ARCH_RH850_INCLUDE_U2AX_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/irq.h>
#ifndef __ASSEMBLY__
#  include <stdint.h>
#endif

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

/* IRQ Stack Frame Format:
 *
 * Context is always saved/restored in the same way:
 *
 *   (1) stmia rx, {r0-r14}
 *   (2) then the PC and CPSR
 *
 * This results in the following set of indices that
 * can be used to access individual registers in the
 * xcp.regs array:
 */

#define REG_R30             (0)   /* EP */
#define REG_R29             (1)
#define REG_R28             (2)
#define REG_R27             (3)
#define REG_R26             (4)
#define REG_R25             (5)
#define REG_R24             (6)
#define REG_R23             (7)
#define REG_R22             (8)
#define REG_R21             (9)
#define REG_R20             (10)
#define REG_EIPC            (11)
#define REG_EIPSW           (12)
#define REG_R1              (13)
#define REG_R19             (14)
#define REG_R18             (15)
#define REG_R17             (16)
#define REG_R16             (17)
#define REG_R15             (18)
#define REG_R14             (19)
#define REG_R13             (20)
#define REG_R12             (21)
#define REG_R11             (22)
#define REG_R10             (23)
#define REG_R9              (24)
#define REG_R8              (25)
#define REG_R7              (26)
#define REG_R6              (27)
#define REG_R31             (28)   /* lp */

#define XCPTCONTEXT_REGS    (29)
#define XCPTCONTEXT_SIZE    (4 * XCPTCONTEXT_REGS)

#define NR_IRQS          (255)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This struct defines the way the registers are stored.  We
 * need to save:
 *
 *  1  CPSR
 *  7  Static registers, v1-v7 (aka r4-r10)
 *  1  Frame pointer, fp (aka r11)
 *  1  Stack pointer, sp (aka r13)
 *  1  Return address, lr (aka r14)
 * ---
 * 11  (XCPTCONTEXT_USER_REG)
 *
 * On interrupts, we also need to save:
 *  4  Volatile registers, a1-a4 (aka r0-r3)
 *  1  Scratch Register, ip (aka r12)
 * ---
 *  5  (XCPTCONTEXT_IRQ_REGS)
 *
 * For a total of 17 (XCPTCONTEXT_REGS)
 */

#ifndef __ASSEMBLY__
struct xcptcontext
{
  /* The following function pointer is non-zero if there
   * are pending signals to be processed.
   */

  void *sigdeliver; /* Actual type is sig_deliver_t */

  /* These are saved copies of the context used during
   * signal processing.
   */

  uint32_t *saved_regs;

  /* Register save area with XCPTCONTEXT_SIZE, only valid when:
   * 1.The task isn't running or
   * 2.The task is interrupted
   * otherwise task is running, and regs contain the stale value.
   */

  uint32_t *regs;

  /* Extra fault address register saved for common paging logic.  In the
   * case of the prefetch abort, this value is the same as regs[REG_R15];
   * For the case of the data abort, this value is the value of the fault
   * address register (FAR) at the time of data abort exception.
   */

#ifdef CONFIG_LEGACY_PAGING
  uintptr_t far;
#endif
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif

#endif /* __ARCH_RH850_INCLUDE_U2AX_IRQ_H */
