/****************************************************************************
 * drivers/note/noteddr_driver.c
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <nuttx/fs/fs.h>
#include <nuttx/kmalloc.h>
#include <nuttx/note/note_driver.h>
#include <nuttx/note/noteddr_driver.h>
#include <nuttx/panic_notifier.h>
#include <nuttx/sched.h>
#include <nuttx/sched_note.h>
#include <nuttx/spinlock.h>
#include <nuttx/streams.h>
#include <stdatomic.h>

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
#  ifdef CONFIG_LIB_SYSCALL
#    include <syscall.h>
#  else
#    define CONFIG_LIB_SYSCALL
#    include <syscall.h>
#    undef CONFIG_LIB_SYSCALL
#  endif
#endif
#include "syslog.h"
#include <execinfo.h>
#include <nuttx/arch.h>
#include <nuttx/cache.h>
#if defined(CONFIG_ARCH_CHIP_SCHU_FSI)
#include "addr_offset.h"
#endif
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

uint64_t control_block_addr = 0;
#define NCPUS CONFIG_SMP_NCPUS

/* Renumber idle task PIDs
 *  In NuttX, PID number less than NCPUS are idle tasks.
 *  In Linux, there is only one idle task of PID 0.
 */

#define get_pid(pid) ((pid) < NCPUS ? 0 : (pid))

#define get_task_state(s)                                                     \
  ((s) == 0 ? 'X' : ((s) <= LAST_READY_TO_RUN_STATE ? 'R' : 'S'))

struct __kfifo_block_ctrl_g;
#ifdef CONFIG_NR_CPUS
#  define CPU_NUMBER CONFIG_NR_CPUS
#elif defined CONFIG_SMP_NCPUS
#  define CPU_NUMBER CONFIG_SMP_NCPUS
#else
#  define CPU_NUMBER 1
#endif
#define BLOCK_NUMBER 8u
#define KFIFO_HEAD_SIZE 8u
#define TOTAL_BLOCK_NUMBER (BLOCK_NUMBER * CPU_NUMBER)

/* MOD 8BYTE */

#define JOB_DOING (1)
#define JOB_DONE (0)
#define BLOCK_JOB_NUMBER (8U)
#define ALIGN_UP(value, alignment)                                            \
  (((value) + (alignment)-1) & ~((alignment)-1))
#define ALIGN_DOWN(value, alignment) ((value) & ~((alignment)-1))
#define TRACE_DATA_ADDR                                                       \
  ALIGN_UP (control_block_addr + sizeof (struct __kfifo_block_ctrl_g), 8)
#define BLOCK_LENGTH                                                          \
  ALIGN_DOWN ((CONFIG_DRIVERS_NOTEDDR_BUFFSIZE                                \
               - sizeof (struct __kfifo_block_ctrl_g))                        \
                  / TOTAL_BLOCK_NUMBER,                                       \
              8)
#define KFIFO_MAX_LENGTH (BLOCK_LENGTH - KFIFO_HEAD_SIZE)
#define DATA_BUFF_SIZE (TOTAL_BLOCK_NUMBER * BLOCK_LENGTH)
#define NX_DDR_CONTROL_HEAD_MAGIC_NUMBER (0XA5A5)

/* LEVEL 0:use self-achieve way lock.LEVEL 1:use system spinlock with irq */
#define NOTE_DDR_SPINLOCK_USE_LEVEL (1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef atomic_int noteddr_atomic;

struct noteddr_driver_s
  {
    struct note_driver_s driver;
    spinlock_t lock[CPU_NUMBER];
  };

/* The structure to hold the context data of trace dump */

struct noteddr_dump_cpu_context_s
  {
    int intr_nest;              /* Interrupt nest level */
    bool pendingswitch;         /* sched_switch pending flag */
    int current_state;          /* Task state of the current line */
    uint32_t current_pid;       /* Task PID of the current line */
    uint32_t next_pid;          /* Task PID of the next line */
    uint8_t current_priority;   /* Task Priority of the current line */
    uint8_t next_priority;      /* Task Priority of the next line */
  };

struct noteddr_dump_context_s
  {
    struct noteddr_dump_cpu_context_s cpu[NCPUS];
    uint8_t cpuid;
  };

struct __kfifo
  {
    uint8_t cpuid;
    uint8_t blockid;
    uint8_t reserve[2];
    noteddr_atomic in;                  /* 入队索引 */
  };

struct __kfifo_job_ctrl
  {
    noteddr_atomic job_channel[BLOCK_NUMBER][BLOCK_JOB_NUMBER];
  };

struct __kfifo_block_ctrl
  {
    noteddr_atomic in;                  /* 入队索引 */
    noteddr_atomic in_fail;             /* 入队索引 */
    noteddr_atomic block_full;          /* 入队索引 */
    unsigned int out;                   /* 出队索引 */
    uint8_t cpuid;
    uint8_t reserve[3];
    uint32_t buff_addr_offset;
    struct __kfifo kfifo[BLOCK_NUMBER]; /* 队列缓存指针 */
    struct __kfifo_job_ctrl kfifo_job;
  };

struct __kfifo_block_ctrl_g
  {
    uint32_t magic_number;
    uint8_t total_cpu_ctrl_number;
    uint8_t init_status;
    uint8_t block_number;
    uint8_t block_job_number;
    uint32_t block_length;
    uint64_t tick_per_sec;
    uint64_t nsec_per_sec;
    struct __kfifo_block_ctrl kfifo_block_ctrl[CPU_NUMBER];
  };

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int noteddr_open(FAR struct file *filep);
static int noteddr_close(FAR struct file *filep);
static ssize_t noteddr_read(FAR struct file *filep, FAR char *buffer,
                            size_t buflen);
