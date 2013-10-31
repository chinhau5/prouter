/*
 * parse_arch.c
 *
 *  Created on: Oct 30, 2013
 *      Author: chinhau5
 */

#include <libxml/parser.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"
#include "vpr_types.h"
//#include <libxml/xmlmemory.h>


xmlNodePtr find_next_element(xmlNodePtr node, const char *element)
{
	while (node) {
		if (!strcmp(node->name, element)) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

int get_child_count(xmlNodePtr parent, const char *element)
{
	xmlNodePtr current;
	int count;

	current = find_next_element(parent->children, element);
	count = 0;
	while (current) {
		count++;
		current = find_next_element(current->next, element);
	}

	return count;
}

void parse_pb_type_ports(xmlNodePtr pb_type_node, s_physical_block *pb)
{
	xmlNodePtr input_node;
	xmlNodePtr output_node;
	int i;

	pb->num_input_ports = get_child_count(pb_type_node, "input");
	pb->input_ports = malloc(sizeof(s_port) * pb->num_input_ports);

	input_node = find_next_element(pb_type_node->children, "input");
	for (i = 0; i < pb->num_input_ports; i++) {
		pb->input_ports[i].num_pins = atoi(xmlGetProp(input_node, "num_pins"));
		pb->input_ports[i].name = xmlGetProp(input_node, "name");
		input_node = find_next_element(input_node->next, "input");
	}

	pb->num_output_ports = get_child_count(pb_type_node, "output");
	pb->output_ports = malloc(sizeof(s_port) * pb->num_output_ports);
	output_node = find_next_element(pb_type_node->children, "output");
	for (i = 0; i < pb->num_output_ports; i++) {
		pb->output_ports[i].num_pins = atoi(xmlGetProp(output_node, "num_pins"));
		pb->output_ports[i].name = xmlGetProp(output_node, "name");
		output_node = find_next_element(output_node->next, "output");
	}
}

void parse_mode(xmlNodePtr mode_node, s_mode *mode, s_physical_block *mode_parent)
{
	int i;
	xmlNodePtr pb_type_node;

	assert(!strcmp(mode_node->name, "mode") || !strcmp(mode_node->name, "pb_type"));

	mode->name = xmlGetProp(mode_node, "name");
	mode->num_children = get_child_count(mode_node, "pb_type");

	mode->children = malloc(sizeof(s_physical_block) * mode->num_children);
	pb_type_node = find_next_element(mode_node->children, "pb_type");
	for (i = 0; i < mode->num_children; i++) {
		parse_pb_type(pb_type_node, &mode->children[i]);
		pb_type_node = find_next_element(pb_type_node->next, "pb_type");
	}

	/*TODO: parse interconnect here*/
}

void parse_pb_type(xmlNodePtr pb_type_node, s_physical_block *pb, s_physical_block *parent)
{
	xmlNodePtr mode_node;
	xmlNodePtr pb_type_child_node;
	xmlChar *prop;
	int count;
	int mode;
	int temp_num_modes;

	assert(!strcmp(pb_type_node->name, "pb_type"));

	pb->name = xmlGetProp(pb_type_node, "name");
	pb->blif_model = xmlGetProp(pb_type_node, "blif_model");
	pb->parent = parent;

	parse_pb_type_ports(pb_type_node, pb);

	/* this is a primitive, no more children */
	if (pb->blif_model) {
		return;
	}

	pb->num_modes = get_child_count(pb_type_node, "mode");

	if (pb->num_modes == 0) {
		mode_node = pb_type_node;
		pb->num_modes = 1;
	} else {
		mode_node =
	}


	if (temp_num_modes > 0) {
		pb->modes = malloc(sizeof(s_mode) * pb->num_modes);

		mode_node = find_next_element(pb_type_node->children, "mode");
		count = 0;
		while (mode_node) {
			parse_mode(mode_node, &pb->modes[count++], pb);
			mode_node = find_next_element(mode_node->next, "mode");
		}
		assert(count == pb->num_modes);
//	} else {
//		pb->num_modes = 1;
//		pb->modes = malloc(sizeof(s_mode) * pb->num_modes);
//
//		pb->modes[0].name = NULL;
//		pb->modes[0].num_children = get_child_count(pb_type_node, "pb_type");
//
//		pb->modes[0].children = malloc(sizeof(s_physical_block) * pb->modes[0].num_children);
//		pb_type_child_node = find_next_element(pb_type_node->children, "pb_type");
//		count = 0;
//		while (pb_type_child_node) {
//			parse_pb_type(pb_type_child_node, pb->modes[0].children, pb);
//			pb_type_child_node = find_next_element(pb_type_child_node->next, "pb_type");
//			count++;
//		}
//		assert(count == pb->modes[0].num_children);
//	}

	if (pb_type_node->parent) { /* not top level */
		count = get_child_count(pb_type_node->parent, "interconnect");
		if (count != 1 && temp_num_modes == 0) {
			printf("Expected 1 interconnect element for %s but found %d\n");
		}
		//node = find_next_element(node->next, "");
		//parse_pb_interconnect(node);
	}
}

s_physical_block *parse_complex_block_list(xmlNodePtr complexblocklist_node)
{
	xmlNodePtr current;
	int count;
	s_physical_block *pbs;
	int num_cb;

	assert(!strcmp(complexblocklist_node->name, "complexblocklist"));

	num_cb = get_child_count(complexblocklist_node, "pb_type");
	pbs = malloc(sizeof(s_physical_block) * num_cb);

	current = find_next_element(complexblocklist_node->children, "pb_type");
	count = 0;
	while (current) {
		parse_pb_type(current, &pbs[count], NULL);
		current = find_next_element(current->next, "pb_type");
		count++;
	}
	assert(count == num_cb);

	return pbs;
}

void parse_arch()
{
	xmlDocPtr doc;
	xmlNodePtr complexblocklist_node;
	xmlNodePtr root_node;

	doc = xmlParseFile("sample_arch.xml");
	root_node = xmlDocGetRootElement(doc);
	assert(!strcmp(root_node->name, "architecture"));
	complexblocklist_node = find_next_element(root_node->children, "complexblocklist");
	parse_complex_block_list(complexblocklist_node);
}
