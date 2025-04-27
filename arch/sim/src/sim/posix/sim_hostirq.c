/****************************************************************************
 * arch/sim/src/sim/posix/sim_hostirq.c
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

#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "sim_internal.h"
#include "sim_hostbmp.h"
/****************************************************************************
 * Public data
 ****************************************************************************/
#ifdef CONFIG_BMP
#define CONFIG_NR_CPUS CONFIG_BMP_NCPUS
#else
#define CONFIG_NR_CPUS CONFIG_SMP_NCPUS
#endif
volatile void *g_current_regs[CONFIG_NR_CPUS];
#ifdef CONFIG_SIM_IRQ_MANAGE
#define HASH_SIZE 1000
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

union sigset_u
{
  uint64_t flags;
  sigset_t sigset;
};

#ifdef CONFIG_SIM_IRQ_MANAGE
typedef struct message
{
  int isrid;
  int isrlevel;
  struct message *next;
} message;

typedef struct
{
  message *head;

  /* Insert Lock. */

  pthread_mutex_t lock;
  pthread_cond_t cond;
  int GlobalSchedstatus;

  /* Global Int Mask */

  uint32_t isrmask;
  int signal_handled;
} messagequeue;

struct data
{
  int irqlevel;
  int coreid;
  int enable_flag;
};

typedef struct node
{
  uint64_t key;
  struct data *value;
  struct node *next;
}hashnode;

typedef struct hashtable
{
  hashnode *table[HASH_SIZE];
}hashtable;

#endif /* CONFIG_SIM_IRQ_MANAGE */

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_SIM_IRQ_MANAGE
static hashtable g_hash_irq_table[CONFIG_NR_CPUS];
#endif /* CONFIG_SIM_IRQ_MANAGE */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_SIM_IRQ_MANAGE
static messagequeue g_queue[CONFIG_NR_CPUS];

static void initqueue(messagequeue *queue);
static void insertmessage(messagequeue *queue, int isrlevel, int isrid);
static message *retrievemessage(messagequeue *queue);
static void freemessage(message *message);
static void up_handle_irqmanage(int irq, siginfo_t *info, void *context);
static void sim_gicinit(void);
struct node *find_node(struct node *hash_table[], int hash_size, \
                     uint64_t key);
static void insert_node(struct node *hash_table[], int hash_size, \
    uint64_t key, int irqlevel, int coreid, int enable_flag);
#endif

#ifndef CONFIG_SIM_IRQ_MANAGE
/****************************************************************************
 * Name: up_handle_irq
 ****************************************************************************/

