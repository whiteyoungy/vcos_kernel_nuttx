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

/* Including File */

#include <nuttx/config.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/hrtimer.h>
#include "hrtimer/hrtimer_queue.h"

static void systick_hrtimer_callback(struct hrtimer_s *hrtimer);

FAR __percpu_bss const struct hrtimer_ops_s *g_hrtimer_ops;
#define g_hrtimer_ops this_cpu_var(g_hrtimer_ops)

static FAR __percpu_data struct hrtimer_s g_systick_timer = {
	.expiration_time = 0,
	.queue = NULL,
	.callback = systick_hrtimer_callback,
};
#define g_systick_timer this_cpu_var(g_systick_timer)

static void systick_hrtimer_callback(struct hrtimer_s *hrtimer)
{
	hrtimer_reload(hrtimer, HRTIMER_USEC2TICKS(USEC_PER_TICK));
	nxsched_process_timer();
}

void up_hrtimer_set_ops(const struct hrtimer_ops_s *ops)
{
	g_hrtimer_ops = ops;
}

void hrtimer_init(struct hrtimer_queue_s *queue, struct hrtimer_s **heap, int queue_size,
		  const struct hrtimer_ops_s *ops)
{
	queue->heap = heap;
	queue->queue_size = queue_size;
	queue->size = 0;

	if (ops == NULL) {
		queue->ops = g_hrtimer_ops;
	} else {
		queue->ops = ops;
	}

	if ((g_systick_timer.queue == NULL) && (queue->ops == g_hrtimer_ops) && (queue->ops != NULL)) {
		g_systick_timer.queue = queue;
	}
}

void hrtimer_start(struct hrtimer_queue_s *queue)
{
	if (g_systick_timer.queue != NULL) {
		hrtimer_add_rel(&g_systick_timer, HRTIMER_USEC2TICKS(USEC_PER_TICK));
	}

	queue->ops->start(queue);
}
