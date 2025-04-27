/****************************************************************************
 * drivers/timers/wdt_notifier.c
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

#include <nuttx/arch.h>
#include <nuttx/notifier.h>

#include <sys/types.h>

#include <nuttx/wdt_notifier.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static __percpu_data ATOMIC_NOTIFIER_HEAD(g_wdt_notifier_list);
#define g_wdt_notifier_list this_cpu_var(g_wdt_notifier_list)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  wdt_notifier_chain_register
 *
 * Description:
 *   Add notifier to the wdt notifier chain
 *
 * Input Parameters:
 *    nb - New entry in notifier chain
 *
 ****************************************************************************/

void wdt_notifier_chain_register(FAR struct notifier_block *nb)
{
  atomic_notifier_chain_register(&g_wdt_notifier_list, nb);
}

/****************************************************************************
 * Name:  wdt_notifier_chain_unregister
 *
 * Description:
 *   Remove notifier from the wdt notifier chain
 *
 * Input Parameters:
 *    nb - Entry to remove from notifier chain
 *
 ****************************************************************************/

void wdt_notifier_chain_unregister(FAR struct notifier_block *nb)
{
  atomic_notifier_chain_unregister(&g_wdt_notifier_list, nb);
}

/****************************************************************************
 * Name:  wdt_notifier_call_chain
 *
 * Description:
 *   Call functions in the wdt notifier chain.
 *
 * Input Parameters:
 *    action - Value passed unmodified to notifier function
 *    data   - Pointer passed unmodified to notifier function
 *
 ****************************************************************************/

void wdt_notifier_call_chain(unsigned long action, FAR void *data)
{
  atomic_notifier_call_chain(&g_wdt_notifier_list, action, data);
}

