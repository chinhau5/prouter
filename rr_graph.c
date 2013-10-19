/*
 * rr_graph.c
 *
 *  Created on: 23 Aug, 2013
 *      Author: chinhau5
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <valgrind/valgrind.h>
#include "list.h"
#include "vpr_types.h"

#define IN
#define OUT
#define INOUT

void update_wire_type(s_wire_details *wire_specs, int num_wire_specs)
{
	e_routing_node_type type;
	int relative_x, relative_y;
	int i;

	for (i = 0; i < num_wire_specs; i++) {
		relative_x = wire_specs[i].relative_x;
		relative_y = wire_specs[i].relative_y;
		if (relative_x == 0 && relative_y == 0) {
			assert(false);
		} else if (relative_x == 0) {
			assert(relative_y != 0);
			if (relative_y > 0) {
				type = WIRE_N;
			} else {
				type = WIRE_S;
			}
		} else if (relative_y == 0) {
			assert(relative_x != 0 && relative_y == 0);
			if (relative_x > 0) {
				type = WIRE_E;
			} else {
				type = WIRE_W;
			}
		} else {
			assert(relative_x != 0 && relative_y != 0);
			if (wire_specs[i].is_horizontal) {
				if (relative_x > 0) {
					if (relative_y > 0) {
						type = WIRE_EN;
					} else {
						type = WIRE_ES;
					}
				} else {
					if (relative_y > 0) {
						type = WIRE_WN;
					} else {
						type = WIRE_WS;
					}
				}
			} else {
				if (relative_x > 0) {
					if (relative_y > 0) {
						type = WIRE_NE;
					} else {
						type = WIRE_SE;
					}
				} else {
					if (relative_y > 0) {
						type = WIRE_NW;
					} else {
						type = WIRE_SW;
					}
				}
			}
		}
		wire_specs[i].direction = type;
	}
}

void update_wire_count(INOUT s_wire_details *wire_specs, IN int num_wire_specs, INOUT int *num_wires)
{
	int total_freq;
	int wire_spec;
	int remainder;

	total_freq = 0;
	for (wire_spec = 0; wire_spec < num_wire_specs; wire_spec++) {
		total_freq += wire_specs[wire_spec].freq;
	}

	remainder = 0;
	for (wire_spec = 0; wire_spec < num_wire_specs; wire_spec++) {
		wire_specs[wire_spec].num_wires = *num_wires * wire_specs[wire_spec].freq / total_freq;
		remainder += (*num_wires * wire_specs[wire_spec].freq) % total_freq;
	}

	wire_spec = 0;
	/* distribute remaining tracks evenly across different track types */
	remainder = remainder / total_freq;
	while (remainder > 0) {
		wire_specs[wire_spec].num_wires++;
		wire_spec = (wire_spec + 1) % num_wire_specs;
		remainder--; //unidirectional routing have tracks in pairs
	}

	/* recalculate total tracks */
	*num_wires = 0;
	for (wire_spec = 0; wire_spec < num_wire_specs; wire_spec++) {
		*num_wires += wire_specs[wire_spec].num_wires;
	}
}

/* wires[clb_x][clb_y][is_horizontal][wire_spec][wire_index] */

/* bubbles up when "compare" returns true */
void sort(void *data, int elem_size, int num_elem, bool (*compare)(void *, void*))
{
	int i;
	bool swapped;
	void *temp;
	void *a;
	void *b;

	temp = malloc(elem_size);

	do {
		swapped = false;
		for (i = 0; i < num_elem-1; i++) {
			a = data + i*elem_size;
			b = data + (i+1)*elem_size;
			if (compare(a, b)) {
				memcpy(temp, a, elem_size);
				memcpy(a, b, elem_size);
				memcpy(b, temp, elem_size);
				swapped = true;
			}
		}
	} while (swapped);

	free(temp);
}

void clip_wire_spec(s_wire_details *wire_details, int x, int y, int clb_nx, int clb_ny)
{
	/* clip end points of wires */
	if (x + wire_details->relative_x < 0) {
		wire_details->relative_x = -x;
	} else 	if (x + wire_details->relative_x >= clb_nx) {
		wire_details->relative_x = clb_nx - 1 - x;
	}

	if (y + wire_details->relative_y < 0) {
		wire_details->relative_y = -y;
	} else if (y + wire_details->relative_y >= clb_ny) {
		wire_details->relative_y = clb_ny - 1 - y;
	}
}

void alloc_starting_wire_lookup(s_switch_box *switch_box)
{
	int i;

	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		switch_box->starting_wires_by_direction_and_type[i] = (s_wire ***)malloc(sizeof(s_wire **) * switch_box->num_wire_details);
	}
}

void alloc_starting_wire_counts(s_switch_box *switch_box)
{
	int i;

	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		switch_box->num_starting_wires_by_direction_and_type[i] = malloc(sizeof(int) * switch_box->num_wire_details);
	}
}

void set_starting_wire_counts_to_zero(s_switch_box *switch_box)
{
	int i, j;

	switch_box->num_starting_wires = 0;
	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		switch_box->num_starting_wire_types_by_direction[i] = 0;
		for (j = 0; j < switch_box->num_wire_details; j++) {
			switch_box->num_starting_wires_by_direction_and_type[i][j] = 0;
		}
	}
}

void count_starting_wires(s_switch_box *switch_box)
{
	int i;

	set_starting_wire_counts_to_zero(switch_box);

	for (i = 0; i < switch_box->num_wire_details; i++) {
		if (switch_box->wire_details[i].id >= 0) {
			switch_box->num_starting_wires += switch_box->wire_details[i].num_wires;
			switch_box->num_starting_wire_types_by_direction[switch_box->wire_details[i].direction]++;
			switch_box->num_starting_wires_by_direction_and_type[switch_box->wire_details[i].direction][switch_box->wire_details[i].id] = switch_box->wire_details[i].num_wires;
		}
	}
}

void alloc_starting_wire_array_and_lookup(s_switch_box *switch_box)
{
	int i, j;

	count_starting_wires(switch_box);

	switch_box->starting_wires = malloc(sizeof(s_wire) * switch_box->num_starting_wires);
	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		switch_box->starting_wires_by_direction_and_type[i] = (s_wire ***)malloc(sizeof(s_wire **) * switch_box->num_wire_details);
		for (j = 0; j < switch_box->num_wire_details; j++) {
			switch_box->starting_wires_by_direction_and_type[i][j] = (s_wire **)malloc(sizeof(s_wire *) * switch_box->num_starting_wires_by_direction_and_type[i][j]);
		}
	}
}

