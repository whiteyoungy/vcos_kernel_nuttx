/****************************************************************************
 * arch/arm/src/armv8-r/arm_dbgmonitor.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

#include <nuttx/arch.h>
#include "dwt.h"
#include <arch/armv8-r/cp15.h>
#include "barriers.h"
#include "arm_dbgmonitor.h"

#define dbg_info                  sinfo
#define dbg_warn                  swarn
#define dbg_error                 serr

struct arm_debug_s
{
  int type;
  void *addr;
  size_t size;
  debug_callback_t callback;
  void *arg;
};

static __percpu_bss struct arm_debug_s __g_arm_debug[ARM_DEBUG_MAX];
#define g_arm_debug this_cpu_var(__g_arm_debug)

static __percpu_bss int __arm_debug_enable = 0;
#define arm_debug_enable this_cpu_var(__arm_debug_enable)

__percpu_bss uint32_t *__debug_registers;
#define debug_registers this_cpu_var(__debug_registers)

__percpu_bss char     *__debug_msg;
#define debug_msg this_cpu_var(__debug_msg)

__percpu_bss uint32_t __debug_msg_len;
#define debug_msg_len this_cpu_var(__debug_msg_len)

void set_debug_msg(uint32_t *regs, char *msg, uint32_t msg_size)
{
  debug_registers = regs;
  debug_msg       = msg;
  debug_msg_len   = msg_size;
}

uint32_t* get_debug_registers(void)
{
  return debug_registers;
}

void show_debug_msg(void)
{
  if ((debug_msg == NULL) || (debug_msg_len == 0))
    {
      return;
    }
  dbg_info("%s", debug_msg);
}

/****************************************************************************
 * Nuttx adapt end
 ****************************************************************************/

static void set_watchpoint(uint32_t type, unsigned int watchpoint_num, unsigned int addr)
{
  struct debugpoint_control_register ctrl = {0};
  unsigned int ctrl_reg = 0;
  unsigned int addr_result;
  unsigned int ctrl_reg_result;

  ctrl.lsc  = type;
  ctrl.bas  = ARM_BREAKPOINT_BAS_1111;
  ctrl.pmc  = ARM_BREAKPOINT_PRIV | ARM_BREAKPOINT_USER;
  ctrl.e    = 1;

  ctrl_reg      = GET_DBCR(ctrl);

  /* Setup the address register. */
  WRITE_WATCHPOINT_VALUE_REGISTER(watchpoint_num, addr);

  /* Setup the control register. */
  WRITE_WATCHPOINT_CONTROL_REGISTER(watchpoint_num, ctrl_reg);

  addr_result = READ_WATCHPOINT_VALUE_REGISTER(watchpoint_num);
  ctrl_reg_result = READ_WATCHPOINT_CONTROL_REGISTER(watchpoint_num);

  if (((addr_result & (~0x3)) != (addr & (~0x3))) || (ctrl_reg_result != ctrl_reg))
    {
      dbg_warn("0x%x 0x%x written to %d failed, curr val is 0x%x 0x%x\n",
        addr, ctrl_reg, watchpoint_num, addr_result, ctrl_reg_result);
    }
}

static void disable_watchpoint(uint32_t type, unsigned int watchpoint_num)
{
  struct debugpoint_control_register ctrl = {0};
  unsigned int ctrl_reg = 0;
  unsigned int ctrl_reg_result;

  ctrl.lsc  = type;
  ctrl.bas  = ARM_BREAKPOINT_BAS_1111;
  ctrl.pmc  = ARM_BREAKPOINT_PRIV | ARM_BREAKPOINT_USER;
  ctrl.e    = 0;

  ctrl_reg      = GET_DBCR(ctrl);

  /* Setup the control register. */
  WRITE_WATCHPOINT_CONTROL_REGISTER(watchpoint_num, ctrl_reg);

  ctrl_reg_result = READ_WATCHPOINT_CONTROL_REGISTER(watchpoint_num);
  if (ctrl_reg_result != ctrl_reg)
    {
      dbg_warn("0x%x written to %d failed, curr val is 0x%x\n",
        ctrl_reg, watchpoint_num, ctrl_reg_result);
    }
}

