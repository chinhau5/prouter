/*
 * placement.h
 *
 *  Created on: Nov 6, 2013
 *      Author: chinhau5
 */

#ifndef PLACEMENT_H_
#define PLACEMENT_H_

#include <glib.h>

void parse_placement(const char *filename, int *nx, int *ny, GHashTable **block_positions);
void alloc_and_init_grid(s_block ***grid, int nx, int ny, s_pb_top_type *pb_top_types, int num_pb_top_types, s_pb *pbs, int num_pbs, GHashTable *block_positions);

#endif /* PLACEMENT_H_ */
