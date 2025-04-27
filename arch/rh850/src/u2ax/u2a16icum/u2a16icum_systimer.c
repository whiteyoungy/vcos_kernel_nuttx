/****************************************************************************
 * arch/rh850/src/u2ax/u2a16icum/u2a16icum_systimer.c
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

#include <nuttx/irq.h>
#include <nuttx/kmalloc.h>

#include <nuttx/timers/oneshot.h>
#include <nuttx/timers/arch_alarm.h>
#include <arch/board/board.h>

#include "rh850_internal.h"
#include "u2a16icum_ostm.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure provides the private representation of the "lower-half"
 * driver state structure.  This structure must be cast-compatible with the
 * oneshot_lowerhalf_s structure.
 */

struct rh850_systimer_lowerhalf_s
{
  struct oneshot_lowerhalf_s lower;
  volatile void              *tbase;
  uint64_t                   freq;
  uint64_t                   alarm;
  oneshot_callback_t         callback;
  void                       *arg;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int rh850_systimer_max_delay(struct oneshot_lowerhalf_s *lower,
                                      struct timespec *ts);
static int rh850_systimer_start(struct oneshot_lowerhalf_s *lower,
                                  oneshot_callback_t callback, void *arg,
                                  const struct timespec *ts);
static int rh850_systimer_cancel(struct oneshot_lowerhalf_s *lower,
                                   struct timespec *ts);
static int rh850_systimer_current(struct oneshot_lowerhalf_s *lower,
                                    struct timespec *ts);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct oneshot_operations_s g_rh850_systimer_ops =
{
  .max_delay = rh850_systimer_max_delay,
  .start     = rh850_systimer_start,
  .cancel    = rh850_systimer_cancel,
  .current   = rh850_systimer_current,
};

static struct rh850_systimer_lowerhalf_s g_systimer_lower =
{
  .lower.ops = &g_rh850_systimer_ops,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint32_t
rh850_systimer_get_time(struct rh850_systimer_lowerhalf_s *priv)
{
  irqstate_t flags;
  uint32_t ticks;

  flags = enter_critical_section();

  ticks = u2a16icum_ostm_gettime(priv->tbase);

  leave_critical_section(flags);

  return ticks;
}

static void
rh850_systimer_set_timecmp(struct rh850_systimer_lowerhalf_s *priv,
                             uint32_t value)
{
  irqstate_t flags;

  flags = enter_critical_section();

  u2a16icum_ostm_set_compare(priv->tbase, value);

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: rh850_systimer_max_delay
 *
 * Description:
 *   Determine the maximum delay of the one-shot timer
 *
 * Input Parameters:
 *   lower   An instance of the lower-half oneshot state structure.  This
 *           structure must have been previously initialized via a call to
 *           oneshot_initialize();
 *   ts      The location in which to return the maximum delay.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

static int rh850_systimer_max_delay(struct oneshot_lowerhalf_s *lower,
                                      struct timespec *ts)
{
  ts->tv_sec  = UINT32_MAX;
  ts->tv_nsec = NSEC_PER_SEC - 1;

  return 0;
}

/****************************************************************************
 * Name: rh850_systimer_start
 *
 * Description:
 *   Start the oneshot timer
 *
 * Input Parameters:
 *   lower   An instance of the lower-half oneshot state structure.  This
 *           structure must have been previously initialized via a call to
 *           oneshot_initialize();
 *   handler The function to call when when the oneshot timer expires.
 *   arg     An opaque argument that will accompany the callback.
 *   ts      Provides the duration of the one shot timer.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

static int rh850_systimer_start(struct oneshot_lowerhalf_s *lower,
                                  oneshot_callback_t callback, void *arg,
                                  const struct timespec *ts)
{
  struct rh850_systimer_lowerhalf_s *priv =
    (struct rh850_systimer_lowerhalf_s *)lower;
  uint32_t mtime = rh850_systimer_get_time(priv);

  priv->alarm = mtime + ts->tv_sec * priv->freq +
                ts->tv_nsec * priv->freq / NSEC_PER_SEC;
  if (priv->alarm < mtime)
    {
      priv->alarm = UINT32_MAX;
    }

  priv->callback = callback;
  priv->arg      = arg;

  rh850_systimer_set_timecmp(priv, priv->alarm);
  u2a16icum_ostm_start(priv->tbase);
  return 0;
}

/****************************************************************************
 * Name: rh850_systimer_cancel
 *
 * Description:
 *   Cancel the oneshot timer and return the time remaining on the timer.
 *
 *   NOTE: This function may execute at a high rate with no timer running (as
 *   when pre-emption is enabled and disabled).
 *
 * Input Parameters:
 *   lower   Caller allocated instance of the oneshot state structure.  This
 *           structure must have been previously initialized via a call to
 *           oneshot_initialize();
 *   ts      The location in which to return the time remaining on the
 *           oneshot timer.  A time of zero is returned if the timer is
 *           not running.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  A call to up_timer_cancel() when
 *   the timer is not active should also return success; a negated errno
 *   value is returned on any failure.
 *
 ****************************************************************************/

static int rh850_systimer_cancel(struct oneshot_lowerhalf_s *lower,
                                   struct timespec *ts)
{
  struct rh850_systimer_lowerhalf_s *priv =
    (struct rh850_systimer_lowerhalf_s *)lower;
  uint32_t mtime;

  u2a16icum_ostm_stop(priv);

  mtime = rh850_systimer_get_time(priv);
  if (priv->alarm > mtime)
    {
      uint32_t nsec = (priv->alarm - mtime) *
                      NSEC_PER_SEC / priv->freq;

      ts->tv_sec  = nsec / NSEC_PER_SEC;
      ts->tv_nsec = nsec % NSEC_PER_SEC;
    }
  else
    {
      ts->tv_sec  = 0;
      ts->tv_nsec = 0;
    }

  priv->alarm    = 0;
  priv->callback = NULL;
  priv->arg      = NULL;

  return 0;
}

/****************************************************************************
 * Name: rh850_systimer_current
 *
 * Description:
 *  Get the current time.
 *
 * Input Parameters:
 *   lower   Caller allocated instance of the oneshot state structure.  This
 *           structure must have been previously initialized via a call to
 *           oneshot_initialize();
 *   ts      The location in which to return the current time. A time of zero
 *           is returned for the initialization moment.
 *
 * Returned Value:
 *   Zero (OK) is returned on success, a negated errno value is returned on
 *   any failure.
 *
 ****************************************************************************/

static int rh850_systimer_current(struct oneshot_lowerhalf_s *lower,
                                    struct timespec *ts)
{
  struct rh850_systimer_lowerhalf_s *priv =
    (struct rh850_systimer_lowerhalf_s *)lower;
  uint32_t mtime = rh850_systimer_get_time(priv);
  uint32_t nsec = mtime / (priv->freq / USEC_PER_SEC) * NSEC_PER_USEC;

  ts->tv_sec  = nsec / NSEC_PER_SEC;
  ts->tv_nsec = nsec % NSEC_PER_SEC;

  return 0;
}

/****************************************************************************
 * Name: rh850_systimer_interrupt
 *
 * Description:
 *   This function is software interrupt handler to proceed
 *   the system timer interrupt.
 *
 ****************************************************************************/

static int rh850_systimer_interrupt(int irq, void *context, void *arg)
{
  nxsched_process_timer();
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rh850_systimer_initialize
 *
 * Description:
 *   This function is called during start-up to initialize
 *   the timer interrupt.
 *
 ****************************************************************************/

struct oneshot_lowerhalf_s *
rh850_systimer_initialize(volatile void *tbase, int irq, uint64_t freq)
{
  struct rh850_systimer_lowerhalf_s *priv = &g_systimer_lower;

  priv->tbase = tbase;
  priv->freq  = freq;

  u2a16icum_ostm_set_ctl(priv->tbase, OSTM_IE);

  /* 1ms */

  u2a16icum_ostm_set_compare(priv->tbase, 59999UL);

  irq_attach(irq, rh850_systimer_interrupt, priv);

  up_enable_irq(irq);

  up_prioritize_irq(irq, 6);

  u2a16icum_ostm_start(priv->tbase);

  up_trigger_irq(irq, 3);

  return (struct oneshot_lowerhalf_s *)priv;
}
