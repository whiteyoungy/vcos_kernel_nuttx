/****************************************************************************
 * sched/irq/irq_spinlock.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/spinlock.h>

#include <assert.h>
#include <sys/types.h>
#include <arch/irq.h>

#include "sched/sched.h"

#if defined(CONFIG_SPINLOCK)

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Used for access control */

#define MMAP_SHARE_VAR_INIT_SECTION         __attribute__((section(".data.Share.VAR_INIT")))
MMAP_SHARE_VAR_INIT_SECTION volatile spinlock_t g_irq_spin = SP_UNLOCKED;

/* Handles nested calls to spin_lock_irqsave and spin_unlock_irqrestore */

#undef g_irq_spin_count
volatile __percpu_bss uint8_t g_irq_spin_count[CONFIG_SMP_NCPUS];

#ifdef CONFIG_RW_SPINLOCK
/* Used for access control */

static volatile rwlock_t g_irq_rwspin = RW_SP_UNLOCKED;

/* Handles nested calls to write_lock_irqsave and write_unlock_irqrestore */

static volatile uint8_t g_irq_rwspin_count[CONFIG_NR_CPUS];

#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_RW_SPINLOCK

/****************************************************************************
 * Name: read_lock_irqsave
 *
 * Description:
 *   If SMP is enabled:
 *     If the 'lock' argument is not specified (i.e. NULL), disable local
 *     interrupts and take the global read write spinlock (g_irq_rwspin)
 *     and increase g_irq_rwspin.
 *
 *     If the 'lock' argument is specified,
 *     disable local interrupts and take the lock spinlock and return
 *     the interrupt state.
 *
 *     NOTE: This API is very simple to protect data (e.g. H/W register
 *     or internal data structure) in SMP mode. Do not use this API
 *     with kernel APIs which suspend a caller thread. (e.g. nxsem_wait)
 *
 *   If SMP is not enabled:
 *     This function is equivalent to up_irq_save().
 *
 * Input Parameters:
 *   lock - Caller specific spinlock. If specified NULL, g_irq_spin is used
 *          and can be nested. Otherwise, nested call for the same lock
 *          would cause a deadlock
 *
 * Returned Value:
 *   An opaque, architecture-specific value that represents the state of
 *   the interrupts prior to the call to write_lock_irqsave(lock);
 *
 ****************************************************************************/

irqstate_t read_lock_irqsave(FAR rwlock_t *lock)
{
  irqstate_t ret;
  ret = up_irq_save();

  if (NULL == lock)
    {
      read_lock(&g_irq_rwspin);
    }
  else
    {
      read_lock(lock);
    }

  return ret;
}

/****************************************************************************
 * Name: read_unlock_irqrestore
 *
 * Description:
 *   If SMP is enabled:
 *     If the argument lock is not specified (i.e. NULL),
 *     decrement the call counter (g_irq_rwspin) and restore the interrupt
 *     state as it was prior to the previous call to read_lock_irqsave(NULL).
 *
 *     If the argument lock is specified, release the lock and
 *     restore the interrupt state as it was prior to the previous call to
 *     read_lock_irqsave(lock).
 *
 *   If SMP is not enabled:
 *     This function is equivalent to up_irq_restore().
 *
 * Input Parameters:
 *   lock - Caller specific spinlock. If specified NULL, g_irq_spin is used.
 *
 *   flags - The architecture-specific value that represents the state of
 *           the interrupts prior to the call to read_lock_irqsave(lock);
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void read_unlock_irqrestore(rwlock_t *lock, irqstate_t flags)
{
  if (NULL == lock)
    {
      read_unlock(&g_irq_rwspin);
    }
  else
    {
      read_unlock(lock);
    }

  up_irq_restore(flags);
}

/****************************************************************************
 * Name: write_lock_irqsave
 *
 * Description:
 *   If SMP is enabled:
 *     If the argument lock is not specified (i.e. NULL),
 *     disable local interrupts and take the global spinlock (g_irq_rwspin)
 *     if the call counter (g_irq_rwspin_count[cpu]) equals to 0. Then
 *     the counter on the CPU is incremented to allow nested calls and return
 *     the interrupt state.
 *
 *     If the argument lock is specified,
 *     disable local interrupts and take the lock spinlock and return
 *     the interrupt state.
 *
 *     NOTE: This API is very simple to protect data (e.g. H/W register
 *     or internal data structure) in SMP mode. But do not use this API
 *     with kernel APIs which suspend a caller thread. (e.g. nxsem_wait)
 *
 *   If SMP is not enabled:
 *     This function is equivalent to up_irq_save().
 *
 * Input Parameters:
 *   lock - Caller specific spinlock. If specified NULL, g_irq_spin is used
 *          and can be nested. Otherwise, nested call for the same lock
 *          would cause a deadlock
 *
 * Returned Value:
 *   An opaque, architecture-specific value that represents the state of
 *   the interrupts prior to the call to write_lock_irqsave(lock);
 *
 ****************************************************************************/

irqstate_t write_lock_irqsave(rwlock_t *lock)
{
  irqstate_t ret;
  ret = up_irq_save();

  if (NULL == lock)
    {
      int me = this_cpu();
      if (0 == g_irq_rwspin_count[me])
        {
          write_lock(&g_irq_rwspin);
        }

      g_irq_rwspin_count[me]++;
      DEBUGASSERT(0 != g_irq_rwspin_count[me]);
    }
  else
    {
      write_lock(lock);
    }

  return ret;
}

/****************************************************************************
 * Name: write_unlock_irqrestore
 *
 * Description:
 *   If SMP is enabled:
 *     If the argument lock is not specified (i.e. NULL),
 *     decrement the call counter (g_irq_rwspin_count[cpu]) and if it
 *     decrements to zero then release the spinlock (g_irq_rwspin) and
 *     restore the interrupt state as it was prior to the previous call to
 *     write_lock_irqsave(NULL).
 *
 *     If the argument lock is specified, release the lock and
 *     restore the interrupt state as it was prior to the previous call to
 *     write_lock_irqsave(lock).
 *
 *   If SMP is not enabled:
 *     This function is equivalent to up_irq_restore().
 *
 * Input Parameters:
 *   lock - Caller specific spinlock. If specified NULL, g_irq_spin is used.
 *
 *   flags - The architecture-specific value that represents the state of
 *           the interrupts prior to the call to write_lock_irqsave(lock);
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void write_unlock_irqrestore(rwlock_t *lock, irqstate_t flags)
{
  if (NULL == lock)
    {
      int me = this_cpu();
      DEBUGASSERT(0 < g_irq_rwspin_count[me]);
      g_irq_rwspin_count[me]--;

      if (0 == g_irq_rwspin_count[me])
        {
          write_unlock(&g_irq_rwspin);
        }
    }
  else
    {
      write_unlock(lock);
    }

  up_irq_restore(flags);
}
#endif /* CONFIG_RW_SPINLOCK */
#endif /* CONFIG_SPINLOCK */
