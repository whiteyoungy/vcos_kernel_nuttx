/****************************************************************************
 * arch/arm64/src/common/arm64_arch_timer.c
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
#include <debug.h>
#include <assert.h>
#include <stdio.h>

#include <nuttx/arch.h>
#include <arch/irq.h>
#include <arch/chip/chip.h>
#include <nuttx/spinlock.h>
#include <nuttx/timers/arch_alarm.h>

#include "arm64_arch.h"
#include "arm64_internal.h"
#include "arm64_gic.h"
#include "arm64_arch_timer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_ARM_TIMER_SECURE_IRQ         (GIC_PPI_INT_BASE + 13)
#define CONFIG_ARM_TIMER_NON_SECURE_IRQ     (GIC_PPI_INT_BASE + 14)
#define CONFIG_ARM_TIMER_VIRTUAL_IRQ        (GIC_PPI_INT_BASE + 11)
#define CONFIG_ARM_TIMER_HYP_IRQ            (GIC_PPI_INT_BASE + 10)

#define ARM_ARCH_TIMER_IRQ                  CONFIG_ARM_TIMER_VIRTUAL_IRQ
#define ARM_ARCH_TIMER_PRIO                 IRQ_DEFAULT_PRIORITY
#define ARM_ARCH_TIMER_FLAGS                IRQ_TYPE_LEVEL

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct arm64_oneshot_lowerhalf_s
{
  /* This is the part of the lower half driver that is visible to the upper-
   * half client of the driver.  This must be the first thing in this
   * structure so that pointers to struct oneshot_lowerhalf_s are cast
   * compatible to struct arm64_oneshot_lowerhalf_s and vice versa.
   */

  struct oneshot_lowerhalf_s lh;      /* Common lower-half driver fields */

  /* Private lower half data follows */

  void *arg;                          /* Argument that is passed to the handler */
  uint64_t cycle_per_tick;            /* cycle per tick */
  oneshot_callback_t callback;        /* Internal handler that receives callback */
  bool init[CONFIG_SMP_NCPUS];        /* True: timer is init */

  /* which cpu timer is running, -1 indicate timer stoppd */

  int running;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void arm64_arch_timer_set_compare(uint64_t value)
{
  write_sysreg(value, cntv_cval_el0);
}

static inline uint64_t arm64_arch_timer_get_compare(void)
{
  return read_sysreg(cntv_cval_el0);
}

static inline void arm64_arch_timer_enable(bool enable)
{
  uint64_t value;

  value = read_sysreg(cntv_ctl_el0);

  if (enable)
    {
      value |= CNTV_CTL_ENABLE_BIT;
    }
  else
    {
      value &= ~CNTV_CTL_ENABLE_BIT;
    }

  write_sysreg(value, cntv_ctl_el0);
}

static inline void arm64_arch_timer_set_irq_mask(bool mask)
{
  uint64_t value;

  value = read_sysreg(cntv_ctl_el0);

  if (mask)
    {
      value |= CNTV_CTL_IMASK_BIT;
    }
  else
    {
      value &= ~CNTV_CTL_IMASK_BIT;
    }

  write_sysreg(value, cntv_ctl_el0);
}

static inline uint64_t arm64_arch_timer_count(void)
{
  return read_sysreg(cntvct_el0);
}

static inline uint64_t arm64_arch_timer_get_cntfrq(void)
{
  return read_sysreg(cntfrq_el0);
}

/****************************************************************************
 * Name: arm64_arch_timer_compare_isr
 *
 * Description:
 *   Common timer interrupt callback.  When any oneshot timer interrupt
 *   expires, this function will be called.  It will forward the call to
 *   the next level up.
 *
 * Input Parameters:
 *   oneshot - The state associated with the expired timer
 *
 * Returned Value:
 *   Always returns OK
 *
 ****************************************************************************/

static int arm64_arch_timer_compare_isr(int irq, void *regs, void *arg)
{
  struct arm64_oneshot_lowerhalf_s *priv =
    (struct arm64_oneshot_lowerhalf_s *)arg;

  arm64_arch_timer_set_irq_mask(true);

  if (priv->callback && priv->running == this_cpu())
    {
      /* Then perform the callback */

      priv->callback(&priv->lh, priv->arg);
    }

  return OK;
}