void alloc_and_init_wire_details(s_switch_box *switch_box, int x, int y, int clb_nx, int clb_ny, s_wire_details *wire_details, int num_wire_details)
{
	int i;
	s_wire_details temp_wire_details;

	switch_box->wire_details = malloc(sizeof(s_wire_details) * num_wire_details);
	switch_box->num_wire_details = num_wire_details;

	for (i = 0; i < switch_box->num_wire_details; i++) {
		temp_wire_details = wire_details[i];

		clip_wire_spec(&temp_wire_details, x, y, clb_nx, clb_ny);

		switch_box->wire_details[temp_wire_details.id] = temp_wire_details;

		/* skip wires that are not moving anywhere */
		if (temp_wire_details.relative_x == 0 && temp_wire_details.relative_y == 0) {
			switch_box->wire_details[temp_wire_details.id].id = -1;
		}
	}
}

void init_starting_wire_array_and_lookup(s_switch_box *switch_box, int x, int y, int *global_routing_node_id)
{
	int i;
	int wire;
	s_wire_details *wire_details;

	set_starting_wire_counts_to_zero(switch_box);

	for (i = 0; i < switch_box->num_wire_details; i++) {
		wire_details = &switch_box->wire_details[i];

		if (wire_details->id >= 0) {
			switch_box->num_starting_wire_types_by_direction[wire_details->direction]++;
			switch_box->num_starting_wires_by_direction_and_type[wire_details->direction][wire_details->id] = wire_details->num_wires;

			for (wire = 0; wire < wire_details->num_wires; wire++) {
				switch_box->starting_wires[switch_box->num_starting_wires].super.type = WIRE;
				switch_box->starting_wires[switch_box->num_starting_wires].super.id = (*global_routing_node_id)++;
				switch_box->starting_wires[switch_box->num_starting_wires].details = &switch_box->wire_details[wire_details->id];
				switch_box->starting_wires[switch_box->num_starting_wires].sb_x = x;
				switch_box->starting_wires[switch_box->num_starting_wires].sb_y = y;
				init_list(&switch_box->starting_wires[switch_box->num_starting_wires].fanout);
				init_list(&switch_box->starting_wires[switch_box->num_starting_wires].fanin);

				switch_box->starting_wires_by_direction_and_type[wire_details->direction][wire_details->id][wire] = &switch_box->starting_wires[switch_box->num_starting_wires];

				switch_box->num_starting_wires++;
			}
		}
	}
}

/* we assume all the wire_specs are unique */
void init_starting_wires(s_block *clb, int clb_nx, int clb_ny, s_wire_details *wire_details, int num_wire_details, int num_wires_per_clb, int *global_routing_node_id)
{
	s_switch_box *switch_box;

	switch_box = clb->switch_box;

	alloc_and_init_wire_details(switch_box, clb->x, clb->y, clb_nx, clb_ny, wire_details, num_wire_details);
	alloc_starting_wire_counts(switch_box);
	alloc_starting_wire_array_and_lookup(switch_box);
	init_starting_wire_array_and_lookup(switch_box, clb->x, clb->y, global_routing_node_id);
}

void init_ending_wires(s_block **clbs, int clb_nx, int clb_ny)
{
	int x;
	int y;
	s_switch_box *src_switch_box;
	s_switch_box *dst_switch_box;
	s_wire_details *starting_wire_spec;
	int wire;
	int dst_clb_x;
	int dst_clb_y;
	int i;
	int j;
	int pass;
	int track;

	for (x = 0; x < clb_nx; x++) {
		for (y = 0; y < clb_ny; y++) {
			for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
				clbs[x][y].switch_box->ending_wires = NULL;
				clbs[x][y].switch_box->num_ending_wires = 0;
				clbs[x][y].switch_box->num_ending_wire_types_by_direction[i] = 0;
				clbs[x][y].switch_box->ending_wires_by_direction_and_type[i] = (s_wire ***)malloc(sizeof(s_wire **) * clbs[x][y].switch_box->num_wire_details);
				clbs[x][y].switch_box->num_ending_wires_by_direction_and_type[i] = malloc(sizeof(int) * clbs[x][y].switch_box->num_wire_details);
				for (j = 0; j < clbs[x][y].switch_box->num_wire_details; j++) {
					clbs[x][y].switch_box->num_ending_wires_by_direction_and_type[i][j] = 0;
				}
			}
		}
	}

	for (pass = 0; pass < 2; pass++) {
		for (x = 0; x < clb_nx; x++) {
			for (y = 0; y < clb_ny; y++) {
				src_switch_box = clbs[x][y].switch_box;

				for (wire = 0; wire < src_switch_box->num_starting_wires; wire++) {
					starting_wire_spec = src_switch_box->starting_wires[wire].details;

					dst_clb_x = x + starting_wire_spec->relative_x;
					dst_clb_y = y + starting_wire_spec->relative_y;

					assert(dst_clb_x >= 0 && dst_clb_x < clb_nx && dst_clb_y >= 0 && dst_clb_y < clb_ny);

					dst_switch_box = clbs[dst_clb_x][dst_clb_y].switch_box;

					if (pass > 0) {
						if (!dst_switch_box->ending_wires) {
							dst_switch_box->ending_wires = malloc(sizeof(s_wire *)*dst_switch_box->num_ending_wires);
							for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
								for (j = 0; j < dst_switch_box->num_wire_details; j++) {
									dst_switch_box->ending_wires_by_direction_and_type[i][j] = (s_wire **)malloc(sizeof(s_wire *) * dst_switch_box->num_ending_wires_by_direction_and_type[i][j]);
									dst_switch_box->num_ending_wires_by_direction_and_type[i][j] = 0;
								}
							}

							dst_switch_box->num_ending_wires = 0;
						}

						dst_switch_box->ending_wires[dst_switch_box->num_ending_wires] = &src_switch_box->starting_wires[wire];
						track = dst_switch_box->num_ending_wires_by_direction_and_type[starting_wire_spec->direction][starting_wire_spec->id];
						dst_switch_box->ending_wires_by_direction_and_type[starting_wire_spec->direction][starting_wire_spec->id][track] = &src_switch_box->starting_wires[wire];

						dst_switch_box->num_ending_wires_by_direction_and_type[starting_wire_spec->direction][starting_wire_spec->id]++;
						dst_switch_box->num_ending_wires++;
					} else {
						if (dst_switch_box->num_ending_wires_by_direction_and_type[starting_wire_spec->direction][starting_wire_spec->id] == 0) {
							dst_switch_box->num_ending_wire_types_by_direction[starting_wire_spec->direction]++; /* TODO: error here */
						}
						dst_switch_box->num_ending_wires_by_direction_and_type[starting_wire_spec->direction][starting_wire_spec->id]++;
						dst_switch_box->num_ending_wires++;
					}
				}
			}
		}
	}