static int noteddr_ioctl(struct file *filep, int cmd, unsigned long arg);
static void noteddr_add(FAR struct note_driver_s *drv, FAR const void *note,
                        size_t len);
static void noteddr_dump_init_context(FAR
                                      struct noteddr_dump_context_s *ctx);
static int noteddr_dump_one(FAR uint8_t * p, FAR struct lib_outstream_s *s,
                            FAR struct noteddr_dump_context_s *ctx);
static int noteddr_dump_block(FAR uint8_t * p, FAR struct lib_outstream_s *s,
                              FAR struct noteddr_dump_context_s *ctx,
                              uint8_t cpuid);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_noteddr_fops =
{
  noteddr_open,                 /* open */
  noteddr_close,                /* close */
  noteddr_read,                 /* read */
  NULL,                         /* write */
  NULL,                         /* seek */
  noteddr_ioctl,                /* ioctl */
};

static const struct note_driver_ops_s g_noteddr_ops =
{
  noteddr_add
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct __kfifo_block_ctrl_g *kfifo_block_ctrl_gs;
struct __kfifo_block_ctrl *kfifo_block_ctrl_g =
{
  NULL
};
static uint8_t(*trace_data_buff)[BLOCK_NUMBER][BLOCK_LENGTH];
struct noteddr_driver_s g_noteddr_driver =
{
  {
#ifdef CONFIG_SCHED_INSTRUMENTATION_FILTER
    "ddr",
    {
      {
      CONFIG_SCHED_INSTRUMENTATION_FILTER_DEFAULT_MODE,
#  ifdef CONFIG_SMP
      CONFIG_SCHED_INSTRUMENTATION_CPUSET
#  endif
      },
    },
#endif
    &g_noteddr_ops
  },
};

static uint16_t cur_read_index[CPU_NUMBER];

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void init_kfifo(void)
{
  uint8_t *buff_temp_addr;
  int cpuid = up_cpu_index();

#if defined(CONFIG_BOARD_HAPS_FSI) || defined(CONFIG_BOARD_ZEBU_FSI)
  if(get_fsi_offset_reg_lock() == 0)
  {
    return ;
  }
  else
  {
    control_block_addr = fsi_trans_addr_by_offset(CONFIG_DRIVERS_NOTEDDR_CONTROL_ADDR_BASE);
  }
#else
  control_block_addr = (CONFIG_DRIVERS_NOTEDDR_CONTROL_ADDR_BASE);
#endif
  kfifo_block_ctrl_gs
  = (struct __kfifo_block_ctrl_g *)control_block_addr;

  trace_data_buff
  = (unsigned char (*)[BLOCK_NUMBER][BLOCK_LENGTH])TRACE_DATA_ADDR;
  kfifo_block_ctrl_g = kfifo_block_ctrl_gs->kfifo_block_ctrl;
#if defined(CONFIG_BOARD_HAPS_FSI) || defined(CONFIG_BOARD_ZEBU_FSI)
  release_fsi_offset_reg_lock();
#endif
  kfifo_block_ctrl_g[cpuid].in = 0;
  kfifo_block_ctrl_g[cpuid].in_fail = 0;
  kfifo_block_ctrl_g[cpuid].block_full = 0;
  kfifo_block_ctrl_g[cpuid].out = 0;
  kfifo_block_ctrl_g[cpuid].cpuid = cpuid;
  kfifo_block_ctrl_g[cpuid].buff_addr_offset
    = (uint8_t *) trace_data_buff[cpuid] - (uint8_t *) control_block_addr;
  for (int i = 0; i < BLOCK_NUMBER; i++)
    {
      kfifo_block_ctrl_g[cpuid].kfifo[i].in = KFIFO_HEAD_SIZE;
      kfifo_block_ctrl_g[cpuid].kfifo[i].cpuid = cpuid;
      kfifo_block_ctrl_g[cpuid].kfifo[i].blockid = i;
      buff_temp_addr = trace_data_buff[cpuid][i];
      if (((uintptr_t) buff_temp_addr) % 8 != 0)
        {
          syslog(0, "init_kfifo error.cpuid:%d.blockid:%d.dataaddr:%p.\n",
                 cpuid, i, buff_temp_addr);
        }
    }
  memset(&(kfifo_block_ctrl_g[cpuid].kfifo_job), JOB_DONE,
         sizeof(struct __kfifo_job_ctrl));
  cur_read_index[cpuid] = KFIFO_HEAD_SIZE;
  if (cpuid == 0)
    {
      kfifo_block_ctrl_gs->magic_number = NX_DDR_CONTROL_HEAD_MAGIC_NUMBER;
      kfifo_block_ctrl_gs->total_cpu_ctrl_number = CPU_NUMBER;
      kfifo_block_ctrl_gs->block_number = BLOCK_NUMBER;
      kfifo_block_ctrl_gs->block_job_number = BLOCK_JOB_NUMBER;
      kfifo_block_ctrl_gs->block_length = BLOCK_LENGTH;
#ifdef CONFIG_ARCH_PERF_EVENTS
      kfifo_block_ctrl_gs->tick_per_sec = up_perf_getfreq();
#else
      kfifo_block_ctrl_gs->tick_per_sec = 1;
#endif
      kfifo_block_ctrl_gs->nsec_per_sec = NSEC_PER_SEC;
      memset(trace_data_buff, 0, DATA_BUFF_SIZE);
      kfifo_block_ctrl_gs->init_status = 1u;
    }
  else
    {
      while (kfifo_block_ctrl_gs->init_status != 1u)
        ;
    }
}

#if (NOTE_DDR_SPINLOCK_USE_LEVEL == 0)
static unsigned int __kfifo_in(struct __kfifo *fifo, const void *buf, unsigned int len)
{
  noteddr_atomic in = 0;
  noteddr_atomic goal_len = 0;
  unsigned int try_cnt = 0;
  do
    {
      try_cnt += 1;
      in = fifo->in;
      if ((len > (BLOCK_LENGTH - in)) || (try_cnt > 5))
        {
          /* syslog(0,"in:%d.try_cnt:%d\n",in,try_cnt); */

          return 0;
        }

      goal_len = in + len;
    }
  while (atomic_compare_exchange_strong(&(fifo->in), &in, goal_len) == 0);

  uint8_t *data = trace_data_buff[fifo->cpuid][fifo->blockid];
  memcpy(&(data[in]), buf, len);
  up_flush_dcache((uintptr_t) & (data[in]), (uintptr_t) & (data[in + len]));
  return len;
}
#endif

#if (NOTE_DDR_SPINLOCK_USE_LEVEL == 1)
static unsigned int
       __kfifo_in_withspinlock(struct __kfifo *fifo, const void *buf,
                               unsigned int len)
{
  noteddr_atomic in = 0;
  noteddr_atomic goal_len = 0;
  in = fifo->in;

  goal_len = in + len;
  fifo->in = goal_len;

  uint8_t *data = trace_data_buff[fifo->cpuid][fifo->blockid];
  memcpy(&(data[in]), buf, len);
  up_flush_dcache((uintptr_t) & (data[in]), (uintptr_t) & (data[in + len]));
  return len;
}
#endif
static signed char
kfifo_job_lock(struct __kfifo_block_ctrl *kfifo_block_ctrl,
               unsigned int in_index)
{
  signed char ret = -1;
  for (uint8_t i = 0; i < BLOCK_JOB_NUMBER; i++)
    {
      if ((kfifo_block_ctrl->kfifo_job.job_channel)[in_index][i]
           == JOB_DOING)
        continue;
      else
        {
          noteddr_atomic jobstatus = JOB_DONE;
          if (atomic_compare_exchange_strong
              (&((kfifo_block_ctrl->kfifo_job.job_channel)[in_index][i]),
               &jobstatus, JOB_DOING) == 0)
            {
              continue;
            }
          else
            {
              ret = i;
              break;
            }
        }
    }

  return ret;
}

void
kfifo_job_unlock(struct __kfifo_block_ctrl *kfifo_block_ctrl,
                 unsigned char in_index, unsigned char channel)
{
  struct __kfifo_job_ctrl *job_ctrl = NULL;
  job_ctrl = &(kfifo_block_ctrl->kfifo_job);
  atomic_exchange(&(job_ctrl->job_channel[in_index][channel]), JOB_DONE);
  return;
}

unsigned int
kfifo_in(struct __kfifo_block_ctrl *kfifo_block_ctrl, const void *ptr,
         unsigned int length)
{
  noteddr_atomic in_index = 0;
  struct __kfifo *current_kfifo = NULL;
  uint32_t ret = 0;
  noteddr_atomic new_index = 0;
#if (NOTE_DDR_SPINLOCK_USE_LEVEL == 0)
  uint8_t try_cnt = 0;
#endif
  if (length > KFIFO_MAX_LENGTH)
    {
      return 0;
    }
#if (NOTE_DDR_SPINLOCK_USE_LEVEL == 0)

  do
    {
      in_index = kfifo_block_ctrl->in;
      current_kfifo = &(kfifo_block_ctrl->kfifo[(uint32_t) in_index]);
      if (length > (BLOCK_LENGTH - current_kfifo->in))
        ret = 0;
      else
        {
          signed int lock_id = kfifo_job_lock(kfifo_block_ctrl, in_index);
          if (lock_id >= 0)
            {
              ret = __kfifo_in(current_kfifo, ptr, length);
              kfifo_job_unlock(kfifo_block_ctrl, in_index, lock_id);
            }
          else
            {
              ret = 0;
            }
        }

      try_cnt += 1;
      if (ret > 0)
        {
          break;
        }
      else if (try_cnt > 5)
        {
          atomic_fetch_add(&(kfifo_block_ctrl->in_fail), 1);
          break;
        }
      else
        {
          new_index = in_index + 1;
          if (new_index >= BLOCK_NUMBER)
            {
              new_index = 0;
            }

          if (new_index == kfifo_block_ctrl->out)
            {
              /* all fifo block full */

              atomic_fetch_add(&(kfifo_block_ctrl->block_full), 1);
              return ret;
            }
          else
            {
              atomic_compare_exchange_strong(&(kfifo_block_ctrl->in),
                                             &in_index, new_index);
            }
        }
    }
  while (ret == 0);
#else
  irqstate_t flags;
  spinlock_t *lock;
  lock = &g_noteddr_driver.lock[kfifo_block_ctrl->cpuid];
  flags = spin_lock_irqsave_wo_note(lock);
#if defined(CONFIG_BOARD_HAPS_FSI) || defined(CONFIG_BOARD_ZEBU_FSI)
  if(get_fsi_offset_reg_lock()==0)
  {
    spin_unlock_irqrestore_wo_note(lock, flags);
    return -EPERM;
  }
  else
  {
    control_block_addr = fsi_trans_addr_by_offset(CONFIG_DRIVERS_NOTEDDR_CONTROL_ADDR_BASE);
    in_index = kfifo_block_ctrl->in;
    current_kfifo = &(kfifo_block_ctrl->kfifo[(uint32_t) in_index]);
  }
  release_fsi_offset_reg_lock();
#else
  in_index = kfifo_block_ctrl->in;
  current_kfifo = &(kfifo_block_ctrl->kfifo[(uint32_t) in_index]);
#endif
  if (length > (BLOCK_LENGTH - current_kfifo->in))
    {
      new_index = in_index + 1;
      if (new_index >= BLOCK_NUMBER)
        {
          new_index = 0;
        }

      if (new_index == kfifo_block_ctrl->out)
        {
          /* all fifo block full */
          uint8_t cpuid = up_cpu_index();
          uint8_t *data = trace_data_buff[current_kfifo->cpuid][current_kfifo->blockid];

          kfifo_out_txconfirm(&(kfifo_block_ctrl_g[cpuid]));

          signed int lock_id = kfifo_job_lock(kfifo_block_ctrl, kfifo_block_ctrl->out);
          current_kfifo = &(kfifo_block_ctrl->kfifo[(uint32_t) kfifo_block_ctrl->out]);
          current_kfifo->in = 0;

          memset(&(data[current_kfifo->in]), 0, BLOCK_LENGTH);
          ret = __kfifo_in_withspinlock(current_kfifo, ptr, length);

          kfifo_block_ctrl->out = (kfifo_block_ctrl->out + 1) % BLOCK_NUMBER;

          spin_unlock_irqrestore_wo_note(lock, flags);
          kfifo_job_unlock(kfifo_block_ctrl, kfifo_block_ctrl->out, lock_id);
          return ret;
        }

      kfifo_block_ctrl->in = new_index;
    }

#if defined(CONFIG_BOARD_HAPS_FSI) || defined(CONFIG_BOARD_ZEBU_FSI)
  if(get_fsi_offset_reg_lock()==0)
  {
    spin_unlock_irqrestore_wo_note(lock, flags);
    return -EPERM;
  }
  else
  {
    control_block_addr = fsi_trans_addr_by_offset(CONFIG_DRIVERS_NOTEDDR_CONTROL_ADDR_BASE);
    in_index = kfifo_block_ctrl->in;
    current_kfifo = &(kfifo_block_ctrl->kfifo[(uint32_t) in_index]);
  }
  release_fsi_offset_reg_lock();
#else
  in_index = kfifo_block_ctrl->in;
  current_kfifo = &(kfifo_block_ctrl->kfifo[(uint32_t) in_index]);
#endif
  /* to prevent core A read buff.release the flag when we have done memcopy
   * job
   */

  signed int lock_id = kfifo_job_lock(kfifo_block_ctrl, in_index);
  ret = __kfifo_in_withspinlock(current_kfifo, ptr, length);
  spin_unlock_irqrestore_wo_note(lock, flags);
  kfifo_job_unlock(kfifo_block_ctrl, in_index, lock_id);
#endif

  return ret;
}

static bool
check_current_block_ready(struct __kfifo_block_ctrl *kfifo_block_ctrl,
                          uint32_t index)
{
  struct __kfifo_job_ctrl *jbctrl = &(kfifo_block_ctrl->kfifo_job);
  noteddr_atomic *channel = jbctrl->job_channel[index];
  for (uint8_t i = 0; i < BLOCK_JOB_NUMBER; i++)
    {
      if (channel[i] == JOB_DOING)
        {
          return JOB_DOING;
        }
    }

  return JOB_DONE;
}

unsigned int
kfifo_out(struct __kfifo_block_ctrl *kfifo_block_ctrl, void *ptr,
          unsigned int length)
{
  if (length < BLOCK_LENGTH)
    {
      return 0;
    }

  noteddr_atomic in_index = kfifo_block_ctrl->in;
  noteddr_atomic new_index = in_index + 1;
  if (kfifo_block_ctrl->out == in_index)
    {
      /* back to in,so add in */

      if (new_index >= BLOCK_NUMBER)
        {
          new_index = 0;
        }

      /* this ops isnot compete with kfifo_in,
       * because we all need in_index+1
       */

      atomic_compare_exchange_strong(&(kfifo_block_ctrl->in), &in_index,
                                     new_index);
      return 0;
    }

  unsigned int out_index = kfifo_block_ctrl->out;
  if (check_current_block_ready(kfifo_block_ctrl, out_index) == JOB_DOING)
    {
      return 0;
    }

  uint8_t *data = trace_data_buff[kfifo_block_ctrl->cpuid][out_index];
  memcpy(ptr, data, BLOCK_LENGTH);
  kfifo_block_ctrl->kfifo[(uint32_t) out_index].in = KFIFO_HEAD_SIZE;
  out_index += 1;
  if (out_index >= BLOCK_NUMBER)
    {
      out_index = 0;
    }

  kfifo_block_ctrl->out = out_index;
  return BLOCK_LENGTH;
}

unsigned char *kfifo_out_ptr(struct __kfifo_block_ctrl *kfifo_block_ctrl,
                             uint32_t * length)
{
  unsigned int out_index = kfifo_block_ctrl->out;
  noteddr_atomic in_index = kfifo_block_ctrl->in;
  noteddr_atomic new_index = in_index + 1;
  if (kfifo_block_ctrl->out == in_index)
    {
      /* back to in,so add in */

      if (new_index >= BLOCK_NUMBER)
        {
          new_index = 0;
        }

      atomic_compare_exchange_strong(&(kfifo_block_ctrl->in), &in_index,
                                     new_index);
      return 0;
    }

  if (check_current_block_ready(kfifo_block_ctrl, out_index) == JOB_DOING)
    {
      return 0;
    }

  *length = kfifo_block_ctrl->kfifo[(uint32_t) out_index].in;
  uint16_t *temp
    = (uint16_t *) (trace_data_buff[kfifo_block_ctrl->cpuid][out_index]);
  *temp = (uint16_t) (*length);
  return (unsigned char *)temp;
}

void kfifo_out_txconfirm(struct __kfifo_block_ctrl *kfifo_block_ctrl)
{
  unsigned int out_index = kfifo_block_ctrl->out;
  kfifo_block_ctrl->kfifo[(uint32_t) out_index].in = KFIFO_HEAD_SIZE;
  out_index += 1;
  if (out_index >= BLOCK_NUMBER)
    {
      out_index = 0;
    }

  kfifo_block_ctrl->out = out_index;
}

/****************************************************************************
 * Name: noteddr_open
 ****************************************************************************/

static int noteddr_open(FAR struct file *filep)
{
  FAR struct noteddr_dump_context_s *ctx;

  /* Reset the read index of the circular buffer */

  ctx = kmm_zalloc(sizeof(*ctx));
  ctx->cpuid = 0;
  if (ctx == NULL)
    {
      return -ENOMEM;
    }

  filep->f_priv = ctx;
  noteddr_dump_init_context(ctx);
  return OK;
}

int noteddr_close(FAR struct file *filep)
{
  FAR struct noteddr_dump_context_s *ctx = filep->f_priv;
  kmm_free(ctx);
  return OK;
}

/****************************************************************************
 * Name: noteddr_read
 ****************************************************************************/

static ssize_t
noteddr_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
  FAR struct noteddr_dump_context_s *ctx = filep->f_priv;
  FAR struct lib_memoutstream_s stream;
  ssize_t read_len = 0;
  unsigned int cur_read_len = 0;
  lib_memoutstream(&stream, buffer, buflen);

  for (int cpuid = ctx->cpuid; cpuid < CPU_NUMBER; cpuid++)
    {
      while (buflen > 0)
        {
          uint32_t length = 0;
          uint8_t *note = kfifo_out_ptr(&(kfifo_block_ctrl_g[cpuid]),
                                       &length);
          if (note == NULL)
            {
              /* try again */

              note = kfifo_out_ptr(&(kfifo_block_ctrl_g[cpuid]), &length);
              if (note == NULL || length == KFIFO_HEAD_SIZE)
                {
                  ctx->cpuid = cpuid + 1;
                  break;
                }
            }

          /* Parse notes into text format */

          cur_read_len = noteddr_dump_block(note,
                          (FAR struct lib_outstream_s *)&stream, ctx, cpuid);
          read_len += cur_read_len;
          if (buflen >= read_len)
            {
              buflen -= read_len;
            }
          else
            {
              buflen = 0;
            }
        }
    }

  return read_len;
}

