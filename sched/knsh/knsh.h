/****************************************************************************
 * sched/knsh/knsh.h
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

#ifndef __SCHED_KNSH_KNSH_H
#define __SCHED_KNSH_KNSH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/list.h>
#include <sys/types.h>

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include <nuttx/kernel_shell.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Argument list size
 *
 *   argv[0]:      The command name.
 *   argv[1]:      The beginning of argument (up to CONFIG_NSH_MAXARGUMENTS)
 *   argv[N]:      The Nth argument
 *   argv[argc]:   NULL terminating pointer
 *
 * Maximum size is CONFIG_NSH_MAXARGUMENTS+5
 */

#define KNSH_MAX_ARGV_ENTRIES (CONFIG_NSH_MAXARGUMENTS+5)

/* Maximum size of one command line */

#ifndef CONFIG_KNSH_LINELEN
#  define CONFIG_KNSH_LINELEN 80
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct knsh_state_s; /* Defined in kernel_shell.h */

typedef CODE int (*knsh_cmd_t)(FAR struct knsh_state_s *state, int argc,
                              FAR char **argv);
struct knsh_cmd
{
  FAR const char * cmd;       /* Name of the command */
  knsh_cmd_t       handler;   /* Function that handles the command */
  uint8_t          minargs;   /* Minimum number of arguments (including command) */
  uint8_t          maxargs;   /* Maximum number of arguments (including command) */
#ifndef CONFIG_KNSH_DISABLE_HELP
  FAR const char * usage;     /* Usage instructions for 'help' command */
#endif
  struct list_node node;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char g_knsh_fmtsyntax[];
extern const char g_knsh_fmtargrequired[];
extern const char g_knsh_fmtnomatching[];
extern const char g_knsh_fmtarginvalid[];
extern const char g_knsh_fmtargrange[];
extern const char g_knsh_fmtcmdnotfound[];
extern const char g_knsh_fmtnosuch[];
extern const char g_knsh_fmttoomanyargs[];
extern const char g_knsh_fmtdeepnesting[];
extern const char g_knsh_fmtcontext[];
extern const char g_knsh_fmtcmdfailed[];
extern const char g_knsh_fmtcmdoutofmemory[];
extern const char g_knsh_fmtinternalerror[];
extern const char g_knsh_fmtsignalrecvd[];

/* knsh cmds */

#ifndef CONFIG_KNSH_DISABLE_HELP
extern struct knsh_cmd knsh_cmd_help;
#endif

#ifndef CONFIG_KNSH_DISABLE_MB
extern struct knsh_cmd knsh_cmd_mb;
#endif

#ifndef CONFIG_KNSH_DISABLE_MH
extern struct knsh_cmd knsh_cmd_mh;
#endif

#ifndef CONFIG_KNSH_DISABLE_MW
extern struct knsh_cmd knsh_cmd_mw;
#endif

#ifndef CONFIG_KNSH_DISABLE_XD
extern struct knsh_cmd knsh_cmd_xd;
#endif

#ifndef CONFIG_KNSH_DISABLE_EXIT
extern struct knsh_cmd knsh_cmd_exit;
#endif
/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#if defined(__cplusplus)
extern "C"
{
#endif

/****************************************************************************
 * Name: knsh_printf
 *
 * Description:
 *  Format the output in knsh.
 *
 ****************************************************************************/

void knsh_printf(FAR struct knsh_state_s *state, const char *format, ...);

/****************************************************************************
 * Name: knsh_prompt
 *
 * Description:
 *   Print prompt.
 *
 ****************************************************************************/

void knsh_prompt(FAR struct knsh_state_s *state);

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

void knsh_initialize(FAR struct knsh_state_s *state);

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

int knsh_command(FAR struct knsh_state_s *state, int argc, FAR char *argv[]);

/****************************************************************************
 * Name: knsh_parse_command
 *
 * Description:
 *   This function parses and executes the line of text received from the
 *   user.
 *
 * Input Parameters:
 *   state   - The pointer to the KNSH state structure.
 *   cmdline - The cmd line read from the KNSH console.
 *
 * Returned Value:
 *   Void
 *
 ****************************************************************************/

int knsh_parse_command(FAR struct knsh_state_s *state, FAR char *cmdline);

/****************************************************************************
 * Name: knsh_register_all_commands
 *
 * Description:
 *   Register all commands. If you need to add commands, you can add them
 *   here with knsh_add_command().
 *
 ****************************************************************************/

void knsh_register_all_commands(void);

#if defined(__cplusplus)
}
#endif

#endif /* __SCHED_KNSH_KNSH_H */
