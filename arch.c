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

void parse_pb_type(xmlNodePtr pb_type_node, s_physical_block *pb, s_physical_block *parent);

void dump_pb(s_physical_block *pb)
{
	int i, j;

	printf("name: %s\n", pb->name);
	printf("num_pbs: %d\n", pb->num_pbs);
	printf("blif_model: %s\n", pb->blif_model);

	for (i = 0; i < pb->num_input_ports; i++) {
		printf("Input port: %s Num pins: %d\n", pb->input_ports[i].name, pb->input_ports[i].num_pins);
	}
	for (i = 0; i < pb->num_output_ports; i++) {
		printf("Output port: %s Num pins: %d\n", pb->output_ports[i].name, pb->output_ports[i].num_pins);
	}

	if (pb->parent) {
		printf("Parent name: %s\n", pb->parent->name);
	}

	for (i = 0; i < pb->num_modes; i++) {
		printf("Mode: %s\n", pb->modes[i].name);
		for (j = 0; j < pb->modes[i].num_interconnects; j++) {
			printf("%s %s %s\n", pb->modes[i].interconnects[j].name, pb->modes[i].interconnects[j].input_string, pb->modes[i].interconnects[j].output_string);
		}
		for (j = 0; j < pb->modes[i].num_children; j++) {
			dump_pb(&pb->modes[i].children[j]);
		}

	}
}

void dump_complex_block(s_complex_block *cb)
{
	dump_pb(&cb->pb);
	printf("capacity: %d\n", cb->capacity);
	printf("height: %d\n", cb->height);
}

void dump_complexblocklist(s_complex_block *cbs, int num_cbs)
{
	int i;
	for (i = 0; i < num_cbs; i++) {
		dump_complex_block(&cbs[i]);
	}
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
	complete_node = find_next_element(interconnect_node->children, "complete");
	while (complete_node) {
		mode->interconnects[count].name = xmlGetProp(complete_node, "name");
		mode->interconnects[count].input_string = xmlGetProp(complete_node, "input");
		mode->interconnects[count].output_string = xmlGetProp(complete_node, "output");
		count++;
		complete_node = find_next_element(complete_node->next, "complete");
	}

	direct_node = find_next_element(interconnect_node->children, "direct");
	while (direct_node) {
		mode->interconnects[count].name = xmlGetProp(direct_node, "name");
		mode->interconnects[count].input_string = xmlGetProp(direct_node, "input");
		mode->interconnects[count].output_string = xmlGetProp(direct_node, "output");
		count++;
		direct_node = find_next_element(direct_node->next, "direct");
	}

	mux_node = find_next_element(interconnect_node->children, "mux");
	while (mux_node) {
		mode->interconnects[count].name = xmlGetProp(mux_node, "name");
		mode->interconnects[count].input_string = xmlGetProp(mux_node, "input");
		mode->interconnects[count].output_string = xmlGetProp(mux_node, "output");
		count++;
		mux_node = find_next_element(mux_node->next, "mux");
	}
	assert(count == mode->num_interconnects);
}

void parse_mode(xmlNodePtr mode_node, s_mode *mode, s_physical_block *mode_parent)
{
	int i;
	xmlNodePtr pb_type_node;
	int interconnect_count;
	xmlNodePtr interconnect_node;

	assert(!strcmp(mode_node->name, "mode") || !strcmp(mode_node->name, "pb_type"));

	mode->name = xmlGetProp(mode_node, "name");
	mode->num_children = get_child_count(mode_node, "pb_type");

	mode->children = malloc(sizeof(s_physical_block) * mode->num_children);
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

void parse_pb_type(xmlNodePtr pb_type_node, s_physical_block *pb, s_physical_block *parent)
{
	xmlNodePtr mode_node;
	xmlNodePtr pb_type_child_node;
	xmlNodePtr interconnect_node;
	xmlChar *prop;
	int count;
	int mode;
	char *num_pbs;

	assert(!strcmp(pb_type_node->name, "pb_type"));

	pb->name = xmlGetProp(pb_type_node, "name");
	pb->blif_model = xmlGetProp(pb_type_node, "blif_model");
	pb->parent = parent;
	num_pbs = xmlGetProp(pb_type_node, "num_pb");
	if (num_pbs) {
		pb->num_pbs = atoi(num_pbs);
	} else {
		pb->num_pbs = 1;
	}


	parse_pb_type_ports(pb_type_node, pb);

	/* this is a primitive, no more children */
	if (pb->blif_model) {
		pb->num_modes = 0;
		pb->modes = NULL;
		return;
	}

	pb->num_modes = get_child_count(pb_type_node, "mode");
	if (pb->num_modes > 0) {
		pb->modes = malloc(sizeof(s_mode) * pb->num_modes);

		mode_node = find_next_element(pb_type_node->children, "mode");
		count = 0;
		while (mode_node) {
			parse_mode(mode_node, &pb->modes[count++], pb);

			mode_node = find_next_element(mode_node->next, "mode");
		}
		assert(count == pb->num_modes);
	} else {
		pb->num_modes = 1;
		pb->modes = malloc(sizeof(s_mode) * pb->num_modes);

		parse_mode(pb_type_node, &pb->modes[0], pb);
	}
}

void parse_complex_block(xmlNodePtr pb_type_node, s_complex_block *cb)
{
	char *str;
	parse_pb_type(pb_type_node, &cb->pb, NULL);
	str = xmlGetProp(pb_type_node, "capacity");
	if (str) {
		cb->capacity = atoi(str);
	} else {
		cb->capacity = 1;
	}
	str = xmlGetProp(pb_type_node, "height");
	if (str) {
		cb->height = atoi(str);
	} else {
		cb->height = 1;
	}
}

s_complex_block *parse_complex_block_list(xmlNodePtr complexblocklist_node, int *num_pbs)
{
	xmlNodePtr current;
	int count;
	s_complex_block *cbs;

	check_element_name(complexblocklist_node, "complexblocklist");

	*num_pbs = get_child_count(complexblocklist_node, "pb_type");
	cbs = malloc(sizeof(s_complex_block) * *num_pbs);

	current = find_next_element(complexblocklist_node->children, "pb_type");
	count = 0;
	while (current) {
		parse_complex_block(current, &cbs[count]);
		current = find_next_element(current->next, "pb_type");
		count++;
	}
	assert(count == *num_pbs);

	return cbs;
}

void parse_arch(const char *filename)
{
	xmlDocPtr doc;
	xmlNodePtr complexblocklist_node;
	xmlNodePtr root_node;
	s_complex_block *cbs;
	int num_cbs;

	doc = xmlParseFile(filename);
	root_node = xmlDocGetRootElement(doc);
	check_element_name(root_node, "architecture");

	complexblocklist_node = find_next_element(root_node->children, "complexblocklist");
	cbs = parse_complex_block_list(complexblocklist_node, &num_cbs);

	dump_complexblocklist(cbs, num_cbs);
}
