/*
 * quadtree.h
 *
 *  Created on: Nov 15, 2013
 *      Author: chinhau5
 */

#ifndef QUADTREE_H_
#define QUADTREE_H_

#include <glib.h>
#include "bounding_box.h"

enum { QUADTREE_TOPLEFT = 0, QUADTREE_TOPRIGHT, QUADTREE_BOTTOMLEFT, QUADTREE_BOTTOMRIGHT };

typedef struct _s_quad_tree {
	struct _s_bounding_box bounding_box;
	struct _s_quad_tree *nodes[4];
	GSList *objects;
	int num_objects;
	int level;
} s_quad_tree;

void quadtree_init(s_quad_tree *quad_tree, s_bounding_box *bounding_box, int level);
void quadtree_insert(s_quad_tree *quad_tree, void *object, s_bounding_box *object_bounding_box);
void quadtree_query(s_quad_tree *quad_tree, s_bounding_box *object_bounding_box, GSList **result);

#endif /* QUADTREE_H_ */