/****************************************************************************
 * Name: noteddr_ioctl
 ****************************************************************************/

static int noteddr_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
  int ret = -ENOSYS;
  return ret;
}

/****************************************************************************
 * Name: noteddr_add
 *
 * Description:
 *   Add the variable length note to the transport layer
 *
 * Input Parameters:
 *   note    - The note buffer
 *   notelen - The buffer length
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   We are within a critical section.
 *
 ****************************************************************************/

static void
noteddr_add(FAR struct note_driver_s *driver, FAR const void *note,
            size_t notelen)
{
  (void)driver;
  uint8_t cpu = up_cpu_index();
  uint8_t align_byte = 0;
  struct note_common_s *common = (struct note_common_s *)note;
  common->nc_cpu = cpu;
  struct __kfifo_block_ctrl *ctrl;
#if defined(CONFIG_BOARD_HAPS_FSI) || defined(CONFIG_BOARD_ZEBU_FSI)
  if(get_fsi_offset_reg_lock()==0)
  {
    return ;
  }
  else
  {
    control_block_addr = fsi_trans_addr_by_offset(CONFIG_DRIVERS_NOTEDDR_CONTROL_ADDR_BASE);
    ctrl = &(kfifo_block_ctrl_g[cpu]);
  }
  release_fsi_offset_reg_lock();
#else
  ctrl = &(kfifo_block_ctrl_g[cpu]);
#endif

  /* To adapt bytealiagn */

  align_byte = (8 - (notelen % 8)) % 8;
  common->nc_length = notelen + align_byte;
  if (((uintptr_t) common) % 8 != 0)
    {
      syslog(0, "[%d].common:%p.\n", cpu, common);
    }

  kfifo_in(ctrl, note, notelen + align_byte);
}

