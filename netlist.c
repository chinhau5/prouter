/*
 * netlist.c
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#include <glib.h>
#include <assert.h>
#include <string.h>
#include "xml_helper.h"
#include "helper.h"
#include "list.h"
#include "vpr_types.h"
#include "pb_graph.h"
#include "netlist.h"

void parse_block_ports(xmlNodePtr block_node, s_pb *pb, GHashTable *nets)
{
	char *inputs;
	char *outputs;
	xmlNodePtr inputs_node;
	xmlNodePtr outputs_node;
	xmlNodePtr port_node;
	char *instance_name;
	char *target_port_name;
	bool instance_no_index;
	bool port_no_index;
	int instance_low;
	int instance_high;
	int pin_low;
	int pin_high;
	s_list tokens;
	s_list instance_and_port_and_interconnect;
	s_list_item *token;
	char *port_name;
	s_port *port;
	s_pb_graph_pin ***pins;
	int num_sets;
	int *num_pins;
	int pin;
	int i, j;

	inputs_node = find_next_element(block_node->children, "inputs");

	if (inputs_node) {
		port_node = find_next_element(inputs_node->children, "port");
		while (port_node) {
			port_name = xmlGetProp(port_node, "name");
			assert(port_name);
			port = find_pb_type_port(pb->type, port_name);
			assert(port);

			tokenize(port_node->children->content, " ", &tokens);
			assert(tokens.num_items == find_pb_type_port(pb->type, xmlGetProp(port_node, "name"))->num_pins);

			/* for all pins */
			token = tokens.head;
			pin = 0;
			while (token) {
				if (!strcmp(token->data, "open") || !pb->parent) {
					//pb->input_pins[port->port_number][pin].next_pin = NULL;
				} else {
					pins = get_pb_pins(pb->parent, pb->parent->children, strtok(token->data, "->"), &num_sets, &num_pins);
					assert(num_sets == 1 && num_pins[0] == 1);
					pins[0][0]->next_pin = &pb->input_pins[port->port_number][pin];
				}

				pin++;
				token = token->next;
			}
			port_node = find_next_element(port_node->next, "port");
		}
	}

	outputs_node = find_next_element(block_node->children, "outputs");

	/* output */
	/* pins = get_pb_pins(pb, pb->children, strtok(token->data, "->"), &num_sets, &num_pins); */

//	if (pb->children) {
//		for (i = 0; i < pb->mode->num_children; i++) {
//			for (j = 0; j < pb->children[i][0].type->num_pbs; j++) {
//				parse_block_ports
//			}
//		}
//	}
}

s_pb_type *get_pb_type_from_instance_name(s_pb_type *pb_types, int num_pb_types, const char *instance_name, int *pb_type_index)
{
	int i;

	for (i = 0; i < num_pb_types; i++) {
		if (!strcmp(pb_types[i].name, instance_name)) {
			*pb_type_index = i;
			return &pb_types[i];
		}
	}
	return NULL;
}

s_mode *get_pb_type_mode(const char *mode_name, s_pb_type *pb_type, int *mode_index)
{
	int i;
	for (i = 0; i < pb_type->num_modes; i++) {
		if (!strcmp(pb_type->modes[i].name, mode_name)) {
			*mode_index = i;
			return &pb_type->modes[i];
		}
	}
	return NULL;
}

void parse_block_common(s_pb *pb, xmlNodePtr block_node, GHashTable *external_nets, s_pb *parent);

void parse_top_level_block(t_block **grid, GHashTable *external_nets, GHashTable *block_positions, xmlNodePtr block_node, s_pb_top_type *pb_top_types, int num_pb_top_types)
{
	int i;
	char *instance_name_and_index;
	char *instance_name;

	int instance_low;
	int instance_high;
	bool instance_no_index;
	s_pb_top_type *pb_top_type;
	char *block_name;
	s_block_position *position;

	check_element_name(block_node, "block");

	instance_name_and_index = xmlGetProp(block_node, "instance");
	assert(instance_name_and_index);
	instance_name = tokenize_name_and_index(instance_name_and_index, &instance_low, &instance_high, &instance_no_index);
	assert(instance_low == instance_high && !instance_no_index);
	for (i = 0; i < num_pb_top_types; i++) {
		if (!strcmp(pb_top_types[i].base.name, instance_name)) {
			pb_top_type = &pb_top_types[i];
			break;
		}
	}
	block_name = xmlGetProp(block_node, "name");
	assert(block_name);
	position = g_hash_table_lookup(block_positions, block_name);
	if (!grid[position->x][position->y].pb) {
		grid[position->x][position->y].pb = malloc(sizeof(s_pb) * pb_top_type->capacity);
	}
	assert(position->z < pb_top_type->capacity);
	grid[position->x][position->y].pb[position->z].type = &pb_top_type->base;
	grid[position->x][position->y].pb[position->z].name = block_name;
	parse_block_common(&grid[position->x][position->y].pb[position->z], block_node, external_nets, NULL);

	//parse_block_ports(block_node, &grid[position->x][position->y].pb[position->z], NULL);
}

