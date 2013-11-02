/*
 * parse_arch.c
 *
 *  Created on: Oct 30, 2013
 *      Author: chinhau5
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"
#include "vpr_types.h"
#include "xml_helper.h"

void parse_pb_type(xmlNodePtr pb_type_node, s_pb_type *pb_type, s_pb_type *parent);

void dump_pb(s_pb_type *pb_type)
{
	int i, j;

	printf("name: %s\n", pb_type->name);
	printf("num_pbs: %d\n", pb_type->num_pbs);
	printf("blif_model: %s\n", pb_type->blif_model);

	for (i = 0; i < pb_type->num_input_ports; i++) {
		printf("Input port: %s Num pins: %d\n", pb_type->input_ports[i].name, pb_type->input_ports[i].num_pins);
	}
	for (i = 0; i < pb_type->num_output_ports; i++) {
		printf("Output port: %s Num pins: %d\n", pb_type->output_ports[i].name, pb_type->output_ports[i].num_pins);
	}

	if (pb_type->parent) {
		printf("Parent name: %s\n", pb_type->parent->name);
	}

	for (i = 0; i < pb_type->num_modes; i++) {
		printf("Mode: %s\n", pb_type->modes[i].name);
		for (j = 0; j < pb_type->modes[i].num_interconnects; j++) {
			printf("%s %s %s\n", pb_type->modes[i].interconnects[j].name, pb_type->modes[i].interconnects[j].input_string, pb_type->modes[i].interconnects[j].output_string);
		}
		for (j = 0; j < pb_type->modes[i].num_children; j++) {
			dump_pb(&pb_type->modes[i].children[j]);
		}

	}
}

void dump_pb_top_type(s_pb_top_type *pb_top_type)
{
	dump_pb(&pb_top_type->pb);
	printf("capacity: %d\n", pb_top_type->capacity);
	printf("height: %d\n", pb_top_type->height);
}

void dump_pb_top_types(s_pb_top_type *pb_top_types, int num_pb_top_types)
{
	int i;
	for (i = 0; i < num_pb_top_types; i++) {
		dump_pb_top_type(&pb_top_types[i]);
	}
}

void parse_pb_type_ports(xmlNodePtr pb_type_node, s_pb_type *pb_type)
{
	xmlNodePtr input_node;
	xmlNodePtr output_node;
	int i;

	pb_type->num_input_ports = get_child_count(pb_type_node, "input");
	pb_type->input_ports = malloc(sizeof(s_port) * pb_type->num_input_ports);

	input_node = find_next_element(pb_type_node->children, "input");
	for (i = 0; i < pb_type->num_input_ports; i++) {
		pb_type->input_ports[i].num_pins = atoi(xmlGetProp(input_node, "num_pins"));
		pb_type->input_ports[i].name = xmlGetProp(input_node, "name");
		input_node = find_next_element(input_node->next, "input");
	}

	pb_type->num_output_ports = get_child_count(pb_type_node, "output");
	pb_type->output_ports = malloc(sizeof(s_port) * pb_type->num_output_ports);
	output_node = find_next_element(pb_type_node->children, "output");
	for (i = 0; i < pb_type->num_output_ports; i++) {
		pb_type->output_ports[i].num_pins = atoi(xmlGetProp(output_node, "num_pins"));
		pb_type->output_ports[i].name = xmlGetProp(output_node, "name");
		output_node = find_next_element(output_node->next, "output");
	}
}

e_interconnect_type get_interconnect_type(const char *interconnect_type)
{

}

void parse_interconnect_type(xmlNodePtr interconnect_node, const char *interconnect_type, s_interconnect *interconnect, int *num_interconnects)
{
	xmlNodePtr interconnect_type_node;
	e_interconnect_type type;

	type = get_interconnect_type

	interconnect_type_node = find_next_element(interconnect_node->children, "interconnect_type");
	while (interconnect_type_node) {
		interconnect[*num_interconnects].type =
		interconnect[*num_interconnects].name = xmlGetProp(interconnect_type_node, "name");
		interconnect[*num_interconnects].input_string = xmlGetProp(interconnect_type_node, "input");
		interconnect[*num_interconnects].output_string = xmlGetProp(interconnect_type_node, "output");

		(*num_interconnects)++;
		interconnect_type_node = find_next_element(interconnect_type_node->next, "interconnect_type");
	}
}

void parse_interconnect(xmlNodePtr interconnect_node, s_mode *mode)
{
	xmlNodePtr complete_node;
	xmlNodePtr direct_node;
	xmlNodePtr mux_node;
	int count;

	mode->num_interconnects = get_child_count(interconnect_node, "complete");
	mode->num_interconnects += get_child_count(interconnect_node, "direct");
	mode->num_interconnects += get_child_count(interconnect_node, "mux");

	mode->interconnects = malloc(sizeof(s_interconnect) * mode->num_interconnects);

	count = 0;
	parse_interconnect_type(interconnect_node, "direct", mode->interconnects, &count);
	parse_interconnect_type(interconnect_node, "complete", mode->interconnects, &count);
	parse_interconnect_type(interconnect_node, "mux", mode->interconnects, &count);
	assert(count == mode->num_interconnects);
}

void parse_mode(xmlNodePtr mode_node, s_mode *mode, s_pb_type *mode_parent)
{
	int i;
	xmlNodePtr pb_type_node;
	int interconnect_count;
	xmlNodePtr interconnect_node;

	assert(!strcmp(mode_node->name, "mode") || !strcmp(mode_node->name, "pb_type"));

	mode->name = xmlGetProp(mode_node, "name");
	mode->num_children = get_child_count(mode_node, "pb_type");

	mode->children = malloc(sizeof(s_pb_type) * mode->num_children);
	pb_type_node = find_next_element(mode_node->children, "pb_type");
	for (i = 0; i < mode->num_children; i++) {
		parse_pb_type(pb_type_node, &mode->children[i], mode_parent);
		pb_type_node = find_next_element(pb_type_node->next, "pb_type");
	}

	interconnect_count = get_child_count(mode_node, "interconnect");
	if (interconnect_count != 1) {
		printf("Expected 1 interconnect element for %s but found %d.\n", mode->name, interconnect_count);
	}
	interconnect_node = find_next_element(mode_node->children, "interconnect");
	parse_interconnect(interconnect_node, mode);
}

void parse_pb_type(xmlNodePtr pb_type_node, s_pb_type *pb_type, s_pb_type *parent)
{
	xmlNodePtr mode_node;
	xmlNodePtr pb_type_child_node;
	xmlNodePtr interconnect_node;
	xmlChar *prop;
	int count;
	int mode;
	char *num_pbs;

	assert(!strcmp(pb_type_node->name, "pb_type"));

	pb_type->name = xmlGetProp(pb_type_node, "name");
	pb_type->blif_model = xmlGetProp(pb_type_node, "blif_model");
	pb_type->parent = parent;
	num_pbs = xmlGetProp(pb_type_node, "num_pb");
	if (num_pbs) {
		pb_type->num_pbs = atoi(num_pbs);
	} else {
		pb_type->num_pbs = 1;
	}


	parse_pb_type_ports(pb_type_node, pb_type);

	/* this is a primitive, no more children */
	if (pb_type->blif_model) {
		pb_type->num_modes = 0;
		pb_type->modes = NULL;
		return;
	}

	pb_type->num_modes = get_child_count(pb_type_node, "mode");
	if (pb_type->num_modes > 0) {
		pb_type->modes = malloc(sizeof(s_mode) * pb_type->num_modes);

		mode_node = find_next_element(pb_type_node->children, "mode");
		count = 0;
		while (mode_node) {
			parse_mode(mode_node, &pb_type->modes[count++], pb_type);

			mode_node = find_next_element(mode_node->next, "mode");
		}
		assert(count == pb_type->num_modes);
	} else {
		pb_type->num_modes = 1;
		pb_type->modes = malloc(sizeof(s_mode) * pb_type->num_modes);

		parse_mode(pb_type_node, &pb_type->modes[0], pb_type);
	}
}

