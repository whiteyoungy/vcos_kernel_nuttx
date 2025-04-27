/****************************************************************************
 * sched/knsh/knsh_dbgcmds.c
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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <nuttx/kernel_shell.h>
#include "knsh.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef KNSH_HAVE_MEMCMDS
#if !defined(CONFIG_KNSH_DISABLE_MB) || !defined(CONFIG_KNSH_DISABLE_MH) || \
    !defined(CONFIG_KNSH_DISABLE_MW)
#  define KNSH_HAVE_MEMCMDS 1
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef KNSH_HAVE_MEMCMDS
struct dbgmem_s
{
  bool         dm_write;  /* true: perform write operation */
  bool         dm_wo_flag;/* true: write the address but not re-read */
  FAR void    *dm_addr;   /* Address to access */
  uint32_t     dm_value;  /* Value to write */
  unsigned int dm_count;  /* The number of bytes to access */
};
#endif

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_MB
static int call_knsh_cmd_mb(FAR struct knsh_state_s *state,
                            int argc, FAR char **argv);
#endif

#ifndef CONFIG_KNSH_DISABLE_MH
static int call_knsh_cmd_mh(FAR struct knsh_state_s *state,
                            int argc, FAR char **argv);
#endif

#ifndef CONFIG_KNSH_DISABLE_MW
static int call_knsh_cmd_mw(FAR struct knsh_state_s *state,
                            int argc, FAR char **argv);
#endif

#ifndef CONFIG_KNSH_DISABLE_XD
static int call_knsh_cmd_xd(FAR struct knsh_state_s *state,
                            int argc, FAR char **argv);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_MB
struct knsh_cmd knsh_cmd_mb =
{
  .cmd     = "mb",
  .handler = call_knsh_cmd_mb,
  .minargs = 2,
  .maxargs = 4,
#ifndef CONFIG_KNSH_DISABLE_HELP
  .usage   = "[-w] <hex-address>[=<hex-value>] [<hex-byte-count>]"
#endif
};
#endif

#ifndef CONFIG_KNSH_DISABLE_MH
struct knsh_cmd knsh_cmd_mh =
{
  .cmd     = "mh",
  .handler = call_knsh_cmd_mh,
  .minargs = 2,
  .maxargs = 4,
#ifndef CONFIG_KNSH_DISABLE_HELP
  .usage   = "[-w] <hex-address>[=<hex-value>] [<hex-byte-count>]"
#endif
};
#endif

#ifndef CONFIG_KNSH_DISABLE_MW
struct knsh_cmd knsh_cmd_mw =
{
  .cmd     = "mw",
  .handler = call_knsh_cmd_mw,
  .minargs = 2,
  .maxargs = 4,
#ifndef CONFIG_KNSH_DISABLE_HELP
  .usage   = "[-w] <hex-address>[=<hex-value>] [<hex-byte-count>]"
#endif
};
#endif

