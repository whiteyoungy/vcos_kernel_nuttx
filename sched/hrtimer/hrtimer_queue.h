
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

#include <nuttx/config.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/hrtimer.h>

#define MAX_HRTIMER_PROCESS_CNT 4u


static inline struct hrtimer_s *hrtimer_get_first(struct hrtimer_queue_s *queue)
{
	if (queue->size == 0) {
		return NULL;
	}
	return queue->heap[0];
}

static inline_function void hrtimer_swap(struct hrtimer_s **a, struct hrtimer_s **b)
{
	struct hrtimer_s *tmp = *a;
	*a = *b;
	*b = tmp;
}

static inline_function int hrtimer_queue_down(struct hrtimer_queue_s *queue, int start)
{
	int current_position = start;
	int left_child = 0;
	int right_child = 0;
	int smallest = 0;

	while (true) {
		left_child = 2 * current_position + 1;
		right_child = 2 * current_position + 2;
		smallest = current_position;

		if ((left_child < queue->size) && (hrtimer_has_expired(queue->heap[left_child]->expiration_time,
								       queue->heap[smallest]->expiration_time))) {
			smallest = left_child;
		}

		if ((right_child < queue->size) && (hrtimer_has_expired(queue->heap[right_child]->expiration_time,
									queue->heap[smallest]->expiration_time))) {
			smallest = right_child;
		}

		if (smallest != current_position) {
			hrtimer_swap(&queue->heap[current_position], &queue->heap[smallest]);
			current_position = smallest;
		} else {
			break;
		}
	}

	return current_position;
}

static inline_function int hrtimer_queue_up(struct hrtimer_queue_s *queue, int start)
{
	int current_position = start;
	int parent_position = 0;

	while (current_position > 0) {
		parent_position = (current_position - 1) / 2;

		if (hrtimer_has_expired(queue->heap[parent_position]->expiration_time,
					queue->heap[current_position]->expiration_time)) {
			break;
		}

		hrtimer_swap(&queue->heap[current_position], &queue->heap[parent_position]);
		current_position = parent_position;
	}

	return current_position;
}

static inline_function void hrtimer_pop(struct hrtimer_queue_s *queue)
{
	if (queue->size == 0) {
		return;
	}

	queue->heap[0] = queue->heap[--queue->size];

	(void)hrtimer_queue_down(queue, 0);
}

static inline_function void hrtimer_insert(struct hrtimer_s *hrtimer)
{
	struct hrtimer_queue_s *queue = hrtimer->queue;
	int current_position = 0;

	if (queue->size >= queue->queue_size) {
		return;
	}

	current_position = queue->size++;
	queue->heap[current_position] = hrtimer;

	(void)hrtimer_queue_up(queue, current_position);
}

static inline_function int hrtimer_remove(struct hrtimer_s *hrtimer)
{
	struct hrtimer_queue_s *queue = hrtimer->queue;
	int current_position = 0;
	int index = -1;
	int i = 0;

	if (queue->size == 0) {
		return ERROR;
	}

	for (i = 0; i < queue->size; i++) {
		if (queue->heap[i] == hrtimer) {
			index = i;
			break;
		}
	}

	if (index == -1) {
		return ERROR;
	}

	queue->heap[index] = queue->heap[--queue->size];
	current_position = index;

	current_position = hrtimer_queue_down(queue, current_position);
	(void)hrtimer_queue_up(queue, current_position);

	return OK;
}