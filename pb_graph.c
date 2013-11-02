/*
 * pb_graph.c
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "pb_graph.h"
#include "helper.h"

s_pb_type * find_child_pb_type(s_pb_type *parent_pb_type, int mode, const char *child_pb_type_name)
{
	int i;

	if (mode >= parent_pb_type->num_modes) {
		return NULL;
	}

	for (i = 0; i < parent_pb_type->modes[mode].num_children; i++) {
		if (!strcmp(child_pb_type_name, parent_pb_type->modes[mode].children[i].name)) {
			return &parent_pb_type->modes[mode].children[i];
		}
	}
	return NULL;
}

s_port *find_pb_type_input_port(s_pb_type *pb_type, const char *port_name)
{
	int i;

	for (i = 0; i < pb_type->num_input_ports; i++) {
		if (!strcmp(port_name, pb_type->input_ports[i].name)) {
			return &pb_type->input_ports[i];
		}
	}
	return NULL;
}

s_port *find_pb_type_output_port(s_pb_type *pb_type, const char *port_name)
{
	int i;

	for (i = 0; i < pb_type->num_output_ports; i++) {
		if (!strcmp(port_name, pb_type->output_ports[i].name)) {
			return &pb_type->output_ports[i];
		}
	}
	return NULL;
}

void alloc_and_init_pb_graph_pins(s_pb_graph_node *node)
{
	int i;
	int mode, j, k;
	s_pb_type *pb_type;
	char *pb_type_name;
	char *port_name;
	s_list tokens;
	int port;
	s_pb_type *child_pb_type;
	s_port *port_ptr;

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
	for (mode = 0; mode < pb_type->num_modes; mode++) {
		for (j = 0; j < pb_type->modes[mode].num_interconnects; j++) {
			tokenize(pb_type->modes[mode].interconnects[j].input_string, ".", &tokens);

			assert(tokens.num_items == 2);
			pb_type_name = tokens.head->data;
			port_name = tokens.head->next->data;

			/* interconnect input can only come from 2 possible sources: parent input port OR child output port */
			if (!strcmp(pb_type_name, pb_type->name)) { /* interconnect input comes from parent input port */
				port_ptr = find_pb_type_input_port(pb_type, port_name);
				assert(port_ptr);
			} else {
				child_pb_type = find_child_pb_type(pb_type, mode, pb_type_name);
				assert(child_pb_type);
				port_ptr = find_pb_type_output_port(child_pb_type, port_name);
				assert(port_ptr);

				node->children[mode]
			}
		}
	}

	for (i = 0; i < pb_type->num_input_ports; i++) {
		//node->input_pins[i][j].edges
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

	alloc_pb_graph_children(node);
	for (i = 0; i < pb_type->num_modes; i++) {
		for (j = 0; j < pb_type->modes[i].num_children; j++) {
			for (k = 0; k < pb_type->modes[i].children[j].num_pbs; k++) {
				build_pb_graph(&node->children[i][j][k], &pb_type->modes[i].children[j], node);
			}
		}
	}

	/* requires children to be loaded first before pins can be connected */
	alloc_and_init_pb_graph_pins(node);
}
