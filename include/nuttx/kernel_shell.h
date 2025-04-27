/****************************************************************************
 * include/nuttx/kernel_shell.h
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

#ifndef __INCLUDE_NUTTX_KERNEL_SHELL_H
#define __INCLUDE_NUTTX_KERNEL_SHELL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <nuttx/arch.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern struct list_node g_knsh_cmd_list;

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

typedef CODE ssize_t (*knsh_send_func_t)(FAR void *priv, FAR void *buf,
                                        size_t len);
typedef CODE ssize_t (*knsh_recv_func_t)(FAR void *priv, FAR void *buf,
                                        size_t len);
struct knsh_state_s
{
  knsh_send_func_t send;                   /* Send buffer to knsh */
  knsh_recv_func_t recv;                   /* Recv buffer from knsh */
  FAR void *priv;                          /* Private data for transport */
#ifndef CONFIG_KNSH_DISABLE_EXIT
  bool exit_state;                         /* Indicate knsh exit state */
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_state_init
 *
 * Description:
 *   Initialize the Kernel NuttShell(KNSH) state structure.
 *
 * Input Parameters:
 *   send    - The pointer to the send function.
 *   recv    - The pointer to the receive function.
 *   priv    - The pointer to the private data.
 *
 * Returned Value:
 *   The pointer to the KNSH state structure on success.
 *   NULL on failure.
 *
 ****************************************************************************/

FAR struct knsh_state_s *knsh_state_init(knsh_send_func_t send,
                                       knsh_recv_func_t recv,
                                       FAR void *priv);

/****************************************************************************
 * Name: knsh_putchar
 *
 * Description:
 *   Put a character to the KNSH console.
 *
 * Input Parameters:
 *   state   - The pointer to the KNSH state structure.
 *   ch      - The character to be sent.
 *
 * Returned Value:
 *   The character sent to the KNSH console.
 *
 ****************************************************************************/

int knsh_putchar(FAR struct knsh_state_s *state, int ch);

/****************************************************************************
 * Name: knsh_process
 *
 * Description:
 *   Main knsh loop. Handles commands.
 *
 * Input Parameters:
 *   state   - The pointer to the KNSH state structure.
 *
 * Returned Value:
 *   Zero if successful.
 *   Negative value on error.
 *
 ****************************************************************************/

int knsh_process(FAR struct knsh_state_s *state);

#endif /* __INCLUDE_NUTTX_KERNEL_SHELL_H */
