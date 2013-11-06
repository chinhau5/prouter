/*
 * pb_graph.c
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#include "pb_graph.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "helper.h"
#include "list.h"
#include "vpr_types.h"

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

s_pb_graph_pin *find_pb_graph_node_pin(s_pb_graph_node *node, s_port *port, int pin_number)
{
	switch (port->type) {
	case INPUT_PORT:
		return &node->input_pins[port->port_number][pin_number];
	case OUTPUT_PORT:
		return &node->output_pins[port->port_number][pin_number];
	case CLOCK_PORT:
		return &node->clock_pins[port->port_number][pin_number];
	default:
		break;
	}

	return NULL;
}

s_port *find_pb_type_port(s_pb_type *pb_type, const char *port_name)
{
	int i;

	for (i = 0; i < pb_type->num_input_ports; i++) {
		if (!strcmp(port_name, pb_type->input_ports[i].name)) {
			return &pb_type->input_ports[i];
		}
	}
	for (i = 0; i < pb_type->num_output_ports; i++) {
		if (!strcmp(port_name, pb_type->output_ports[i].name)) {
			return &pb_type->output_ports[i];
		}
	}
	for (i = 0; i < pb_type->num_clock_ports; i++) {
		if (!strcmp(port_name, pb_type->clock_ports[i].name)) {
			return &pb_type->clock_ports[i];
		}
	}
	return NULL;
}
/* s_pb_graph_pin ***pins pins[set][pin_index]-> */
s_pb_graph_pin ***get_interconnect_pb_graph_pins(s_pb_type *pb_type, int mode, const char *interconnect_string, s_pb_graph_node *node, int *num_sets, int **num_pins)
{
	s_list tokens;
	s_list_item *item;
	s_list instance_name_and_port_name;
	char *pb_type_name;
	char *pb_type_name_and_index;
	char *port_name_and_index;
	char *port_name;
	int instance_low;
	int instance_high;
	int pin_low;
	int pin_high;
	s_pb_type *child_pb_type;
	s_port *port_ptr;
	int pb_type_index;
	int pin;
	int instance;
	s_pb_graph_pin ***pins;
	int set;
	bool pb_type_no_index;
	bool port_no_index;

	tokenize(interconnect_string, " ", &tokens);

	*num_sets = tokens.num_items;
	pins = malloc(sizeof(s_pb_graph_pin **) * *num_sets);
	*num_pins = malloc(sizeof(int) * *num_sets);

	item = tokens.head;
	set = 0;
	while (item) {
		tokenize(item->data, ".", &instance_name_and_port_name);

		assert(instance_name_and_port_name.num_items == 2);

		pb_type_name_and_index = instance_name_and_port_name.head->data;
		pb_type_name = tokenize_name_and_index(pb_type_name_and_index, &instance_low, &instance_high, &pb_type_no_index);
		port_name_and_index = instance_name_and_port_name.head->next->data;
		port_name = tokenize_name_and_index(port_name_and_index, &pin_low, &pin_high, &port_no_index);

		/* interconnect input can only come from 2 possible sources: parent input port OR child output port */
		/* interconnect output can only go to 2 possible sinks: parent output port OR child input port */
		if (!strcmp(pb_type_name, pb_type->name)) { /* parent */
			port_ptr = find_pb_type_port(pb_type, port_name);
			assert(port_ptr);

			if (port_no_index) {
				pin_low = 0;
				pin_high = port_ptr->num_pins-1;
			}

			(*num_pins)[set] = pin_high-pin_low+1;
			pins[set] = malloc(sizeof(s_pb_graph_pin *) * (*num_pins)[set]);

			for (pin = pin_low; pin <= pin_high; pin++) {
				pins[set][pin-pin_low] = find_pb_graph_node_pin(node, port_ptr, pin);
				assert(pins[set][pin-pin_low]);
			}
		} else { /* child */
			child_pb_type = find_child_pb_type(pb_type, mode, pb_type_name, &pb_type_index);
			assert(child_pb_type);

			port_ptr = find_pb_type_port(child_pb_type, port_name);
			assert(port_ptr);

			if (pb_type_no_index) {
				instance_low = 0;
				instance_high = child_pb_type->num_pbs-1;
			}

			if (port_no_index) {
				pin_low = 0;
				pin_high = port_ptr->num_pins-1;
			}

			(*num_pins)[set] = (pin_high-pin_low+1) * (instance_high-instance_low+1);
			pins[set] = malloc(sizeof(s_pb_graph_pin *) * (*num_pins)[set]);

			for (instance = instance_low; instance <= instance_high; instance++) {
				for (pin = pin_low; pin <= pin_high; pin++) {
					pins[set][(instance-instance_low)*(pin_high-pin_low+1) + pin-pin_low] =
							find_pb_graph_node_pin(&node->children[mode][pb_type_index][instance], port_ptr, pin);
					assert(pins[set][(instance-instance_low)*(pin_high-pin_low+1) + pin-pin_low]);
				}
			}
		}

		item = item->next;
		set++;
	}

	return pins;
}

