/*
 * pb_graph.c
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#include "pb_graph.h"
#include "helper.h"

void alloc_and_init_pb_graph_pins(s_pb_graph_node *node)
{
	int i, j;
	s_pb_type *pb_type;

	pb_type = node->type;

	node->input_pins = malloc(sizeof(s_pb_graph_pin *) * pb_type->num_input_ports);
	for (i = 0; i < pb_type->num_input_ports; i++) {
		node->input_pins[i] = malloc(sizeof(s_pb_graph_pin) * pb_type->input_ports[i].num_pins);
	}

	node->output_pins = malloc(sizeof(s_pb_graph_pin *) * pb_type->num_output_ports);
	for (i = 0; i < pb_type->num_output_ports; i++) {
		node->output_pins[i] = malloc(sizeof(s_pb_graph_pin) * pb_type->output_ports[i].num_pins);
	}

	/* connect all pb_type->modes to pb_type->input_ports and pb_type->output_ports */
	for (i = 0; i < pb_type->num_modes; i++) {
		for (j = 0; j < pb_type->modes[i].num_interconnects; j++) {
			tokenize(pb_type->modes[i].interconnects[j].input_string, ".");
		}
	}

	for (i = 0; i < pb_type->num_input_ports; i++) {
		node->input_pins[i][j].edges
	}
}

void alloc_pb_graph_children(s_pb_graph_node *node)
{
	int i, j;
	s_pb_type *pb_type;

	pb_type = node->type;

	node->children = malloc(sizeof(s_pb_graph_node **) * pb_type->num_modes);
	for (i = 0; i < pb_type->num_modes; i++) {
		node->children[i] = malloc(sizeof(s_pb_graph_node *) * pb_type->modes[i].num_children);
		for (j = 0; j < pb_type->modes[i].num_children; j++) {
			node->children[i][j] = malloc(sizeof(s_pb_graph_node) * pb_type->modes[i].children[j].num_pbs);
		}
	}
}

void build_pb_graph(s_pb_graph_node *node, s_pb_type *pb_type, s_pb_graph_node *parent_node)
{
	int i, j, k;

	node->type = pb_type;
	node->parent = parent_node;

	alloc_pb_graph_children(node, pb_type);
	for (i = 0; i < pb_type->num_modes; i++) {
		for (j = 0; j < pb_type->modes[i].num_children; j++) {
			for (k = 0; k < pb_type->modes[i].children[j].num_pbs; k++) {
				build_pb_graph(&node->children[i][j][k], pb_type, node);
			}
		}
	}

	/* requires children to be loaded first before pins can be connected */
	alloc_and_init_pb_graph_pins(node, pb_type);
}
