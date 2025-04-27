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

#include <sys/types.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/sched.h>
#include <nuttx/sched_note.h>
#include <nuttx/note/handshake_driver.h>
#include <nuttx/fs/fs.h>
#include <nuttx/streams.h>

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
#ifdef CONFIG_LIB_SYSCALL
#include <syscall.h>
#else
#define CONFIG_LIB_SYSCALL
#include <syscall.h>
#undef CONFIG_LIB_SYSCALL
#endif
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int handshake_open(struct file *filep);
static int handshake_close(struct file *filep);
static int handshake_ioctl(struct file *filep, int cmd, unsigned long arg);

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct handshake_driver_s
{
  bool trans_switch;                /* Transmission switch */
  bool all_tsk;                     /* All tasks need to be sent this time */
  int step_len;                     /* Number of tasks sent at a time */
  int curr_idx;                     /* Current sending location */
  int core;                         /* curr process core */
};

struct handshake_task
{
  struct lib_memoutstream_s *stream;
  int len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static __percpu_bss struct handshake_driver_s __g_handshake_driver =
{
  0
};

#define g_handshake_driver this_cpu_var(__g_handshake_driver)

static const struct file_operations g_handshake_fops =
{
  handshake_open,                   /* open */
  handshake_close,                  /* close */
  NULL,                             /* read */
  NULL,                             /* write */
  NULL,                             /* seek */
  handshake_ioctl,                  /* ioctl */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void handshake_init(void)
{
  g_handshake_driver.core           = -1;
  g_handshake_driver.curr_idx       = 0;
  g_handshake_driver.step_len       = 0;
  g_handshake_driver.trans_switch   = false;
  g_handshake_driver.all_tsk        = false;
}

static void handshake_finish(void)
{
  g_handshake_driver.core           = -1;
  g_handshake_driver.curr_idx       = 0;
  g_handshake_driver.step_len       = 0;
  g_handshake_driver.trans_switch   = false;
  g_handshake_driver.all_tsk        = false;
}

static void handshake_step_finish(void)
{
  g_handshake_driver.core           = -1;
  g_handshake_driver.step_len       = 0;
  g_handshake_driver.trans_switch   = false;
  g_handshake_driver.all_tsk        = false;
}

static int handshake_open(struct file *filep)
{
  return OK;
}

static int handshake_close(struct file *filep)
{
  return OK;
}

static void dump_task(struct tcb_s *tcb, void *arg)
{
  struct handshake_task *handshake_info = (struct handshake_task *)arg;

  struct ftrace_taskinfo_cmd_type handshake_task_info;

  handshake_task_info.common.nc_type    = (uint8_t)ftrace_type_last;
  handshake_task_info.common.nc_cmd_id  = (uint8_t)ftrace_taskinfo_cmd_id;
  handshake_task_info.common.nc_length  =
                                  sizeof(struct ftrace_taskinfo_cmd_type);

  handshake_task_info.nc_cpu        = up_cpu_index();
  handshake_task_info.nc_period     = 0;
  handshake_task_info.nc_pid        = tcb->pid;
  handshake_task_info.nc_priority   = tcb->init_priority;
  strncpy((char *)handshake_task_info.taskname, tcb->name, MAX_TASK_NAME);

  handshake_info->len += lib_stream_puts(handshake_info->stream,
            &handshake_task_info, sizeof(struct ftrace_taskinfo_cmd_type));
}

static int handshake_ioctl(struct file *filep, int cmd, unsigned long arg)
{
  struct handshake_driver_s *drv = filep->f_inode->i_private;
  int target_step_len;

  switch (cmd)
    {
  case 0:
      target_step_len = *((int *)arg);
      if (target_step_len < 0)
        {
          drv->all_tsk = true;
          target_step_len = CONFIG_NOTE_HANDSHAKE_BUFFSIZE /
                                sizeof(struct ftrace_taskinfo_cmd_type);
        }
      else
        {
          drv->all_tsk = false;
        }

      drv->step_len = target_step_len;
      drv->trans_switch = true;
      drv->core = up_cpu_index();

      break;
  case 1:
      handshake_init();
      break;
  default:
      break;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int handshake_register(void)
{
  handshake_init();
  return register_driver("/dev/note/handshake", &g_handshake_fops,
                         0666, &g_handshake_driver);
}

int handshake_core_get(void)
{
  struct handshake_driver_s *drv        = &g_handshake_driver;
  return drv->core;
}

int handshake_task_info_get(struct lib_memoutstream_s *stream)
{
  struct handshake_driver_s *drv        = &g_handshake_driver;
  int step_len  = drv->step_len;
  struct handshake_task handshake_info  =
    {
      .stream = stream,
      .len = 0,
    };

  if (drv->trans_switch == false)
    {
      return handshake_info.len;
    }

  if (drv->step_len == 0)
    {
      return handshake_info.len;
    }

  while (true)
    {
      drv->curr_idx = nxsched_handshake(dump_task, drv->curr_idx,
                                        &handshake_info);

      /* return -1 means end, init */

      if (drv->curr_idx < 0)
        {
          handshake_finish();
          return handshake_info.len;
        }

      (drv->curr_idx)++;

      step_len--;
      if (step_len == 0)
        {
          if (drv->all_tsk != true)
            {
              handshake_step_finish();
            }
          break;
        }
    }

  return handshake_info.len;
}