void alloc_and_init_pb_graph_pins(s_pb_graph_node *node)
{
	int i, j;
	int mode, interconnect, k;
	int pb_type_index;
	int port_index;
	int instance;
	int pin;
	s_pb_type *pb_type;
	int *num_input_pins;
	int num_input_sets;
	int *num_output_pins;
	int num_output_sets;
	s_pb_graph_pin ***input_pins;
	s_pb_graph_pin ***output_pins;

	pb_type = node->type;

	node->input_pins = malloc(sizeof(s_pb_graph_pin *) * pb_type->num_input_ports);
	for (i = 0; i < pb_type->num_input_ports; i++) {
		node->input_pins[i] = malloc(sizeof(s_pb_graph_pin) * pb_type->input_ports[i].num_pins);
	}

	for (i = 0; i < pb_type->num_input_ports; i++) {
		for (j = 0; j < pb_type->input_ports[i].num_pins; j++) {
			node->input_pins[pb_type->input_ports[i].port_number][j].port = &pb_type->input_ports[i];
			node->input_pins[pb_type->input_ports[i].port_number][j].pin_number = j;
		}
	}

	node->output_pins = malloc(sizeof(s_pb_graph_pin *) * pb_type->num_output_ports);
	for (i = 0; i < pb_type->num_output_ports; i++) {
		node->output_pins[i] = malloc(sizeof(s_pb_graph_pin) * pb_type->output_ports[i].num_pins);
	}

	for (i = 0; i < pb_type->num_output_ports; i++) {
		for (j = 0; j < pb_type->output_ports[i].num_pins; j++) {
			node->output_pins[pb_type->output_ports[i].port_number][j].port = &pb_type->output_ports[i];
			node->output_pins[pb_type->output_ports[i].port_number][j].pin_number = j;
		}
	}

	node->clock_pins = malloc(sizeof(s_pb_graph_pin *) * pb_type->num_clock_ports);
	for (i = 0; i < pb_type->num_clock_ports; i++) {
		node->clock_pins[i] = malloc(sizeof(s_pb_graph_pin) * pb_type->clock_ports[i].num_pins);
	}

	for (i = 0; i < pb_type->num_clock_ports; i++) {
		for (j = 0; j < pb_type->clock_ports[i].num_pins; j++) {
			node->clock_pins[pb_type->clock_ports[i].port_number][j].port = &pb_type->clock_ports[i];
			node->clock_pins[pb_type->clock_ports[i].port_number][j].pin_number = j;
		}
	}

	/* connect all pb_type->modes to pb_type->input_ports and pb_type->output_ports */
	for (mode = 0; mode < pb_type->num_modes; mode++) {
		for (interconnect = 0; interconnect < pb_type->modes[mode].num_interconnects; interconnect++) {
			input_pins = get_interconnect_pb_graph_pins(pb_type, mode, pb_type->modes[mode].interconnects[interconnect].input_string, node, &num_input_sets, &num_input_pins);
			output_pins = get_interconnect_pb_graph_pins(pb_type, mode, pb_type->modes[mode].interconnects[interconnect].output_string, node, &num_output_sets, &num_output_pins);

			switch (pb_type->modes[mode].interconnects[interconnect].type) {
			case DIRECT:
				assert(num_input_sets == 1 && num_output_sets == 1);
				break;
			case COMPLETE:
				break;
			case MUX:
				assert(num_output_sets == 1);
				for (i = 0; i < num_input_sets; i++) {
					assert(num_output_pins[0] == num_input_pins[i]);
				}
				break;
			default:
				break;
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
