/****************************************************************************
 * sched/knsh/knsh_command.c
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
#include <nuttx/list.h>

#include <string.h>
#include <stdlib.h>

#include "knsh.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Help command summary layout */

#define KNSH_HELP_LINELEN  80
#define KNSH_HELP_TABSIZE  4
#define KNSH_HELP_CMDCOLS  6

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_HELP
static int call_knsh_cmd_help(FAR struct knsh_state_s *state,
                              int argc, FAR char **argv);
#endif

#ifndef CONFIG_KNSH_DISABLE_EXIT
static int call_knsh_cmd_exit(FAR struct knsh_state_s *state,
                              int argc, FAR char **argv);
#endif

#ifdef CONFIG_KNSH_SC
static int call_knsh_cmd_sc(FAR struct knsh_state_s *state,
                              int argc, FAR char **argv);
#endif

static int  knsh_cmd_unrecognized(FAR struct knsh_state_s *state, int argc,
                                  FAR char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_HELP
struct knsh_cmd knsh_cmd_help =
{
    .cmd     = "help",
    .handler = call_knsh_cmd_help,
    .minargs = 1,
    .maxargs = 2,
#ifndef CONFIG_KNSH_DISABLE_HELP
    .usage   = "[<cmd>]"
#endif
};
#endif

#ifndef CONFIG_KNSH_DISABLE_EXIT
struct knsh_cmd knsh_cmd_exit =
{
    .cmd     = "exit",
    .handler = call_knsh_cmd_exit,
    .minargs = 1,
    .maxargs = 1,
#ifndef CONFIG_KNSH_DISABLE_HELP
    .usage   = NULL
#endif
};
#endif

#ifdef CONFIG_KNSH_SC
struct knsh_cmd knsh_cmd_sc =
{
    .cmd     = "sc",
    .handler = call_knsh_cmd_sc,
    .minargs = 2,
    .maxargs = 2,
#ifndef CONFIG_KNSH_DISABLE_HELP
    .usage   = "<dst-core-id>"
#endif
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_help_cmdlist
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_HELP
static inline void knsh_help_cmdlist(FAR struct knsh_state_s *state)
{
  struct knsh_cmd *knsh_cmd_entry;
  int cmd_count = 0;
  int colwidth = 0;
  int cmdwidth;
  int ncmdrows;
  int i;
  int k;
  int offset;
  char line[256];

  list_for_every_entry(&g_knsh_cmd_list, knsh_cmd_entry, struct knsh_cmd,
                       node)
    {
      cmd_count++;
      cmdwidth = strlen(knsh_cmd_entry->cmd);
      if (cmdwidth > colwidth)
        {
          colwidth = cmdwidth;
        }
    }

  colwidth += KNSH_HELP_TABSIZE;
  ncmdrows = (cmd_count + KNSH_HELP_CMDCOLS - 1) / KNSH_HELP_CMDCOLS;

  for (i = 0; i < ncmdrows; i++)
    {
      offset = KNSH_HELP_TABSIZE;
      memset(line, ' ', offset);

      k = 0;
      list_for_every_entry(&g_knsh_cmd_list, knsh_cmd_entry, struct knsh_cmd, node)
        {
          if (k >= i && (k - i) % ncmdrows == 0)
            {
              offset += strlcpy(line + offset, knsh_cmd_entry->cmd, sizeof(line) - offset);
              for (cmdwidth = strlen(knsh_cmd_entry->cmd); cmdwidth < colwidth; cmdwidth++)
                {
                  line[offset++] = ' ';
                }
            }

          k += 1;
        }

      line[offset++] = '\n';
      knsh_printf(state, "%.*s", offset, line);
    }
}
#endif

/****************************************************************************
 * Name: knsh_help_usage
 ****************************************************************************/

#if !defined(CONFIG_KNSH_DISABLE_HELP)
static inline void knsh_help_usage(FAR struct knsh_state_s *state)
{
  knsh_printf(state, "KNSH command forms:\n");
  knsh_printf(state, "  <cmd> [> <file>|>> <file>]\n\n");
}
#endif

/****************************************************************************
 * Name: knsh_help_showcmd
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_HELP
static void knsh_help_showcmd(FAR struct knsh_state_s *state,
                         struct knsh_cmd *cmdnode)
{
  if (cmdnode->usage)
    {
      knsh_printf(state, "  %s %s\n", cmdnode->cmd, cmdnode->usage);
    }
  else
    {
      knsh_printf(state, "  %s\n", cmdnode->cmd);
    }
}
#endif

/****************************************************************************
 * Name: help_cmd
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_HELP
static int knsh_help_cmd(FAR struct knsh_state_s *state, FAR const char *cmd)
{
  /* Find the command in the command list */

  struct knsh_cmd *knsh_cmd_entry;
  list_for_every_entry(&g_knsh_cmd_list, knsh_cmd_entry, struct knsh_cmd,
                       node)
    {
      /* Is this the one we are looking for? */

      if (strcmp(knsh_cmd_entry->cmd, cmd) == 0)
        {
          knsh_printf(state, "%s usage:", cmd);
          knsh_help_showcmd(state, knsh_cmd_entry);
          return OK;
        }
    }

  knsh_printf(state, g_knsh_fmtcmdnotfound, cmd);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: help_allcmds
 ****************************************************************************/

#if !defined(CONFIG_KNSH_DISABLE_HELP)
static inline void knsh_help_allcmds(FAR struct knsh_state_s *state)
{
  struct knsh_cmd *knsh_cmd_entry;

  /* Show all of the commands in the command table */

  list_for_every_entry(&g_knsh_cmd_list, knsh_cmd_entry, struct knsh_cmd,
                       node)
    {
      /* Is this the one we are looking for? */

      knsh_help_showcmd(state, knsh_cmd_entry);
    }
}
#endif

/****************************************************************************
 * Name: cmd_help
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_HELP
static int call_knsh_cmd_help(FAR struct knsh_state_s *state,
                              int argc, FAR char **argv)
{
  FAR const char *cmd = NULL;
  if (argc > 1)
    {
      cmd = argv[1];
    }

  /* Are we showing help on a single command? */

  if (cmd)
    {
      /* Yes.. show the single command */

      knsh_help_cmd(state, cmd);
    }
  else
    {
        knsh_help_cmd(state, "help");
        knsh_printf(state, "\n");
        knsh_help_cmdlist(state);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_exit
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_EXIT
static int call_knsh_cmd_exit(FAR struct knsh_state_s *state,
                              int argc, FAR char **argv)
{
  state->exit_state = TRUE;
  return OK;
}
#endif

/****************************************************************************
 * Name: call_knsh_cmd_sc
 ****************************************************************************/

#ifdef CONFIG_KNSH_SC
int call_knsh_cmd_sc(FAR struct knsh_state_s *state, int argc, FAR char **argv)
{
  FAR char *endptr;
  int dst_core_id = 0;

  if (argc == 2)
    {
      dst_core_id = strtol(argv[1], &endptr, 0);
      if (*endptr != '\0')
        goto out;
      if ((dst_core_id >= 0) && (dst_core_id != up_cpu_index()))
          state->exit_state = TRUE;
          up_affinity_irq(CONFIG_KNSH_UART_IRQ, 1 << dst_core_id);
      return OK;
    }

out:
  knsh_printf(state, "Usage: sc <dst-core-id>\n");
  return ERROR;
}
#endif

/****************************************************************************
 * Name: knsh_cmd_unrecognized
 ****************************************************************************/

static int knsh_cmd_unrecognized(FAR struct knsh_state_s *state, int argc,
                            FAR char **argv)
{
  UNUSED(argc);
  knsh_printf(state, g_knsh_fmtcmdnotfound, argv[0]);
  return ERROR;
}

/****************************************************************************
 * Name: knsh_add_command
 *
 * Description:
 *   Insert the command into the command linked list.
 *
 * Input Parameters:
 *   cmd_head                - The head of the command linked list.
 *   added_command_node      - The node of the inserted command.
 *
 * Returned Value:
 *   Void
 *
 ****************************************************************************/

static void knsh_add_command(struct list_node * cmd_head,
                             struct knsh_cmd * added_command_node)
{
  list_initialize(&added_command_node->node);
  list_add_tail(&g_knsh_cmd_list, &(added_command_node->node));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_command
 *
 * Description:
 *   Execute the command in argv[0]
 *
 * Returned Value:
 *   -1 (ERROR) if the command was unsuccessful
 *    0 (OK)     if the command was successful
 *
 ****************************************************************************/

int knsh_command(FAR struct knsh_state_s *state, int argc, FAR char *argv[])
{
  struct knsh_cmd *knsh_cmd_entry;
  const char      *cmd;
  knsh_cmd_t       handler = knsh_cmd_unrecognized;
  int              ret;

  /* The form of argv is:
   *
   * argv[0]:      The command name.
   * argv[1]:      The beginning of argument (up to CONFIG_KNSH_MAXARGUMENTS)
   * argv[argc]:   NULL terminating pointer
   */

  cmd = argv[0];

  /* See if the command is one that we understand */

  list_for_every_entry(&g_knsh_cmd_list, knsh_cmd_entry, struct knsh_cmd,
                       node)
    {
      if (strcmp(knsh_cmd_entry->cmd, cmd) == 0)
        {
          /* Check if a valid number of arguments was provided.  We
           * do this simple, imperfect checking here so that it does
           * not have to be performed in each command.
           */

          if (argc < knsh_cmd_entry->minargs)
            {
              /* Fewer than the minimum number were provided */

              knsh_printf(state, g_knsh_fmtargrequired, cmd);
              return ERROR;
            }
          else if (argc > knsh_cmd_entry->maxargs)
            {
              /* More than the maximum number were provided */

              knsh_printf(state, g_knsh_fmttoomanyargs, cmd);
              return ERROR;
            }
          else
            {
              /* A valid number of arguments were provided (this does
               * not mean they are right).
               */

              handler = knsh_cmd_entry->handler;
              break;
            }
        }
    }

  ret = handler(state, argc, argv);
  return ret;
}

/****************************************************************************
 * Name: knsh_register_all_commands
 *
 * Description:
 *   Register all commands. If you need to add commands, you can add them
 *   here with knsh_add_command().
 *
 ****************************************************************************/

void knsh_register_all_commands(void)
{
#ifndef CONFIG_KNSH_DISABLE_HELP
  knsh_add_command(&g_knsh_cmd_list, &knsh_cmd_help);
#endif

#ifndef CONFIG_KNSH_DISABLE_MB
  knsh_add_command(&g_knsh_cmd_list, &knsh_cmd_mb);
#endif

#ifndef CONFIG_KNSH_DISABLE_MH
  knsh_add_command(&g_knsh_cmd_list, &knsh_cmd_mh);
#endif

#ifndef CONFIG_KNSH_DISABLE_MW
  knsh_add_command(&g_knsh_cmd_list, &knsh_cmd_mw);
#endif

#ifndef CONFIG_KNSH_DISABLE_XD
  knsh_add_command(&g_knsh_cmd_list, &knsh_cmd_xd);
#endif

#ifndef CONFIG_KNSH_DISABLE_EXIT
  knsh_add_command(&g_knsh_cmd_list, &knsh_cmd_exit);
#endif

#ifdef CONFIG_KNSH_SC
  knsh_add_command(&g_knsh_cmd_list, &knsh_cmd_sc);
#endif
}
