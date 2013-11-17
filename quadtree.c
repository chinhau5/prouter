/*
 * quadtree.c
 *
 *  Created on: Nov 15, 2013
 *      Author: chinhau5
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "quadtree.h"

void quadtree_init(s_quad_tree *quad_tree, s_bounding_box *bounding_box, int level)
{
	int i;
	quad_tree->bounding_box = *bounding_box;
	for (i = 0; i < 4; i++) {
		quad_tree->nodes[i] = NULL;
	}
	quad_tree->objects = NULL;
	quad_tree->level = level;
}

/* assuming uniform segmentation of nodes */
GSList *quadtree_get_object_node_index(s_quad_tree *quad_tree, s_bounding_box *object_bounding_box)
{
	//bool top_left, bottom_right, top_right, bottom_left;
	GSList *indices;
	int i;

	assert(quad_tree->nodes[0]);

	indices = NULL;
	for (i = 0; i < 4; i++) {
		if (aabb_intersect(object_bounding_box, &quad_tree->nodes[i]->bounding_box)) {
			indices = g_slist_prepend(indices, (gpointer)i);
		}
	}

//	top_left =
//	bottom_right = aabb_intersect(object_bounding_box, &quad_tree->nodes[QUADTREE_BOTTOMRIGHT]->bounding_box);
//	top_right = aabb_intersect(object_bounding_box, &quad_tree->nodes[QUADTREE_TOPRIGHT]->bounding_box);
//	bottom_left = aabb_intersect(object_bounding_box, &quad_tree->nodes[QUADTREE_BOTTOMLEFT]->bounding_box);
//
//	if (top_left && !bottom_right && !bottom_left && !top_right) {
//		index = QUADTREE_TOPLEFT;
//	} else if (!top_left && bottom_right && !bottom_left && !top_right) {
//		index = QUADTREE_BOTTOMRIGHT;
//	} else if (!top_left && !bottom_right && bottom_left && !top_right) {
//		index = QUADTREE_BOTTOMLEFT;
//	} else if (!top_left && !bottom_right && !bottom_left && top_right) {
//		index = QUADTREE_TOPRIGHT;
//	} else {
//		index = -1;
//	}

	return indices;
}

bool quadtree_partition(s_quad_tree *quad_tree)
{
	int i;
	bool can_partition;

	s_bounding_box partition_bounding_box;
	for (i = 0; i < 4; i++) {
		quad_tree->nodes[i] = malloc(sizeof(s_quad_tree));
	}

	if (abs(quad_tree->bounding_box.top - quad_tree->bounding_box.bottom) < 2 || abs(quad_tree->bounding_box.left - quad_tree->bounding_box.right) < 2) {
		can_partition = false;
	} else {
		can_partition = true;
	}

	if (can_partition) {
		partition_bounding_box.top = quad_tree->bounding_box.top;
		partition_bounding_box.bottom = (quad_tree->bounding_box.top + quad_tree->bounding_box.bottom) / 2;
		partition_bounding_box.left = quad_tree->bounding_box.left;
		partition_bounding_box.right = (quad_tree->bounding_box.left + quad_tree->bounding_box.right) / 2;
		quadtree_init(quad_tree->nodes[QUADTREE_TOPLEFT], &partition_bounding_box, quad_tree->level+1);

		partition_bounding_box.left = partition_bounding_box.right + 1;
		partition_bounding_box.right = quad_tree->bounding_box.right;
		quadtree_init(quad_tree->nodes[QUADTREE_TOPRIGHT], &partition_bounding_box, quad_tree->level+1);

		partition_bounding_box.top = partition_bounding_box.bottom-1;
		partition_bounding_box.bottom = quad_tree->bounding_box.bottom;
		partition_bounding_box.left = quad_tree->bounding_box.left;
		partition_bounding_box.right = (quad_tree->bounding_box.left + quad_tree->bounding_box.right) / 2;
		quadtree_init(quad_tree->nodes[QUADTREE_BOTTOMLEFT], &partition_bounding_box, quad_tree->level+1);

		partition_bounding_box.left = partition_bounding_box.right + 1;
		partition_bounding_box.right = quad_tree->bounding_box.right;
		quadtree_init(quad_tree->nodes[QUADTREE_BOTTOMRIGHT], &partition_bounding_box, quad_tree->level+1);
	}

	return can_partition;
}

