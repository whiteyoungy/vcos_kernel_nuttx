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

#ifndef __INCLUDE_NUTTX_TIMERS_HRTIMER_H
#define __INCLUDE_NUTTX_TIMERS_HRTIMER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef CONFIG_SYSTEM_TIME64
typedef uint64_t hrtimer_tick_t;
#define HRTIMER_MAX_VALUE (hrtimer_tick_t)(0xffffffffffffffff)
#else
typedef uint32_t hrtimer_tick_t;
#define HRTIMER_MAX_VALUE (hrtimer_tick_t)(0xffffffff)
#endif

#define HRTIMER_MAX_INCREMENT (HRTIMER_MAX_VALUE >> 1)

#define HRTIMER_DEFAULT_INCREMENT ((hrtimer_tick_t)0xffffffff >> 2)

#define HRTIMER_USEC2TICKS(us) (((hrtimer_tick_t)(us)) * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / USEC_PER_SEC)

struct hrtimer_s;
typedef void (*hrtimer_callback_t)(struct hrtimer_s *);
struct hrtimer_queue_s;
struct hrtimer_ops_s {
	void (*set_expire)(struct hrtimer_queue_s *, hrtimer_tick_t expiration_time);
	hrtimer_tick_t (*current)(struct hrtimer_queue_s *);
	void (*start)(struct hrtimer_queue_s *);
	void (*trigger)(struct hrtimer_queue_s *);
};

struct hrtimer_s {
	hrtimer_tick_t expiration_time;
	struct hrtimer_queue_s *queue;
	hrtimer_callback_t callback;
	void *arg;
};

struct hrtimer_queue_s {
	struct hrtimer_s **heap;
	const struct hrtimer_ops_s *ops;
	int size;
	int queue_size;
};

void up_hrtimer_set_ops(const struct hrtimer_ops_s *ops);

void hrtimer_init(struct hrtimer_queue_s *queue, struct hrtimer_s **heap, int queue_size, const struct hrtimer_ops_s *ops);

void hrtimer_start(struct hrtimer_queue_s *queue);

void hrtimer_process(struct hrtimer_queue_s *queue);

void hrtimer_add_abs(struct hrtimer_s *hrtimer, hrtimer_tick_t start);

void hrtimer_add_rel(struct hrtimer_s *hrtimer, hrtimer_tick_t increment);

void hrtimer_reload(struct hrtimer_s *hrtimer, hrtimer_tick_t time);

void hrtimer_delete(struct hrtimer_s *hrtimer);

static inline_function hrtimer_tick_t hrtimer_current_value(struct hrtimer_queue_s *queue)
{
	return queue->ops->current(queue);
}

static inline_function hrtimer_tick_t hrtimer_passed(hrtimer_tick_t current, hrtimer_tick_t before)
{
	if (current >= before) {
		return current - before;
	} else {
		return current - before + HRTIMER_MAX_VALUE;
	}
}


static inline_function bool hrtimer_has_expired(hrtimer_tick_t expiration_time, hrtimer_tick_t current_time)
{
	if (expiration_time > current_time) {
		return (expiration_time - current_time) > HRTIMER_MAX_INCREMENT;
	} else if (expiration_time < current_time) {
		return (current_time - expiration_time) <= HRTIMER_MAX_INCREMENT;
	} else {
		return true;
	}
}

static inline_function void hrtimer_set_expire(struct hrtimer_queue_s *queue, hrtimer_tick_t expiration_time)
{
	hrtimer_tick_t now;

	queue->ops->set_expire(queue, expiration_time);
	now = queue->ops->current(queue);

	if (hrtimer_has_expired(expiration_time, now)) {
		queue->ops->trigger(queue);
	}
}

static inline_function hrtimer_tick_t hrtimer_get_time(struct hrtimer_queue_s *queue)
{
	return queue->ops->current(queue);
}

#endif /* __INCLUDE_NUTTX_TIMERS_HRTIMER_H */