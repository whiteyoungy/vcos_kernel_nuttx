/****************************************************************************
 * arch/tricore/src/tc3xx/tc3xx_irq.c
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

#include "tricore_internal.h"

#include "IfxSrc.h"
#include "IfxCpu.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

 #define INT_SRC_SIZE  4

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_disable_irq(int irq)
{
  volatile Ifx_SRC_SRCR *src = NULL;
  src = (Ifx_SRC_SRCR *)((unsigned int)(&SRC_CPU_CPU0_SB) + irq);
  if (src != NULL)
    {
      IfxSrc_disable(src);
    }
}

/****************************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   Enable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_enable_irq(int irq)
{
  volatile Ifx_SRC_SRCR *src = NULL;
  int cpu = up_cpu_index();
  src = (Ifx_SRC_SRCR *)((unsigned int)(&SRC_CPU_CPU0_SB) + irq);
  if (src != NULL)
    {
      IfxSrc_init(src, (cpu == 0) ? 0 : (cpu + 1), irq / INT_SRC_SIZE);
      IfxSrc_enable(src);
    }
}

/****************************************************************************
 * Name: up_affinity_irq
 *
 * Description:
 *   Set an IRQ affinity by software.
 *
 ****************************************************************************/

#ifdef CONFIG_BMP
 void up_affinity_irq(int irq, cpu_set_t cpuset)
 {
   volatile Ifx_SRC_SRCR *src = NULL;
   src = (Ifx_SRC_SRCR *)((unsigned int)(&SRC_CPU_CPU0_SB) + irq);
   int i;

   for (i = 0; i < CONFIG_NR_CPUS; i++)
     {
       if (CPU_ISSET(i, &cpuset))
         {
           IfxSrc_init(src, (i == 0) ? 0 : (i + 1), irq / INT_SRC_SIZE);
           IfxSrc_enable(src);
         }
     }
 }
#endif

#ifdef CONFIG_ARCH_HAVE_IRQTRIGGER

/****************************************************************************
 * Name: up_trigger_irq
 *
 * Description:
 *   Trigger an IRQ by software.
 *
 ****************************************************************************/

void up_trigger_irq(int irq, cpu_set_t cpuset)
{
  (void) cpuset;

  volatile Ifx_SRC_SRCR *src = NULL;
  src = (Ifx_SRC_SRCR *)((unsigned int)(&SRC_CPU_CPU0_SB) + irq);

  IfxSrc_setRequest(src);
}

#endif