/****************************************************************************
 * arch/sim/src/sim/posix/sim_hostbmp.c
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
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include "sim_hostbmp.h"
#include "sim_internal.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint64_t coreid_info[CONFIG_BMP_NCPUS];
static pthread_attr_t thread_attributes[CONFIG_BMP_NCPUS];
static pthread_t cpu_pthread[CONFIG_BMP_NCPUS];
static pthread_attr_t timer_thread_attr[CONFIG_BMP_NCPUS];
static pthread_t timer_pthread[CONFIG_BMP_NCPUS];
extern unsigned long g_cpu_data_size ;
pthread_key_t g_cpu_key;

static void timer_thread_setup(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void *cpu_start(void *arg)
{
  char thread_name[16];
  uint64_t coreid = *((uint64_t *)(arg));
  pthread_setspecific(g_cpu_key, (const void *)coreid_info[coreid]);
  sprintf(thread_name, "cpu[%ld]", coreid);
  pthread_setname_np(pthread_self(), thread_name);
  timer_thread_setup();
  extern void nx_start(void);
  nx_start();
  return NULL;
}

void bmp_start(void)
{
  int iret;
  int ret;
  ret = pthread_key_create(&g_cpu_key, NULL);
  if (ret != 0)
    {
      return;
    }

  pthread_setspecific(g_cpu_key, (const void *)(coreid_info[0]));
  for (uint32_t i = 0; i < CONFIG_BMP_NCPUS; i++)
    {
      /* create core cnt temp thread */

      coreid_info[i] = i;
      iret = pthread_create(&cpu_pthread[i], &thread_attributes[i], \
                            cpu_start, &(coreid_info[i]));
      if (iret != 0)
        {
          printf("cpu[%d] pthread_create failed \n", i);
        }
    }

  while (1)
    {
    };
}

pthread_t get_bmp_cpu_pthread(uint64_t coreid)
{
  return cpu_pthread[coreid];
}

/****************************************************************************
 * Name: timer_thread
 ****************************************************************************/

static void *timer_thread(void *arg)
{
  uint32_t coreid = *((uint32_t *)(arg));
  char thread_name[32];
  sprintf(thread_name, "timer_thread[%d]", coreid);
#ifdef __APPLE__
  pthread_setname_np(thread_name);
#else
  pthread_setname_np(pthread_self(), thread_name);
#endif
  pthread_setspecific(g_cpu_key, (const void *)(coreid_info[coreid]));
  while (1)
    {
      if (up_irq_is_enabled(up_get_systick_irqbase()))
        {
          sim_gic_raise_irq_bycoreid(up_get_systick_irqbase(), coreid);
        }

      usleep(CONFIG_USEC_PER_TICK);
    }

  return NULL;
}

/****************************************************************************
 * Name: timer_thread_setup
 ****************************************************************************/

static void timer_thread_setup(void)
{
  int iret;
  uint32_t coreid = up_cpu_index();

  /* create core cnt temp thread */

  iret = pthread_create(&timer_pthread[coreid], &timer_thread_attr[coreid], \
                        timer_thread, &(coreid_info[coreid]));
  if (iret != 0)
    {
      printf("pthread_create\n");
    }
}

/****************************************************************************
 * Name: up_cpu_index
 *
 * Description:
 *   Return the real core number regardless CONFIG_SMP setting
 *
 ****************************************************************************/

 #ifdef CONFIG_ARCH_HAVE_MULTICPU
int up_cpu_index(void)
{
  void *value = pthread_getspecific(g_cpu_key);
  return (int)((uintptr_t)value);
}
 #endif
