/****************************************************************************
 * sched/knsh/knsh_prompt.c
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "knsh.h"

/****************************************************************************
 * Preprocessor Macros
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static char g_knshprompt[CONFIG_KNSH_PROMPT_MAX] = CONFIG_KNSH_PROMPT_STRING;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_update_prompt
 *
 * Description:
 *   update prompt.
 *
 ****************************************************************************/
static void knsh_update_prompt(void)
{
  if (CONFIG_KNSH_PROMPT_STRING[0])
    {
      strcpy(g_knshprompt, CONFIG_KNSH_PROMPT_STRING);
    }
  else
    {
      #ifdef CONFIG_BMP
        snprintf(g_knshprompt, sizeof(g_knshprompt),
                 "knsh[%d]"CONFIG_KNSH_PROMPT_SUFFIX, up_cpu_index());
      #else
        strcpy(g_knshprompt, CONFIG_KNSH_PROMPT_SUFFIX);
      #endif
    }
}
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: knsh_prompt
 *
 * Description:
 *   Print prompt.
 *
 ****************************************************************************/

void knsh_prompt(FAR struct knsh_state_s *state)
{
  knsh_update_prompt();
  knsh_printf(state, "%s", g_knshprompt);
}