void quadtree_insert(s_quad_tree *quad_tree, void *object, s_bounding_box *object_bounding_box)
{
	GSList *indices;

	if (quad_tree->nodes[0] || quadtree_partition(quad_tree)) { /* there are sub-partitions */
		indices = quadtree_get_object_node_index(quad_tree, object_bounding_box);
		if (g_slist_length(indices) > 1) { /* object overlaps more than one sub-partition */
			quad_tree->objects = g_slist_prepend(quad_tree->objects, object);
			quad_tree->num_objects++;
		} else { /* object fits into one sub-partition, recurse into that partition */
			quadtree_insert(quad_tree->nodes[(int)indices->data], object, object_bounding_box);
		}
	} else {
		quad_tree->objects = g_slist_prepend(quad_tree->objects, object);
		quad_tree->num_objects++;
	}
}

void quadtree_query(s_quad_tree *quad_tree, s_bounding_box *object_bounding_box, GSList **result)
{
	GSList *indices;
	GSList *indices_item;
	int i;
	s_bounding_box *box;
	GSList *objects_item;

	printf("quad[%d]: l=%d r=%d t=%d b=%d\n", quad_tree->level, quad_tree->bounding_box.left, quad_tree->bounding_box.right, quad_tree->bounding_box.top, quad_tree->bounding_box.bottom);

	if (quad_tree->nodes[0]) {
		indices = quadtree_get_object_node_index(quad_tree, object_bounding_box);
		if (g_slist_length(indices) > 1) { /* object overlaps more than one sub-partition */
			objects_item = quad_tree->objects;
			while (objects_item) {
				box = objects_item->data;
				printf("object: l=%d r=%d t=%d b=%d\n", box->left, box->right, box->top, box->bottom);

				*result = g_slist_prepend(*result, objects_item->data);
				objects_item = objects_item->next;
			}

			indices_item = indices;
			while (indices_item) {
				quadtree_query(quad_tree->nodes[(int)indices_item->data], object_bounding_box, result);
				indices_item = indices_item->next;
			}
		} else {
			quadtree_query(quad_tree->nodes[(int)indices->data], object_bounding_box, result);
		}
	} else {
		objects_item = quad_tree->objects;
		while (objects_item) {
			box = objects_item->data;
			printf("object base: l=%d r=%d t=%d b=%d\n", box->left, box->right, box->top, box->bottom);

			*result = g_slist_prepend(*result, objects_item->data);
			objects_item = objects_item->next;
		}
	}
}

void quadtree_test()
{
	s_quad_tree quad_tree;
	s_bounding_box *box;
	GSList *result;
	GSList *result_item;

	box = malloc(sizeof(s_bounding_box));
	box->left = 0;
	box->bottom = 0;
	box->right = 15;
	box->top = 15;
	quadtree_init(&quad_tree, box, 0);

	box = malloc(sizeof(s_bounding_box));
	box->left = 0; box->bottom = 0; box->right = 2; box->top = 15;
	quadtree_insert(&quad_tree, box, box);

	box = malloc(sizeof(s_bounding_box));
	box->left = 9; box->bottom = 2; box->right = 12; box->top = 3;
	quadtree_insert(&quad_tree, box, box);

	box = malloc(sizeof(s_bounding_box));
	box->left = 8;
		box->bottom = 0;
		box->right = 15;
		box->top = 15;
		result = NULL;
	quadtree_query(&quad_tree, box, &result);
	result_item = result;
	while (result_item) {
		box = result_item->data;
		printf("l=%d r=%d t=%d b=%d\n", box->left, box->right, box->top, box->bottom);
		result_item = result_item->next;
	}

}