/****************************************************************************
 * Name: arm64_time_max_delay
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

static int arm64_time_max_delay(struct oneshot_lowerhalf_s *lower,
                                  struct timespec *ts)
{
  DEBUGASSERT(ts != NULL);

  ts->tv_sec  = UINT64_MAX;
  ts->tv_nsec = NSEC_PER_SEC - 1;

  return 0;
}

/****************************************************************************
 * Name: arm64_time_start
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

static int arm64_time_start(struct oneshot_lowerhalf_s *lower,
                              oneshot_callback_t callback, void *arg,
                              const struct timespec *ts)
{
  struct arm64_oneshot_lowerhalf_s *priv =
    (struct arm64_oneshot_lowerhalf_s *)lower;
  uint64_t timercycles;
  uint64_t freq;

  DEBUGASSERT(priv != NULL && callback != NULL);

  /* Save the new handler and its argument */

  freq = arm64_arch_timer_get_cntfrq();
  timercycles = ts->tv_sec * freq + ts->tv_nsec * freq / NSEC_PER_SEC;

  priv->callback = callback;
  priv->arg = arg;

  /* Set the timeout */

  arm64_arch_timer_set_compare(arm64_arch_timer_count() + timercycles);

  /* Try to unmask the timer irq in timer controller
   * even through it's not ever masked.
   */

  arm64_arch_timer_set_irq_mask(false);

  return 0;
}

/****************************************************************************
 * Name: arm64_time_cancel
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

static int arm64_time_cancel(struct oneshot_lowerhalf_s *lower,
                               struct timespec *ts)
{
  struct arm64_oneshot_lowerhalf_s *priv =
    (struct arm64_oneshot_lowerhalf_s *)lower;

  DEBUGASSERT(priv != NULL && ts != NULL);

  /* Disable int */

  arm64_arch_timer_set_irq_mask(true);

  return 0;
}

/****************************************************************************
 * Name: arm64_time_current
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

static int arm64_time_current(struct oneshot_lowerhalf_s *lower,
                                struct timespec *ts)
{
  struct arm64_oneshot_lowerhalf_s *priv =
    (struct arm64_oneshot_lowerhalf_s *)lower;
  uint64_t current;
  uint64_t left;
  uint64_t freq;

  DEBUGASSERT(priv != NULL && ts != NULL);

  current = arm64_arch_timer_count();
  freq = arm64_arch_timer_get_cntfrq();

  ts->tv_sec  = current / freq;
  left = current - ts->tv_sec * freq;
  ts->tv_nsec = NSEC_PER_SEC * left / freq;

  return 0;
}

/****************************************************************************
 * Name: arm64_tick_max_delay
 *
 * Description:
 *   Determine the maximum delay of the one-shot timer (in microseconds)
 *
 * Input Parameters:
 *   lower   An instance of the lower-half oneshot state structure.  This
 *           structure must have been previously initialized via a call to
 *           oneshot_initialize();
 *   ticks   The location in which to return the maximum delay.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

static int arm64_tick_max_delay(struct oneshot_lowerhalf_s *lower,
                                clock_t *ticks)
{
  DEBUGASSERT(ticks != NULL);

  *ticks = (clock_t)UINT64_MAX;

  return OK;
}

/****************************************************************************
 * Name: arm64_tick_cancel
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
 *   ticks   The location in which to return the time remaining on the
 *           oneshot timer.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  A call to up_timer_cancel() when
 *   the timer is not active should also return success; a negated errno
 *   value is returned on any failure.
 *
 ****************************************************************************/

static int arm64_tick_cancel(struct oneshot_lowerhalf_s *lower,
                             clock_t *ticks)
{
  struct arm64_oneshot_lowerhalf_s *priv =
    (struct arm64_oneshot_lowerhalf_s *)lower;

  DEBUGASSERT(priv != NULL && ticks != NULL);

  /* Disable int */

  priv->running = -1;
  arm64_arch_timer_set_irq_mask(true);

  return OK;
}