static void set_breakpoint(unsigned int breakpoint_num, unsigned int addr)
{
  struct debugpoint_control_register ctrl = {0};
  unsigned int ctrl_reg = 0;
  unsigned int addr_result;
  unsigned int ctrl_reg_result;

  ctrl.bas  = ARM_BREAKPOINT_BAS_1111;
  ctrl.pmc  = ARM_BREAKPOINT_PRIV | ARM_BREAKPOINT_USER;
  ctrl.e    = 1;

  ctrl_reg      = GET_DBCR(ctrl);

  /* Setup the address register. */
  WRITE_BREAKPOINT_VALUE_REGISTER(breakpoint_num, addr);

  /* Setup the control register. */
  WRITE_BREAKPOINT_CONTROL_REGISTER(breakpoint_num, ctrl_reg);

  addr_result = READ_BREAKPOINT_VALUE_REGISTER(breakpoint_num);
  ctrl_reg_result = READ_BREAKPOINT_CONTROL_REGISTER(breakpoint_num);

  if (((addr_result & (~0x3)) != (addr & (~0x3))) || (ctrl_reg_result != ctrl_reg))
    {
      dbg_warn("0x%x  0x%x written to %d failed, curr val is 0x%x 0x%x\n",
        addr, ctrl_reg, breakpoint_num, addr_result, ctrl_reg_result);
    }
}

static void disable_breakpoint(unsigned int breakpoint_num)
{
  struct debugpoint_control_register ctrl = {0};
  unsigned int ctrl_reg = 0;
  unsigned int ctrl_reg_result;

  ctrl.bas  = ARM_BREAKPOINT_BAS_1111;
  ctrl.pmc  = ARM_BREAKPOINT_PRIV | ARM_BREAKPOINT_USER;
  ctrl.e    = 0;

  ctrl_reg      = GET_DBCR(ctrl);

  /* Setup the control register. */
  WRITE_BREAKPOINT_CONTROL_REGISTER(breakpoint_num, ctrl_reg);

  ctrl_reg_result = READ_BREAKPOINT_CONTROL_REGISTER(breakpoint_num);
  if (ctrl_reg_result != ctrl_reg)
    {
      dbg_warn("0x%x written to %d failed, curr val is 0x%x\n",
        ctrl_reg, breakpoint_num, ctrl_reg_result);
    }
}

static void enable_monitor_mode(void)
{
  u32 dscr;

  /*
   * Unconditionally clear the OS lock by writing a value
   * other than CS_LAR_KEY to the access register.
   */
  CP14_SET(DBGOSLAR, ~CORESIGHT_UNLOCK);
  ARM_ISB();

  dscr = CP14_GET(DBGDSCRint);
  CP14_SET(DBGDSCRext, (dscr | ARM_DSCR_MDBGEN));

  dscr = CP14_GET(DBGDSCRint);
  if (!(dscr & ARM_DSCR_MDBGEN))
    {
      dbg_error("Failed to enable monitor mode.\n");
    }
  else
    {
      dbg_info("success to enable monitor mode.\n");
    }
}

used_code
static void disable_monitor_mode(void)
{
  u32 dscr;

  dscr = CP14_GET(DBGDSCRint);
  CP14_SET(DBGDSCRext, (dscr & ~ARM_DSCR_MDBGEN));

  dscr = CP14_GET(DBGDSCRint);
  if ((dscr & ARM_DSCR_MDBGEN))
    {
      dbg_error("Failed to disable monitor mode.\n");
    }
  else
    {
      dbg_info("success to disable monitor mode.\n");
    }

  CP14_SET(DBGOSLAR, CORESIGHT_UNLOCK);
  ARM_ISB();
}

/****************************************************************************
 * Nuttx adapt
 ****************************************************************************/

/****************************************************************************
 * Name: arm_watchpoint_add
 *
 * Description:
 *   Add a watchpoint on the address.
 *
 * Input Parameters:
 *  type - The type of the watchpoint
 *  addr - The address to be watched
 *  size - The size of the address to be watched
 *
 * Returned Value:
 *  Zero on success; a negated errno value on failure
 *
 * Notes:
 *  The size of the watchpoint is determined by the hardware.
 *
 ****************************************************************************/