#ifndef CONFIG_KNSH_DISABLE_XD
struct knsh_cmd knsh_cmd_xd =
{
  .cmd     = "xd",
  .handler = call_knsh_cmd_xd,
  .minargs = 3,
  .maxargs = 3,
#ifndef CONFIG_KNSH_DISABLE_HELP
  .usage   = "<hex-address> <byte-count>"
#endif
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mem_parse
 ****************************************************************************/

#ifdef KNSH_HAVE_MEMCMDS
static int mem_parse(FAR struct knsh_state_s *state, int argc,
                     FAR char **argv, FAR struct dbgmem_s *mem)
{
  UNUSED(state);

  int option;
  mem->dm_wo_flag = false;

  while ((option = getopt(argc, argv, "w")) != ERROR)
    {
      switch (option)
        {
        case 'w':
          mem->dm_wo_flag = true;
          break;

        default:
          return -EINVAL;
        }
    }

  if (optind >= argc)
    {
      return -EINVAL;
    }

  FAR char *pcvalue = strchr(argv[optind], '=');
  unsigned long lvalue = 0;

  /* Check if we are writing a value */

  if (pcvalue)
    {
      *pcvalue = '\0';
      pcvalue++;

      lvalue = strtoul(pcvalue, NULL, 16);
      if (lvalue > 0xffffffffl)
        {
          return -EINVAL;
        }

      mem->dm_write = true;
      mem->dm_value = (uint32_t)lvalue;
    }
  else
    {
      mem->dm_write = false;
      mem->dm_value = 0;
    }

  /* Get the address to be accessed */

  mem->dm_addr = (FAR void *)((uintptr_t)strtoul(argv[optind], NULL, 16));

  /* Get the number of bytes to access */

  if (optind + 1 < argc)
    {
      mem->dm_count = (unsigned int)strtoul(argv[optind + 1], NULL, 16);
    }
  else
    {
      mem->dm_count = 1;
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: call_knsh_cmd_mb
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_MB
static int call_knsh_cmd_mb(FAR struct knsh_state_s *state,
                            int argc, FAR char **argv)
{
  struct dbgmem_s mem;
  FAR volatile uint8_t *ptr;
  unsigned int i;
  int ret;

  ret = mem_parse(state, argc, argv, &mem);
  if (ret == 0)
    {
      /* Loop for the number of requested bytes */

      for (i = 0, ptr = (volatile uint8_t *)mem.dm_addr; i < mem.dm_count;
           i++, ptr++)
        {
          if (!mem.dm_wo_flag)
            {
              /* Print the value at the address */

              knsh_printf(state, "  %p = 0x%02x", ptr, *ptr);

              /* Are we supposed to write a value to this address? */

              if (mem.dm_write)
                {
                  /* Yes, was the supplied value within range? */

                  if (mem.dm_value > 0x000000ff)
                    {
                      knsh_printf(state, g_knsh_fmtargrange, argv[0]);
                      return ERROR;
                    }

                  /* Write the value and re-read the address so that we print its
                   * current value (if the address is a process address, then the
                   * value read might not necessarily be the value written).
                   */

                  *ptr = (uint8_t)mem.dm_value;
                  knsh_printf(state, " -> 0x%02x", *ptr);
                }
            }
          else
            {
              *ptr = (uint8_t)mem.dm_value;
              knsh_printf(state, "  write 0x%02x in %p", mem.dm_value, ptr);
            }
          /* Make sure we end it with a newline */

          knsh_printf(state, "\n");
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: call_knsh_cmd_mh
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_MH
static int call_knsh_cmd_mh(FAR struct knsh_state_s *state,
                            int argc, FAR char **argv)
{
  struct dbgmem_s mem;
  FAR volatile uint16_t *ptr;
  unsigned int i;
  int ret;

  ret = mem_parse(state, argc, argv, &mem);
  if (ret == 0)
    {
      /* Loop for the number of requested bytes */

      for (i = 0, ptr = (volatile uint16_t *)mem.dm_addr;
           i < mem.dm_count;
           i += 2, ptr++)
        {
          if (!mem.dm_wo_flag)
            {
              /* Print the value at the address */

              knsh_printf(state, "  %p = 0x%04x", ptr, *ptr);

              /* Are we supposed to write a value to this address? */

              if (mem.dm_write)
                {
                  /* Yes, was the supplied value within range? */

                  if (mem.dm_value > 0x0000ffff)
                    {
                      knsh_printf(state, g_knsh_fmtargrange, argv[0]);
                      return ERROR;
                    }

                  /* Write the value and re-read the address so that we print its
                   * current value (if the address is a process address, then the
                   * value read might not necessarily be the value written).
                   */

                  *ptr = (uint16_t)mem.dm_value;
                  knsh_printf(state, " -> 0x%04x", *ptr);
                }
            }
          else
            {
              *ptr = (uint16_t)mem.dm_value;
              knsh_printf(state, "  write 0x%04x in %p", mem.dm_value, ptr);
            }

          /* Make sure we end it with a newline */

          knsh_printf(state, "\n");
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: call_knsh_cmd_mw
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_MW
static int call_knsh_cmd_mw(FAR struct knsh_state_s *state,
                            int argc, FAR char **argv)
{
  struct dbgmem_s mem;
  FAR volatile uint32_t *ptr;
  unsigned int i;
  int ret;
  ret = mem_parse(state, argc, argv, &mem);
  if (ret == 0)
    {
      /* Loop for the number of requested bytes */

      for (i = 0, ptr = (volatile uint32_t *)mem.dm_addr; i < mem.dm_count;
           i += 4, ptr++)
        {
          if (!mem.dm_wo_flag)
            {
              /* Print the value at the address */

              knsh_printf(state, "  %p = 0x%08lx", ptr, *ptr);

              /* Are we supposed to write a value to this address? */

              if (mem.dm_write)
                {
                  /* Write the value and re-read the address so that we print its
                   * current value (if the address is a process address, then the
                   * value read might not necessarily be the value written).
                   */

                  *ptr = mem.dm_value;
                  knsh_printf(state, " -> 0x%08lx", *ptr);
                }
            }
          else
            {
              *ptr = mem.dm_value;
              knsh_printf(state, "  write 0x%08" PRIx32 " in %p", mem.dm_value, ptr);
            }

          /* Make sure we end it with a newline */

          knsh_printf(state, "\n");
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: knsh_dumpbuffer
 ****************************************************************************/

void knsh_dumpbuffer(FAR struct knsh_state_s *state, FAR const char *msg,
                    FAR const uint8_t *buffer, ssize_t nbytes)
{
  char line[128];
  size_t size;
  int ch;
  int i;
  int j;

  knsh_printf(state, "%s:\n", msg);
  for (i = 0; i < nbytes; i += 16)
    {
      snprintf(line, sizeof(line), "%04x: ", i);
      size = strlen(line);

      for (j = 0; j < 16; j++)
        {
          if (i + j < nbytes)
            {
              snprintf(&line[size], sizeof(line) - size,
                       "%02x ", buffer[i + j]);
            }
          else
            {
              strlcpy(&line[size], "   ", sizeof(line) - size);
            }

          size += strlen(&line[size]);
        }

      for (j = 0; j < 16; j++)
        {
          if (i + j < nbytes)
            {
              ch = buffer[i + j];
              snprintf(&line[size], sizeof(line) - size,
                       "%c", ch >= 0x20 && ch <= 0x7e ? ch : '.');
              size += strlen(&line[size]);
            }
        }

      knsh_printf(state, "%s\n", line);
    }
}

/****************************************************************************
 * Name: call_knsh_cmd_xd, hex dump of memory
 ****************************************************************************/

#ifndef CONFIG_KNSH_DISABLE_XD
int call_knsh_cmd_xd(FAR struct knsh_state_s *state,
                     int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *addr;
  FAR char *endptr;
  int       nbytes;

  addr = (FAR char *)((uintptr_t)strtoul(argv[1], &endptr, 16));
  if (argv[0][0] == '\0' || *endptr != '\0')
    {
      return ERROR;
    }

  nbytes = (int)strtol(argv[2], &endptr, 0);
  if (argv[0][0] == '\0' || *endptr != '\0' || nbytes < 0)
    {
      return ERROR;
    }

  knsh_dumpbuffer(state, "Hex dump", (uint8_t *)addr, nbytes);
  return OK;
}
#endif