/*
 * rr_graph.h
 *
 *  Created on: Oct 19, 2013
 *      Author: chinhau5
 */

#ifndef RR_GRAPH_H_
#define RR_GRAPH_H_

void init_block_wires(s_block **grid, int nx, int ny, s_wire_type *wire_types, int num_wire_types, int num_wires_per_clb, int *global_routing_node_id, GHashTable *id_to_node);

#endif /* RR_GRAPH_H_ */
