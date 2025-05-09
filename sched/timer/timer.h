/****************************************************************************
 * sched/timer/timer.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __SCHED_TIMER_TIMER_H
#define __SCHED_TIMER_TIMER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

#include <nuttx/compiler.h>
#include <nuttx/signal.h>
#include <nuttx/wdog.h>

#ifndef CONFIG_DISABLE_POSIX_TIMERS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PT_FLAGS_PREALLOCATED 0x01 /* Timer comes from a pool of preallocated timers */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure represents one POSIX timer */

struct posix_timer_s
{
  FAR struct posix_timer_s *flink;

  clockid_t        pt_clock;       /* Specifies the clock to use as the timing base. */
  uint8_t          pt_flags;       /* See PT_FLAGS_* definitions */
  uint8_t          pt_crefs;       /* Reference count */
  pid_t            pt_owner;       /* Creator of timer */
  int              pt_overrun;     /* Overrun time */
  sclock_t         pt_delay;       /* If non-zero, used to reset repetitive timers */
  clock_t          pt_expected;    /* Expected absolute time */
  struct wdog_s    pt_wdog;        /* The watchdog that provides the timing */
  struct sigevent  pt_event;       /* Notification information */
#ifdef CONFIG_SIG_EVTHREAD
  struct sigwork_s pt_work;
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#if CONFIG_PREALLOC_TIMERS > 0
/* This is a list of free, preallocated timer structures */

extern volatile sq_queue_t g_freetimers;
#define g_freetimers       this_cpu_var(g_freetimers)
#endif

/* This is a list of instantiated timer structures -- active and inactive.
 * The timers are place on this list by timer_create() and removed from the
 * list by timer_delete() or when the owning thread exits.
 */

extern volatile sq_queue_t g_alloctimers;
#define g_alloctimers      this_cpu_var(g_alloctimers)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void timer_initialize(void);
void timer_deleteall(pid_t pid);
int timer_release(FAR struct posix_timer_s *timer);
FAR struct posix_timer_s *timer_gethandle(timer_t timerid);

#endif /* CONFIG_DISABLE_POSIX_TIMERS */
#endif /* __SCHED_TIMER_TIMER_H */
