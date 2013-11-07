/*
 * netlist.h
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#ifndef NETLIST_H_
#define NETLIST_H_

void parse_netlist(const char *filename, t_block **grid, GHashTable *block_positions, s_pb_top_type *pb_top_types, int num_pb_top_types,
		int *num_blocks, GHashTable **external_nets);

#endif /* NETLIST_H_ */
