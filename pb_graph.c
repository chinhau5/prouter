/*
 * pb_graph.c
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#include "pb_graph.h"

void alloc_and_init_pb_graph_pins(s_pb_graph_node *node, s_physical_block *pb)
{
	int i, j;

	node->input_pins = malloc(sizeof(s_pb_graph_pin *) * pb->num_input_ports);
	for (i = 0; i < pb->num_input_ports; i++) {
		node->input_pins[i] = malloc(sizeof(s_pb_graph_pin) * pb->input_ports[i].num_pins);
	}

	node->output_pins = malloc(sizeof(s_pb_graph_pin *) * pb->num_output_ports);
	for (i = 0; i < pb->num_output_ports; i++) {
		node->output_pins[i] = malloc(sizeof(s_pb_graph_pin) * pb->output_ports[i].num_pins);
	}

	for (i = 0; i < pb->num_input_ports; i++) {
		//node->input_pins[i][j].
	}
}

void alloc_pb_graph_children(s_pb_graph_node *node, s_physical_block *pb)
{
	int i, j;

	node->children = malloc(sizeof(s_pb_graph_node **) * pb->num_modes);
	for (i = 0; i < pb->num_modes; i++) {
		node->children[i] = malloc(sizeof(s_pb_graph_node *) * pb->modes[i].num_children);
		for (j = 0; j < pb->modes[i].num_children; j++) {
			node->children[i][j] = malloc(sizeof(s_pb_graph_node) * pb->modes[i].children[j].num_pbs);
		}
	}
}

void build_pb_graph(s_pb_graph_node *node, s_physical_block *pb)
{
	int i, j, k;

	alloc_and_init_pb_graph_pins(node, pb);

	alloc_pb_graph_children(node, pb);
	for (i = 0; i < pb->num_modes; i++) {
		for (j = 0; j < pb->modes[i].num_children; j++) {
			for (k = 0; k < pb->modes[i].children[j].num_pbs; k++) {
				build_pb_graph(&node->children[i][j][k], pb);
			}
		}
	}
}
