/****************************************************************************
 * include/nuttx/wdt_notifier.h
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

#ifndef __INCLUDE_NUTTX_WDT_NOTIFIER_H
#define __INCLUDE_NUTTX_WDT_NOTIFIER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/notifier.h>
#include <nuttx/sched.h>

#include <sys/types.h>

#ifdef CONFIG_WATCHDOG_TIMEOUT_NOTIFIER

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum wdt_type_e
{
  WDT_KEEPALIVE_BY_ONESHOT         =  0,
  WDT_KEEPALIVE_BY_TIMER           =  1,
  WDT_KEEPALIVE_BY_WDOG            =  2,
  WDT_KEEPALIVE_BY_WORKER          =  3,
  WDT_KEEPALIVE_BY_CAPTURE         =  4,
  WDT_KEEPALIVE_BY_IDLE            =  5
};

/****************************************************************************
 * Public Function
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

void wdt_notifier_chain_register(FAR struct notifier_block *nb);

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

void wdt_notifier_chain_unregister(FAR struct notifier_block *nb);

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

void wdt_notifier_call_chain(unsigned long action, FAR void *data);

#endif /* CONFIG_WATCHDOG_TIMEOUT_NOTIFIER */
#endif /* __INCLUDE_NUTTX_WDT_NOTIFIER_H */