static void up_handle_irq(int irq, siginfo_t *info, void *context)
{
  sim_doirq(irq, context);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_irq_flags
 *
 * Description:
 *   Get the current irq flags
 *
 ****************************************************************************/

uint64_t up_irq_flags(void)
{
  union sigset_u omask;
  int ret;
  ret = pthread_sigmask(SIG_SETMASK, NULL, &omask.sigset);
  assert(ret == 0);

  return omask.flags;
}

/****************************************************************************
 * Name: up_irq_save
 *
 * Description:
 *   Disable interrupts and returned the mask before disabling them.
 *
 ****************************************************************************/

__attribute__((no_sanitize_address))
uint64_t up_irq_save(void)
{
  union sigset_u nmask;
  union sigset_u omask;
  int ret;

  sigfillset(&nmask.sigset);
  ret = pthread_sigmask(SIG_SETMASK, &nmask.sigset, &omask.sigset);
  assert(ret == 0);

  return omask.flags;
}

/****************************************************************************
 * Name: up_irq_disable
 *
 * Description:
 *   Disable interrupts and returned the mask before disabling them.
 *
 ****************************************************************************/

 __attribute__((no_sanitize_address))
 void up_irq_disable(void)
 {
   union sigset_u nmask;
   union sigset_u omask;
   int ret;

   sigfillset(&nmask.sigset);
   ret = pthread_sigmask(SIG_SETMASK, &nmask.sigset, &omask.sigset);
   assert(ret == 0);
 }

/****************************************************************************
 * Name: up_irq_restore
 *
 * Input Parameters:
 *   flags - the mask used to restore interrupts
 *
 * Description:
 *   Re-enable interrupts using the specified mask in flags argument.
 *
 ****************************************************************************/

void up_irq_restore(uint64_t flags)
{
  union sigset_u nmask;
  int ret;

  ret = sigemptyset(&nmask.sigset);
  assert(ret == 0);
  nmask.flags = flags;
  ret = pthread_sigmask(SIG_SETMASK, &nmask.sigset, NULL);
  assert(ret == 0);
}

/****************************************************************************
 * Name: up_irq_enable
 *
 * Description:
 *   Enable interrupts.
 *
 ****************************************************************************/

void up_irq_enable(void)
{
  up_irq_restore(0);
}

/****************************************************************************
 * Name: up_irqinitialize
 ****************************************************************************/

void up_irqinitialize(void)
{
#ifdef CONFIG_SMP
  /* Register the pause handler */

  sim_init_ipi(SIGUSR1);
  sim_init_func_call_ipi(SIGUSR2);
#elif defined CONFIG_SIM_IRQ_MANAGE
  sim_gicinit();
#endif
}

/****************************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   Enable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_enable_irq(int irq)
{
#ifndef CONFIG_SIM_IRQ_MANAGE
  struct sigaction act;
  sigset_t set;

  /* Register signal handler */

  memset(&act, 0, sizeof(act));
  act.sa_sigaction = up_handle_irq;
  act.sa_flags     = SA_SIGINFO;
  sigfillset(&act.sa_mask);
  sigaction(irq, &act, NULL);

  /* Unmask the signal */

  sigemptyset(&set);
  sigaddset(&set, irq);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
#else
  hashnode * nd = find_node(g_hash_irq_table[up_cpu_index()].table, \
                            HASH_SIZE, irq);
  if (nd)
    {
      nd->value->enable_flag = true;
    }
  else
    {
      insert_node(g_hash_irq_table[up_cpu_index()].table, HASH_SIZE, irq, \
                  0, up_cpu_index(), true);
    }
#endif
}

/****************************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_disable_irq(int irq)
{
  /* Since it's hard to mask the signal on all threads,
   * let's change the signal handler to ignore instead.
   */

#ifndef CONFIG_SIM_IRQ_MANAGE
  signal(irq, SIG_IGN);
#else
  hashnode * nd = find_node(g_hash_irq_table[up_cpu_index()].table, \
                            HASH_SIZE, irq);
  if (nd)
    {
    nd->value->enable_flag = false;
    }
  else
    {
      insert_node(g_hash_irq_table[up_cpu_index()].table, HASH_SIZE, \
                  irq, 0, up_cpu_index(), false);
    }
#endif
}

/****************************************************************************
 * Name: up_affinity_irq
 *
 * Description:
 *   Set an IRQ affinity by software. the cpuset will inherit irqhandler.
 *
 ****************************************************************************/

void up_affinity_irq(int irq, uint32_t cpuset)
{
#ifdef CONFIG_SIM_IRQ_MANAGE
  int coreid = 0;

  while (((cpuset >> coreid) & 1) == 0)
    {
      coreid++;
    }

  hashnode * nd = find_node(g_hash_irq_table[up_cpu_index()].table, \
                            HASH_SIZE, irq);
  if (nd)
    {
      nd->value->coreid = coreid;
    }
  else
    {
      insert_node(g_hash_irq_table[up_cpu_index()].table, HASH_SIZE, \
                  irq, 0, coreid, false);
    }
#endif
}

#ifdef CONFIG_SIM_IRQ_MANAGE

/****************************************************************************
 * Name: up_trigger_irq
 *
 * Description:
 *   Trigger an IRQ by software.
 *
 ****************************************************************************/

void up_trigger_irq(int irq, uint32_t cpuset)
{
  (void) cpuset;
  int coreid = 0;
  if (cpuset == 0)
    {
      sim_gic_raise_irq_bycoreid(irq, 0);
      return;
    }

  while (coreid < CONFIG_BMP_NCPUS)
    {
      /* if cpuset !=0, sim will trigger multicore's irq */

      if (((cpuset >> coreid) & 1) == 1)
        sim_gic_raise_irq_bycoreid(irq, coreid);
      coreid++;
    }
}