void parse_pb_top_type(xmlNodePtr pb_type_node, s_pb_top_type *pb_top_type)
{
	char *str;
	parse_pb_type(pb_type_node, &pb_top_type->pb, NULL);
	str = xmlGetProp(pb_type_node, "capacity");
	if (str) {
		pb_top_type->capacity = atoi(str);
	} else {
		pb_top_type->capacity = 1;
	}
	str = xmlGetProp(pb_type_node, "height");
	if (str) {
		pb_top_type->height = atoi(str);
	} else {
		pb_top_type->height = 1;
	}
}

s_pb_top_type *parse_complex_block_list(xmlNodePtr complexblocklist_node, int *num_pb_top_types)
{
	xmlNodePtr current;
	int count;
	s_pb_top_type *pb_top_types;

	check_element_name(complexblocklist_node, "complexblocklist");

	*num_pb_top_types = get_child_count(complexblocklist_node, "pb_type");
	pb_top_types = malloc(sizeof(s_pb_top_type) * *num_pb_top_types);

	current = find_next_element(complexblocklist_node->children, "pb_type");
	count = 0;
	while (current) {
		parse_pb_top_type(current, &pb_top_types[count]);
		current = find_next_element(current->next, "pb_type");
		count++;
	}
	assert(count == *num_pb_top_types);

	return pb_top_types;
}

void parse_arch(const char *filename)
{
	xmlDocPtr doc;
	xmlNodePtr complexblocklist_node;
	xmlNodePtr root_node;
	s_pb_top_type *pb_top_types;
	int num_pb_top_types;

	doc = xmlParseFile(filename);
	root_node = xmlDocGetRootElement(doc);
	check_element_name(root_node, "architecture");

	complexblocklist_node = find_next_element(root_node->children, "complexblocklist");
	pb_top_types = parse_complex_block_list(complexblocklist_node, &num_pb_top_types);

	dump_pb_top_types(pb_top_types, num_pb_top_types);
}
