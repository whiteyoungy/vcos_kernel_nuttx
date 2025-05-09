/****************************************************************************
 * libs/libc/time/lib_gmtime.c
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
#include <nuttx/irq.h>
#include <time.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/clock.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static __percpu_bss struct tm g_gmtime;
#define g_gmtime this_cpu_var(g_gmtime)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  gmtime
 *
 * Description:
 *  Similar to gmtime_r, but not thread-safe
 *
 ****************************************************************************/

FAR struct tm *gmtime(FAR const time_t *timep)
{
  return gmtime_r(timep, &g_gmtime);
}

FAR struct tm *localtime(FAR const time_t *timep)
{
  return gmtime(timep);
}