/****************************************************************************
 * Name: noteddr_dump_init_context
 ****************************************************************************/

static void noteddr_dump_init_context(FAR struct noteddr_dump_context_s *ctx)
{
  int cpu;

  /* Initialize the trace dump context */

  for (cpu = 0; cpu < NCPUS; cpu++)
    {
      ctx->cpu[cpu].intr_nest = 0;
      ctx->cpu[cpu].pendingswitch = false;
      ctx->cpu[cpu].current_state = TSTATE_TASK_RUNNING;
      ctx->cpu[cpu].current_pid = -1;
      ctx->cpu[cpu].next_pid = -1;
      ctx->cpu[cpu].current_priority = -1;
      ctx->cpu[cpu].next_priority = -1;
    }
}

/****************************************************************************
 * Name: myget_task_name
 ****************************************************************************/

static const char *myget_task_name(int pid)
{
#if CONFIG_DRIVERS_NOTE_TASKNAME_BUFSIZE > 0
  FAR const char *taskname;

  taskname = note_get_taskname(pid);
  if (taskname != NULL)
    {
      return taskname;
    }

#endif
  return "<noname>";
}

/****************************************************************************
 * Name: noteddr_dump_header
 ****************************************************************************/

static int
noteddr_dump_header(FAR struct lib_outstream_s *s,
                    FAR struct note_common_s *note,
                    FAR struct noteddr_dump_context_s *ctx)
{
  struct timespec ts;
  int pid;
  int cpu;
  int ret = 0;
  if ((uintptr_t) note % 8 != 0)
    {
      syslog(0, "type:%d.noteaddr:%p.pidaddr:%p.\n", note->nc_type, note,
             &note->nc_pid);
    }

  perf_convert(note->nc_systime, &ts);

  pid = note->nc_pid;
  cpu = note->nc_cpu;
  ret =
    lib_sprintf(s, "%8s-%-3u [%d] %3" PRIu64 ".%09lu: ",
                myget_task_name(pid), get_pid(pid), cpu,
                (uint64_t) ts.tv_sec, ts.tv_nsec);
  return ret;
}

