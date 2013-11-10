/*
 * heap.h
 *
 *  Created on: Oct 24, 2013
 *      Author: chinhau5
 */

#ifndef HEAP_H_
#define HEAP_H_

typedef struct _s_heap_item {
	void *data;
	float cost;
} s_heap_item;

typedef struct _s_heap {
	struct _s_heap_item *buffer;
	int size;
	int tail;
} s_heap;

#define INITIAL_HEAP_SIZE 50

void heap_init(s_heap *heap);
void heap_push(s_heap *heap, float cost, void *data);
void *heap_pop(s_heap *heap);
bool heap_is_empty(s_heap *heap);

#endif /* HEAP_H_ */
