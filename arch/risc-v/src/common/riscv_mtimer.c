/****************************************************************************
 * arch/risc-v/src/common/riscv_mtimer.c
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

#include <arch/barriers.h>

#include "riscv_internal.h"
#include "riscv_mtimer.h"
#include "riscv_sbi.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure provides the private representation of the "lower-half"
 * driver state structure.  This structure must be cast-compatible with the
 * oneshot_lowerhalf_s structure.
 */

struct riscv_mtimer_lowerhalf_s
{
  struct oneshot_lowerhalf_s lower;
  uintreg_t                  mtime;
  uintreg_t                  mtimecmp;
  uint64_t                   freq;
  uint64_t                   alarm;
  oneshot_callback_t         callback;
  void                       *arg;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int riscv_mtimer_max_delay(struct oneshot_lowerhalf_s *lower,
                                  struct timespec *ts);
static int riscv_mtimer_start(struct oneshot_lowerhalf_s *lower,
                              oneshot_callback_t callback, void *arg,
                              const struct timespec *ts);
static int riscv_mtimer_cancel(struct oneshot_lowerhalf_s *lower,
                               struct timespec *ts);
static int riscv_mtimer_current(struct oneshot_lowerhalf_s *lower,
                                struct timespec *ts);

static int riscv_tick_max_delay(struct oneshot_lowerhalf_s *lower,
                                  clock_t *ticks);
static int riscv_tick_start(struct oneshot_lowerhalf_s *lower,
                              oneshot_callback_t callback, void *arg,
                              clock_t ticks);
static int riscv_tick_cancel(struct oneshot_lowerhalf_s *lower,
                               clock_t *ticks);
static int riscv_tick_current(struct oneshot_lowerhalf_s *lower,
                                clock_t *ticks);
/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct oneshot_operations_s g_riscv_mtimer_ops =
{
  .max_delay      = riscv_mtimer_max_delay,
  .start          = riscv_mtimer_start,
  .cancel         = riscv_mtimer_cancel,
  .current        = riscv_mtimer_current,
  .tick_start     = riscv_tick_start,
  .tick_current   = riscv_tick_current,
  .tick_max_delay = riscv_tick_max_delay,
  .tick_cancel    = riscv_tick_cancel,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifndef CONFIG_ARCH_USE_S_MODE
static uint64_t riscv_mtimer_get_mtime(struct riscv_mtimer_lowerhalf_s *priv)
{
#if CONFIG_ARCH_RV_MMIO_BITS == 64
  /* priv->mtime is -1, means this SoC:
   * 1. does NOT support 64bit/DWORD write for the mtimer compare value regs,
   * 2. has NO memory mapped regs which hold the value of mtimer counter,
   *    it could be read from the CSR "time".
   */

  return -1 == priv->mtime ? READ_CSR(CSR_TIME) : getreg64(priv->mtime);
#else
  uint32_t hi;
  uint32_t lo;

  do
    {
      hi = getreg32(priv->mtime + 4);
      lo = getreg32(priv->mtime);
    }
  while (getreg32(priv->mtime + 4) != hi);

  return ((uint64_t)hi << 32) | lo;
#endif
}

static void riscv_mtimer_set_mtimecmp(struct riscv_mtimer_lowerhalf_s *priv,
                                      uint64_t value)
{
#if CONFIG_ARCH_RV_MMIO_BITS == 64
  if (-1 != priv->mtime)
    {
      putreg64(value, priv->mtimecmp);
    }
  else
#endif
    {
      putreg32(UINT32_MAX, priv->mtimecmp + 4);
      putreg32(value, priv->mtimecmp);
      putreg32(value >> 32, priv->mtimecmp + 4);
    }

  /* Make sure it sticks */

  __MB();
}
#else

#ifdef CONFIG_ARCH_RV_EXT_SSTC
static inline void riscv_write_stime(uint64_t value)
{
#ifdef CONFIG_ARCH_RV64
  WRITE_CSR(CSR_STIMECMP, value);
#else
  WRITE_CSR(CSR_STIMECMP, (uint32_t)value);
  WRITE_CSR(CSR_STIMECMPH, (uint32_t)(value >> 32));
#endif
}
#endif

static uint64_t riscv_mtimer_get_mtime(struct riscv_mtimer_lowerhalf_s *priv)
{
  UNUSED(priv);
  return riscv_sbi_get_time();
}

static void riscv_mtimer_set_mtimecmp(struct riscv_mtimer_lowerhalf_s *priv,
                                      uint64_t value)
{
  UNUSED(priv);
#ifndef CONFIG_ARCH_RV_EXT_SSTC
  riscv_sbi_set_timer(value);
#else
  riscv_write_stime(value);
#endif
}
#endif

/****************************************************************************
 * Name: riscv_mtimer_max_delay
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

static int riscv_mtimer_max_delay(struct oneshot_lowerhalf_s *lower,
                                  struct timespec *ts)
{
  DEBUGASSERT(ts != NULL);

  ts->tv_sec  = UINT64_MAX;
  ts->tv_nsec = NSEC_PER_SEC - 1;

  return 0;
}

/****************************************************************************
 * Name: riscv_mtimer_start
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

static int riscv_mtimer_start(struct oneshot_lowerhalf_s *lower,
                              oneshot_callback_t callback, void *arg,
                              const struct timespec *ts)
{
  struct riscv_mtimer_lowerhalf_s *priv =
    (struct riscv_mtimer_lowerhalf_s *)lower;
  irqstate_t flags;
  uint64_t mtime;
  uint64_t alarm;

  flags = up_irq_save();

  mtime = riscv_mtimer_get_mtime(priv);
  alarm = mtime + ts->tv_sec * priv->freq +
          ts->tv_nsec * priv->freq / NSEC_PER_SEC;

  priv->alarm    = alarm;
  priv->callback = callback;
  priv->arg      = arg;

  riscv_mtimer_set_mtimecmp(priv, priv->alarm);
  up_irq_restore(flags);

  return 0;
}

/****************************************************************************
 * Name: riscv_mtimer_cancel
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

static int riscv_mtimer_cancel(struct oneshot_lowerhalf_s *lower,
                               struct timespec *ts)
{
  struct riscv_mtimer_lowerhalf_s *priv =
    (struct riscv_mtimer_lowerhalf_s *)lower;
  uint64_t mtime;
  uint64_t alarm;
  uint64_t nsec;
  irqstate_t flags;

  flags = up_irq_save();

  alarm = priv->alarm;

  mtime = riscv_mtimer_get_mtime(priv);

  riscv_mtimer_set_mtimecmp(priv, mtime + UINT64_MAX);

  nsec = (alarm - mtime) * NSEC_PER_SEC / priv->freq;
  ts->tv_sec  = nsec / NSEC_PER_SEC;
  ts->tv_nsec = nsec % NSEC_PER_SEC;

  priv->alarm    = 0;
  priv->callback = NULL;
  priv->arg      = NULL;

  up_irq_restore(flags);

  return 0;
}

/****************************************************************************
 * Name: riscv_mtimer_current
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

static int riscv_mtimer_current(struct oneshot_lowerhalf_s *lower,
                                struct timespec *ts)
{
  struct riscv_mtimer_lowerhalf_s *priv =
    (struct riscv_mtimer_lowerhalf_s *)lower;
  uint64_t mtime = riscv_mtimer_get_mtime(priv);
  uint64_t left;

  ts->tv_sec  = mtime / priv->freq;
  left        = mtime - ts->tv_sec * priv->freq;
  ts->tv_nsec = NSEC_PER_SEC * left / priv->freq;

  return 0;
}

/****************************************************************************
 * Name: riscv_tick_max_delay
 *
 * Description:
 *   Determine the maximum delay of the one-shot timer
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

static int riscv_tick_max_delay(struct oneshot_lowerhalf_s *lower,
                                  clock_t *ticks)
{
  DEBUGASSERT(ticks != NULL);

  *ticks = (clock_t)UINT64_MAX;

  return 0;
}

/****************************************************************************
 * Name: riscv_tick_start
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
 *   ticks    Provides the duration of the one shot timer.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned
 *   on failure.
 *
 ****************************************************************************/

static int riscv_tick_start(struct oneshot_lowerhalf_s *lower,
                              oneshot_callback_t callback, void *arg,
                              clock_t ticks)
{
  struct riscv_mtimer_lowerhalf_s *priv =
    (struct riscv_mtimer_lowerhalf_s *)lower;

  DEBUGASSERT(priv != NULL && callback != NULL);

  /* Save the new handler and its argument */

  priv->alarm    = riscv_mtimer_get_mtime(priv) +
                   ticks * priv->freq / TICK_PER_SEC;
  priv->callback = callback;
  priv->arg      = arg;

  /* Set the timeout */

  riscv_mtimer_set_mtimecmp(priv, priv->alarm);

  return 0;
}

/****************************************************************************
 * Name: riscv_tick_cancel
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

static int riscv_tick_cancel(struct oneshot_lowerhalf_s *lower,
                               clock_t *ticks)
{
  struct riscv_mtimer_lowerhalf_s *priv =
    (struct riscv_mtimer_lowerhalf_s *)lower;

  DEBUGASSERT(priv != NULL && ticks != NULL);

  /* set max timer */

  priv->alarm = UINT64_MAX;
  riscv_mtimer_set_mtimecmp(priv, priv->alarm);

  return 0;
}

/****************************************************************************
 * Name: riscv_tick_current
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

static int riscv_tick_current(struct oneshot_lowerhalf_s *lower,
                                clock_t *ticks)
{
  struct riscv_mtimer_lowerhalf_s *priv =
    (struct riscv_mtimer_lowerhalf_s *)lower;

  DEBUGASSERT(ticks != NULL);

  *ticks = riscv_mtimer_get_mtime(priv) / (priv->freq / TICK_PER_SEC);

  return 0;
}

static int riscv_mtimer_interrupt(int irq, void *context, void *arg)
{
  struct riscv_mtimer_lowerhalf_s *priv =
    (struct riscv_mtimer_lowerhalf_s *)arg;

  if (priv->callback != NULL)
    {
      /* Then perform the callback */

      priv->callback(&priv->lower, priv->arg);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct oneshot_lowerhalf_s *
riscv_mtimer_initialize(uintreg_t mtime, uintreg_t mtimecmp,
                        int irq, uint64_t freq)
{
  struct riscv_mtimer_lowerhalf_s *priv;

  priv = kmm_zalloc(sizeof(*priv));
  if (priv != NULL)
    {
      priv->lower.ops = &g_riscv_mtimer_ops;
      priv->mtime     = mtime;
      priv->mtimecmp  = mtimecmp;
      priv->freq      = freq;
      priv->alarm     = UINT64_MAX;

      riscv_mtimer_set_mtimecmp(priv, priv->alarm);
      irq_attach(irq, riscv_mtimer_interrupt, priv);
      up_enable_irq(irq);
    }

  return (struct oneshot_lowerhalf_s *)priv;
}
