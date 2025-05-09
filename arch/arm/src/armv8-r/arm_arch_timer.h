/****************************************************************************
 * arch/arm/src/armv8-r/arm_arch_timer.h
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

#ifndef __ARCH_ARM_SRC_ARMV8_R_ARM_ARCH_TIMER_H
#define __ARCH_ARM_SRC_ARMV8_R_ARM_ARCH_TIMER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "arm_gic.h"
#include "arm_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_ARM_TIMER_SECURE_IRQ         (GIC_PPI_INT_BASE + 13)
#define CONFIG_ARM_TIMER_NON_SECURE_IRQ     (GIC_PPI_INT_BASE + 14)
#define CONFIG_ARM_TIMER_VIRTUAL_IRQ        (GIC_PPI_INT_BASE + 11)
#define CONFIG_ARM_TIMER_HYP_IRQ            (GIC_PPI_INT_BASE + 10)

#if defined(CONFIG_ARMV8R_GTIMER_VIRTUAL)
#define ARM_ARCH_TIMER_IRQ    CONFIG_ARM_TIMER_VIRTUAL_IRQ
#elif defined(CONFIG_ARMV8R_GTIMER_PHYSICAL)
#define ARM_ARCH_TIMER_IRQ    CONFIG_ARM_TIMER_NON_SECURE_IRQ
#elif defined(CONFIG_ARMV8R_GTIMER_HYPERVISOR)
#define ARM_ARCH_TIMER_IRQ    CONFIG_ARM_TIMER_HYP_IRQ
#endif

#define ARM_ARCH_TIMER_PRIO   IRQ_DEFAULT_PRIORITY
#define ARM_ARCH_TIMER_FLAGS  IRQ_TYPE_LEVEL

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
#ifdef CONFIG_SMP
void arm_arch_timer_secondary_init(void);
#endif

#endif /* __ARCH_ARM_SRC_ARMV8_R_ARM_ARCH_TIMER_H */
