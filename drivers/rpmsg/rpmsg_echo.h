/****************************************************************************
 * drivers/rpmsg/rpmsg_echo.h
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

#ifndef __DRIVERS_RPMSG_RPMSG_ECHO_H
#define __DRIVERS_RPMSG_RPMSG_ECHO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/rpmsg/rpmsg.h>

#ifdef CONFIG_RPMSG_ECHO

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int rpmsg_echo_init(FAR struct rpmsg_device *rdev,
                    FAR struct rpmsg_endpoint *ept);
void rpmsg_echo_deinit(FAR struct rpmsg_endpoint *ept);
int rpmsg_echo(FAR struct rpmsg_endpoint *ept,
               FAR const struct rpmsg_echo_s *echo);

#endif /* CONFIG_RPMSG_ECHO */
#endif /* __DRIVERS_RPMSG_RPMSG_ECHO_H */
