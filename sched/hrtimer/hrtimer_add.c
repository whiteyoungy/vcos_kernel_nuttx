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

void hrtimer_add_abs(struct hrtimer_s *hrtimer, hrtimer_tick_t start)
{
	struct hrtimer_queue_s *queue = hrtimer->queue;

	hrtimer->expiration_time = start;
	hrtimer_insert(hrtimer);

	if (hrtimer == hrtimer_get_first(queue)) {
		hrtimer_set_expire(queue, start);
	}
}

void hrtimer_add_rel(struct hrtimer_s *hrtimer, hrtimer_tick_t increment)
{
	struct hrtimer_queue_s *queue = hrtimer->queue;

	hrtimer_tick_t now = queue->ops->current(queue);
	hrtimer_tick_t expiration_value = now + increment;

	hrtimer->expiration_time = expiration_value;
	hrtimer_insert(hrtimer);
	if (hrtimer == hrtimer_get_first(queue)) {
		hrtimer_set_expire(queue, expiration_value);
	}
}