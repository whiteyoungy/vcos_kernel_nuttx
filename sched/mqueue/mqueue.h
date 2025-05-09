/****************************************************************************
 * sched/mqueue/mqueue.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __SCHED_MQUEUE_MQUEUE_H
#define __SCHED_MQUEUE_MQUEUE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <mqueue.h>
#include <sched.h>

#include <nuttx/mqueue.h>

#if defined(CONFIG_MQ_MAXMSGSIZE) && CONFIG_MQ_MAXMSGSIZE > 0

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MQ_MAX_BYTES   CONFIG_MQ_MAXMSGSIZE
#define MQ_MAX_MSGS    16
#define MQ_PRIO_MAX    _POSIX_MQ_PRIO_MAX

#define MQ_MSG_SIZE(n) (sizeof(struct mqueue_msg_s) + (n) - 1)

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

enum mqalloc_e
{
  MQ_ALLOC_FIXED = 0,  /* Pre-allocated; never freed */
  MQ_ALLOC_DYN,        /* Dynamically allocated; free when unused */
  MQ_ALLOC_IRQ         /* Preallocated, reserved for interrupt handling */
};

/* This structure describes one buffered POSIX message. */

struct mqueue_msg_s
{
  struct list_node node;   /* Link node to message */
  uint8_t type;            /* (Used to manage allocations) */
  uint8_t priority;        /* Priority of message */
#if MQ_MAX_BYTES < 256
  uint8_t msglen;          /* Message data length */
#else
  uint16_t msglen;         /* Message data length */
#endif
  char mail[1];            /* Message data */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* The g_msgfree is a list of messages that are available for general use.
 * The number of messages in this list is a system configuration item.
 */

EXTERN struct list_node g_msgfree;
#define g_msgfree       this_cpu_var(g_msgfree)

/* The g_msgfreeirq is a list of messages that are reserved for use by
 * interrupt handlers.
 */

EXTERN struct list_node g_msgfreeirq;
#define g_msgfreeirq    this_cpu_var(g_msgfreeirq)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct tcb_s;        /* Forward reference */
struct task_group_s; /* Forward reference */

/* Functions defined in mq_initialize.c *************************************/

void nxmq_initialize(void);

/* mq_msgfree.c *************************************************************/

void nxmq_free_msg(FAR struct mqueue_msg_s *mqmsg);

/* mq_waitirq.c *************************************************************/

void nxmq_wait_irq(FAR struct tcb_s *wtcb, int errcode);

/* mq_rcvinternal.c *********************************************************/

int nxmq_wait_receive(FAR struct mqueue_inode_s *msgq,
                      FAR struct mqueue_msg_s **rcvmsg,
                      FAR const struct timespec *abstime,
                      sclock_t ticks);
void nxmq_notify_receive(FAR struct mqueue_inode_s *msgq);

/* mq_sndinternal.c *********************************************************/

int nxmq_wait_send(FAR struct mqueue_inode_s *msgq,
                   FAR const struct timespec *abstime,
                   sclock_t ticks);
void nxmq_notify_send(FAR struct mqueue_inode_s *msgq);

/* mq_recover.c *************************************************************/

void nxmq_recover(FAR struct tcb_s *tcb);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* defined(CONFIG_MQ_MAXMSGSIZE) && CONFIG_MQ_MAXMSGSIZE > 0 */
#endif /* __SCHED_MQUEUE_MQUEUE_H */