//	for (x = 0; x < clb_nx; x++) {
//		for (y = 0; y < clb_ny; y++) {
//			src_switch_box = clbs[x][y].switch_box;
//
//			sort(src_switch_box->ending_wires, sizeof(s_wire *), src_switch_box->num_ending_wires, wire_type_ptr_greater_than);
//
//			for (i = 0; i < NUM_WIRE_TYPE; i++) {
//				src_switch_box->ending_wire_offset[i] = src_switch_box->num_ending_wires; /* invalid offset */
//			}
//			type = src_switch_box->ending_wires[0]->spec.type;
//			src_switch_box->ending_wire_offset[type] = 0;
//			for (i = 1; i < src_switch_box->num_ending_wires; i++) {
//				if (src_switch_box->ending_wires[i]->spec.type != type) {
//					type = src_switch_box->ending_wires[i]->spec.type;
//					src_switch_box->ending_wire_offset[type] = i;
//				}
//			}
//		}
//	}
}

int get_next_non_zero_element_offset(int *array, int size, int current_offset, int forward)
{
	current_offset = current_offset % size;
	while (forward > 0) {
		if (array[current_offset] > 0) {
			forward--;
		}

		if (forward > 0) {
			current_offset = (current_offset+1) % size;
		}
	}
	return current_offset;
}

/* three level offset [direction][type][track] */
void init_clb_output_pins(s_block *clb, int num_output_pins, float fc_out, int *global_routing_node_id)
{
	s_switch_box *switch_box;
	int output_pin;
	int actual_fc_out;
	int direction;
	int types[NUM_WIRE_DIRECTIONS];
	int *tracks[NUM_WIRE_DIRECTIONS];
	int type, track;
	int num_connected_wires;
	int i, j;

	switch_box = clb->switch_box;
	actual_fc_out = switch_box->num_starting_wires * fc_out;

	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		tracks[i] = malloc(sizeof(int) * switch_box->num_wire_details);
	}
	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		for (j = 0; j < switch_box->num_wire_details; j++) {
			tracks[i][j] = 0;
		}
	}

	clb->output_pins = malloc(sizeof(s_pin) * num_output_pins);
	clb->num_output_pins = num_output_pins;

	for (output_pin = 0; output_pin < clb->num_output_pins; output_pin++) {
		clb->output_pins[output_pin].super.id = (*global_routing_node_id)++;
		clb->output_pins[output_pin].super.type = OPIN;
		init_list(&clb->output_pins[output_pin].connections);

		direction = get_next_non_zero_element_offset(switch_box->num_starting_wire_types_by_direction, NUM_WIRE_DIRECTIONS, 0, output_pin+1);
		for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
			if (switch_box->num_starting_wire_types_by_direction[i] > 0) {
				types[i] = get_next_non_zero_element_offset(switch_box->num_starting_wires_by_direction_and_type[i], switch_box->num_wire_details, 0, 1);
			}
		}

		num_connected_wires = 0;
		while (num_connected_wires < actual_fc_out) {
			type = types[direction];
			track = tracks[direction][type];
			//VALGRIND_PRINTF_BACKTRACE("%d %d %d %d", clb->x, clb->y, type, direction);
			insert_into_list(&clb->output_pins[output_pin].connections, switch_box->starting_wires_by_direction_and_type[direction][type][track]);
			num_connected_wires++;

			tracks[direction][type] = (track+1) % switch_box->num_starting_wires_by_direction_and_type[direction][type];
			types[direction] = get_next_non_zero_element_offset(switch_box->num_starting_wires_by_direction_and_type[direction], switch_box->num_wire_details, type+1, 1);

			direction = get_next_non_zero_element_offset(switch_box->num_starting_wire_types_by_direction, NUM_WIRE_DIRECTIONS, direction+1, 1);
		}
	}

	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		free(tracks[i]);
	}
}

void init_clb_input_pins(s_block *clb, int num_input_pins, float fc_in, int *global_routing_node_id)
{
	int input_pin;
	int actual_fc_in;
	int num_connected_wires;
	int i;
	int j;
	int type;
	int track;
	int direction;
	int types[NUM_WIRE_DIRECTIONS];
	int *tracks[NUM_WIRE_DIRECTIONS];
	s_switch_box *switch_box;

	switch_box = clb->switch_box;
	actual_fc_in = switch_box->num_ending_wires * fc_in;
	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		tracks[i] = malloc(sizeof(int) * switch_box->num_wire_details);
	}
	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		for (j = 0; j < switch_box->num_wire_details; j++) {
			tracks[i][j] = 0;
		}
	}
	clb->input_pins = malloc(sizeof(s_pin) * num_input_pins);
	clb->num_input_pins = num_input_pins;
	for (input_pin = 0; input_pin < clb->num_input_pins; input_pin++) {
		clb->input_pins[input_pin].super.id = (*global_routing_node_id)++;
		clb->input_pins[input_pin].super.type = IPIN;
		init_list(&clb->input_pins[input_pin].connections);

		direction = get_next_non_zero_element_offset(switch_box->num_ending_wire_types_by_direction, NUM_WIRE_DIRECTIONS, 0, input_pin+1);
		for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
			if (switch_box->num_ending_wire_types_by_direction[i] > 0) {
				types[i] = get_next_non_zero_element_offset(switch_box->num_ending_wires_by_direction_and_type[i], switch_box->num_wire_details, 0, 1);
			}
		}

		num_connected_wires = 0;
		while (num_connected_wires < actual_fc_in) {
			type = types[direction];
			track = tracks[direction][type];
			//VALGRIND_PRINTF_BACKTRACE("%d %d %d %d", clb->x, clb->y, type, direction);
			insert_into_list(&clb->input_pins[input_pin].connections, switch_box->ending_wires_by_direction_and_type[direction][type][track]);
			num_connected_wires++;

			tracks[direction][type] = (track+1) % switch_box->num_ending_wires_by_direction_and_type[direction][type];
			types[direction] = get_next_non_zero_element_offset(switch_box->num_ending_wires_by_direction_and_type[direction], switch_box->num_wire_details, type+1, 1);

			direction = get_next_non_zero_element_offset(switch_box->num_ending_wire_types_by_direction, NUM_WIRE_DIRECTIONS, direction+1, 1);
		}
	}
}