#if (defined CONFIG_SCHED_INSTRUMENTATION_SWITCH)                             \
    || (defined CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER)

/****************************************************************************
 * Name: noteddr_dump_sched_switch
 ****************************************************************************/

static int
noteddr_dump_sched_switch(FAR struct lib_outstream_s *s,
                          FAR struct note_common_s *note,
                          FAR struct noteddr_dump_context_s *ctx)
{
  FAR struct noteddr_dump_cpu_context_s *cctx;
  uint8_t current_priority;
  uint8_t next_priority;
  int current_pid;
  int next_pid;
  int ret;
  int cpu = note->nc_cpu;

  cctx = &ctx->cpu[cpu];
  current_pid = cctx->current_pid;
  next_pid = cctx->next_pid;

  current_priority = cctx->current_priority;
  next_priority = cctx->next_priority;

  ret = lib_sprintf(s,
                    "sched_switch: prev_comm=%s prev_pid=%u "
                    "prev_prio=%u prev_state=%c ==> "
                    "next_comm=%s next_pid=%u next_prio=%u\n",
                    myget_task_name(current_pid), get_pid(current_pid),
                    current_priority, get_task_state(cctx->current_state),
                    myget_task_name(next_pid), get_pid(next_pid),
                    next_priority);

  cctx->current_pid = cctx->next_pid;
  cctx->current_priority = cctx->next_priority;
  cctx->pendingswitch = false;
  return ret;
}
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_DUMP
static int noteram_dump_printf(FAR struct lib_outstream_s *s,
                               FAR struct note_printf_s *note)
{
  size_t ret = 0;

  if (note->npt_type == 0)
    {
      ret = lib_bsprintf(s, note->npt_fmt, note->npt_data);
    }
  else
    {
      size_t count = NOTE_PRINTF_GET_COUNT(note->npt_type);
      char fmt[128];
      size_t i;

      fmt[0] = '\0';
      ret += lib_sprintf(s, "%p", note->npt_fmt);
      for (i = 0; i < count; i++)
        {
          int type = NOTE_PRINTF_GET_TYPE(note->npt_type, i);

          switch (type)
            {
              case NOTE_PRINTF_UINT32:
                {
                  strcat(fmt, " %u");
                }
                break;
              case NOTE_PRINTF_UINT64:
                {
                  strcat(fmt, " %llu");
                }
                break;
              case NOTE_PRINTF_STRING:
                {
                  strcat(fmt, " %s");
                }
                break;
              case NOTE_PRINTF_DOUBLE:
                {
                  strcat(fmt, " %f");
                }
            }
        }

        ret += lib_bsprintf(s, fmt, note->npt_data);
        lib_stream_putc(s, '\n');
        ret++;
    }

  return ret;
}
#endif

