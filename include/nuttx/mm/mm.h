/****************************************************************************
 * include/nuttx/mm/mm.h
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

#ifndef __INCLUDE_NUTTX_MM_MM_H
#define __INCLUDE_NUTTX_MM_MM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/userspace.h>

#include <sys/types.h>
#include <stdbool.h>
#include <malloc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#if CONFIG_MM_HEAP_MEMPOOL_THRESHOLD >= 0
#  define CONFIG_MM_HEAP_MEMPOOL
#endif

/* If the MCU has a small (16-bit) address capability, then we will use
 * a smaller chunk header that contains 16-bit size/offset information.
 * We will also use the smaller header on MCUs with wider addresses if
 * CONFIG_MM_SMALL is selected.  This configuration is common with MCUs
 * that have a large FLASH space, but only a tiny internal SRAM.
 */

#ifdef CONFIG_SMALL_MEMORY
  /* If the MCU has a small addressing capability, then force the smaller
   * chunk header.
   */

#  undef  CONFIG_MM_SMALL
#  define CONFIG_MM_SMALL 1
#endif

/* Terminology:
 *
 * - Flat Build: In the flat build (CONFIG_BUILD_FLAT=y), there is only a
 *   single heap access with the standard allocations (malloc/free).  This
 *   heap is referred to as the user heap.  The kernel logic must
 *   initialize this single heap at boot time.
 * - Protected build: In the protected build (CONFIG_BUILD_PROTECTED=y)
 *   where an MPU is used to protect a region of otherwise flat memory,
 *   there will be two allocators:  One that allocates protected (kernel)
 *   memory and one that allocates unprotected (user) memory.  These are
 *   referred to as the kernel and user heaps, respectively.  Both must be
 *   initialized by the kernel logic at boot time.
 * - Kernel Build: If the architecture has an MMU, then it may support the
 *   kernel build (CONFIG_BUILD_KERNEL=y).  In this configuration, there
 *   is one kernel heap but multiple user heaps:  One per task group.
 *   However, in this case, the kernel need only be concerned about
 *   initializing the single kernel heap here.  User heaps will be created
 *   as tasks are created.
 *
 * These special definitions are provided:
 *
 *   MM_KERNEL_USRHEAP_INIT
 *     Special kernel interfaces to the kernel user-heap are required
 *     for heap initialization.
 *   CONFIG_MM_KERNEL_HEAP
 *     The configuration requires a kernel heap that must initialized
 *     at boot-up.
 */

#undef MM_KERNEL_USRHEAP_INIT
#if !defined(CONFIG_BUILD_KERNEL) && defined(__KERNEL__)
#  define MM_KERNEL_USRHEAP_INIT 1
#endif

/* When building the Userspace image under CONFIG_BUILD_KERNEL or
 * CONFIG_BUILD_PROTECTED (i.e. !defined(__KERNEL__)), CONFIG_MM_KERNEL_HEAP
 * must be undefined to ensure the kernel heap is never accessible from user
 * code.
 */

#if defined(CONFIG_BUILD_KERNEL) || defined(CONFIG_BUILD_PROTECTED)
#  ifndef __KERNEL__
#    undef CONFIG_MM_KERNEL_HEAP
#  endif
#endif

#define mm_memdump_s malltask

#if defined(CONFIG_ARCH_ADDRENV) && defined(CONFIG_BUILD_KERNEL)
/* In the kernel build, there are multiple user heaps; one for each task
 * group.  In this build configuration, the user heap structure lies
 * in a reserved region at the beginning of the .bss/.data address
 * space (CONFIG_ARCH_DATA_VBASE).  The size of that region is given by
 * ARCH_DATA_RESERVE_SIZE
 */

#  define USR_HEAP (ARCH_DATA_RESERVE->ar_usrheap)

#elif defined(CONFIG_BUILD_PROTECTED) && defined(__KERNEL__)
/* In the protected mode, there are two heaps:  A kernel heap and a single
 * user heap.  Kernel code must obtain the address of the user heap data
 * structure from the userspace interface.
 */

#  define USR_HEAP (*USERSPACE->us_heap)

#else
/* Otherwise, the user heap data structures are in common .bss */

#  define USR_HEAP g_mmheap
#endif

#ifdef CONFIG_MM_KERNEL_HEAP
#  define MM_INTERNAL_HEAP(heap) ((heap) == USR_HEAP || (heap) == g_kmmheap)
#else
#  define MM_INTERNAL_HEAP(heap) ((heap) == USR_HEAP)
#endif

#if CONFIG_MM_BACKTRACE >= 0
#  define MM_DUMP_ALLOC(dump, node) \
    ((node) != NULL && (dump)->pid == PID_MM_ALLOC && \
     (node)->pid != PID_MM_MEMPOOL)