void init_inter_switch_box(s_switch_box *switch_box, int fs)
{
	s_wire *ending_wire;
	s_wire_details *ending_wire_details;
	int i;
	int j;
	int direction;
	int type;
	int track;
	int types[NUM_WIRE_DIRECTIONS];
	int *tracks[NUM_WIRE_DIRECTIONS];
	e_wire_direction possible_directions[3];
	int num_possible_connections;
	int actual_fs;

	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		tracks[i] = malloc(sizeof(int) * switch_box->num_wire_details);
	}

	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		for (j = 0; j < switch_box->num_wire_details; j++) {
			tracks[i][j] = 0;
		}
	}

	/* loop for each ending wire */
	for (i = 0; i < switch_box->num_ending_wires; i++) {
		ending_wire = switch_box->ending_wires[i];
		ending_wire_details = ending_wire->details;

		switch (ending_wire_details->direction) {
		case WIRE_E:
			possible_directions[0] = WIRE_N;
			possible_directions[1] = WIRE_S;
			possible_directions[2] = WIRE_E;
			break;
		case WIRE_W:
			possible_directions[0] = WIRE_N;
			possible_directions[1] = WIRE_S;
			possible_directions[2] = WIRE_W;
			break;
		case WIRE_N:
			possible_directions[0] = WIRE_E;
			possible_directions[1] = WIRE_W;
			possible_directions[2] = WIRE_N;
			break;
		case WIRE_S:
			possible_directions[0] = WIRE_E;
			possible_directions[1] = WIRE_W;
			possible_directions[2] = WIRE_S;
			break;
		default:
			/* not handling bent wires for now */
			assert(false);
			break;
		}

		/* no restriction to the dsetination type */
		num_possible_connections = 0;
		for (j = 0; j < 3; j++) {
			direction = possible_directions[j];
			if (direction == ending_wire_details->direction) {
				type = ending_wire_details->id;
				num_possible_connections += switch_box->num_starting_wires_by_direction_and_type[direction][type];
			} else {
				for (type = 0; type < switch_box->num_wire_details; type++) {
					num_possible_connections += switch_box->num_starting_wires_by_direction_and_type[direction][type];
				}
			}
		}
		actual_fs = fmin(fs, num_possible_connections);

		for (j = 0; j < 3; j++) {
			direction = possible_directions[j];
			if (switch_box->num_starting_wire_types_by_direction[direction] > 0) {
				types[direction] = get_next_non_zero_element_offset(switch_box->num_starting_wires_by_direction_and_type[direction], switch_box->num_wire_details, 0, 1);
			}
		}

		while (actual_fs > 0) {
			for (j = 0; j < 3; j++) {
				direction = possible_directions[j];
				if (direction == ending_wire_details->direction) {
					type = ending_wire_details->id;
					track = tracks[direction][type];
					if (switch_box->num_starting_wires_by_direction_and_type[direction][type] > 0) {
						insert_into_list(&ending_wire->fanout, switch_box->starting_wires_by_direction_and_type[direction][type][track]);

						tracks[direction][type] = (track+1) % switch_box->num_starting_wires_by_direction_and_type[direction][type];
						actual_fs--;
					}
				} else {
					if (switch_box->num_starting_wire_types_by_direction[direction] > 0) {
						type = types[direction];
						track = tracks[direction][type];

						insert_into_list(&ending_wire->fanout, switch_box->starting_wires_by_direction_and_type[direction][type][track]);

						types[direction] = get_next_non_zero_element_offset(switch_box->num_starting_wires_by_direction_and_type[direction], switch_box->num_wire_details, type+1, 1);
						tracks[direction][type] = (track+1) % switch_box->num_starting_wires_by_direction_and_type[direction][type];
						actual_fs--;
					}
				}
			}
		}
	}
}

void dump_wire(s_wire *wire)
{
	assert(wire->super.type == WIRE);
	printf("%10s[id=%4d,x=%3d,y=%3d,type=%3d,rel_x=%3d,rel_y=%3d] ",
			wire->details->name, wire->super.id, wire->sb_x, wire->sb_y,
			wire->details->id, wire->details->relative_x, wire->details->relative_y);
}

void dump_clb(s_block *clb)
{
	int pin;
	s_list_item *item;
	s_wire *wire;
	int i, j, k;
	int count;

	printf("CLB [%d,%d]\n", clb->x, clb->y);

	for (pin = 0; pin < clb->num_output_pins; pin++) {
		item = clb->output_pins[pin].connections.head;
		printf("OPIN %d -> \n", pin);
		while (item) {
			wire = (s_wire *)item->data;
			printf("\t"); dump_wire(wire); printf("\n");
			item = item->next;
		}
		printf("\n");
	}
	printf("\n");

	printf("Starting wires\n");
	for (pin = 0; pin < clb->switch_box->num_starting_wires; pin++) {
		item = clb->switch_box->starting_wires[pin].fanout.head;
		dump_wire(&clb->switch_box->starting_wires[pin]); printf(" ->\n");
		while (item) {
			wire = (s_wire *)item->data;
			printf("\t"); dump_wire(wire); printf("\n");
			item = item->next;
		}
		printf("\n");
	}
	printf("\n");

	printf("Starting wires by type\n");
	count = 0;
	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		for (j = 0; j < clb->switch_box->num_wire_details; j++) {
			for (k = 0; k < clb->switch_box->num_starting_wires_by_direction_and_type[i][j]; k++) {
				wire = clb->switch_box->starting_wires_by_direction_and_type[i][j][k];
				assert(wire);
				dump_wire(wire);
				printf("\n");
				count++;
			}
		}
	}
	printf("\n");
	assert(count == clb->switch_box->num_starting_wires);

	for (pin = 0; pin < clb->num_input_pins; pin++) {
		item = clb->input_pins[pin].connections.head;
		printf("IPIN %d -> \n", pin);
		while (item) {
			wire = (s_wire *)item->data;
			printf("\t"); dump_wire(wire); printf("\n");
			item = item->next;
		}
		printf("\n");
	}
	printf("\n");

	printf("Ending wires\n");
	for (pin = 0; pin < clb->switch_box->num_ending_wires; pin++) {
		item = clb->switch_box->ending_wires[pin]->fanout.head;
		dump_wire(clb->switch_box->ending_wires[pin]); printf(" ->\n");
		while (item) {
			wire = (s_wire *)item->data;
			printf("\t"); dump_wire(wire); printf("\n");
			item = item->next;
		}
		printf("\n");
	}
	printf("\n");

	printf("Ending wires by type\n");
	count = 0;
	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
		for (j = 0; j < clb->switch_box->num_wire_details; j++) {
			for (k = 0; k < clb->switch_box->num_ending_wires_by_direction_and_type[i][j]; k++) {
				wire = clb->switch_box->ending_wires_by_direction_and_type[i][j][k];
				assert(wire);
				dump_wire(wire);
				printf("\n");
				count++;
			}
		}
	}
	printf("\n");
	assert(count == clb->switch_box->num_ending_wires);
}

