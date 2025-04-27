/****************************************************************************
 * sched/knsh/kernel_shell.c
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

#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>

#include <nuttx/nuttx.h>
#include <nuttx/sched.h>
#include <nuttx/ascii.h>
#include <nuttx/kernel_shell.h>
#include <nuttx/syslog/syslog.h>
#include <debug.h>
#include <nuttx/list.h>
#include "knsh.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char knsh_line_buffer[CONFIG_KNSH_LINELEN];
struct list_node g_knsh_cmd_list = LIST_INITIAL_VALUE(g_knsh_cmd_list);

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int knsh_getchar(FAR struct knsh_state_s *state);
static int knsh_readline(FAR struct knsh_state_s *state,
                         char *buffer, size_t buflen);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_getchar
 *
 * Description:
 *   Read a character from the KNSH console.
 *
 * Input Parameters:
 *   state   - The pointer to the KNSH state structure.
 *
 * Returned Value:
 *   The character read from the KNSH console.
 *
 ****************************************************************************/

static int knsh_getchar(FAR struct knsh_state_s *state)
{
  unsigned char tmp;
  ssize_t ret;

  ret = state->recv(state->priv, &tmp, 1);
  if (ret < 0)
    {
      return ret;
    }

  return tmp;
}

/****************************************************************************
 * Name: knsh_readline
 *
 * Description:
 *   Read a line from the KNSH console.
 *
 * Input Parameters:
 *   state   - The pointer to the KNSH state structure.
 *   buffer  - The buffer used to store the read line.
 *   buflen  - The size of the buffer.
 *
 * Returned Value:
 *   The length of the read line.
 *
 ****************************************************************************/

static int knsh_readline(FAR struct knsh_state_s *state,
                         char *buffer, size_t buflen)
{
  size_t index = 0;
  int ch;

  if (state == NULL || buffer == NULL || buflen == 0)
    {
      return -EINVAL;
    }

  memset(buffer, 0, buflen);

  while (index < buflen - 1)
    {
      ch = knsh_getchar(state);
      if (ch < 0)
        {
          return ch;
        }

      if (ch == '\r' || ch == '\n')
        {
          knsh_putchar(state, '\r');
          knsh_putchar(state, '\n');
          break;
        }

      if (ch == '\b' || ch == 127)
        {
          if (index > 0)
            {
              index--;
              buffer[index] = '\0';
              knsh_putchar(state, '\b');
              knsh_putchar(state, ' ');
              knsh_putchar(state, '\b');
            }

          continue;
        }

      buffer[index++] = (char)ch;
      knsh_putchar(state, ch);
    }

  buffer[index] = '\0';
  return index;
}

/****************************************************************************
 * Public Functions
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
                                       FAR void *priv)
{
  FAR struct knsh_state_s *state = kmm_zalloc(sizeof(*state));

  if (state == NULL)
    {
      return NULL;
    }

  state->send       = send;
  state->recv       = recv;
  state->priv       = priv;
#ifndef CONFIG_KNSH_DISABLE_EXIT
  state->exit_state = FALSE;
#endif

  return state;
}

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

int knsh_putchar(FAR struct knsh_state_s *state, int ch)
{
  unsigned char tmp = ch & 0xff;
  ssize_t ret;

  ret = state->send(state->priv, &tmp, 1);
  if (ret < 0)
    {
      return ret;
    }

  return tmp;
}

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

int knsh_process(FAR struct knsh_state_s *state)
{
  int ret;
  knsh_initialize(state);
  while ((ret = knsh_readline(state, knsh_line_buffer, sizeof(knsh_line_buffer))) >= 0)
    {
      ret = knsh_parse_command(state, knsh_line_buffer);
#ifndef CONFIG_KNSH_DISABLE_EXIT
      if (state->exit_state == TRUE)
        {
          goto knsh_exit;
        }

#endif
      knsh_prompt(state);
    }

  return ret;
#ifndef CONFIG_KNSH_DISABLE_EXIT
knsh_exit:
  kmm_free(state);
  return ret;
#endif
}