static int arm_watchpoint_add(int type, uintptr_t addr, size_t size)
{
  uint32_t fun;
  uint32_t num = ARM_RWPT_NUM();
  int32_t watchpoint_num = -1;
  uint32_t i;

  switch (type)
    {
      case DEBUGPOINT_WATCHPOINT_RO:
        fun = DWT_FUNCTION_WATCHPOINT_RO;
        break;
      case DEBUGPOINT_WATCHPOINT_WO:
        fun = DWT_FUNCTION_WATCHPOINT_WO;
        break;
      case DEBUGPOINT_WATCHPOINT_RW:
        fun = DWT_FUNCTION_WATCHPOINT_RW;
        break;
      default:
        return -EINVAL;
    }

  for (i = 0; i < num; i++)
    {
      uint32_t rwpt_control = READ_WATCHPOINT_CONTROL_REGISTER(i);
      if ((rwpt_control & 0x1) == 0)
      {
        watchpoint_num = i; // update to the last free
        continue;
      }
      uint32_t rwpt_addr = READ_WATCHPOINT_VALUE_REGISTER(i);
      if ((rwpt_addr == addr) && (fun == ((rwpt_control & (0x3 << 3)) >> 3))) /* Already set */
      {
        return 0;
      }
    }

  if (watchpoint_num != -1)
    {
      set_watchpoint(fun, watchpoint_num, addr);
      return 0;
    }

  return -ENOSPC;
}

/****************************************************************************
 * Name: arm_watchpoint_remove
 *
 * Description:
 *   Remove a watchpoint on the address.
 *
 * Input Parameters:
 *   type - The type of the watchpoint.
 *   addr - The address to be watched.
 *   size - The size of the address to be watched.
 *
 * Returned Value:
 *  Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int arm_watchpoint_remove(int type, uintptr_t addr, size_t size)
{
  uint32_t fun;
  uint32_t num = ARM_RWPT_NUM();
  uint32_t i;

  switch (type)
    {
      case DEBUGPOINT_WATCHPOINT_RO:
        fun = DWT_FUNCTION_WATCHPOINT_RO;
        break;
      case DEBUGPOINT_WATCHPOINT_WO:
        fun = DWT_FUNCTION_WATCHPOINT_WO;
        break;
      case DEBUGPOINT_WATCHPOINT_RW:
        fun = DWT_FUNCTION_WATCHPOINT_RW;
        break;
      default:
        return -EINVAL;
    }

  for (i = 0; i < num; i++)
    {
      uint32_t rwpt_control = READ_WATCHPOINT_CONTROL_REGISTER(i);
      if ((rwpt_control & 0x1) == 0)
        {
          continue;
        }
      uint32_t rwpt_addr = READ_WATCHPOINT_VALUE_REGISTER(i);
      if ((rwpt_addr == addr) && (fun == ((rwpt_control & (0x3 << 3)) >> 3))) /* Already set */
        {
          disable_watchpoint(fun, i);
          return 0;
        }
    }

  return -EINVAL;
}

/****************************************************************************
 * Name: arm_watchpoint_match
 *
 * Description:
 *   This function will be called when watchpoint match.
 *
 ****************************************************************************/

static void arm_watchpoint_match(void)
{
  uint32_t i;
  uint32_t dfar;
  dfar = CP15_GET(DFAR);

  for (i = 0; i < ARM_DEBUG_MAX; i++)
    {
      if (g_arm_debug[i].addr == (void *)dfar && g_arm_debug[i].callback)
        {
          g_arm_debug[i].callback(g_arm_debug[i].type,
                                  g_arm_debug[i].addr,
                                  g_arm_debug[i].size,
                                  g_arm_debug[i].arg);
          break;
        }
    }

  return;
}

/****************************************************************************
 * Name: arm_breakpoint_add
 *
 * Description:
 *   Add a breakpoint on addr.
 *
 * Input Parameters:
 *  addr - The address to break.
 *
 * Returned Value:
 *  Zero on success; a negated errno value on failure
 *
 * Notes:
 *  1. If breakpoint is already set, it will do nothing.
 *  2. If all comparators are in use, it will return -ENOSPC.
 *  3. When the breakpoint trigger, if enable monitor exception already ,
 *     will cause a debug monitor exception, otherwise will cause
 *     a hard fault.
 *
 ****************************************************************************/
