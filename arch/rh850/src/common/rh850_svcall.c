/****************************************************************************
 * arch/rh850/src/common/rh850_svcall.c
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

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <debug.h>
#include <syscall.h>

#include <arch/irq.h>
#include <sched/sched.h>
#include <nuttx/sched.h>

#include "rh850_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tricore_svcall
 *
 * Description:
 *   This is SVCall exception handler that performs context switching
 *
 ****************************************************************************/

void * rh850_svcall(volatile void *context)
{
  struct tcb_s *tcb;
  int cpu;

  uintptr_t *regs = (uintptr_t *)context;
  uint32_t cmd;

  up_set_current_regs(regs);

  cmd = regs[REG_R20];

  /* Handle the SVCall according to the command in R0 */

  switch (cmd)
    {
      case SYS_restore_context:
        {
          struct tcb_s *next = (struct tcb_s *)(uintptr_t)regs[REG_R21];
          DEBUGASSERT(regs[REG_R21] != 0);
          rh850_restorecontext(next);
        }
        break;

      case SYS_switch_context:
        {
          struct tcb_s *prev = (struct tcb_s *)(uintptr_t)regs[REG_R21];
          struct tcb_s *next = (struct tcb_s *)(uintptr_t)regs[REG_R22];

          DEBUGASSERT(regs[REG_R21] != 0 && regs[REG_R22] != 0);
          rh850_savecontext(prev);
          rh850_restorecontext(next);
        }
        break;

      default:
        {
          svcerr("ERROR: Bad SYS call: %d\n", (int)regs[REG_R20]);
        }
        break;
    }

  if (regs != up_current_regs())
    {
      /* Record the new "running" task.  g_running_tasks[] is only used by
       * assertion logic for reporting crashes.
       */

      cpu = this_cpu();
      tcb = current_task(cpu);
      g_running_tasks[cpu] = tcb;

      /* Restore the cpu lock */

      restore_critical_section(tcb, cpu);

      /* If a context switch occurred while processing the interrupt then
       * up_current_regs() may have change value.  If we return any value
       * different from the input regs, then the lower level will know
       * that a context switch occurred during interrupt processing.
       */

      regs = (uintptr_t *)up_current_regs();
    }

   up_set_current_regs(NULL);

  return regs;
}