#  define MM_DUMP_SEQNO(dump, node) \
    ((node)->seqno >= (dump)->seqmin && (node)->seqno <= (dump)->seqmax)
#  define MM_DUMP_ASSIGN(dump, node) \
    ((node) != NULL && (dump)->pid == (node)->pid)
#  define MM_DUMP_LEAK(dump, node) \
    ((node) != NULL && (dump)->pid == PID_MM_LEAK && (node)->pid >= 0 && \
     nxsched_get_tcb((node)->pid) == NULL)
#else
#  define MM_DUMP_ALLOC(dump,node)  ((dump)->pid == PID_MM_ALLOC)
#  define MM_DUMP_SEQNO(dump,node)  (true)
#  define MM_DUMP_ASSIGN(dump,node) (false)
#  define MM_DUMP_LEAK(dump,pid)    (false)
#endif

#define MM_INIT_MAGIC    0xcc
#define MM_ALLOC_MAGIC   0xaa
#define MM_FREE_MAGIC    0x55

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct mm_heap_s; /* Forward reference */

struct mempool_init_s
{
  FAR const size_t *poolsize;
  size_t            npools;
  size_t            threshold;
  size_t            chunksize;
  size_t            expandsize;
  size_t            dict_expendsize;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#if CONFIG_MM_BACKTRACE >= 0
extern unsigned long g_mm_seqno;
#endif

/* User heap structure:
 *
 * - Flat build:  In the FLAT build, the user heap structure is a globally
 *   accessible variable.
 * - Protected build:  The user heap structure is directly available only
 *   in user space.
 * - Kernel build: There are multiple heaps, one per process.  The heap
 *   structure is associated with the address environment and there is
 *   no global user heap structure.
 */

/* In the kernel build, there a multiple user heaps; one for each task
 * group.  In this build configuration, the user heap structure lies
 * in a reserved region at the beginning of the .bss/.data address
 * space (CONFIG_ARCH_DATA_VBASE).  The size of that region is given by
 * ARCH_DATA_RESERVE_SIZE
 */

/* In the protected mode, there are two heaps:  A kernel heap and a single
 * user heap.  In that case the user heap structure lies in the user space
 * (with a reference in the userspace interface).
 */

#if defined(CONFIG_BUILD_FLAT) || !defined(__KERNEL__)
/* Otherwise, the user heap data structures are in common .bss */

EXTERN FAR __percpu_bss struct mm_heap_s *g_mmheap;
#define g_mmheap this_cpu_var(g_mmheap)
#endif

#ifdef CONFIG_MM_KERNEL_HEAP
/* This is the kernel heap */

EXTERN FAR struct mm_heap_s *g_kmmheap;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Functions contained in mm_initialize.c ***********************************/

FAR struct mm_heap_s *mm_initialize(FAR const char *name,
                                    FAR void *heap_start, size_t heap_size);

#ifdef CONFIG_MM_HEAP_MEMPOOL
FAR struct mm_heap_s *
mm_initialize_pool(FAR const char *name,
                   FAR void *heap_start, size_t heap_size,
                   FAR const struct mempool_init_s *init);

#else
#  define mm_initialize_pool(name, heap_start, heap_size, init) \
          mm_initialize(name, heap_start, heap_size)
#endif

void mm_addregion(FAR struct mm_heap_s *heap, FAR void *heapstart,
                  size_t heapsize);
void mm_uninitialize(FAR struct mm_heap_s *heap);

/* Functions contained in umm_initialize.c **********************************/

void umm_initialize(FAR void *heap_start, size_t heap_size);

/* Functions contained in kmm_initialize.c **********************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_initialize(FAR void *heap_start, size_t heap_size);
#endif

/* Functions contained in umm_addregion.c ***********************************/

void umm_addregion(FAR void *heapstart, size_t heapsize);

/* Functions contained in kmm_addregion.c ***********************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_addregion(FAR void *heapstart, size_t heapsize);
#endif

/* Functions contained in mm_malloc.c ***************************************/

FAR void *mm_malloc(FAR struct mm_heap_s *heap, size_t size) malloc_like1(2);

void mm_free_delaylist(FAR struct mm_heap_s *heap);

/* Functions contained in kmm_malloc.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_malloc(size_t size) malloc_like1(1);
#endif

/* Functions contained in mm_malloc_size.c **********************************/

size_t mm_malloc_size(FAR struct mm_heap_s *heap, FAR void *mem);

/* Functions contained in kmm_malloc_size.c *********************************/

#ifdef CONFIG_MM_KERNEL_HEAP
size_t kmm_malloc_size(FAR void *mem);
#endif

/* Functions contained in mm_free.c *****************************************/

void mm_free(FAR struct mm_heap_s *heap, FAR void *mem);

/* Functions contained in kmm_free.c ****************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_free(FAR void *mem);
#endif

/* Functions contained in mm_realloc.c **************************************/

FAR void *mm_realloc(FAR struct mm_heap_s *heap, FAR void *oldmem,
                     size_t size) realloc_like(3);

/* Functions contained in kmm_realloc.c *************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_realloc(FAR void *oldmem, size_t newsize) realloc_like(2);
#endif

/* Functions contained in mm_calloc.c ***************************************/

FAR void *mm_calloc(FAR struct mm_heap_s *heap, size_t n,
                    size_t elem_size) malloc_like2(2, 3);

/* Functions contained in kmm_calloc.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_calloc(size_t n, size_t elem_size) malloc_like2(1, 2);
#endif

/* Functions contained in mm_zalloc.c ***************************************/

FAR void *mm_zalloc(FAR struct mm_heap_s *heap, size_t size) malloc_like1(2);

/* Functions contained in kmm_zalloc.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_zalloc(size_t size) malloc_like1(1);
#endif

/* Functions contained in kmm_memdump.c *************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_memdump(FAR const struct mm_memdump_s *dump);
#endif

/* Functions contained in mm_memalign.c *************************************/

FAR void *mm_memalign(FAR struct mm_heap_s *heap, size_t alignment,
                      size_t size) malloc_like1(3);

/* Functions contained in kmm_memalign.c ************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_memalign(size_t alignment, size_t size) malloc_like1(2);
#endif

/* Functions contained in mm_heapmember.c ***********************************/

bool mm_heapmember(FAR struct mm_heap_s *heap, FAR void *mem);

/* Functions contained in mm_uheapmember.c **********************************/

bool umm_heapmember(FAR void *mem);

/* Functions contained in kmm_heapmember.c **********************************/

#ifdef CONFIG_MM_KERNEL_HEAP
bool kmm_heapmember(FAR void *mem);
#endif

/* Functions contained in mm_brkaddr.c **************************************/

FAR void *mm_brkaddr(FAR struct mm_heap_s *heap, int region);

/* Functions contained in umm_brkaddr.c *************************************/

FAR void *umm_brkaddr(int region);

/* Functions contained in kmm_brkaddr.c *************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_brkaddr(int region);
#endif

/* Functions contained in mm_extend.c ***************************************/

void mm_extend(FAR struct mm_heap_s *heap, FAR void *mem, size_t size,
               int region);

/* Functions contained in umm_extend.c **************************************/

void umm_extend(FAR void *mem, size_t size, int region);

/* Functions contained in kmm_extend.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_extend(FAR void *mem, size_t size, int region);
#endif

/* Functions contained in mm_mallinfo.c *************************************/

struct mallinfo mm_mallinfo(FAR struct mm_heap_s *heap);
struct mallinfo_task mm_mallinfo_task(FAR struct mm_heap_s *heap,
                                      FAR const struct malltask *task);

size_t mm_heapfree(FAR struct mm_heap_s *heap);
size_t mm_heapfree_largest(FAR struct mm_heap_s *heap);

/* Functions contained in kmm_mallinfo.c ************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
struct mallinfo kmm_mallinfo(void);
#  if CONFIG_MM_BACKTRACE >= 0
struct mallinfo_task kmm_mallinfo_task(FAR const struct malltask *task);
#  endif
#endif

/* Functions contained in mm_memdump.c **************************************/

void mm_memdump(FAR struct mm_heap_s *heap,
                FAR const struct mm_memdump_s *dump);

/* Functions contained in umm_memdump.c *************************************/

void umm_memdump(FAR const struct mm_memdump_s *dump);

#ifdef CONFIG_DEBUG_MM
/* Functions contained in mm_checkcorruption.c ******************************/

void mm_checkcorruption(FAR struct mm_heap_s *heap);

/* Functions contained in umm_checkcorruption.c *****************************/

FAR void umm_checkcorruption(void);

/* Functions contained in kmm_checkcorruption.c *****************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void kmm_checkcorruption(void);
#else
#define kmm_checkcorruption()  umm_checkcorruption()
#endif

#else /* CONFIG_DEBUG_MM */

#define mm_checkcorruption(h)
#define umm_checkcorruption()
#define kmm_checkcorruption()

#endif /* CONFIG_DEBUG_MM */

/* Functions contained in fs_procfspressure.c *******************************/

#ifdef CONFIG_FS_PROCFS_INCLUDE_PRESSURE
void mm_notify_pressure(size_t remaining, size_t largest);
#else
#  define mm_notify_pressure(remaining, largest)
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_MM_MM_H */
