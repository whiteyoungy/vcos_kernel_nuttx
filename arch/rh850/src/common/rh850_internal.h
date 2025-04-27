/****************************************************************************
 * arch/rh850/src/common/rh850_internal.h
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

#ifndef __ARCH_RH850_SRC_COMMON_RH850_INTERNAL_H
#define __ARCH_RH850_SRC_COMMON_RH850_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
#  include <nuttx/compiler.h>
#  include <nuttx/arch.h>
#  include <sys/types.h>
#  include <stdint.h>
#  include <syscall.h>

#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Determine which (if any) console driver to use.  If a console is enabled
 * and no other console device is specified, then a serial console is
 * assumed.
 */

#ifndef CONFIG_DEV_CONSOLE
#  undef  USE_SERIALDRIVER
#  undef  USE_EARLYSERIALINIT
#else
#  if defined(CONFIG_LWL_CONSOLE)
#    undef  USE_SERIALDRIVER
#    undef  USE_EARLYSERIALINIT
#  elif defined(CONFIG_CONSOLE_SYSLOG)
#    undef  USE_SERIALDRIVER
#    undef  USE_EARLYSERIALINIT
#  elif defined(CONFIG_SERIAL_RTT_CONSOLE)
#    undef  USE_SERIALDRIVER
#    undef  USE_EARLYSERIALINIT
#  elif defined(CONFIG_RPMSG_UART_CONSOLE)
#    undef  USE_SERIALDRIVER
#    undef  USE_EARLYSERIALINIT
#  else
#    define USE_SERIALDRIVER 1
#    define USE_EARLYSERIALINIT 1
#  endif
#endif

/* If some other device is used as the console, then the serial driver may
 * still be needed.  Let's assume that if the upper half serial driver is
 * built, then the lower half will also be needed.  There is no need for
 * the early serial initialization in this case.
 */

#if !defined(USE_SERIALDRIVER) && defined(CONFIG_STANDARD_SERIAL)
#  define USE_SERIALDRIVER 1
#endif

/* For use with EABI and floating point, the stack must be aligned to 8-byte
 * addresses.
 */

#define STACK_ALIGNMENT     8

#define PSW_ID              (1 << 5)

#define INITIAL_PSW             ( 0x00018000 )                  /* PSW. and PSW.EBV bit */
#define INITIAL_PSW_USER        ( 0x40018000 )                  /* PSW.UM, PSW.CU0 and PSW.EBV bit */
#define INITIAL_PSW_INT_DISABLE (( 0x00018000 ) | (0x1 << 5))   /* PSW. and PSW.EBV  PSW.EI bit */

/* Stack alignment macros */

#define STACK_ALIGN_MASK    (STACK_ALIGNMENT - 1)
#define STACK_ALIGN_DOWN(a) ((a) & ~STACK_ALIGN_MASK)
#define STACK_ALIGN_UP(a)   (((a) + STACK_ALIGN_MASK) & ~STACK_ALIGN_MASK)

/* Check if an interrupt stack size is configured */

#ifndef CONFIG_ARCH_INTERRUPTSTACK
#  define CONFIG_ARCH_INTERRUPTSTACK 0
#endif

#define INTSTACK_SIZE (CONFIG_ARCH_INTERRUPTSTACK & ~STACK_ALIGN_MASK)

/* This is the value used to mark the stack for subsequent stack monitoring
 * logic.
 */

#define STACK_COLOR    0xdeadbeef
#define INTSTACK_COLOR 0xdeadbeef
#define HEAP_COLOR     'h'

#define getreg8(a)     (*(volatile uint8_t *)(a))
#define putreg8(v,a)   (*(volatile uint8_t *)(a) = (v))
#define getreg16(a)    (*(volatile uint16_t *)(a))
#define putreg16(v,a)  (*(volatile uint16_t *)(a) = (v))
#define getreg32(a)    (*(volatile uint32_t *)(a))
#define putreg32(v,a)  (*(volatile uint32_t *)(a) = (v))
#define getreg64(a)    (*(volatile uint64_t *)(a))
#define putreg64(v,a)  (*(volatile uint64_t *)(a) = (v))

/* Non-atomic, but more effective modification of registers */

#define modreg8(v,m,a)  putreg8((getreg8(a) & ~(m)) | ((v) & (m)), (a))
#define modreg16(v,m,a) putreg16((getreg16(a) & ~(m)) | ((v) & (m)), (a))
#define modreg32(v,m,a) putreg32((getreg32(a) & ~(m)) | ((v) & (m)), (a))
#define modreg64(v,m,a) putreg64((getreg64(a) & ~(m)) | ((v) & (m)), (a))

/* Context switching */

#ifndef rh850_fullcontextrestore
#  define rh850_fullcontextrestore(next) \
    sys_call1(SYS_restore_context, (uintptr_t)next);
#else
extern void rh850_fullcontextrestore(uintptr_t *next);
#endif

#ifndef rh850_switchcontext
#  define rh850_switchcontext(prev, next) \
    sys_call2(SYS_switch_context, (uintptr_t)prev, (uintptr_t)next);
#else
extern void rh850_switchcontext((uintptr_t)prev,
                                  (uintptr_t)next);
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifndef __ASSEMBLY__
typedef void (*up_vector_t)(void);
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

/* These symbols are setup by the linker script. */

extern uintptr_t        _heap_start[];
extern uintptr_t        _heap_limit[];

extern uintptr_t        _idle_stack_start[];
extern uintptr_t        _idle_stack_limit[];
#define g_idle_topstack _idle_stack_limit

#if CONFIG_ARCH_INTERRUPTSTACK > 3
extern uintptr_t        _int_stack_start[];
extern uintptr_t        _int_stack_limit[];
#define g_intstackalloc _int_stack_start
#define g_intstacktop   _int_stack_limit
#endif

#endif

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

static inline void rh850_savecontext(struct tcb_s *tcb)
{
  tcb->xcp.regs = (uint32_t *)up_current_regs();
}

static inline void rh850_restorecontext(struct tcb_s *tcb)
{
  up_set_current_regs((uint32_t *)(tcb->xcp.regs));
}

static inline unsigned char
rh850_atomic_cas(volatile void *target, int old_val, int new_val)
{
  register int val;
  register int val2;
  register int ret;
  __asm__ __volatile__(
      "1:mov  %5, %2\n"
      "ldl.w  [%3], %1\n"
      "cmp    %1, %4\n"
      "bnz    3f\n"
      "stc.w  %2, [%3]\n"
      "cmp    r0, %2\n"
      "bnz    2f\n"
      "br    1b\n"
      "2:mov    1, %0\n"
      "br    4f\n"
      "3:cll\n"
      "mov    0, %0\n"
      "4:"
      : "=&r"(ret), "=&r"(val), "=&r"(val2)
      : "r"(target), "r"(old_val), "r"(new_val)
      : "cc", "memory");

  return (unsigned char)ret;
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Signal handling **********************************************************/

void rh850_sigdeliver(void);

/* Exception Handler ********************************************************/

void * rh850_svcall(volatile void *trap);
void rh850_trapcall(volatile void *trap);

/* Debug ********************************************************************/

#ifdef CONFIG_STACK_COLORATION
size_t rh850_stack_check(uintptr_t alloc, size_t size);
void rh850_stack_color(void *stackbase, size_t nbytes);
#endif

#endif /* __ARCH_RH850_SRC_COMMON_RH850_INTERNAL_H */