static int
noteddr_dump_block(FAR uint8_t * p, FAR struct lib_outstream_s *s,
                   FAR struct noteddr_dump_context_s *ctx, uint8_t cpuid)
{
  FAR struct note_common_s *note = NULL;
  int ret = 0;
  uint16_t length = *((uint16_t *) p);

  if (cur_read_index[cpuid] < length)
    {
      note = (FAR struct note_common_s *)&p[cur_read_index[cpuid]];
      ret += noteddr_dump_one((uint8_t *) note, s, ctx);
      cur_read_index[cpuid] = cur_read_index[cpuid] + note->nc_length;
    }
  else
    {
      cur_read_index[cpuid] = KFIFO_HEAD_SIZE;

      kfifo_out_txconfirm(&(kfifo_block_ctrl_g[cpuid]));
    }

  return ret;
}

/****************************************************************************
 * Name: noteddr_dump_one
 ****************************************************************************/

static int
noteddr_dump_one(FAR uint8_t * p, FAR struct lib_outstream_s *s,
                 FAR struct noteddr_dump_context_s *ctx)
{
  FAR struct note_common_s *note = (FAR struct note_common_s *)p;
  FAR struct noteddr_dump_cpu_context_s *cctx;
  int ret = 0;
  int pid;
  int cpu = note->nc_cpu;

  cctx = &ctx->cpu[cpu];
  pid = note->nc_pid;

  if (cctx->current_pid < 0)
    {
      cctx->current_pid = pid;
    }

  /* Output one note */

  switch (note->nc_type)
    {
    case NOTE_START:
      {
        ret += noteddr_dump_header(s, note, ctx);
        ret += lib_sprintf(s,
                           "sched_wakeup_new: comm=%s pid=%d "
                           "target_cpu=%d\n",
                           myget_task_name(pid), get_pid(pid), cpu);
      }
      break;

    case NOTE_STOP:
      {
        /* This note informs the task to be stopped.
         * Change current task state for the succeeding NOTE_RESUME.
         */

        cctx->current_state = 0;
      }
      break;

#ifdef CONFIG_SCHED_INSTRUMENTATION_SWITCH
    case NOTE_SUSPEND:
      {
        FAR struct note_suspend_s *nsu = (FAR struct note_suspend_s *)p;

        /* This note informs the task to be suspended.
         * Preserve the information for the succeeding NOTE_RESUME.
         */

        cctx->current_state = nsu->nsu_state;
      }
      break;

    case NOTE_RESUME:
      {
        /* This note informs the task to be resumed.
         * The task switch timing depends on the running context.
         */

        cctx->next_pid = pid;
        cctx->next_priority = note->nc_priority;

        if (cctx->intr_nest == 0)
          {
            /* If not in the interrupt context, the task switch is
             * executed immediately.
             */

            ret += noteddr_dump_header(s, note, ctx);
            ret += noteddr_dump_sched_switch(s, note, ctx);
          }
        else
          {
            /* If in the interrupt context, the task switch is postponed
             * until leaving the interrupt handler.
             */

            ret += noteddr_dump_header(s, note, ctx);
            ret += lib_sprintf(s,
                               "sched_waking: comm=%s "
                               "pid=%ld target_cpu=%d\n",
                               myget_task_name(cctx->next_pid),
                               get_pid(cctx->next_pid), cpu);
            cctx->pendingswitch = true;
          }
      }
      break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_SYSCALL
    case NOTE_SYSCALL_ENTER:
      {
        FAR struct note_syscall_enter_s *nsc;
        int i;
        int j;
        uintptr_t arg;

        nsc = (FAR struct note_syscall_enter_s *)p;
        if (nsc->nsc_nr < CONFIG_SYS_RESERVED
        || nsc->nsc_nr >= SYS_maxsyscall)
          {
            break;
          }

        ret += noteddr_dump_header(s, note, ctx);
        ret += lib_sprintf(s, "sys_%s(",
                           g_funcnames[nsc->nsc_nr - CONFIG_SYS_RESERVED]);

        for (i = j = 0; i < nsc->nsc_argc; i++)
          {
            arg = nsc->nsc_args[i];
            if (i == 0)
              {
                ret += lib_sprintf(s, "arg%d: 0x%" PRIxPTR, i, arg);
              }
            else
              {
                ret += lib_sprintf(s, ", arg%d: 0x%" PRIxPTR, i, arg);
              }
          }

        ret += lib_sprintf(s, ")\n");
      }
      break;

    case NOTE_SYSCALL_LEAVE:
      {
        FAR struct note_syscall_leave_s *nsc;
        uintptr_t result;

        nsc = (FAR struct note_syscall_leave_s *)p;
        if (nsc->nsc_nr < CONFIG_SYS_RESERVED
        || nsc->nsc_nr >= SYS_maxsyscall)
          {
            break;
          }

        ret += noteddr_dump_header(s, note, ctx);
        result = nsc->nsc_result;
        ret += lib_sprintf(s, "sys_%s -> 0x%" PRIxPTR "\n",
                           g_funcnames[nsc->nsc_nr - CONFIG_SYS_RESERVED],
                           result);
      }
      break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_IRQHANDLER
    case NOTE_IRQ_ENTER:
      {
        FAR struct note_irqhandler_s *nih;

        nih = (FAR struct note_irqhandler_s *)p;
        ret += noteddr_dump_header(s, note, ctx);
        ret += lib_sprintf(s, "irq_handler_entry: irq=%u name=%pS\n",
                           nih->nih_irq, (FAR void *)nih->nih_handler);
        cctx->intr_nest++;
      }
      break;

    case NOTE_IRQ_LEAVE:
      {
        FAR struct note_irqhandler_s *nih;

        nih = (FAR struct note_irqhandler_s *)p;
        ret += noteddr_dump_header(s, note, ctx);
        ret += lib_sprintf(s, "irq_handler_exit: irq=%u ret=handled\n",
                           nih->nih_irq);
        cctx->intr_nest--;

        if (cctx->intr_nest <= 0)
          {
            cctx->intr_nest = 0;
            if (cctx->pendingswitch)
              {
                /* If the pending task switch exists, it is executed here */

                ret += noteddr_dump_header(s, note, ctx);
                ret += noteddr_dump_sched_switch(s, note, ctx);
              }
          }
      }
      break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_CSECTION
    case NOTE_CSECTION_ENTER:
    case NOTE_CSECTION_LEAVE:
      {
        struct note_csection_s *ncs;
        ncs = (FAR struct note_csection_s *)p;
        ret += noteddr_dump_header(s, &ncs->ncs_cmn, ctx);
        ret += lib_sprintf(s, "tracing_mark_write: %c|%d|critical_section\n",
                           note->nc_type == NOTE_CSECTION_ENTER ? 'B' : 'E',
                           pid);
      }
      break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_PREEMPTION
    case NOTE_PREEMPT_LOCK:
    case NOTE_PREEMPT_UNLOCK:
      {
        struct note_preempt_s *npr;
        int16_t count;
        npr = (FAR struct note_preempt_s *)p;
        count = npr->npr_count;
        ret += noteddr_dump_header(s, &npr->npr_cmn, ctx);
        ret += lib_sprintf(s,
                           "tracing_mark_write: "
                           "%c|%d|sched_lock:%d\n",
                           note->nc_type == NOTE_PREEMPT_LOCK ? 'B' : 'E',
                           pid, count);
      }
      break;
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION_DUMP
    case NOTE_DUMP_PRINTF:
      {
        FAR struct note_printf_s *npt;

        npt = (FAR struct note_printf_s *)p;
        ret += noteddr_dump_header(s, &npt->npt_cmn, ctx);
        ret += lib_sprintf(s, "tracing_mark_write: ");
        ret += noteram_dump_printf(s, npt);
      }
      break;
    case NOTE_DUMP_BEGIN:
    case NOTE_DUMP_END:
      {
        FAR struct note_event_s *nbi = (FAR struct note_event_s *)p;
        char c = note->nc_type == NOTE_DUMP_BEGIN ? 'B' : 'E';
        int len = note->nc_length - SIZEOF_NOTE_EVENT(0);
        uintptr_t ip;

        ip = nbi->nev_ip;
        ret += noteddr_dump_header(s, &nbi->nev_cmn, ctx);
        if (len > 0)
          {
            ret += lib_sprintf(s, "tracing_mark_write: %c|%d|%.*s\n",
                               c, pid, len, (FAR const char *)nbi->nev_data);
          }
        else
          {
            ret += lib_sprintf(s, "tracing_mark_write: %c|%d|%pS\n",
                               c, pid, (FAR void *)ip);
          }
      }
      break;
    case NOTE_DUMP_MARK:
      {
        int len = note->nc_length - sizeof(struct note_event_s);
        FAR struct note_event_s *nbi = (FAR struct note_event_s *)p;
        ret += noteddr_dump_header(s, &nbi->nev_cmn, ctx);
        ret += lib_sprintf(s, "tracing_mark_write: I|%d|%.*s\n",
                           pid, len, (FAR const char *)nbi->nev_data);
      }
      break;
    case NOTE_DUMP_COUNTER:
      {
        FAR struct note_event_s *nbi = (FAR struct note_event_s *)p;
        FAR struct note_counter_s *counter;
        counter = (FAR struct note_counter_s *)nbi->nev_data;
        ret += noteddr_dump_header(s, &nbi->nev_cmn, ctx);
        ret += lib_sprintf(s, "tracing_mark_write: C|%d|%s|%ld\n",
                           pid, counter->name, counter->value);
      }
      break;
#endif
#ifdef CONFIG_SCHED_INSTRUMENTATION_HEAP
    case NOTE_HEAP_ADD:
    case NOTE_HEAP_REMOVE:
    case NOTE_HEAP_ALLOC:
    case NOTE_HEAP_FREE:
      {
        FAR struct note_heap_s *nmm = (FAR struct note_heap_s *)p;
        FAR const char *name[] =
          {
            "add", "remove", "malloc", "free"
          };

        ret += noteddr_dump_header(s, &nmm->nhp_cmn, ctx);
        ret += lib_sprintf(s, "tracing_mark_write: C|%d|Heap Usage|%d|%s"
                           ": heap: %p size:%" PRIiPTR ", address: %p\n",
                           pid, nmm->used,
                           name[note->nc_type - NOTE_HEAP_ADD],
                           nmm->heap, nmm->size, nmm->mem);
      }
      break;
#endif
    default:
      break;
    }

  /* Return the length of the processed note */

  return ret;
}

