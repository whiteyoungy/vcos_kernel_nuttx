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

#ifndef __INCLUDE_NUTTX_NOTE_HANDSHAKE_DRIVER_H
#define __INCLUDE_NUTTX_NOTE_HANDSHAKE_DRIVER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <nuttx/streams.h>

#define ftrace_type_last 0xff

typedef enum
{
  ftrace_taskinfo_cmd_id = 0,
  ftrace_otherinfo_cmd_id = 1,
  ftrace_runableinfo_cmd_id = 2,
  ftrace_isrinfo_cmd_id = 3,
  ftrace_snapshot_cmd_id = 4,
  ftrace_cmd_status_id = 0xe0,
  ftrace_cmd_shakehand_id = 0xe1,
  ftrace_filter_all_close_cmd_id = 0xf0,
  ftrace_filter_all_open_cmd_id = 0xf1,
  ftrace_filter_one_ctrl_cmd_id = 0xf2,
  ftrace_disconnect_cmd_id = 0xfe,
  ftrace_connect_cmd_id = 0xff,
} ftrace_cmdid_type_e;

#define MAX_TASK_NAME 64u
struct ftrace_common_cmd_type
{
  uint8_t nc_length;           /* Length of the trace */
  uint8_t nc_type;             /* See enum trace_type_e */
  uint8_t nc_cmd_id;
  uint8_t cmd_cnt;
};

struct ftrace_taskinfo_cmd_type
{
  struct ftrace_common_cmd_type common;
  uint8_t nc_priority;         /* Current task priority */
  uint8_t nc_cpu;              /* CPU task running on */
  uint8_t reserve[2];
  uint32_t nc_pid;             /* ID of the current task */
  uint32_t nc_period;
  uint8_t taskname[MAX_TASK_NAME];
};

int handshake_core_get(void);
int handshake_task_info_get(struct lib_memoutstream_s *stream);

int handshake_register(void);

#endif /* __INCLUDE_NUTTX_NOTE_HANDSHAKE_DRIVER_H */