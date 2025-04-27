/****************************************************************************
 * arch/tricore/src/tc3xx/tc3xx_timerisr.c
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

#include "chip.h"

#include "IfxStm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SCU_FREQUENCY 100000000UL
#define STM0_SRC_NUM  768
#define STM1_SRC_NUM  776
#define STM2_SRC_NUM  784
#define STM3_SRC_NUM  792
#define STM4_SRC_NUM  800
#define STM5_SRC_NUM  808


/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/** \brief Module address and index map */

struct systimer_map_s
{
  volatile void *timer; /**< \brief Module address */
  int            irq;   /**< \brief Module irq */
};


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_timer_initialize
 *
 * Description:
 *   This function is called during start-up to initialize
 *   the timer interrupt.
 *
 ****************************************************************************/

void up_timer_initialize(void)
{
  struct oneshot_lowerhalf_s *lower;
  int cpu = up_cpu_index();

  struct systimer_map_s stmaps[] =
  {
    {&MODULE_STM0, STM0_SRC_NUM},
    {&MODULE_STM1, STM1_SRC_NUM},
    {&MODULE_STM2, STM2_SRC_NUM},
    {&MODULE_STM3, STM3_SRC_NUM},
    {&MODULE_STM4, STM4_SRC_NUM},
    {&MODULE_STM5, STM5_SRC_NUM}
  };

  lower = tc3xx_systimer_initialize(stmaps[cpu].timer,
    stmaps[cpu].irq, SCU_FREQUENCY);

  DEBUGASSERT(lower != NULL);

  up_alarm_set_lowerhalf(lower);
}
