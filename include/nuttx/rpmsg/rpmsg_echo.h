/****************************************************************************
 * include/nuttx/rpmsg/rpmsg_echo.h
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

#ifndef __INCLUDE_NUTTX_RPMSG_RPMSG_ECHO_H
#define __INCLUDE_NUTTX_RPMSG_RPMSG_ECHO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_RPMSG_ECHO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RPMSG_MAX_ECHO_MSG_LEN  64

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* used for ioctl RPMSGIOC_ECHO */

struct rpmsg_echo_s
{
  char   msg[RPMSG_MAX_ECHO_MSG_LEN];
  size_t len;
};

#endif /* CONFIG_RPMSG_ECHO */
#endif /* __INCLUDE_NUTTX_RPMSG_RPMSG_ECHO_H */