void parse_block(s_pb **pbs, GHashTable *external_nets, xmlNodePtr block_node, s_pb_type *pb_types, int num_pb_types, s_pb *parent)
{
	char *instance_name_and_index;
	char *instance_name;
	int instance_low;
	int instance_high;
	bool instance_no_index;
	int pb_type_index;
	s_pb_type *pb_type;
	s_pb *pb;

	check_element_name(block_node, "block");

	instance_name_and_index = xmlGetProp(block_node, "instance");
	assert(instance_name_and_index);
	instance_name = tokenize_name_and_index(instance_name_and_index, &instance_low, &instance_high, &instance_no_index);
	assert(instance_low == instance_high && !instance_no_index); /* debug */
	pb_type = get_pb_type_from_instance_name(pb_types, num_pb_types, instance_name, &pb_type_index);
	assert(pb_type); /* debug */
	assert(instance_low < pb_type->num_pbs); /* debug */
	pb = &pbs[pb_type_index][instance_low];
	pb->type = pb_type;
	pb->name = xmlGetProp(block_node, "name");
	assert(pb->name);
	parse_block_common(pb, block_node, external_nets, parent);
}

void parse_block_common(s_pb *pb, xmlNodePtr block_node, GHashTable *external_nets, s_pb *parent)
{
	int i;
	xmlNodePtr child_block_node;
	char *mode_name;
	int mode_index;

	mode_name = xmlGetProp(block_node, "mode");
	if (mode_name) {
		pb->mode = get_pb_type_mode(mode_name, pb->type, &mode_index);
		assert(pb->mode);
	} else {
		pb->mode = NULL;
	}

	pb->parent = parent;

	init_pb_pins(pb);

	/* we allocate and parse pb children only when the arch has children and the netlist has children */
	child_block_node = find_next_element(block_node->children, "block");
	if (pb->mode && child_block_node) {
		pb->children = malloc(sizeof(s_pb *) * pb->mode->num_children);
		for (i = 0; i < pb->mode->num_children; i++) {
			pb->children[i] = malloc(sizeof(s_pb) * pb->mode->children[i].num_pbs);
		}

		while (child_block_node) {
			parse_block(pb->children, external_nets, child_block_node, pb->mode->children, pb->mode->num_children, pb);
			child_block_node = find_next_element(child_block_node->next, "block");
		}
	} else {
		pb->children = NULL;
	}

	//port parsing has to be done after pb rr graph has been loaded
//	if (!pb->parent) { /* top level input ports are net sinks */
//
//	}
//	if (!pb->mode) { /* leaf level output ports are net drivers */
//
//	}
}

void parse_netlist(const char *filename, t_block **grid, GHashTable *block_positions, s_pb_top_type *pb_top_types, int num_pb_top_types,
		int *num_blocks, GHashTable **external_nets)
{
	xmlDocPtr netlist;
	xmlNodePtr root_node;
	xmlNodePtr block_node;
	int count;

	netlist = xmlParseFile(filename);

	root_node = xmlDocGetRootElement(netlist);

	check_element_name(root_node, "block");

	*num_blocks = get_child_count(root_node, "block");

	/* iterate through all the top level blocks */
	count = 0;
	block_node = find_next_element(root_node->children, "block");
	while (block_node) {
		parse_top_level_block(grid, *external_nets, block_positions, block_node, pb_top_types, num_pb_top_types);
		block_node = find_next_element(block_node->next, "block");
		count++;
	}
	assert(count == *num_blocks);
}
