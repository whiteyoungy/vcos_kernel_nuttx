/****************************************************************************
 * sched/knsh/knsh_printf.c
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/kernel_shell.h>

#include "knsh.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 * **************************************************************************/

/****************************************************************************
 * Name: knsh_printf
 *
 * Description:
 *  Format the output in knsh.
 *
 ****************************************************************************/

void knsh_printf(FAR struct knsh_state_s *state, const char *format, ...)
{
  char buffer[1024];
  va_list args;
  int printed_chars;

  va_start(args, format);

  printed_chars = vsnprintf(buffer, sizeof(buffer), format, args);

  if (printed_chars > 0)
    {
      size_t tail = 0;
      for (int i = 0; i < printed_chars; i++)
        {
          char ch = buffer[i];

          if (ch == '\r')
            {
              if (i > tail)
                {
                  for (int j = tail; j < i; j++)
                    {
                      knsh_putchar(state, buffer[j]);
                    }
                }

              knsh_putchar(state, '\n');
              tail = i + 1;
            }

          if (ch == '\n')
            {
              if (i > tail)
                {
                  for (int j = tail; j < i; j++)
                    {
                      knsh_putchar(state, buffer[j]);
                    }
                }

              knsh_putchar(state, '\r');
              tail = i;
            }
        }

      for (int j = tail; j < printed_chars; j++)
        {
          knsh_putchar(state, buffer[j]);
        }
    }

  va_end(args);
}
