/****************************************************************************
 * drivers/rpmsg/rpmsg_echo.c
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
#include <nuttx/arch.h>
#include <nuttx/lib/lib.h>
#include <nuttx/signal.h>

#include <inttypes.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>
#include <syslog.h>

#include "rpmsg_echo.h"

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

#define RPMSG_ECHO_EPT_NAME         "rpmsg-echo"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int rpmsg_echo_ept_cb(FAR struct rpmsg_endpoint *ept,
                             FAR void *data, size_t len, uint32_t src,
                             FAR void *priv)
{
  syslog(LOG_EMERG,
         "[RPMSG_ECHO] received data[%s] len[%zu] from src[%" PRIu32 "]\n",
         (char *)data, len, src);

  return rpmsg_send(ept, (char *)data, len);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int rpmsg_echo(FAR struct rpmsg_endpoint *ept,
               FAR const struct rpmsg_echo_s *echo)
{
  int ret = 0;

  if (!ept || !echo)
    {
      return -EINVAL;
    }

  ret = rpmsg_send(ept, echo->msg, echo->len);

  if (ret < 0)
    {
      syslog(LOG_EMERG, "Call remote failed, no echo\n");
    }

  return ret;
}

int rpmsg_echo_init(FAR struct rpmsg_device *rdev,
                    FAR struct rpmsg_endpoint *ept)
{
  return rpmsg_create_ept(ept, rdev, RPMSG_ECHO_EPT_NAME,
                          RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
                          rpmsg_echo_ept_cb, NULL);
}

void rpmsg_echo_deinit(FAR struct rpmsg_endpoint *ept)
{
  rpmsg_destroy_ept(ept);
}
