/*
 * quadtree.c
 *
 *  Created on: Nov 15, 2013
 *      Author: chinhau5
 */

#include <stdlib.h>
#include <assert.h>
#include "quadtree.h"

void quadtree_init(s_quad_tree *quad_tree, s_bounding_box *bounding_box, int max_num_objects)
{
	int i;
	quad_tree->bounding_box = *bounding_box;
	for (i = 0; i < 4; i++) {
		quad_tree->nodes[i] = NULL;
	}
	quad_tree->objects = NULL;
	quad_tree->max_num_objects = max_num_objects;
}

/* assuming uniform segmentation of nodes */
int quadtree_get_object_node_index(s_quad_tree *quad_tree, s_bounding_box *object_bounding_box)
{
	bool top_left, bottom_right, top_right, bottom_left;
	int index;

	assert(quad_tree->nodes[0]);

	top_left = aabb_intersect(object_bounding_box, &quad_tree->nodes[QUADTREE_TOPLEFT]->bounding_box);
	bottom_right = aabb_intersect(object_bounding_box, &quad_tree->nodes[QUADTREE_BOTTOMRIGHT]->bounding_box);
	top_right = aabb_intersect(object_bounding_box, &quad_tree->nodes[QUADTREE_TOPRIGHT]->bounding_box);
	bottom_left = aabb_intersect(object_bounding_box, &quad_tree->nodes[QUADTREE_BOTTOMLEFT]->bounding_box);

	if (top_left && !bottom_right && !bottom_left && !top_right) {
		index = QUADTREE_TOPLEFT;
	} else if (!top_left && bottom_right && !bottom_left && !top_right) {
		index = QUADTREE_BOTTOMRIGHT;
	} else if (!top_left && !bottom_right && bottom_left && !top_right) {
		index = QUADTREE_BOTTOMLEFT;
	} else if (!top_left && !bottom_right && !bottom_left && top_right) {
		index = QUADTREE_TOPRIGHT;
	} else {
		index = -1;
	}

	return index;
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
		quadtree_init(quad_tree->nodes[QUADTREE_TOPLEFT], &partition_bounding_box, quad_tree->max_num_objects);

		partition_bounding_box.left = partition_bounding_box.right + 1;
		partition_bounding_box.right = quad_tree->bounding_box.right;
		quadtree_init(quad_tree->nodes[QUADTREE_TOPRIGHT], &partition_bounding_box, quad_tree->max_num_objects);

		partition_bounding_box.top = partition_bounding_box.bottom-1;
		partition_bounding_box.bottom = quad_tree->bounding_box.bottom;
		partition_bounding_box.left = quad_tree->bounding_box.left;
		partition_bounding_box.right = (quad_tree->bounding_box.left + quad_tree->bounding_box.right) / 2;
		quadtree_init(quad_tree->nodes[QUADTREE_BOTTOMLEFT], &partition_bounding_box, quad_tree->max_num_objects);

		partition_bounding_box.left = partition_bounding_box.right + 1;
		partition_bounding_box.right = quad_tree->bounding_box.right;
		quadtree_init(quad_tree->nodes[QUADTREE_BOTTOMRIGHT], &partition_bounding_box, quad_tree->max_num_objects);
	}

	return can_partition;
}

bool quadtree_insert(s_quad_tree *quad_tree, void *object, s_bounding_box *object_bounding_box)
{
	int index;
	int i;

	if (quad_tree->nodes[0]) { /* there are sub-partitions */
		index = quadtree_get_object_node_index(quad_tree, object_bounding_box);
		if (index == -1) { /* object overlaps more than one sub-partition */
			if (quad_tree->num_objects >= quad_tree->max_num_objects) {

			}
		} else {
			quadtree_insert(quad_tree->nodes[index], object, object_bounding_box);
		}
	} else {
		if (quad_tree->num_objects >= quad_tree->max_num_objects) {
			if ()
		}
		if (quadtree_partition(s_quad_tree *quad_tree)) { /* managed to create sub-partitions */

		} else { /* just add object to the current node */
			quad_tree->objects = g_slist_prepend(quad_tree->objects, object);
		}
	}
}