/****************************************************************************
 * Name: arm64_tick_start
 *
 * Description:
 *   Start the oneshot timer
 *
 * Input Parameters:
 *   lower    An instance of the lower-half oneshot state structure.  This
 *            structure must have been previously initialized via a call to
 *            oneshot_initialize();
 *   handler  The function to call when when the oneshot timer expires.
 *   arg      An opaque argument that will accompany the callback.
 *   ticks    Provides the duration of the one shot timer.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

static int arm64_tick_start(struct oneshot_lowerhalf_s *lower,
                            oneshot_callback_t callback, void *arg,
                            clock_t ticks)
{
  struct arm64_oneshot_lowerhalf_s *priv =
    (struct arm64_oneshot_lowerhalf_s *)lower;
  uint64_t next_cycle;

  DEBUGASSERT(priv != NULL && callback != NULL);

  /* Save the new handler and its argument */

  priv->callback = callback;
  priv->arg = arg;

  if (!priv->init[this_cpu()])
    {
      /* Enable int */

      up_enable_irq(ARM_ARCH_TIMER_IRQ);

      /* Start timer */

      arm64_arch_timer_enable(true);
      priv->init[this_cpu()] = true;
    }

  priv->running = this_cpu();

  next_cycle =
    arm64_arch_timer_count() / priv->cycle_per_tick * priv->cycle_per_tick +
    ticks * priv->cycle_per_tick;

  arm64_arch_timer_set_compare(next_cycle);
  arm64_arch_timer_set_irq_mask(false);

  return OK;
}

/****************************************************************************
 * Name: arm64_tick_current
 *
 * Description:
 *  Get the current time.
 *
 * Input Parameters:
 *   lower   Caller allocated instance of the oneshot state structure.  This
 *           structure must have been previously initialized via a call to
 *           oneshot_initialize();
 *   ticks   The location in which to return the current time.
 *
 * Returned Value:
 *   Zero (OK) is returned on success, a negated errno value is returned on
 *   any failure.
 *
 ****************************************************************************/

static int arm64_tick_current(struct oneshot_lowerhalf_s *lower,
                              clock_t *ticks)
{
  struct arm64_oneshot_lowerhalf_s *priv =
    (struct arm64_oneshot_lowerhalf_s *)lower;

  DEBUGASSERT(ticks != NULL);

  *ticks = arm64_arch_timer_count() / priv->cycle_per_tick;

  return OK;
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct oneshot_operations_s g_oneshot_ops =
{
  .max_delay      = arm64_time_max_delay,
  .start          = arm64_time_start,
  .cancel         = arm64_time_cancel,
  .current        = arm64_time_current,
  .tick_start     = arm64_tick_start,
  .tick_current   = arm64_tick_current,
  .tick_max_delay = arm64_tick_max_delay,
  .tick_cancel    = arm64_tick_cancel,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: oneshot_initialize
 *
 * Description:
 *   Initialize the oneshot timer and return a oneshot lower half driver
 *   instance.
 *
 * Returned Value:
 *   On success, a non-NULL instance of the oneshot lower-half driver is
 *   returned.  NULL is return on any failure.
 *
 ****************************************************************************/

struct oneshot_lowerhalf_s *arm64_oneshot_initialize(void)
{
  struct arm64_oneshot_lowerhalf_s *priv;

  tmrinfo("oneshot_initialize\n");

  /* Allocate an instance of the lower half driver */

  priv = (struct arm64_oneshot_lowerhalf_s *)
    kmm_zalloc(sizeof(struct arm64_oneshot_lowerhalf_s));

  if (priv == NULL)
    {
      tmrerr("ERROR: Failed to initialized state structure\n");

      return NULL;
    }

  /* Initialize the lower-half driver structure */

  priv->lh.ops = &g_oneshot_ops;
  priv->running = -1;
  priv->cycle_per_tick = arm64_arch_timer_get_cntfrq() / TICK_PER_SEC;
  tmrinfo("cycle_per_tick %" PRIu64 "\n", priv->cycle_per_tick);

  /* Attach handler */

  irq_attach(ARM_ARCH_TIMER_IRQ,
             arm64_arch_timer_compare_isr, priv);

  tmrinfo("oneshot_initialize ok %p \n", &priv->lh);

  return &priv->lh;
}
