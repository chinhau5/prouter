/*
 * placement.h
 *
 *  Created on: Nov 6, 2013
 *      Author: chinhau5
 */

#ifndef PLACEMENT_H_
#define PLACEMENT_H_

#include <glib.h>

void parse_placement(const char *filename, s_pb_top_type *pb_top_types, int num_pb_top_types, int *nx, int *ny, t_block ***grid, GHashTable **block_positions);

#endif /* PLACEMENT_H_ */
