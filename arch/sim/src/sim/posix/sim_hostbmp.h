/****************************************************************************
 * arch/sim/src/sim/posix/sim_hostbmp.h
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

#ifndef __ARCH_SIM_SRC_POSIX_SIM_HOSTBMP_H
#define __ARCH_SIM_SRC_POSIX_SIM_HOSTBMP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern pthread_key_t g_cpu_key;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

pthread_t get_bmp_cpu_pthread(uint64_t coreid);
int pthread_setname_np (pthread_t __target_thread, \
                                const char *__name);
int up_cpu_index(void);

#endif /* __ARCH_SIM_SRC_POSIX_SIM_HOSTBMP_H */