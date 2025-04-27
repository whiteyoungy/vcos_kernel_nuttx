/****************************************************************************
 * arch/rh850/include/syscall.h
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
 * through include/syscall.h or include/sys/sycall.h
 */

#ifndef __ARCH_RH850_INCLUDE_SYSCALL_H
#define __ARCH_RH850_INCLUDE_SYSCALL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
#  include <stdint.h>
#endif

/****************************************************************************
 * Pre-processor Prototypes
 ****************************************************************************/

#define SYS_syscall 0x00

/* Configuration ************************************************************/

/* This logic uses three system calls {0,1,2} for context switching and one
 * for the syscall return.
 * So a minimum of four syscall values must be reserved.
 * If CONFIG_BUILD_FLAT isn't defined, then four more syscall values must
 * be reserved.
 */

#ifndef CONFIG_BUILD_FLAT
#  define CONFIG_SYS_RESERVED 8
#else
#  define CONFIG_SYS_RESERVED 4
#endif

/* Cortex-M system calls ****************************************************/

/* SYS call 1:
 *
 * void rh850_fullcontextrestore(uint32_t *restoreregs) noreturn_function;
 */

#define SYS_restore_context       (1)

/* SYS call 2:
 *
 * void rh850_switchcontext(uint32_t **saveregs, uint32_t *restoreregs);
 */

#define SYS_switch_context        (2)

#ifdef CONFIG_LIB_SYSCALL
/* SYS call 3:
 *
 * void rh850_syscall_return(void);
 */

#define SYS_syscall_return        (3)
#endif /* CONFIG_LIB_SYSCALL */

#ifndef CONFIG_BUILD_FLAT
/* SYS call 4:
 *
 * void up_task_start(main_t taskentry, int argc, char *argv[])
 *        noreturn_function;
 */

#define SYS_task_start            (4)

/* SYS call 5:
 *
 * void up_pthread_start((pthread_startroutine_t startup,
 *                        pthread_startroutine_t entrypt, pthread_addr_t arg)
 *        noreturn_function
 */

#define SYS_pthread_start         (5)

/* SYS call 6:
 *
 * void signal_handler(_sa_sigaction_t sighand,
 *                     int signo, siginfo_t *info,
 *                     void *ucontext);
 */

#define SYS_signal_handler        (6)

/* SYS call 7:
 *
 * void signal_handler_return(void);
 */

#define SYS_signal_handler_return (7)
#endif /* !CONFIG_BUILD_FLAT */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Inline functions
 ****************************************************************************/

#ifndef __ASSEMBLY__

/* SVC with SYS_ call number and no parameters */

static inline uintptr_t sys_call0(unsigned int nbr)
{
  register long reg20 = (long)(nbr);
  __asm __volatile__
  (
    "mov %0, r20\n\t"
    "trap 0"
    :: "r"(reg20)
    : "memory"
  );

  return reg20;
}

/* SVC with SYS_ call number and one parameter */

static inline uintptr_t sys_call1(unsigned int nbr, uintptr_t parm1)
{
  register long reg20 = (long)(nbr);
  register long reg21 = (long)(parm1);

  __asm __volatile__
  (
    "mov %0, r20\n\t"
    "mov %1, r21\n\t"
    "trap 0"
    :: "r"(reg20), "r"(reg21)
    : "memory"
  );

  return reg20;
}

/* SVC with SYS_ call number and two parameters */

static inline uintptr_t sys_call2(unsigned int nbr, uintptr_t parm1,
                                  uintptr_t parm2)
{
  register long reg20 = (long)(nbr);
  register long reg21  = (long)(parm1);
  register long reg22  = (long)(parm2);

  __asm __volatile__
  (
    "mov %0, r20\n\t"
    "mov %1, r21\n\t"
    "mov %2, r22\n\t"
    "trap 0"
    :: "r"(reg20), "r"(reg21), "r"(reg22)
    : "memory"
  );

  return reg20;
}

/* SVC with SYS_ call number and three parameters */

static inline uintptr_t sys_call3(unsigned int nbr, uintptr_t parm1,
                                  uintptr_t parm2, uintptr_t parm3)
{
  register long reg20 = (long)(nbr);
  register long reg21  = (long)(parm1);
  register long reg22  = (long)(parm2);
  register long reg23  = (long)(parm3);

  __asm __volatile__
  (
    "mov %0, r20\n\t"
    "mov %1, r21\n\t"
    "mov %2, r22\n\t"
    "mov %3, r23\n\t"
    "trap 0"
    :: "r"(reg20), "r"(reg21), "r"(reg22), "r"(reg23)
    : "memory"
  );

  return reg20;
}

/* SVC with SYS_ call number and four parameters */

static inline uintptr_t sys_call4(unsigned int nbr, uintptr_t parm1,
                                  uintptr_t parm2, uintptr_t parm3,
                                  uintptr_t parm4)
{
  register long reg20 = (long)(nbr);
  register long reg21  = (long)(parm1);
  register long reg22  = (long)(parm2);
  register long reg23  = (long)(parm3);
  register long reg24  = (long)(parm4);

  __asm __volatile__
  (
    "mov %0, r20\n\t"
    "mov %1, r21\n\t"
    "mov %2, r22\n\t"
    "mov %3, r23\n\t"
    "mov %4, r24\n\t"
    "trap 0"
    :: "r"(reg20), "r"(reg21), "r"(reg22), "r"(reg23), "r"(reg24)
    : "memory"
  );

  return reg20;
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

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

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_RH850_INCLUDE_SYSCALL_H */
