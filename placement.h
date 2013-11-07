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

#endif /* PLACEMENT_H_ */