//void init_clb(s_block *clb, int x, int y,
//		s_wire_specs *wire_specs, int num_wire_specs,
//		int num_wires_per_clb, int num_input_pins, int num_output_pins,
//		int *global_routing_node_id)
//{
//	clb->x = x;
//	clb->y = y;
//
//	clb->switch_box = malloc(sizeof(s_switch_box));
//	init_switch_box(clb->switch_box, clb->x, clb->y, wire_specs, num_wire_specs, num_wires_per_clb, global_routing_node_id);
//}

void init(s_wire_details *wire_specs, int num_wire_specs,
		int num_wires_per_clb, int num_input_pins, int num_output_pins)
{
	s_block **clbs;
	const int clb_nx = 4;
	const int clb_ny = 4;
	const float fc_out = 0.5;
	const float fc_in = 1;
	const int fs = 3;
	int x, y;
	int global_routing_node_id;

	clbs = malloc(clb_nx * sizeof(s_block *));
	for (x = 0; x < clb_nx; x++) {
		clbs[x] = malloc(clb_ny * sizeof(s_block));
	}

	global_routing_node_id = 0;
	for (x = 0; x < clb_nx; x++) {
		for (y = 0; y < clb_ny; y++) {
			clbs[x][y].x = x;
			clbs[x][y].y = y;
			clbs[x][y].switch_box = malloc(sizeof(s_switch_box));
			init_starting_wires(&clbs[x][y], clb_nx, clb_ny, wire_specs, num_wire_specs, num_wires_per_clb, &global_routing_node_id);
		}
	}

	init_ending_wires(clbs, clb_nx, clb_ny);
	for (x = 0; x < clb_nx; x++) {
		for (y = 0; y < clb_ny; y++) {
			init_clb_output_pins(&clbs[x][y], num_output_pins, fc_out, &global_routing_node_id);
			init_clb_input_pins(&clbs[x][y], num_input_pins, fc_in, &global_routing_node_id);
			init_inter_switch_box(clbs[x][y].switch_box, fs);
		}
	}

	for (x = 0; x < clb_nx; x++) {
		for (y = 0; y < clb_ny; y++) {
			dump_clb(&clbs[x][y]);
		}
	}
}
//
//void connect_channel_to_switch_box(s_block *channel, s_block *switch_box)
//{
//}
//
//void setup_block_connection(s_block *from_block, s_block *to_block, s_list ***lookup) /* lookup[from_side][from_pin][to_side] */
//{
//	e_side from_side, to_side;
//	int from_pin, to_pin;
//	for (from_side = TOP; from_side < SIDE_END; from_side++) {
//		to_pin = lookup[from_side][from_pin][to_side];
//
//		if (from_block == to_block) {
//			assert(from_block->pins[from_side][from_pin].type == BLOCK_INPUT && to_block->pins[to_side][to_pin].type == BLOCK_OUTPUT);
//		} else {
//			assert(from_block->pins[from_side][from_pin].type == BLOCK_OUTPUT && to_block->pins[to_side][to_pin].type == BLOCK_INPUT);
//		}
//
//		insert_into_list(from_block->pins[from_side][from_pin].fanout, &to_block->pins[to_side][to_pin]);
//	}
//}

//int get_track_low_segment(s_track *tracks, int channel, int track, int segment)
//{
//	int low_seg;
//	int staggering_offset;
//	int staggered_start;
//
//	staggering_offset = channel - 1;
//	staggered_start = tracks[track].start - (staggering_offset % tracks[track].length);
//
//	/* adding tracks[track].length is to make sure segment - staggered_start is always positive */
//	low_seg = segment - (segment - staggered_start + tracks[track].length) % tracks[track].length;
//	if (low_seg < 2) {
//		low_seg = 2;
//	}
//
//	return low_seg;
//}
//
//int get_track_high_segment(s_track *tracks, int channel, int track, int segment, int low_seg, int seg_max)
//{
//	int high_seg;
//	int first_full_seg_low;
//	int staggering_offset;
//	int staggered_start;
//
//	staggering_offset = channel - 1;
//	staggered_start = tracks[track].start - (staggering_offset % tracks[track].length);
//
//	high_seg = low_seg + tracks[track].length - 2;
//
//	if (low_seg == 2) {
//		/* adding tracks[track].length is required to make sure staggered_start is positive */
//		first_full_seg_low = (staggered_start + tracks[track].length) % tracks[track].length;
//		if(first_full_seg_low > 2)
//		{
//			/* then we stop just before the first full seg */
//			high_seg = first_full_seg_low - 2;
//		}
//	}
//
//	if (high_seg > seg_max) {
//		high_seg = seg_max;
//	}
//
//	return high_seg;
//}
//
//int *get_starting_tracks(int channel, int segment, bool is_increasing, int seg_max, s_track *tracks, int num_tracks, int *num_starting_tracks)
//{
//	int itrack;
//	int *starting_tracks;
//	int low_seg, high_seg;
//
//	assert(channel >= 1 && channel%2 == 1 && segment >= 2 && segment%2 == 0);
//
//	*num_starting_tracks = 0;
//	for (itrack = 0; itrack < num_tracks; itrack++) {
//		if (tracks[itrack].is_increasing == is_increasing) {
//			low_seg = get_track_low_segment(tracks, channel, itrack, segment);
//			high_seg = get_track_high_segment(tracks, channel, itrack, segment, low_seg, seg_max);
//			if ((is_increasing && segment == low_seg) || (!is_increasing && segment == high_seg)) {
//				(*num_starting_tracks)++;
//			}
//		}
//	}
//	starting_tracks = malloc(*num_starting_tracks * sizeof(int));
//	*num_starting_tracks = 0;
//	for (itrack = 0; itrack < num_tracks; itrack++) {
//		if (tracks[itrack].is_increasing == is_increasing) {
//			low_seg = get_track_low_segment(tracks, channel, itrack, segment);
//			high_seg = get_track_high_segment(tracks, channel, itrack, segment, low_seg, seg_max);
//			if ((is_increasing && segment == low_seg) || (!is_increasing && segment == high_seg)) {
//				starting_tracks[(*num_starting_tracks)++] = itrack;
//			}
//		}
//	}
//
//	return starting_tracks;
//}

