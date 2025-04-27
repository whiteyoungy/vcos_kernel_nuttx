/****************************************************************************
 * sched/knsh/knsh_parse.c
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <sched.h>
#include <unistd.h>

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

static FAR char *knsh_argument(FAR struct knsh_state_s *state,
                              FAR char **saveptr);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char   g_token_separator[] = " \t\n";

/****************************************************************************
 * Public Data
 ****************************************************************************/

const char g_knsh_fmtsyntax[]         = "knsh: %s: syntax error\n";
const char g_knsh_fmtargrequired[]    = "knsh: %s: missing required argument"
                                        "(s)\n";
const char g_knsh_fmtnomatching[]     = "knsh: %s: no matching %s\n";
const char g_knsh_fmtarginvalid[]     = "knsh: %s: argument invalid\n";
const char g_knsh_fmtargrange[]       = "knsh: %s: value out of range\n";
const char g_knsh_fmtcmdnotfound[]    = "knsh: %s: command not found\n";
const char g_knsh_fmtnosuch[]         = "knsh: %s: no such %s: %s\n";
const char g_knsh_fmttoomanyargs[]    = "knsh: %s: too many arguments\n";
const char g_knsh_fmtdeepnesting[]    = "knsh: %s: nesting too deep\n";
const char g_knsh_fmtcontext[]        = "knsh: %s: not valid in this context"
                                        "\n";
const char g_knsh_fmtcmdfailed[]      = "knsh: %s: %s failed: %d\n";
const char g_knsh_fmtcmdoutofmemory[] = "knsh: %s: out of memory\n";
const char g_knsh_fmtinternalerror[]  = "knsh: %s: Internal error\n";
const char g_knsh_fmtsignalrecvd[]    = "knsh: %s: Interrupted by signal\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_argument
 ****************************************************************************/

static FAR char *knsh_argument(FAR struct knsh_state_s *state,
                              FAR char **saveptr)
{
  FAR char *pbegin = *saveptr;
  FAR char *pend = NULL;

  /* Find the beginning of the next token */

  for (; *pbegin && strchr(g_token_separator, *pbegin) != NULL; pbegin++);

  /* If we are at the end of the string with nothing but delimiters found,
   * then return NULL, meaning that there are no further arguments on the
   * line.
   */

  if (!*pbegin)
    {
      return NULL;
    }

  for (pend = pbegin; *pend != '\0'; pend++)
    {
      if (strchr(g_token_separator, *pend) != NULL)
        {
          break;
        }
    }

  /* pend either points to the end of the string or to the first
   * delimiter after the string.
   */

  if (*pend)
    {
      /* Turn the delimiter into a NUL terminator */

      *pend++ = '\0';
    }

  /* Save the pointer where we left off */

  *saveptr = pend;

  /* Return the parsed argument */

  return pbegin;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_parse_command
 *
 * Description:
 *   This function parses and executes the line of text received from the
 *   user.
 *
 ****************************************************************************/

int knsh_parse_command(FAR struct knsh_state_s *state, FAR char *cmdline)
{
  FAR char *argv[KNSH_MAX_ARGV_ENTRIES];
  FAR char *saveptr;
  FAR char *cmd;
  int       argc;
  int       ret;

  memset(argv, 0, KNSH_MAX_ARGV_ENTRIES * sizeof(FAR char *));
  saveptr = cmdline;
  cmd = knsh_argument(state, &saveptr);

  if (!cmd)
    {
      return OK;
    }

  argv[0] = cmd;

  for (argc = 1; argc < KNSH_MAX_ARGV_ENTRIES - 1; )
    {
      argv[argc] = knsh_argument(state, &saveptr);
      if (!argv[argc])
        {
          break;
        }

      argc++;
    }

    argv[argc] = NULL;
  if (argc > CONFIG_KNSH_MAXARGUMENTS)
    {
      knsh_printf(state, g_knsh_fmttoomanyargs, cmd);
    }

  /* execute the command */

  ret = knsh_command(state, argc, argv);
  return ret;
}