#ifdef CONFIG_DRIVERS_NOTEDDR_CRASH_DUMP

/****************************************************************************
 * Name: noteddr_dump
 ****************************************************************************/

static void noteddr_dump(FAR struct noteddr_driver_s *drv)
{
  struct noteddr_dump_context_s ctx;
  struct lib_syslograwstream_s stream;
  uint8_t note[64];

  lib_syslograwstream_open(&stream);
  lib_sprintf(&stream.common, "# tracer:nop\n#\n");

  while (1)
    {
      ssize_t ret;

      ret = noteddr_get(drv, note, sizeof(note));
      if (ret <= 0)
        {
          break;
        }

      noteddr_dump_one(note, &stream.common, &ctx);
    }

  lib_syslograwstream_close(&stream);
}

/****************************************************************************
 * Name: noteddr_crash_dump
 ****************************************************************************/

static int
noteddr_crash_dump(FAR struct notifier_block *nb, unsigned long action,
                   FAR void *data)
{
  if (action == PANIC_KERNEL)
    {
      noteddr_dump(&g_noteddr_driver);
    }

  return 0;
}

static void noteddr_crash_dump_register(void)
{
  static struct notifier_block nb;
  nb.notifier_call = noteddr_crash_dump;
  panic_notifier_chain_register(&nb);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: noteddr_register
 *
 * Description:
 *   Register a serial driver at /dev/note/ram that can be used by an
 *   application to read data from the circular note buffer.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Zero on succress. A negated errno value is returned on a failure.
 *
 ****************************************************************************/

int noteddr_register(void)
{
#ifdef CONFIG_DRIVERS_NOTEDDR_CRASH_DUMP
  noteddr_crash_dump_register();
#endif
  init_kfifo();
  note_driver_register((FAR struct note_driver_s *)&g_noteddr_driver);
  return register_driver("/dev/note/ddr", &g_noteddr_fops, 0666,
                         &g_noteddr_driver);
}

/****************************************************************************
 * Name: noteddr_initialize
 *
 * Description:
 *   Register a serial driver at /dev/note/ram that can be used by an
 *   application to read data from the circular note buffer.
 *
 * Input Parameters:
 *  devpath: The path of the Noteddr device
 *  bufsize: The size of the circular buffer
 *  overwrite: The overwrite mode
 *
 * Returned Value:
 *   Zero on succress. A negated errno value is returned on a failure.
 *
 ****************************************************************************/

FAR struct note_driver_s *noteddr_initialize(FAR const char *devpath,
                                             size_t bufsize, bool overwrite)
{
  FAR struct noteddr_driver_s *drv;
  int ret;

  drv = kmm_malloc(sizeof(*drv) + bufsize);
  if (drv == NULL)
    {
      return NULL;
    }

  drv->driver.ops = &g_noteddr_ops;
  init_kfifo();
  ret = note_driver_register(&drv->driver);
  if (ret < 0)
    {
      kmm_free(drv);
      return NULL;
    }

  if (devpath == NULL)
    {
      return &drv->driver;
    }

  ret = register_driver(devpath, &g_noteddr_fops, 0666, drv);
  if (ret < 0)
    {
      kmm_free(drv);
      return NULL;
    }

  return &drv->driver;
}