//void load_track_start(s_track_info *track_info, int num_track_types, int *num_tracks, int *track_start)
//{
//	int itype;
//	int *
//	for (itype = 0; itype < num_track_types; itype++) {
//		num_tracks
//	}
//}
//
///* need to consider pass-throughs for long tracks */
//void setup_disjoint_switch_box_internal_connection(s_block **grid, int x, int y, int num_track)
//{
//	int itrack, iside;
//	int *starting_tracks[4];
//	char is_core;
//
//	assert(grid[x][y].type == SWITCH_BOX);
//
//	is_core = (x >= 3 && y >= 3);
//
//	for (iside = 0; iside < SIDE_END; iside++) {
//
//	}
//
//
//
//	get_starting_tracks();
//}
//
//void create_channels(s_block **grid, int nx, int ny, s_segment_info *seg_info, int num_tracks)
//{
//	int x, y;
//	int itrack;
//
//	/* x channels */
//	for (x = 1; x < nx; x += 2) {
//		for (y = 1; y < ny; y += 2) {
//			grid[x][y].pins[LEFT] = malloc(num_tracks*sizeof(s_block_pin));
//			grid[x][y].pins[RIGHT] = malloc(num_tracks*sizeof(s_block_pin));
//			grid[x][y].pins[TOP] = NULL;
//			grid[x][y].pins[BOTTOM] = NULL;
//
//			for (itrack = 0; itrack < num_tracks; itrack++) {
//				grid[x][y].type = X_CHANNEL;
//				if (seg_info[itrack].direction == INC_DIRECTION) {
//					grid[x][y].pins[LEFT][itrack].type = BLOCK_INPUT;
//					grid[x][y].pins[LEFT][itrack].fanout = alloc_list_node();
//					insert_into_list(grid[x][y].pins[LEFT][itrack].fanout, &grid[x][y].pins[RIGHT][itrack]);
//
//					grid[x][y].pins[RIGHT][itrack].type = BLOCK_OUTPUT;
//				} else {
//					assert(seg_info[itrack].direction == DEC_DIRECTION);
//					grid[x][y].pins[RIGHT][itrack].type = BLOCK_INPUT;
//					grid[x][y].pins[RIGHT][itrack].fanout = alloc_list_node();
//					insert_into_list(grid[x][y].pins[RIGHT][itrack].fanout, &grid[x][y].pins[LEFT][itrack]);
//
//					grid[x][y].pins[LEFT][itrack].type = BLOCK_OUTPUT;
//				}
//			}
//		}
//	}
//
//	/* y channels */
//	for (x = 1; x < nx; x += 2) {
//		for (y = 1; y < ny; y += 2) {
//			grid[x][y].pins[TOP] = malloc(num_tracks*sizeof(s_block_pin));
//			grid[x][y].pins[BOTTOM] = malloc(num_tracks*sizeof(s_block_pin));
//			grid[x][y].pins[LEFT] = NULL;
//			grid[x][y].pins[RIGHT] = NULL;
//
//			for (itrack = 0; itrack < num_tracks; itrack++) {
//				grid[x][y].type = Y_CHANNEL;
//				if (seg_info[itrack].direction == INC_DIRECTION) {
//					grid[x][y].pins[BOTTOM][itrack].type = BLOCK_INPUT;
//					grid[x][y].pins[BOTTOM][itrack].fanout = alloc_list_node();
//					insert_into_list(grid[x][y].pins[BOTTOM][itrack].fanout, &grid[x][y].pins[TOP][itrack]);
//
//					grid[x][y].pins[TOP][itrack].type = BLOCK_OUTPUT;
//				} else {
//					assert(seg_info[itrack].direction == DEC_DIRECTION);
//					grid[x][y].pins[TOP][itrack].type = BLOCK_INPUT;
//					grid[x][y].pins[TOP][itrack].fanout = alloc_list_node();
//					insert_into_list(grid[x][y].pins[TOP][itrack].fanout, &grid[x][y].pins[BOTTOM][itrack]);
//
//					grid[x][y].pins[BOTTOM][itrack].type = BLOCK_OUTPUT;
//				}
//			}
//		}
//	}
//}
//
//void create_switch_boxes(s_block **grid, int nx, int ny)
//{
//	int x, y;
//
//	for (x = 1; x < nx; x += 2) {
//		for (y = 1; y < ny; y += 2) {
//			setup_disjoint_switch_box_internal_connection(grid, x, y)
//		}
//	}
//}
//
//void connect_channels_and_switch_boxes()
//{
//
//}
//
///* CLB OPIN -> CHAN IPIN -> CHAN OPIN -> SB IPIN -> SB OPIN -> CHAN IPIN -> CHAN OPIN -> CLB IPIN */
//void build_rr_from_source(s_block_pin *pin)
//{
//	s_list *current_fanout;
//	s_block_pin *current_pin;
//	s_rr_node *rr_node;
//	switch (pin->block->type) {
//	case CLB:
//		current_fanout = pin->fanout;
//		while (current_fanout) {
//			current_pin = current_fanout->data;
//			assert(current_pin->block->type == X_CHANNEL || current_pin->block->type == Y_CHANNEL);
//
//
//
//			/* find existing rr_node, if not found, alloc node */
//			rr_node = find_rr_node();
//			if (rr_node == NULL) {
//				rr_node = alloc_rr_node();
//			}
//
//			if (rr_node->children) {
//				insert_into_list(rr_node->children, data);
//			} else {
//				rr_node->children = create_list(data);
//			}
//
//			build_rr_from_source(current_pin);
//			current_fanout = current_fanout->next;
//		}
//		break;
//	case SWITCH_BOX:
//		current_fanout = pin->fanout;
//		while (current_fanout) {
//			current_pin = current_fanout->data;
//
//			if (current_pin->block->type == X_CHANNEL || current_pin->block->type == Y_CHANNEL) {
//				/* check whether the chanx or chany rr node exists */
//				/* connect current
//			}
//
//			/* find existing rr_node, if not found, alloc node */
//			rr_node = find_rr_node();
//			if (rr_node == NULL) {
//				rr_node = alloc_rr_node();
//			}
//
//			if (rr_node->children) {
//				insert_into_list(rr_node->children, data);
//			} else {
//				rr_node->children = create_list(data);
//			}
//
//			build_rr_from_source(current_pin);
//			current_fanout = current_fanout->next;
//		}
//	case X_CHANNEL:
//	case Y_CHANNEL:
//	default:
//		printf("Unexpected block type");
//		break
//	}
//}
//
//void build_actual_rr(s_block **grid, int nx, int ny)
//{
//	int x, y;
//	int side;
//	int pin;
//
//	for (x = 0; x < nx; x++) {
//		for (y = 0; y < ny; y++) {
//			if (grid[x][y].type == CLB) {
//				for (side = 0; side < SIDE_END; side++) {
//					for (pin = 0; pin < grid[x][y].num_pins[side]; pin++) {
//						if (grid[x][y].pins[side][pin].type == BLOCK_OUTPUT) {
//							build_rr_from_source(&grid[x][y].pins[side][pin]);
//						}
//					}
//				}
//			}
//		}
//	}
//}
//

