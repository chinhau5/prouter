/*
 * netlist.h
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#ifndef NETLIST_H_
#define NETLIST_H_

t_block *parse_netlist(const char *filename, int *num_blocks, s_physical_block *types, int num_types);

#endif /* NETLIST_H_ */
