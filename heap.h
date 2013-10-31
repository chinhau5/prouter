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

void init_heap(s_heap *heap);
void insert_to_heap(s_heap *heap, float cost, void *data);
bool get_heap_head(s_heap *heap, s_heap_item *output);

#endif /* HEAP_H_ */
