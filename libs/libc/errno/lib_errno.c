/****************************************************************************
 * libs/libc/errno/lib_errno.c
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

#include <nuttx/tls.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static __percpu_bss int g_errno;
#define g_errno this_cpu_var(g_errno)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __errno
 *
 * Description:
 *   Return a pointer to the thread specific errno.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   A pointer to the per-thread errno variable.
 *
 * Assumptions:
 *
 ****************************************************************************/

FAR int *__errno(void)
{
  /* Get the TLS tls_info_s structure instance for this thread */

  FAR struct tls_info_s *tlsinfo = tls_get_info();

  /* And return the return reference to the error number */

  return tlsinfo ? &tlsinfo->tl_errno : &g_errno;
}