/* lookup[x][y][rr_type][ptc_number] */
//s_rr_node *****alloc_rr_node_lookup(int nx, int ny)
//{
//	int x, y, type, ptc;
//	s_rr_node *****lookup;
//
//	lookup = malloc(sizeof(void *) * nx);
//	for (x = 0; x < nx; x++) {
//		lookup[x] = malloc(sizeof(void *) * ny);
//		for (y = 0; y < ny; y++) {
//			lookup[x][y] = malloc(sizeof(void *) * RR_TYPE_END);
//			for (type = 0; type < RR_TYPE_END; type++) {
//				lookup[x][y][type] = malloc(sizeof(void *) * 200);
//				for (ptc = 0; ptc < 200; ptc++) {
//					lookup[x][y][type][ptc] = NULL;
//				}
//			}
//		}
//	}
//
//	return lookup;
//}
//
//void add_rr_node_to_lookup(s_rr_node *node, s_rr_node *****rr_node_lookup, int *num_rr_nodes)
//{
//	int x, y;
//
//	switch (node->type) {
//	case CHANX:
//		assert (node->ylow == node->yhigh);
//
//		y = node->ylow;
//
//		for (x = node->xlow; x <= node->xhigh; x++) {
//			rr_node_lookup[x][y][node->type][node->ptc_number] = node;
//		}
//
//		break;
//	case CHANY:
//		assert (node->xlow == node->xhigh);
//
//		x = node->xlow;
//
//		for (y = node->ylow; y <= node->yhigh; y++) {
//			rr_node_lookup[x][y][node->type][node->ptc_number] = node;
//		}
//
//		break;
//	default:
//		break;
//	}
//
//}
//
//s_rr_node *alloc_rr_node()
//{
//	s_rr_node *node;
//	node = malloc(sizeof(s_rr_node));
//	//node->children = NULL;
//	return node;
//}

//bool is_valid_channel(int x, int y) {
//	return true;
//}
//
//int get_seg_max(bool is_horizontal, int nx, int ny)
//{
//	if (is_horizontal) {
//		return nx - 1 - 1;
//	} else {
//		return ny - 1 - 1;
//	}
//}
//
//void build_block_pins(int x, int y)
//{
//
//}

//void build_channel(int channel, int segment, bool is_increasing, bool is_horizontal,
//		int nx, int ny, int num_tracks, s_track *tracks,
//		s_rr_node *****rr_node_lookup, int *num_rr_nodes)
//{
//	int num_starting_tracks;
//	int *starting_tracks;
//	int i;
//	int track;
//	s_rr_node *node;
//
//	starting_tracks = get_starting_tracks(channel, segment, is_increasing, get_seg_max(is_horizontal, nx, ny), tracks, num_tracks, &num_starting_tracks);
//
//	for (i = 0; i < num_starting_tracks; i++) {
//		track = starting_tracks[i];
//
//		assert(tracks[track].is_increasing == is_increasing);
//
//		node = alloc_rr_node();
//		node->index = *num_rr_nodes;
//		node->ptc_number = track;
//		node->is_increasing = tracks[track].is_increasing;
//		if (is_horizontal) {
//			node->type = CHANX;
//			node->xlow = get_track_low_segment(tracks, channel, track, segment);
//			node->xhigh = get_track_high_segment(tracks, channel, track, segment, node->xlow, get_seg_max(is_horizontal, nx, ny));
//			node->ylow = node->yhigh = channel;
//		} else {
//			node->type = CHANY;
//			node->ylow = get_track_low_segment(tracks, channel, track, segment);
//			node->yhigh = get_track_high_segment(tracks, channel, track, segment, node->ylow, get_seg_max(is_horizontal, nx, ny));
//			node->xlow = node->xhigh = channel;
//		}
//
//		add_rr_node_to_lookup(node, rr_node_lookup, num_rr_nodes);
//	}
//}

//s_rr_node *alloc_and_load_rr_node_array(s_rr_node *****rr_node_lookup, int *num_rr_nodes)
//{
//	s_rr_node *rr_nodes;
//	rr_nodes = malloc(sizeof(s_rr_node) * *num_rr_nodes);
//
//	//for
//	return rr_nodes;
//}
//void build_channels()
//{
//	//for ()
//}

//s_rr_node *get_rr_node(int x, int y, e_rr_type type, int ptc_number, s_rr_node *****rr_node_lookup)
//{
//	return rr_node_lookup[x][y][type][ptc_number];
//}
//
//void add_rr_node_fanout(s_rr_node *src_node, s_rr_node *dst_node)
//{
//	if (src_node->children) {
//		insert_into_list(src_node->children, dst_node);
//	} else {
//		src_node->children = create_list(dst_node);
//	}
//}