/****************************************************************************
 * Name: up_init_irq
 *
 * Description:
 *   Init the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_init_irq(int irq, int irq_prio)
{
  hashnode * nd = find_node(g_hash_irq_table[up_cpu_index()].table, \
                            HASH_SIZE, irq);
  if (nd)
    {
      nd->value->irqlevel = irq_prio;
    }
  else
    {
      insert_node(g_hash_irq_table[up_cpu_index()].table, HASH_SIZE, \
                  irq, irq_prio, up_cpu_index(), false);
    }
}

/****************************************************************************
 * Name: up_irq_is_enabled
 *
 * Description:
 *   Determine if an IRQ is enabled.
 *
 ****************************************************************************/

bool up_irq_is_enabled(int irq)
{
  hashnode * nd = find_node(g_hash_irq_table[up_cpu_index()].table, \
                            HASH_SIZE, irq);
  if (nd)
    {
      return nd->value->enable_flag;
    }
  else
    {
      return false;
    }
}

/****************************************************************************
 * Name: up_clear_irq
 *
 * Description:
 *   Clear the pending IRQ. No need clear.
 *
 ****************************************************************************/

void up_clear_irq(int irq)
{
}

/****************************************************************************
 * Name: up_get_irqlevel
 *
 * Description:
 *   return irq's level
 *
 ****************************************************************************/

uint32_t up_get_irqlevel(int irq, uint32_t coreid)
{
  hashnode * nd = find_node(g_hash_irq_table[coreid].table, \
                            HASH_SIZE, irq);
  if (nd)
    {
      return nd->value->irqlevel;
    }
  else
    {
      return 0;
    }
}

/****************************************************************************
 * Name: initqueue
 *
 * Description:
 *   Ininit queue
 *
 ****************************************************************************/

static void initqueue(messagequeue *que)
{
    que->head = NULL;
    int ret;
    ret = pthread_mutex_init(&que->lock, NULL);
    assert(ret == 0);
    ret = pthread_cond_init(&que->cond, NULL);
    assert(ret == 0);
    que->signal_handled = 0;
}

/****************************************************************************
 * Name: insertmessage
 *
 * Description:
 *   Insert a message into the queue, sorted by size
 *
 ****************************************************************************/

static void insertmessage(messagequeue *queue, int isrlevel, int isrid)
{
  pthread_mutex_lock(&queue->lock);
  int ret;

  message *newmessage = (message *)malloc(sizeof(message));
  newmessage->isrlevel = isrlevel;
  newmessage->isrid = isrid;
  newmessage->next = NULL;

  if (queue->head == NULL ||
#ifdef SIM_IRQ_PRIORITY_SMALL_IS_HIGH
    queue->head->isrlevel > isrlevel
#else
    queue->head->isrlevel < isrlevel
#endif
  )
    {
      newmessage->next = queue->head;
      queue->head = newmessage;
    }
  else
    {
      message *current2 = queue->head;
      while (current2->next != NULL &&
#ifdef SIM_IRQ_PRIORITY_SMALL_IS_HIGH
        current2->next->isrlevel <= isrlevel
#else
        current2->next->isrlevel >= isrlevel
#endif
      )
        {
          current2 = current2->next;
        }

      newmessage->next = current2->next;
      current2->next = newmessage;
    }

  ret = pthread_cond_signal(&queue->cond);
  assert(ret == 0);
  ret = pthread_mutex_unlock(&queue->lock);
  assert(ret == 0);
}

/****************************************************************************
 * Name: retrievemessage
 *
 * Description:
 *   Retrieve and remove the first message from the queue
 *
 ****************************************************************************/

static message *retrievemessage(messagequeue *queue)
{
  int ret;
  ret = pthread_mutex_lock(&queue->lock);
  assert(ret == 0);
  message *currentmessage = queue->head;
  if (currentmessage != NULL)
    {
      queue->head = currentmessage->next;
    }

  ret = pthread_mutex_unlock(&queue->lock);
  assert(ret == 0);
  return currentmessage;
}

