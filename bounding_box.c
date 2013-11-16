/*
 * bounding_box.c
 *
 *  Created on: Nov 15, 2013
 *      Author: chinhau5
 */

#include "bounding_box.h"

bool aabb_intersect(s_bounding_box *a, s_bounding_box *b)
{
	return a->left <= b->right && b->left <= a->right && a->bottom <= b->top && b->bottom <= a->top;
}
