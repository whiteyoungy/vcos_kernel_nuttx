/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 * Licensed under the Apache License, Version 2.0(the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sched.h>
#include "sched/sched.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int nxsched_handshake(nxsched_handshake_t handler, int start_ndx,
                      FAR void *arg)
{
  int ndx;

  /* Visit each active task */

  for (ndx = start_ndx; ndx < g_npidhash; ndx++)
    {
      if (g_pidhash[ndx])
        {
          handler(g_pidhash[ndx], arg);
          break;
        }
    }

  if (ndx < (g_npidhash - 1))
    {
      return ndx;
    }

  /* return -1 means finish handshake */

  return -1;
}
