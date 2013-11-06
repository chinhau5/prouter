/*
 * netlist.c
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#include <glib.h>
#include <assert.h>
#include "xml_helper.h"
#include "helper.h"
#include "list.h"
#include "vpr_types.h"



void read_placement(const char *filename, int *nx, int *ny, GHashTable **block_positions)
{
	FILE *file;
	char buffer[256];
	char block_name[256];
	int block_number;
	s_block_position *block_position;
	int x, y, z;
	int val;
	int count;

	*block_positions = g_hash_table_new(g_str_hash, g_str_equal);

	file = fopen(filename, "r");

	fgets(buffer, sizeof(buffer), file);

	fscanf(file, "Array size : %d x %d logic blocks", nx, ny);
	*nx += 2;
	*ny += 2;

	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);

	count = 0;
	while (fscanf(file, "%s %d %d %d #%d ", block_name, &x, &y, &z, &block_number) == 5) {
		block_position = malloc(sizeof(s_block_position));
		block_position->x = x;
		block_position->y = y;
		block_position->z = z;

		//printf("%d %s %d %d %d %d\n", val, block_name, block_position->x, block_position->y, block_position->z, block_number);
		assert(!g_hash_table_contains(*block_positions, block_name));
		g_hash_table_insert(*block_positions, strdup(block_name), block_position);
	}
	fclose(file);
}

void parse_block_ports(xmlNodePtr block_node, s_pb *pb, GHashTable *nets)
{
	char *inputs;
	char *outputs;
	xmlNodePtr inputs_node;
	xmlNodePtr outputs_node;
	xmlNodePtr port_node;
	s_list tokens;
	s_list_item *token;

	inputs_node = find_next_element(block_node->children, "inputs");
	port_node = find_next_element(inputs_node->children, "port");
	while (port_node) {
		tokenize(port_node->children->content, " ", &tokens);
		token = tokens.head;
		while (token) {
			printf("%s\n", token->data);
			token = token->next;
		}
		port_node = find_next_element(port_node->next, "port");
	}


	outputs_node = find_next_element(block_node->children, "outputs");
}

s_pb_type *get_pb_type_from_instance_name(void *pb_types, int num_pb_types, bool top_level, const char *instance_name, int *pb_type_index)
{
	int i;

	for (i = 0; i < num_pb_types; i++) {
		if (top_level) {
			if (!strcmp(((s_pb_top_type *)pb_types)[i].pb.name, instance_name)) {
				*pb_type_index = i;
				return &((s_pb_top_type *)pb_types)[i].pb;
			}
		} else {
			if (!strcmp(((s_pb_type *)pb_types)[i].name, instance_name)) {
				*pb_type_index = i;
				return &((s_pb_type *)pb_types)[i];
			}
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

typedef struct _s_hash_table_net {

} s_hash_table_net;

void parse_block(s_pb **pbs, GHashTable *external_nets, xmlNodePtr block_node, s_pb_type *pb_types, int num_pb_types, s_pb *parent, bool top_level)
{
	char *instance_name_and_index;
	char *instance_name;
	char *mode_name;
	int instance_low;
	int instance_high;
	int i;
	xmlNodePtr child_block_node;
	int pb_type_index;
	s_pb_type *pb_type;
	s_pb *pb;
	int mode_index;

	check_element_name(block_node, "block");

	instance_name_and_index = xmlGetProp(block_node, "instance");

	pb_type = get_pb_type_from_instance_name(pb_types, num_pb_types, top_level, instance_name_and_index, &pb_type_index);

	instance_name = tokenize_name_and_index(instance_name_and_index, &instance_low, &instance_high, pb_type->num_pbs-1);
	mode_name = xmlGetProp(block_node, "mode");

	assert(instance_low == instance_high);

	if (top_level) {
		pb = &pbs[0][instance_low];
	} else {
		pb = &pbs[pb_type_index][instance_low];
	}

	pb->type = pb_type;

	if (mode_name) {
		pb->mode = get_pb_type_mode(mode_name, pb->type, &mode_index);
	} else {
		pb->mode = NULL;
	}

	pb->parent = parent;

	if (!pb->parent) { /* top level input ports are net sinks */

	}
	if (!pb->mode) { /* leaf level output ports are net drivers */

	}

	if (pb->mode) {
		pb->children = malloc(sizeof(s_pb *) * pb->mode->num_children);
		for (i = 0; i < pb->mode->num_children; i++) {
			pb->children[i] = malloc(sizeof(s_pb) * pb->mode->children[i].num_pbs);
		}

		child_block_node = find_next_element(block_node->children, "block");
		while (child_block_node) {
			parse_block(pb->children, external_nets, child_block_node, pb->mode->children, pb->mode->num_children, pb, false);
			child_block_node = find_next_element(child_block_node->next, "block");
		}
	} else {
		pb->children = NULL;
	}
	//parse_block_ports(block_node, pb, NULL);
}

t_block *parse_netlist(const char *filename, int *num_blocks, s_pb_top_type *pb_top_types, int num_pb_top_types, GHashTable *external_nets)
{
	xmlDocPtr netlist;
	xmlNodePtr root_node;
	xmlNodePtr block_node;
	t_block *blocks;
	int count;

	netlist = xmlParseFile(filename);

	root_node = xmlDocGetRootElement(netlist);

	check_element_name(root_node, "block");

	*num_blocks = get_child_count(root_node, "block");
	blocks = malloc(sizeof(t_block) * *num_blocks);

	/* iterate through all the top level blocks */
	count = 0;
	block_node = find_next_element(root_node->children, "block");
	while (block_node) {
		blocks[count].pb = malloc(sizeof(s_pb));
		parse_block(&blocks[count++].pb, external_nets, block_node, pb_top_types, num_pb_top_types, NULL, true);
		block_node = find_next_element(block_node->next, "block");
	}
	assert(count == *num_blocks);
	return blocks;
}
