/****************************************************************************
 * arch/rh850/src/rh850g3k/rh850g3k_irq.c
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
#include <assert.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <arch/board/board.h>

#include "rh850_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   Enable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_enable_irq(int irq)
{
  volatile uint16_t *src;

  src = (uint16_t *)(((irq < 32)? INTC1_BASE : INTC2_BASE) +
                          ((irq < 32)? irq * 2 : (irq - 32) * 2));

  /* clear interrupt flag, direct vector method */

  *src = (0u << 12u) | (0u << 7u) | (0u << 6u) | (0u << 0u);
}

/****************************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_disable_irq(int irq)
{
    volatile uint16_t *src;

    src = (uint16_t *)(((irq < 32)? INTC1_BASE : INTC2_BASE) +
                           ((irq < 32)? irq * 2 : (irq - 32) * 2));

    (*src) |= (1u << 7u);
}

/****************************************************************************
 * Name: up_prioritize_irq
 *
 * Description:
 *   Set the priority of an IRQ.
 *
 *   Since this API is not supported on all architectures, it should be
 *   avoided in common implementations where possible.
 *
 ****************************************************************************/

#ifdef CONFIG_ARCH_IRQPRIO
int up_prioritize_irq(int irq, int priority)
{
  volatile uint16_t *src;

  src = (uint16_t *)(((irq < 32)? INTC1_BASE : INTC2_BASE) +
                          ((irq < 32)? irq * 2 : (irq - 32) * 2));

  (*src) &= (~0x7u);

  if (priority > 7)
    {
      priority = 7;
    }

  (*src) |= ((uint16_t)priority << 0u);
}
#endif

void up_clear_irq(int irq)
{
  volatile uint16_t *src;
  src = (uint16_t *)(((irq < 32)? INTC1_BASE : INTC2_BASE) +
                          ((irq < 32)? irq * 2 : (irq - 32) * 2));
  (*src) &= ~(1 << 12);
}

#ifdef CONFIG_ARCH_HAVE_IRQTRIGGER

void up_trigger_irq(int irq, cpu_set_t cpuset)
{
  (void) cpuset;

  volatile uint16_t *src;
  src = (uint16_t *)(((irq < 32)? INTC1_BASE : INTC2_BASE) +
                      ((irq < 32)? irq * 2 : (irq - 32) * 2));

  (*src) |= (1u << 12u);
}

#endif