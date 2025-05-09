/****************************************************************************
 * arch/sim/src/sim/sim_head.c
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

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <syslog.h>
#include <assert.h>

#include <nuttx/init.h>
#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/symtab.h>
#include <nuttx/rptun/rptun.h>
#include <nuttx/syslog/syslog_rpmsg.h>

#include "sim_internal.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern uint8_t _sbss[];
extern uint8_t _ebss[];
extern uint8_t _sdata[];
extern uint8_t _edata[];

#define ROUND_UP(a, b)          (((a) + (b) - 1) & ~((b) - 1))
#ifndef IDLE_STACK_BASE
#  ifdef CONFIG_SMP
#    define IDLE_STACK_BASE _enoinit
#  else
#    define IDLE_STACK_BASE _ebss
#  endif
#endif

#define IDLE_STACK_TOP  (uintptr_t)IDLE_STACK_BASE+CONFIG_IDLETHREAD_STACKSIZE

int g_argc;
char **g_argv;
#ifdef CONFIG_BMP
unsigned long g_cpu_data_size ;
#endif
unsigned long g_idle_topstack = IDLE_STACK_TOP;

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_SYSLOG_RPMSG
static char g_logbuffer[4096];
#endif

#ifdef CONFIG_ALLSYMS
extern struct symtab_s g_allsyms[];
extern int             g_nallsyms;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: allsyms_relocate
 *
 * Description:
 *   Simple relocate to redirect the address from symbol table.
 *
 ****************************************************************************/

#ifdef CONFIG_ALLSYMS
static void allsyms_relocate(void)
{
  uintptr_t offset;
  int       i;

  for (i = 0; i < g_nallsyms; i++)
    {
      if (strcmp("allsyms_relocate", g_allsyms[i].sym_name) == 0)
        {
          offset = (uintptr_t)allsyms_relocate -
                   (uintptr_t)g_allsyms[i].sym_value;
          for (i = 0; i < g_nallsyms; i++)
            {
              g_allsyms[i].sym_value =
                (void *)((uintptr_t)g_allsyms[i].sym_value + offset);
            }
          break;
        }
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __xsan_default_options
 *
 * Description:
 *   This function may be optionally provided by user and should return
 *   a string containing sanitizer runtime options.
 *
 ****************************************************************************/

#ifdef CONFIG_SIM_ASAN
noprofile_function const char *__asan_default_options(void)
{
  return "abort_on_error=1"
         " alloc_dealloc_mismatch=0"
         " allocator_frees_and_returns_null_on_realloc_zero=0"
         " check_initialization_order=1"
         " fast_unwind_on_malloc=0"
         " strict_init_order=1"
         " detect_stack_use_after_return=0";
}

noprofile_function const char *__lsan_default_options(void)
{
  /* The fast-unwind implementation of leak-sanitizer will obtain the
   * current stack top/bottom and frame address(Stack Pointer) for
   * backtrace calculation:
   *
   * https://github.com/gcc-mirror/gcc/blob/releases/gcc-13/libsanitizer/
   *   lsan/lsan.cpp#L39-L42
   *
   * Since the scheduling mechanism of NuttX sim is coroutine
   * (setjmp/longjmp), if the Stack Pointer is switched, the fast-unwind
   * will unable to get the available address, so the memory leaks on the
   * system/application side that cannot be caught normally. This PR will
   * disable fast-unwind by default to avoid unwind failure.
   */

  return "detect_leaks=1"
         " fast_unwind_on_malloc=0";
}
#endif

#ifdef CONFIG_SIM_UBSAN
noprofile_function const char *__ubsan_default_options(void)
{
#ifdef CONFIG_SIM_UBSAN_DUMMY
  return "";
#else
  return "print_stacktrace=1"
         " fast_unwind_on_malloc=0";
#endif
}
#endif

/****************************************************************************
 * Name: sim_boot
 * Description:
 *   sim_boot is used to init data section and bss section with per_cpu.
 *
 ****************************************************************************/

void sim_boot(void)
{
#ifdef CONFIG_BMP
  g_cpu_data_size = ROUND_UP((uintptr_t)g_idle_topstack, 64)  \
  - (uintptr_t)_sdata;

  /* Init per-cpu data */

  for (int i = 1; i < CONFIG_BMP_NCPUS; i++)
    {
      memcpy((void *)((uintptr_t)_sdata + g_cpu_data_size * i),
          _sdata, g_cpu_data_size);
    }
#endif
}

/****************************************************************************
 * Name: main
 *
 * Description:
 *   This is the main entry point into the simulation.
 *
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  g_argc = argc;
  g_argv = argv;
  sim_boot();
#ifdef CONFIG_ALLSYMS
  allsyms_relocate();
#endif

#ifdef CONFIG_SYSLOG_RPMSG
  syslog_rpmsg_init_early(g_logbuffer, sizeof(g_logbuffer));
#endif

  /* Start NuttX */

#ifdef CONFIG_SMP
  /* Start the CPU0 emulation.  This should not return. */

  host_cpu0_start();
#endif
  /* Start the NuttX emulation.  This should not return. */

#ifdef CONFIG_BMP
  extern void bmp_start(void);
  bmp_start();
#else
  nx_start();
#endif
  return EXIT_FAILURE;
}

/****************************************************************************
 * Name: board_power_off
 *
 * Description:
 *   Power off the board.  This function may or may not be supported by a
 *   particular board architecture.
 *
 * Input Parameters:
 *   status - Status information provided with the power off event.
 *
 * Returned Value:
 *   If this function returns, then it was not possible to power-off the
 *   board due to some constraints.  The return value int this case is a
 *   board-specific reason for the failure to shutdown.
 *
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_POWEROFF
int board_power_off(int status)
{
#ifdef CONFIG_RPTUN
  rptun_poweroff(NULL);
#endif

  /* Abort simulator */

  host_abort(status);

  /* Does not really return */

  return 0;
}
#endif
