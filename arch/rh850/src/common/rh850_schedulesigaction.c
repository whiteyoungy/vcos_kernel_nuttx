/****************************************************************************
 * arch/rh850/src/common/rh850_schedulesigaction.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <inttypes.h>
#include <stdint.h>
#include <sched.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/spinlock.h>

#include "sched/sched.h"
#include "rh850_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_schedule_sigaction
 *
 * Description:
 *   This function is called by the OS when one or more
 *   signal handling actions have been queued for execution.
 *   The architecture specific code must configure things so
 *   that the 'sigdeliver' callback is executed on the thread
 *   specified by 'tcb' as soon as possible.
 *
 *   This function may be called from interrupt handling logic.
 *
 *   This operation should not cause the task to be unblocked
 *   nor should it cause any immediate execution of sigdeliver.
 *   Typically, a few cases need to be considered:
 *
 *   (1) This function may be called from an interrupt handler
 *       During interrupt processing, all xcptcontext structures
 *       should be valid for all tasks.  That structure should
 *       be modified to invoke sigdeliver() either on return
 *       from (this) interrupt or on some subsequent context
 *       switch to the recipient task.
 *   (2) If not in an interrupt handler and the tcb is NOT
 *       the currently executing task, then again just modify
 *       the saved xcptcontext structure for the recipient
 *       task so it will invoke sigdeliver when that task is
 *       later resumed.
 *   (3) If not in an interrupt handler and the tcb IS the
 *       currently executing task -- just call the signal
 *       handler now.
 *
 * Assumptions:
 *   Called from critical section
 *
 ****************************************************************************/

void up_schedule_sigaction(struct tcb_s *tcb)
{
  uintptr_t int_ctx;

  /* First, handle some special cases when the signal is being delivered
   * to task that is currently executing on any CPU.
   */

  if (tcb == this_task() && !up_interrupt_context())
    {
      /* In this case just deliver the signal now.
       * REVISIT:  Signal handler will run in a critical section!
       */

      (tcb->sigdeliver)(tcb);
      tcb->sigdeliver = NULL;
    }
  else
    {
      /* Save the return EPC and STATUS registers.  These will be
        * restored by the signal trampoline after the signals have
        * been delivered.
        */

      /* Save the current register context location */

      tcb->xcp.saved_regs = tcb->xcp.regs;

      /* Duplicate the register context.  These will be
        * restored by the signal trampoline after the signal has been
        * delivered.
        */

      tcb->xcp.regs = (uintptr_t *)((uintptr_t)tcb->xcp.regs -
                                                XCPTCONTEXT_SIZE);

      memcpy(tcb->xcp.regs, tcb->xcp.saved_regs, XCPTCONTEXT_SIZE);

      tcb->xcp.regs[REG_EIPC]      = (uintptr_t)rh850_sigdeliver;
      int_ctx                     = tcb->xcp.regs[REG_EIPSW];
      int_ctx                    &= ~PSW_ID;
      tcb->xcp.regs[REG_EIPSW]  = int_ctx;
    }
}