/****************************************************************************
 * Name: freemessage
 *
 * Description:
 *   free message
 *
 ****************************************************************************/

static void freemessage(message *msg)
{
  free(msg);
}

/****************************************************************************
 * Name: sim_gicinit
 *
 * Description:
 *   gic manage init .
 *
 ****************************************************************************/

static void sim_gicinit(void)
{
  uint64_t coreid = up_cpu_index();
  struct sigaction act;
  sigset_t set;

  /* Register signal handler */

  memset(&act, 0, sizeof(act));
  act.sa_sigaction = up_handle_irqmanage;
  act.sa_flags     = SA_SIGINFO;
  sigfillset(&act.sa_mask);
  sigaction(SIGUSR1, &act, NULL);

  /* Unmask the signal */

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);

  messagequeue *queue_temp = &g_queue[coreid];
  initqueue(queue_temp);
}

/****************************************************************************
 * Name: up_handle_irqmanage
 *
 * Description:
 *   handle irq.
 *
 ****************************************************************************/

static void up_handle_irqmanage(int irq, siginfo_t *info, void *context)
{
  message *msg;
  uint64_t coreid = up_cpu_index();
  messagequeue *queue_temp = &g_queue[coreid];

  while (1)
    {
      msg = retrievemessage(queue_temp);
      if (msg)
        {
          if (up_irq_is_enabled(msg->isrid))
#ifdef CONFIG_SIM_IRQ_MANAGE_DISPATCH_BY_IRQID
            sim_doirq(msg->isrid, context);
#else
            sim_doirq(msg->isrlevel, context);
#endif
          freemessage(msg);
        }
      else
        {
          break;
        }
    }
}

/****************************************************************************
 * Name: hash_function
 *
 * Description:
 *   return hash mod value.
 *
 ****************************************************************************/

void sim_gic_raise_irq_bycoreid(uint32_t irq_number, uint32_t coreid)
{
  uint32_t irqlevel = 0;
  int ret;
  irqlevel = up_get_irqlevel(irq_number, coreid);
  uint64_t irq_flags = up_irq_save();
  insertmessage(&g_queue[coreid], irqlevel, irq_number);
  up_irq_restore(irq_flags);
  ret = pthread_kill(get_bmp_cpu_pthread(coreid), SIGUSR1);
  assert(ret == 0);
}

/****************************************************************************
 * Name: hash_function
 *
 * Description:
 *   return hash mod value.
 *
 ****************************************************************************/

int hash_function(uint64_t key)
{
  return key % HASH_SIZE;
}

/****************************************************************************
 * Name: find_node
 *
 * Description:
 *   find node in hashtable.if key exist ,will return node ptr.
 *
 ****************************************************************************/

struct node *find_node(struct node *hash_table[], int hash_size, \
             uint64_t key)
{
  int index = hash_function(key);
  struct node *node = hash_table[index];
  while (node != NULL)
    {
      if (node->key == key)
        {
          return node;
        }

      node = node->next;
    }

  return NULL;
}

/****************************************************************************
 * Name: insert_node
 *
 * Description:
 *   insert node in hashtable.if key exist ,will update data
 *
 ****************************************************************************/

static void insert_node(struct node *hash_table[], int hash_size, \
    uint64_t key, int irqlevel, int coreid, int enable_flag)
{
  int index = hash_function(key);
  struct node *node = find_node(hash_table, hash_size, key);
  if (node == NULL)
    {
      /* Key 不存在，插入新节点 */

      struct node *new_node = (struct node *)malloc(sizeof(struct node));
      new_node->key = key;
      new_node->value = (struct data *)malloc(sizeof(struct data));
      new_node->value->irqlevel = irqlevel;
      new_node->value->coreid = coreid;
      new_node->value->enable_flag = enable_flag;
      new_node->next = hash_table[index];
      hash_table[index] = new_node;
    }
  else
    {
      node->value->irqlevel = irqlevel;
      node->value->coreid = coreid;
      node->value->enable_flag = enable_flag;
    }
}

#endif /* CONFIG_SIM_IRQ_MANAGE */
