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

void hrtimer_process(struct hrtimer_queue_s *queue)
{
	struct hrtimer_s *hrtimer;
	hrtimer_tick_t now, expiration_value;
	uint8_t process_cnt = 0u;

	irqstate_t irq_state = up_irq_save();
	hrtimer = hrtimer_get_first(queue);
	while (NULL != hrtimer) {
		now = queue->ops->current(queue);
		if (hrtimer_has_expired(hrtimer->expiration_time, now)) {
			hrtimer_pop(queue);
			hrtimer->callback(hrtimer);
			process_cnt++;
			if (process_cnt >= MAX_HRTIMER_PROCESS_CNT) {
				process_cnt = 0u;
				up_irq_restore(irq_state);
				irq_state = up_irq_save();
			}
		} else {
			break;
		}
		hrtimer = hrtimer_get_first(queue);
	}

	if (NULL != hrtimer) {
		hrtimer_set_expire(queue, hrtimer->expiration_time);
	} else {
		now = queue->ops->current(queue);
		expiration_value = now + HRTIMER_DEFAULT_INCREMENT;
		hrtimer_set_expire(queue, expiration_value);
	}
	up_irq_restore(irq_state);
}