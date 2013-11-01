/*
 * netlist.c
 *
 *  Created on: Nov 1, 2013
 *      Author: chinhau5
 */

#include <glib.h>
#include <assert.h>
#include "xml_helper.h"
#include "list.h"
#include "vpr_types.h"

s_list tokenize(char *str, const char *delim)
{
	s_list tokens;
	char *token;

	init_list(&tokens);
	token = strtok(str, delim);
	while (token) {
		insert_into_list(&tokens, token);
		token = strtok(NULL, " ");
	}
	return tokens;
}

void parse_block_ports(xmlNodePtr block_node, s_pb *pb, GHashTable *nets)
{
	char *inputs;
	char *outputs;
	xmlNodePtr inputs_node;
	xmlNodePtr port_node;
	s_list tokens;
	s_list_item *token;

	inputs_node = find_next_element(block_node->children, "inputs");
	port_node = find_next_element(inputs_node->children, "port");
	while (port_node) {
		tokens = tokenize(port_node->children->content, " ");
		token = tokens.head;
		while (token) {
			printf("%s\n", token->data);
			token = token->next;
		}
		port_node = find_next_element(port_node->next, "port");
	}


	outputs = find_next_element(block_node->children, "outputs");
}

void parse_block(xmlNodePtr block_node, s_physical_block *types, int num_types, s_pb *pb, s_pb *parent)
{
	char *instance;
	s_list tokens;
	s_list_item *token;
	s_physical_block *block_type;
	int i;

	check_element_name(block_node, "block");

	pb->parent = parent;

	instance = xmlGetProp(block_node, "instance");
	tokens = tokenize(instance, "[]");
	assert(tokens.num_items == 2); /* instance name and instance number */
	block_type = NULL;
	for (i = 0; i < num_types; i++) {
		if (!strcmp(types[i].name, tokens.head->data)) {
			block_type = &types[i];
			break;
		}
	}
	if (!block_type) {
		printf("Unable to find block type '%s'\n", tokens.head->data);
		exit(1);
	}

	parse_block_ports(block_node, pb, NULL);
}

t_block *parse_netlist(const char *filename, int *num_blocks, s_physical_block *types, int num_types)
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
		parse_block(block_node, types, num_types, blocks[count++].pb, NULL);
		block_node = find_next_element(block_node->next, "block");
	}
	assert(count == *num_blocks);
	return blocks;
}
