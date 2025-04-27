/****************************************************************************
 * sched/knsh/knsh_init.c
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

#include "knsh.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_initialize
 *
 * Description:
 *   This interface is used to initialize the Kernel NuttShell (KNSH).
 *   All commands will be registered in nsh_initialize(), and the first
 *   prompt will also be printed.
 *
 * Input Parameters:
 *   state   - The pointer to the KNSH state structure.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void knsh_initialize(FAR struct knsh_state_s *state)
{
  knsh_register_all_commands();
  knsh_prompt(state);
}