static int arm_breakpoint_add(uintptr_t addr)
{
  int32_t breakpoint_num = -1;
  uint32_t num = ARM_BKPT_NUM();
  uint32_t i;

  for (i = 0; i < num; i++)
    {
      uint32_t bkpt_control = READ_BREAKPOINT_CONTROL_REGISTER(i);
      if ((bkpt_control & 0x1) == 0)
        {
          breakpoint_num = i; // update to the last free
          continue;
        }
      uint32_t bkpt_addr = READ_BREAKPOINT_VALUE_REGISTER(i);
      if (bkpt_addr == addr) /* Already set */
        {
          return 0;
        }
    }
  if (breakpoint_num != -1) /* Find a free comparators */
    {
      set_breakpoint(breakpoint_num, addr);
      return 0;
    }

  return -ENOSPC;
}
/****************************************************************************
 * Name: arm_breakpoint_remove
 *
 * Description:
 *   Remove a breakpoint on addr.
 *
 * Input Parameters:
 *  addr - The address to remove.
 *
 * Returned Value:
 *  Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int arm_breakpoint_remove(uintptr_t addr)
{
  uint32_t num = ARM_BKPT_NUM();
  uint32_t i;

  for (i = 0; i < num; i++)
    {
      uint32_t bkpt_control = READ_BREAKPOINT_CONTROL_REGISTER(i);
      if ((bkpt_control & 0x1) == 0)
        {
          continue;
        }
      uint32_t bkpt_addr = READ_BREAKPOINT_VALUE_REGISTER(i);
      if (bkpt_addr == addr) /* Already set */
        {
          disable_breakpoint(i);
          return 0;
        }
    }

  return -EINVAL;
}

/****************************************************************************
 * Name: arm_breakpoint_match
 *
 * Description:
 *   This function will be called when breakpoint match.
 *
 ****************************************************************************/

static void arm_breakpoint_match(uintptr_t pc)
{
  int i;

  for (i = 0; i < ARM_DEBUG_MAX; i++)
    {
      if (g_arm_debug[i].type == DEBUGPOINT_BREAKPOINT &&
          g_arm_debug[i].addr == (void *)pc &&
          g_arm_debug[i].callback != NULL)
        {
          g_arm_debug[i].callback(g_arm_debug[i].type,
                                  g_arm_debug[i].addr,
                                  g_arm_debug[i].size,
                                  g_arm_debug[i].arg);
          break;
        }
    }
}

/****************************************************************************
 * Name: arm_steppoint
 *
 * Description:
 *   Enable/disable single step.
 *
 * Input Parameters:
 *  enable - True: enable single step; False: disable single step.
 *
 * Returned Value:
 *  Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int arm_steppoint(bool enable)
{
  dbg_warn("step point not support\n");
  return 0;
}

/****************************************************************************
 * Name: arm_steppoint_match
 *
 * Description:
 *   This function will be called when single step match.
 *
 ****************************************************************************/

used_code
static void arm_steppoint_match(void)
{
  int i;

  for (i = 0; i < ARM_DEBUG_MAX; i++)
    {
      if (g_arm_debug[i].type == DEBUGPOINT_STEPPOINT &&
          g_arm_debug[i].callback != NULL)
        {
          g_arm_debug[i].callback(g_arm_debug[i].type,
                                  g_arm_debug[i].addr,
                                  g_arm_debug[i].size,
                                  g_arm_debug[i].arg);
          break;
        }
    }
}

