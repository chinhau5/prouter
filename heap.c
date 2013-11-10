/*
 * heap.c
 *
 *  Created on: Oct 24, 2013
 *      Author: chinhau5
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "heap.h"

void heap_init(s_heap *heap)
{
	heap->buffer = malloc(sizeof(s_heap_item) * INITIAL_HEAP_SIZE);
	heap->size = INITIAL_HEAP_SIZE;
	heap->tail = -1;
}

void heap_push(s_heap *heap, float cost, void *data)
{
	int current;
	int parent;
	s_heap_item temp;

	if (heap->tail >= heap->size) {
		heap->size *= 2;
		heap->buffer = realloc(heap->buffer, sizeof(s_heap_item) * heap->size);
	}

	current = ++heap->tail;
	heap->buffer[current].cost = cost;
	heap->buffer[current].data = data;

	while (current > 0) {
		parent = (current-1)/2;
		if (heap->buffer[current].cost < heap->buffer[parent].cost) {
			temp = heap->buffer[parent];
			heap->buffer[parent] = heap->buffer[current];
			heap->buffer[current] = temp;
		}
		current = parent;
	}
}

void *heap_pop(s_heap *heap)
{
	int current;
	int child;
	bool empty;
	bool done;
	s_heap_item temp;
	void *output;

	if (heap->tail < 0) {
		output = NULL;
	} else {
		output = heap->buffer[0].data;

		heap->buffer[0] = heap->buffer[heap->tail];
		heap->tail--;
		current = 0;
		done = false;
		while (current < heap->tail && !done) {
			child = 2*current+1;
			if (heap->buffer[current].cost < heap->buffer[child].cost) {
				child++;
			}
			if (heap->buffer[current].cost < heap->buffer[child].cost) {
				done = true;
			} else {
				temp = heap->buffer[current];
				heap->buffer[current] = heap->buffer[child];
				heap->buffer[child] = temp;
				current = child;
			}
		}
	}

	return output;
}

bool heap_is_empty(s_heap *heap)
{
	return heap->tail < 0;
}