//void connect_channel(int channel, int segment, bool is_increasing, bool is_horizontal,
//		int nx, int ny, int num_tracks, s_track *tracks, s_rr_node *****rr_node_lookup)
//{
//	int seg;
//	int num_starting_tracks;
//	int *starting_tracks;
//	int track;
//	int i;
//	int low_seg, high_seg;
//	int direction;
//	s_list *fanouts, *fanout;
//	s_rr_node *dst_node, *src_node;
//
//	//starting_tracks = get_starting_tracks(channel, segment, is_increasing, tracks, num_tracks, &num_starting_tracks);
//	for (i = 0; i < num_starting_tracks; i++) {
//		track = starting_tracks[i];
//
//		if (is_horizontal) {
//			src_node = get_rr_node()
//		} else {
//			src_node = get_rr_node()
//		}
//
//		low_seg = get_track_low_segment(tracks, channel, track, segment);
//		high_seg = get_track_high_segment(tracks, channel, track, segment, low_seg, 50);
//		for (seg = low_seg; seg < high_seg; seg += 2) {
//			if (seg == end_seg) {
//				fanouts = get_end_seg_connection(channel, track, seg);
//			} else {
//				fanouts = get_mid_seg_connection(channel, track, seg);
//			}
//
//			fanout = fanouts;
//			while (fanout) {
//				add_rr_node_fanout()
//			}
//		}
//	}
//}
//
//void build_main()
//{
//	s_grid **grid;
//
//	build_channels(grid, nx, ny);
//	connect_channels();
//	connect_clb_to_channel();
//	connect_channel_to_clb();
//}
//
//void load_channel_lookup(s_block)
//
//
//
//void get_tile_segment_info()
//{
//	get_tile_hori
//}
//
//void connect_track_to_track(switch_box_topology)
//{
//	//connection based on architectural specs
//}
//
//void connect_clb_output_to_track(fc_out)
//{
//	//connection based on architectural specs
//}
//
//void connect_drivers_to_track()
//{
//	connect_track_to_track(rr_node);
//	connect_clb_output_to_track(rr_node);
//}
//
//void connect_track_to_clb_input(fc_in)
//{
//	//connection based on architectural specs
//}
//
//void add_track_to_lookup()
//{
//	tile_track_rr_node[x][y][track] = rr_node;
//}
//

//
//void free_rr_node_array(t_rr_node **array, int array_size)
//{
//	int i;
//	for (i = 0; i < array_size; i++) {
//		free(array[i]);
//	}
//}
//
//void connect_clb_and_channel(int x, int y, char is_horizontal)
//{
//	int i;
//	int num_tracks;
//	t_rr_node **tracks;
//	t_rr_node *rr_node;
//
//	tracks = get_tracks_starting_at_tile(x, y, &num_tracks);
//	for (i = 0; i < num_tracks; i++) {
//		connect_drivers_to_track(x, y, rr_node);
//	}
//	free_rr_node_array(tracks, num_tracks);
//
//	tracks = get_tracks_passing_or_ending_at_tile(x, y, &num_tracks);
//	for (i = 0; i < num_tracks; i++) {
//		connect_track_to_clb_input(tracks[i], x, y);
//	}
//	free_rr_node_array(tracks, num_tracks);
//}
//
//void build_tile_rr_graph(se)
//{
//	build_tile_channel(x, y, TRUE);
//	build_tile_channel(x, y, FALSE);
//}
//
////assume only one track type for now
//void build_channel(t_track_info *track_info, int num_track_info, char is_horizontal)
//{
//	int i;
//
//	for (i = 0; i < num_track_info; i++) {
//
//	}
//	rr_node = alloc_rr_node();
//
//			add_track_to_lookup(rr_node);
//}
//l
//void build_channels(int num_rows, int num_columns)
//{
//	int i
//	for all rows {
//		build_channel()
//	}
//
//	for all cols {
//
//	}
//}
//
///* required inputs
// * FPGA array size
// * cluster info
// * types of segments
// *
// */
//void build_rr_graph(int nx, int ny,
//		t_cluster_info *cluster_info, int num_cluster_types,
//		t_segment_info *segment_info, int num_segment_types
//		)
//{
//	build_channels();
//
//	for all tiles {
//		build_tile_rr_graph(tile_x, tile_y, segment_info);
//	}
//
//}

//s_track *alloc_and_init_tracks(s_wire_specs *track_specs, int num_track_specs, int *num_tracks)
//{
//	int info;
//
//	int *num_tracks_by_type;
//	int track;
//	s_track *track_instances;
//	int start;
//	int direction;
//	bool is_increasing;
//	int ntrack;
//
//	num_tracks_by_type = get_number_of_tracks_by_type(track_specs, num_track_specs, num_tracks);
//	track_instances = malloc(*num_tracks * sizeof(s_track));
//
//	ntrack = 0;
//	for (info = 0; info < num_track_specs; info++) {
//		start = 0;
//		for (track = 0; track < num_tracks_by_type[info]; track += 2) {
//			is_increasing = true;
//			for (direction = 0; direction < 2; direction++) {
//				track_instances[ntrack].start = 2 + start;
//				track_instances[ntrack].length = track_specs[info].length;
//				track_instances[ntrack].is_increasing = is_increasing;
//
//				ntrack++;
//				is_increasing = !is_increasing;
//			}
//			start = (start + 2) % track_specs[info].length;
//		}
//	}
//
//	return track_instances;
//}

//void dump_tracks(s_track *tracks, int num_tracks)
//{
//	int track;
//	for (track = 0; track < num_tracks; track++) {
//		printf("Track: %d Start: %d Length: %d is_increasing: %d\n", track, tracks[track].start, tracks[track].length, tracks[track].is_increasing);
//	}
//}
//
//void dump_rr_nodes(s_rr_node *rr_nodes, int num_rr_nodes)
//{
//	int i;
//	char *rr_type_name[] = { "CHANX", "CHANY" };
//	for (i = 0; i < num_rr_nodes; i++) {
//		printf("Node %d: %s (%d,%d) -> (%d,%d) is_increasing: %d\n",
//				rr_nodes[i].index, rr_type_name[rr_nodes[i].type], rr_nodes[i].xlow, rr_nodes[i].ylow, rr_nodes[i].xhigh, rr_nodes[i].yhigh,
//				rr_nodes[i].is_increasing);
//	}
//}
//
//void clb_array_size_to_grid_size(int clb_nx, int clb_ny, int *grid_nx, int *grid_ny)
//{
//	int num_connection_box;
//	const int num_io = 2;
//
//	num_connection_box = clb_nx + 1;
//	*grid_nx = clb_nx + num_connection_box + num_io;
//
//	num_connection_box = clb_ny + 1;
//	*grid_ny = clb_ny + num_connection_box + num_io;
//}

//static void
//close_window (GtkWidget *widget, gpointer lol)
//{
////  if (surface)
////    cairo_surface_destroy (surface);
//	printf("%X\n", lol);
//
//  //gtk_main_quit ();
//}

//int main2(int argc, char *argv[])
//{
//	GtkWidget *window;
//	GtkWidget *da;
//	gtk_init(&argc, &argv);
//	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
//	gtk_window_set_title(GTK_WINDOW(window), "prouter");
//	g_signal_connect(window, "destroy", G_CALLBACK(close_window), 0xdeadbeef);
//	da = gtk_drawing_area_new();
//	gtk_container_add(GTK_CONTAINER(window), da);
//
//	gtk_widget_show(window);
//	gtk_main();
//}
