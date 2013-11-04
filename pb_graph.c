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

s_pb_type * find_child_pb_type(s_pb_type *parent_pb_type, int mode, const char *child_pb_type_name, int *index)
{
	int i;

	if (mode >= parent_pb_type->num_modes) {
		return NULL;
	}

	for (i = 0; i < parent_pb_type->modes[mode].num_children; i++) {
		if (!strcmp(child_pb_type_name, parent_pb_type->modes[mode].children[i].name)) {
			*index = i;
			return &parent_pb_type->modes[mode].children[i];
		}
	}
	return NULL;
}

s_port *find_pb_type_input_port(s_pb_type *pb_type, const char *port_name, int *index)
{
	int i;

	for (i = 0; i < pb_type->num_input_ports; i++) {
		if (!strcmp(port_name, pb_type->input_ports[i].name)) {
			*index = i;
			return &pb_type->input_ports[i];
		}
	}
	return NULL;
}

s_port *find_pb_type_output_port(s_pb_type *pb_type, const char *port_name, int *index)
{
	int i;

	for (i = 0; i < pb_type->num_output_ports; i++) {
		if (!strcmp(port_name, pb_type->output_ports[i].name)) {
			*index = i;
			return &pb_type->output_ports[i];
		}
	}
	return NULL;
}

void alloc_and_init_pb_graph_pins(s_pb_graph_node *node)
{
	int i;
	int mode, j, k;
	int pb_type_index;
	int port_index;
	int instance;
	int pin;
	s_pb_type *pb_type;
	char *pb_type_name;
	char *port_name;
	char *pb_type_reference;
	char *port_reference;
	s_list_item *item;
	s_list input_string_tokens;
	s_list port_name_tokens;
	s_list pb_type_name_tokens;
	s_list instances_tokens;
	s_list pins_tokens;
	s_list input_tokens;
	int instance_low;
	int instance_high;
	int pin_low;
	int pin_high;
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
			tokenize(pb_type->modes[mode].interconnects[j].input_string, " ", &input_tokens);

			item = input_tokens.head;
			while (item) {
				tokenize(item->data, ".", &input_string_tokens);

				assert(input_string_tokens.num_items == 2);

				pb_type_reference = input_string_tokens.head->data;
				port_reference = input_string_tokens.head->next->data;

				pb_type_name = tokenize_name_and_index(pb_type_reference, &instance_low, &instance_high);
				port_name = tokenize_name_and_index(port_reference, &pin_low, &pin_high);

				/* interconnect input can only come from 2 possible sources: parent input port OR child output port */
//				if (!strcmp(pb_type_name, pb_type->name)) { /* interconnect input comes from parent input port */
//					port_ptr = find_pb_type_input_port(pb_type, port_name, &port_index);
//					assert(port_ptr);
//
//					for (pin = pin_low; pin <= pin_high; pin++) {
//						insert_to_listnode->input_pins[port_index][pin].edges
//				} else { /* interconnect input comes from child output port */
//					child_pb_type = find_child_pb_type(pb_type, mode, pb_type_name, &pb_type_index);
//					assert(child_pb_type);
//					port_ptr = find_pb_type_output_port(child_pb_type, port_name, &port_index);
//					assert(port_ptr);
//
//					if (pb_type_name_tokens.num_items == 1) { /* no subscript */
//						assert(child_pb_type->num_pbs == 1);
//
//						pin = atoi(port_name_tokens.head->next->data);
//						node->children[mode][pb_type_index][0].output_pins[pin]->edges[]
//					} else {
//						assert(pb_type_name_tokens.num_items == 2);
//
//						tokenize(pb_type_name_tokens.head->next->data, "[]", &instances_tokens);
//
//						for (instance = )
//						node->children[mode][pb_type_index][instance].output_pins[pin]->edges[]
//					}
//
//
//					//node->children[mode][pb_type_index][instance].output_pins
//				}

				item = item->next;
			}


			printf("");

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