/****************************************************************************
 * Name: up_debugpoint_add
 *
 * Description:
 *   Add a debugpoint.
 *
 * Input Parameters:
 *   type     - The debugpoint type. optional value:
 *              DEBUGPOINT_WATCHPOINT_RO - Read only watchpoint.
 *              DEBUGPOINT_WATCHPOINT_WO - Write only watchpoint.
 *              DEBUGPOINT_WATCHPOINT_RW - Read and write watchpoint.
 *              DEBUGPOINT_BREAKPOINT    - Breakpoint.
 *              DEBUGPOINT_STEPPOINT     - Single step.
 *   addr     - The address to be debugged.
 *   size     - The watchpoint size. only for watchpoint.
 *   callback - The callback function when debugpoint triggered.
 *              if NULL, the debugpoint will be removed.
 *   arg      - The argument of callback function.
 *
 * Returned Value:
 *  Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int up_debugpoint_add(int type, void *addr, size_t size,
                      debug_callback_t callback, void *arg)
{
  int ret = -EINVAL;
  int i;

  if (arm_debug_enable == 0)
    {
      enable_monitor_mode();
      arm_debug_enable = 1;
    }

  /* Thumb mode breakpoint address must be word-aligned */
  addr = (void *)((uintptr_t)addr & ~0x3);

  if (type == DEBUGPOINT_BREAKPOINT)
    {
      ret = arm_breakpoint_add((uintptr_t)addr);
    }
  else if (type == DEBUGPOINT_WATCHPOINT_RO ||
           type == DEBUGPOINT_WATCHPOINT_WO ||
           type == DEBUGPOINT_WATCHPOINT_RW)
    {
      ret = arm_watchpoint_add(type, (uintptr_t)addr, size);
    }
  else if (type == DEBUGPOINT_STEPPOINT)
    {
      ret = arm_steppoint(true);
    }

  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < ARM_DEBUG_MAX; i++)
    {
      if (g_arm_debug[i].type == DEBUGPOINT_NONE)
        {
          g_arm_debug[i].type = type;
          g_arm_debug[i].addr = addr;
          g_arm_debug[i].size = size;
          g_arm_debug[i].callback = callback;
          g_arm_debug[i].arg = arg;
          break;
        }
    }

  return ret;
}


/****************************************************************************
 * Name: up_debugpoint_remove
 *
 * Description:
 *   Remove a debugpoint.
 *
 * Input Parameters:
 *   type     - The debugpoint type. optional value:
 *              DEBUGPOINT_WATCHPOINT_RO - Read only watchpoint.
 *              DEBUGPOINT_WATCHPOINT_WO - Write only watchpoint.
 *              DEBUGPOINT_WATCHPOINT_RW - Read and write watchpoint.
 *              DEBUGPOINT_BREAKPOINT    - Breakpoint.
 *              DEBUGPOINT_STEPPOINT     - Single step.
 *   addr     - The address to be debugged.
 *   size     - The watchpoint size. only for watchpoint.
 *
 * Returned Value:
 *  Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int up_debugpoint_remove(int type, void *addr, size_t size)
{
  int ret = -EINVAL;
  int i;

  /* Thumb mode breakpoint address must be word-aligned */
  addr = (void *)((uintptr_t)addr & ~0x3);

  if (type == DEBUGPOINT_BREAKPOINT)
    {
      ret = arm_breakpoint_remove((uintptr_t)addr);
    }
  else if (type == DEBUGPOINT_WATCHPOINT_RO ||
           type == DEBUGPOINT_WATCHPOINT_WO ||
           type == DEBUGPOINT_WATCHPOINT_RW)
    {
      ret = arm_watchpoint_remove(type, (uintptr_t)addr, size);
    }
  else if (type == DEBUGPOINT_STEPPOINT)
    {
      ret = arm_steppoint(false);
    }

  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < ARM_DEBUG_MAX; i++)
    {
      if (g_arm_debug[i].type == type &&
          g_arm_debug[i].size == size &&
          g_arm_debug[i].addr == addr)
        {
          g_arm_debug[i].type = DEBUGPOINT_NONE;
          break;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: arm_dbgmonitor
 *
 * Description:
 *   This is Debug Monitor exception handler.  This function is entered when
 *   the processor enters debug mode.  The debug monitor handler will handle
 *   debug events, and resume execution.
 *
 ****************************************************************************/

int arm_dbgmonitor(int irq, void *context, void *arg)
{
  uint32_t *regs = (uint32_t *)context;

  if (irq == 0xC)
    {
      arm_breakpoint_match(regs[REG_PC]);
    }

  if (irq == 0x10)
    {
      arm_watchpoint_match();
    }

  return OK;
}