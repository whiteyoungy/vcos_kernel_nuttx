/****************************************************************************
 * arch/arm/src/armv8-r/arm_dataabort.c
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
#include <stdio.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/irq.h>

#include "sched/sched.h"
#include "arm_internal.h"
#include "arm_dbgmonitor.h"

#ifdef CONFIG_ARCH_HAVE_DEBUG
#define GET_STATUS_VALUE(val)               ((val) & 0x3f)
#define DEBUG_EXCEPTION                     (0b100010)
extern void set_debug_msg(uint32_t *regs, char *msg, uint32_t msg_size);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_dataabort
 *
 * Input Parameters:
 *   regs - The standard, ARM register save array.
 *   dfar - Fault address register.
 *   dfsr - Fault status register.
 *
 * Description:
 *   This is the data abort exception handler. The ARM data abort exception
 *   occurs when a memory fault is detected during a data transfer.
 *
 ****************************************************************************/

uint32_t *arm_dataabort(uint32_t *regs, uint32_t dfar, uint32_t dfsr)
{
#ifdef CONFIG_ARCH_HAVE_DEBUG
  volatile uint32_t status_resgister_value = 0;
  char bkpt_msg[128] = {0};
  uint32_t bkpt_msg_len = 0;
  /* Offload debug monitor processing flow */
  status_resgister_value = GET_STATUS_VALUE(dfsr);
  if (status_resgister_value == DEBUG_EXCEPTION)
    {
      bkpt_msg_len = sprintf(bkpt_msg, "Data abort. PC: %08" PRIx32 " DFAR: %08" PRIx32 " DFSR: %08"
            PRIx32 "\n", regs[REG_PC], dfar, dfsr);
      set_debug_msg(regs, bkpt_msg, bkpt_msg_len);
      arm_dbgmonitor(0x10, regs, NULL);
      set_debug_msg(NULL, NULL, 0);
      return regs;
    }
#endif
  /* Save the saved processor context in current_regs where it can be
   * accessed for register dumps and possibly context switching.
   */

  up_set_current_regs(regs);

  /* Crash -- possibly showing diagnostic debug information. */

  _alert("Data abort. PC: %08" PRIx32 " DFAR: %08" PRIx32 " DFSR: %08"
         PRIx32 "\n", regs[REG_PC], dfar, dfsr);
  PANIC_WITH_REGS("panic", regs);
  return regs; /* To keep the compiler happy */
}
