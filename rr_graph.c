/*
 * rr_graph.c
 *
 *  Created on: 23 Aug, 2013
 *      Author: chinhau5
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <valgrind/valgrind.h>
#include "list.h"
#include "vpr_types.h"

#define IN
#define OUT
#define INOUT

void create_dot(s_routing_node *parent, FILE *file, int num_levels)
{
	GSList *child_item;
	s_routing_node *child;

	if (num_levels >= 0) {
		child_item = parent->children;
		while (child_item) {
			child = child_item->data;
			fprintf(file, "%d -- %d;\n", parent->id, child->id);
			create_dot(child, file, num_levels-1);
			child_item = child_item->next;
		}
	}
}
void create_dot_file(s_routing_node *starting_node, FILE *file, int num_levels)
{
	fprintf(file, "graph test {\n");
	create_dot(starting_node, file, num_levels);
	fprintf(file, "}\n");
}

void update_wire_type(s_wire_type *wire_specs, int num_wire_specs)
{
	e_wire_direction direction;
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
				direction = WIRE_N;
			} else {
				direction = WIRE_S;
			}
		} else if (relative_y == 0) {
			assert(relative_x != 0 && relative_y == 0);
			if (relative_x > 0) {
				direction = WIRE_E;
			} else {
				direction = WIRE_W;
			}
		} else {
			assert(relative_x != 0 && relative_y != 0);
			if (wire_specs[i].is_horizontal) {
				if (relative_x > 0) {
					if (relative_y > 0) {
						direction = WIRE_EN;
					} else {
						direction = WIRE_ES;
					}
				} else {
					if (relative_y > 0) {
						direction = WIRE_WN;
					} else {
						direction = WIRE_WS;
					}
				}
			} else {
				if (relative_x > 0) {
					if (relative_y > 0) {
						direction = WIRE_NE;
					} else {
						direction = WIRE_SE;
					}
				} else {
					if (relative_y > 0) {
						direction = WIRE_NW;
					} else {
						direction = WIRE_SW;
					}
				}
			}
		}
		wire_specs[i].direction = direction;
	}
}

void update_wire_count(INOUT s_wire_type *wire_types, IN int num_wire_types, INOUT int *num_wires)
{
	int total_freq;
	int wire_spec;
	int remainder;

	total_freq = 0;
	for (wire_spec = 0; wire_spec < num_wire_types; wire_spec++) {
		total_freq += wire_types[wire_spec].freq;
	}

	remainder = 0;
	for (wire_spec = 0; wire_spec < num_wire_types; wire_spec++) {
		wire_types[wire_spec].num_wires = *num_wires * wire_types[wire_spec].freq / total_freq;
		remainder += (*num_wires * wire_types[wire_spec].freq) % total_freq;
	}

	wire_spec = 0;
	/* distribute remaining tracks evenly across different track types */
	remainder = remainder / total_freq;
	while (remainder > 0) {
		wire_types[wire_spec].num_wires++;
		wire_spec = (wire_spec + 1) % num_wire_types;
		remainder--; //unidirectional routing have tracks in pairs
	}

	/* recalculate total tracks */
	*num_wires = 0;
	for (wire_spec = 0; wire_spec < num_wire_types; wire_spec++) {
		*num_wires += wire_types[wire_spec].num_wires;
	}
}

/* wires[clb_x][clb_y][is_horizontal][wire_spec][wire_index] */

/* bubbles up when "compare" returns true */
//void sort(void *data, int elem_size, int num_elem, bool (*compare)(void *, void*))
//{
//	int i;
//	bool swapped;
//	void *temp;
//	void *a;
//	void *b;
//
//	temp = malloc(elem_size);
//
//	do {
//		swapped = false;
//		for (i = 0; i < num_elem-1; i++) {
//			a = data + i*elem_size;
//			b = data + (i+1)*elem_size;
//			if (compare(a, b)) {
//				memcpy(temp, a, elem_size);
//				memcpy(a, b, elem_size);
//				memcpy(b, temp, elem_size);
//				swapped = true;
//			}
//		}
//	} while (swapped);
//
//	free(temp);
//}

void clip_wire_spec(s_wire_type *wire_details, int x, int y, int block_nx, int block_ny)
{
	/* clip end points of wires */
	if (x + wire_details->relative_x < 0) {
		wire_details->relative_x = -x;
	} else 	if (x + wire_details->relative_x >= block_nx) {
		wire_details->relative_x = block_nx - 1 - x;
	}

	if (y + wire_details->relative_y < 0) {
		wire_details->relative_y = -y;
	} else if (y + wire_details->relative_y >= block_ny) {
		wire_details->relative_y = block_ny - 1 - y;
	}
}

//void alloc_and_init_wire_details(s_switch_box *switch_box, int x, int y, int block_nx, int block_ny, s_wire_type *wire_details, int num_wire_details)
//{
//	int i;
//	s_wire_type temp_wire_details;
//
//	switch_box->num_wire_details = num_wire_details;
//	switch_box->wire_details = malloc(sizeof(s_wire_type) * switch_box->num_wire_details);
//
//	for (i = 0; i < switch_box->num_wire_details; i++) {
//		temp_wire_details = wire_details[i];
//
//		clip_wire_spec(&temp_wire_details, x, y, block_nx, block_ny);
//
//		switch_box->wire_details[temp_wire_details.id] = temp_wire_details;
//
//		/* skip wires that are not moving anywhere */
//		if (temp_wire_details.relative_x == 0 && temp_wire_details.relative_y == 0) {
//			switch_box->wire_details[temp_wire_details.id].id = -1;
//		}
//	}
//}

bool valid_wire_type(s_wire_type *wire_type, int x, int y, int nx, int ny)
{
	return x + wire_type->relative_x >= 0 && x + wire_type->relative_x < nx &&
			y + wire_type->relative_y >= 0 && y + wire_type->relative_y < ny;
}

void count_starting_wires(s_switch_box *switch_box, s_wire_type *wire_types, int num_wire_types, int x, int y, int nx, int ny)
{
	int i;
	int num_directions;
	int direction_index;

	switch_box->num_starting_wires = 0;
	switch_box->starting_direction_to_index = g_hash_table_new(g_direct_hash, g_direct_equal);
	for (i = 0; i < num_wire_types; i++) {
		if (valid_wire_type(&wire_types[i], x, y, nx, ny)) {
			switch_box->num_starting_wires += wire_types[i].num_wires;

			if (!g_hash_table_contains(switch_box->starting_direction_to_index, (gpointer)wire_types[i].direction)) {
				g_hash_table_insert(switch_box->starting_direction_to_index, (gpointer)wire_types[i].direction, (gpointer)g_hash_table_size(switch_box->starting_direction_to_index));
			}
		}
	}

	num_directions = g_hash_table_size(switch_box->starting_direction_to_index);
	switch_box->starting_shape_to_index = malloc(num_directions * sizeof(GHashTable *));
	for (i = 0; i < num_directions; i++) {
		switch_box->starting_shape_to_index[i] = g_hash_table_new(g_direct_hash, g_direct_equal);
	}
	for (i = 0; i < num_wire_types; i++) {
		if (valid_wire_type(&wire_types[i], x, y, nx, ny)) {
			direction_index = (int)g_hash_table_lookup(switch_box->starting_direction_to_index, (gpointer)wire_types[i].direction);

			if (!g_hash_table_contains(switch_box->starting_shape_to_index[direction_index], wire_types[i].shape)) {
				g_hash_table_insert(switch_box->starting_shape_to_index[direction_index],
						wire_types[i].shape, g_hash_table_size(switch_box->starting_shape_to_index[direction_index]));
			}
		}
	}

	switch_box->starting_wire_directions = g_hash_table_get_keys(switch_box->starting_direction_to_index);
	switch_box->starting_wire_shapes = malloc(sizeof(GList *) * num_directions);
	for (i = 0; i < num_directions; i++) {
		switch_box->starting_wire_shapes[i] = g_hash_table_get_keys(switch_box->starting_shape_to_index[i]);
	}

	switch_box->num_starting_wires_by_type = malloc(num_directions * sizeof(int *));
	for (i = 0; i < num_directions; i++) {
		switch_box->num_starting_wires_by_type[i] = calloc(g_hash_table_size(switch_box->starting_shape_to_index[i]), sizeof(int));
	}
}

void count_ending_wires(t_block **grid, int nx, int ny)
{
	int x, y;
	int i;
	int wire;
	int sink_x;
	int sink_y;
	s_switch_box *source_switch_box;
	s_switch_box *sink_switch_box;
	s_wire_type *starting_wire_type;
	int num_directions;
	int direction_index;

	/* DIRECTION */
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			grid[x][y].switch_box->num_ending_wires = 0;
			grid[x][y].switch_box->ending_direction_to_index = g_hash_table_new(g_direct_hash, g_direct_equal);
		}
	}
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			source_switch_box = grid[x][y].switch_box;

			for (wire = 0; wire < source_switch_box->num_starting_wires; wire++) {
				starting_wire_type = source_switch_box->starting_wires[wire].type;

				sink_x = x + starting_wire_type->relative_x;
				sink_y = y + starting_wire_type->relative_y;

				assert(sink_x >= 0 && sink_x < nx && sink_y >= 0 && sink_y < ny);

				sink_switch_box = grid[sink_x][sink_y].switch_box;
				sink_switch_box->num_ending_wires++;
				if (!g_hash_table_contains(sink_switch_box->ending_direction_to_index, (gpointer)starting_wire_type->direction)) {
					g_hash_table_insert(sink_switch_box->ending_direction_to_index, (gpointer)starting_wire_type->direction, (gpointer)g_hash_table_size(sink_switch_box->ending_direction_to_index));
				}
			}
		}
	}

	/* SHAPE */
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			num_directions = g_hash_table_size(grid[x][y].switch_box->ending_direction_to_index);
			grid[x][y].switch_box->ending_shape_to_index = malloc(sizeof(GHashTable *) * num_directions);
			for (i = 0; i < num_directions; i++) {
				grid[x][y].switch_box->ending_shape_to_index[i] = g_hash_table_new(g_direct_hash, g_direct_equal);
			}
		}
	}
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			source_switch_box = grid[x][y].switch_box;

			for (wire = 0; wire < source_switch_box->num_starting_wires; wire++) {
				starting_wire_type = source_switch_box->starting_wires[wire].type;

				sink_x = x + starting_wire_type->relative_x;
				sink_y = y + starting_wire_type->relative_y;

				assert(sink_x >= 0 && sink_x < nx && sink_y >= 0 && sink_y < ny);

				sink_switch_box = grid[sink_x][sink_y].switch_box;

				direction_index = g_hash_table_lookup(sink_switch_box->ending_direction_to_index, (gpointer)starting_wire_type->direction);
				if (!g_hash_table_contains(sink_switch_box->ending_shape_to_index[direction_index], (gpointer)starting_wire_type->shape)) {
					g_hash_table_insert(sink_switch_box->ending_shape_to_index[direction_index],
												starting_wire_type->shape, g_hash_table_size(sink_switch_box->ending_shape_to_index[direction_index]));
				}
			}
		}
	}

	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			num_directions = g_hash_table_size(grid[x][y].switch_box->ending_direction_to_index);
			grid[x][y].switch_box->num_ending_wires_by_type = malloc(num_directions * sizeof(int *));
			for (i = 0; i < num_directions; i++) {
				grid[x][y].switch_box->num_ending_wires_by_type[i] = calloc(g_hash_table_size(grid[x][y].switch_box->ending_shape_to_index[i]), sizeof(int));
			}
		}
	}

	/* BACKWARD COMPATIBILITY */
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			grid[x][y].switch_box->ending_wire_directions = g_hash_table_get_keys(grid[x][y].switch_box->ending_direction_to_index);
			num_directions = g_hash_table_size(grid[x][y].switch_box->ending_direction_to_index);
			grid[x][y].switch_box->ending_wire_shapes = malloc(sizeof(GList *) * num_directions);
			for (i = 0; i < num_directions; i++) {
				grid[x][y].switch_box->ending_wire_shapes[i] = g_hash_table_get_keys(grid[x][y].switch_box->ending_shape_to_index[i]);
			}
		}
	}
}

void alloc_and_init_starting_wire_array(s_switch_box *switch_box, s_wire_type *wire_types, int num_wire_types, int x, int y, int nx, int ny, int *global_routing_node_id)
{
	int i;
	int wire;
	int direction_index;
	int shape_index;

	switch_box->starting_wires = calloc(switch_box->num_starting_wires, sizeof(s_wire));
	switch_box->num_starting_wires = 0;

	for (i = 0; i < num_wire_types; i++) {
		if (valid_wire_type(&wire_types[i], x, y, nx, ny)) {
			direction_index = g_hash_table_lookup(switch_box->starting_direction_to_index, (gpointer)wire_types[i].direction);
			shape_index = g_hash_table_lookup(switch_box->starting_shape_to_index[direction_index], wire_types[i].shape);

			for (wire = 0; wire < wire_types[i].num_wires; wire++) {
				switch_box->starting_wires[switch_box->num_starting_wires].base.type = WIRE;
				switch_box->starting_wires[switch_box->num_starting_wires].base.id = (*global_routing_node_id)++;
				switch_box->starting_wires[switch_box->num_starting_wires].base.children = NULL;

				switch_box->starting_wires[switch_box->num_starting_wires].type = &wire_types[i];
				switch_box->starting_wires[switch_box->num_starting_wires].track = switch_box->num_starting_wires_by_type[direction_index][shape_index];
				switch_box->starting_wires[switch_box->num_starting_wires].base.x = x;
				switch_box->starting_wires[switch_box->num_starting_wires].base.y = y;

				switch_box->num_starting_wires++;
				switch_box->num_starting_wires_by_type[direction_index][shape_index]++;
			}
		}
	}
}

void alloc_and_init_ending_wire_array(t_block **blocks, int nx, int ny)
{
	int x, y;
	int wire;
	int sink_x;
	int sink_y;
	s_switch_box *source_switch_box;
	s_switch_box *sink_switch_box;
	s_wire_type *starting_wire_type;
	int direction_index;
	int shape_index;

	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			blocks[x][y].switch_box->ending_wires = calloc(blocks[x][y].switch_box->num_ending_wires, sizeof(s_wire *));
			blocks[x][y].switch_box->num_ending_wires = 0;
		}
	}

	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			source_switch_box = blocks[x][y].switch_box;

			for (wire = 0; wire < source_switch_box->num_starting_wires; wire++) {
				starting_wire_type = source_switch_box->starting_wires[wire].type;

				sink_x = x + starting_wire_type->relative_x;
				sink_y = y + starting_wire_type->relative_y;

				assert(sink_x >= 0 && sink_x < nx && sink_y >= 0 && sink_y < ny);

				sink_switch_box = blocks[sink_x][sink_y].switch_box;
				sink_switch_box->ending_wires[sink_switch_box->num_ending_wires] = &source_switch_box->starting_wires[wire];
				sink_switch_box->num_ending_wires++;

				direction_index = g_hash_table_lookup(sink_switch_box->ending_direction_to_index, (gpointer)starting_wire_type->direction);
				shape_index = g_hash_table_lookup(sink_switch_box->ending_shape_to_index[direction_index], starting_wire_type->shape);
				sink_switch_box->num_ending_wires_by_type[direction_index][shape_index]++;
			}
		}
	}
}

void init_starting_wires(t_block *block, int nx, int ny, s_wire_type *wire_types, int num_wire_types, int num_wires_per_clb, int *global_routing_node_id)
{
	count_starting_wires(block->switch_box, wire_types, num_wire_types, block->x, block->y, nx, ny);
	alloc_and_init_starting_wire_array(block->switch_box, wire_types, num_wire_types, block->x, block->y, nx, ny, global_routing_node_id);
}

void init_ending_wires(t_block **grid, int nx, int ny)
{
	count_ending_wires(grid, nx, ny);
	alloc_and_init_ending_wire_array(grid, nx, ny);
}

//void set_ending_wire_counts_to_zero(s_switch_box *switch_box)
//{
//	int i, j;
//
//	switch_box->num_ending_wires = 0;
//	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
//		switch_box->num_ending_wire_types_by_direction[i] = 0;
//		for (j = 0; j < switch_box->num_wire_details; j++) {
//			switch_box->num_ending_wires_by_direction_and_type[i][j] = 0;
//		}
//	}
//}
//
//void alloc_ending_wire_counts(s_switch_box *switch_box)
//{
//	int i;
//
//	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
//		switch_box->num_ending_wires_by_direction_and_type[i] = malloc(sizeof(int) * switch_box->num_wire_details);
//	}
//}
//

//
//void alloc_ending_wire_array_and_lookup(s_switch_box *switch_box)
//{
//	int i, j;
//
//	switch_box->ending_wires = malloc(sizeof(s_wire) * switch_box->num_ending_wires);
//	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
//		switch_box->ending_wires_by_direction_and_type[i] = (s_wire ***)malloc(sizeof(s_wire **) * switch_box->num_wire_details);
//		for (j = 0; j < switch_box->num_wire_details; j++) {
//			if (switch_box->num_ending_wires_by_direction_and_type[i][j] > 0) {
//				switch_box->ending_wires_by_direction_and_type[i][j] = (s_wire **)malloc(sizeof(s_wire *) * switch_box->num_ending_wires_by_direction_and_type[i][j]);
//			} else {
//				switch_box->ending_wires_by_direction_and_type[i][j] = NULL;
//			}
//		}
//	}
//}
//

//int get_next_non_zero_element_offset(int *array, int size, int current_offset, int forward)
//{
//	current_offset = current_offset % size;
//	while (forward > 0) {
//		if (array[current_offset] > 0) {
//			forward--;
//		}
//
//		if (forward > 0) {
//			current_offset = (current_offset+1) % size;
//		}
//	}
//	return current_offset;
//}

GList *get_next_wire_direction(GList *wire_directions_head, GList *current_wire_directions_item, GHashTable *valid_directions, int offset)
{
	GList *wire_directions_item;
	int i;

	wire_directions_item = current_wire_directions_item;

	assert(wire_directions_item);

	/* make sure we start off with a valid direction */
	if (valid_directions) {
		while (!g_hash_table_contains(valid_directions, wire_directions_item->data)) {
			if (wire_directions_item->next) {
				wire_directions_item = wire_directions_item->next;
			} else { /* wrap around */
				wire_directions_item = wire_directions_head;
			}
		}
	}

	i = 0;
	while (i < offset) {
		if (wire_directions_item->next) {
			wire_directions_item = wire_directions_item->next;
		} else { /* wrap around */
			wire_directions_item = wire_directions_head;
		}
		if (!valid_directions || g_hash_table_contains(valid_directions, wire_directions_item->data)) {
			i++;
		}
	}

	return wire_directions_item;
}

GList * get_next_wire_shape(GList *wire_shapes_head, GList *current_wire_shapes_item, int offset)
{
	GList *wire_shapes_item;
	int i;

	wire_shapes_item = current_wire_shapes_item;

	assert(wire_shapes_item);

	for (i = 0; i < offset; i++) {
		if (wire_shapes_item->next) {
			wire_shapes_item = wire_shapes_item->next;
		} else { /* wrap around */
			wire_shapes_item = wire_shapes_head;
		}
	}

	return wire_shapes_item;
}

int select_wire_uniformly(s_wire **wires, int total_num_wires, int **num_wires_by_type,
		GList *wire_directions_head, GHashTable *direction_to_index, GList **wire_shapes_head, GHashTable **shape_to_index,
		GHashTable *valid_directions, int num_required_wires, int direction_offset, int *shape_offset, int **track_offset)
{
	int i, j;

	GList *wire_directions_item;

	GList *required_wire_direction_item;
	e_wire_direction required_direction;
	int required_direction_index;

	GList **required_wire_shape_item;
	int *required_shape;
	int *required_shape_index;

	int **required_track;

	int selected_wire;
	s_wire *temp;
	bool found;
	int direction_index;
	int num_directions;

	num_directions = g_list_length(wire_directions_head);

	required_wire_shape_item = malloc(sizeof(GList *) * num_directions);
	required_shape = malloc(sizeof(int) * num_directions);
	required_shape_index = malloc(sizeof(int) * num_directions);

	required_track = malloc(sizeof(int *) * num_directions);
	for (i = 0; i < num_directions; i++) {
		required_track[i] = malloc(sizeof(int) * g_list_length(wire_shapes_head[i]));
	}

	required_wire_direction_item = get_next_wire_direction(wire_directions_head, wire_directions_head, valid_directions, direction_offset);
	required_direction = (int)required_wire_direction_item->data;
	required_direction_index = (int)g_hash_table_lookup(direction_to_index, (gpointer)required_direction);

	wire_directions_item = wire_directions_head;
	while (wire_directions_item) {
		direction_index = (int)g_hash_table_lookup(direction_to_index, wire_directions_item->data);
		required_wire_shape_item[direction_index] = get_next_wire_shape(wire_shapes_head[direction_index], wire_shapes_head[direction_index], shape_offset[direction_index]);
		required_shape[direction_index] = (int)required_wire_shape_item[direction_index]->data;
		required_shape_index[direction_index] = (int)g_hash_table_lookup(shape_to_index[direction_index], required_shape[direction_index]);
		wire_directions_item = wire_directions_item->next;
	}

	for (i = 0; i < num_directions; i++) {
		for (j = 0; j < g_list_length(wire_shapes_head[i]); j++) {
			required_track[i][j] = track_offset[i][j] % num_wires_by_type[i][j];
		}
	}

	found = true;
	for (i = 0; i < num_required_wires && found; i++) {
		found = false;

		for (j = i; j < total_num_wires && !found; j++) {
			if (wires[j]->type->direction == required_direction &&
				wires[j]->type->shape == required_shape[required_direction_index] &&
				wires[j]->track == required_track[required_direction_index][required_shape_index[required_direction_index]]) {
				selected_wire = j;
				found = true;
			}
		}

		assert(found);

		track_offset[required_direction_index][required_shape_index[required_direction_index]]++;

		required_track[required_direction_index][required_shape_index[required_direction_index]] =
				(required_track[required_direction_index][required_shape_index[required_direction_index]] + 1) %
				num_wires_by_type[required_direction_index][required_shape_index[required_direction_index]];

		required_wire_shape_item[required_direction_index] =
				get_next_wire_shape(wire_shapes_head[required_direction_index], required_wire_shape_item[required_direction_index], 1);
		required_shape[required_direction_index] = (int)required_wire_shape_item[required_direction_index]->data;
		required_shape_index[direction_index] = (int)g_hash_table_lookup(shape_to_index[direction_index], required_shape[required_direction_index]);

		required_wire_direction_item = get_next_wire_direction(wire_directions_head, required_wire_direction_item, valid_directions, 1);
		required_direction = (int)required_wire_direction_item->data;
		required_direction_index = (int)g_hash_table_lookup(direction_to_index, (gpointer)required_direction);

		/* swap */
		temp = wires[i];
		wires[i] = wires[selected_wire];
		wires[selected_wire] = temp;
	}

	free(required_wire_shape_item);
	free(required_shape);
	free(required_shape_index);
	for (i = 0; i < num_directions; i++) {
		free(required_track[i]);
	}
	free(required_track);

	return i; /* return the number of wire actually found */
}

/* three level offset [direction][type][track] */
void init_clb_output_pins(t_block *block, float fc_out, s_wire_type *wire_types, int num_wire_types, int *global_routing_node_id)
{
	s_switch_box *switch_box;
	int actual_fc_out;
	int instance, port, pin;
	int shape_offset[NUM_WIRE_DIRECTIONS] = { 0 };
	int **track_offset;
	s_wire **wires;
	int i;

	switch_box = block->switch_box;
	actual_fc_out = switch_box->num_starting_wires * fc_out;

	wires = malloc(sizeof(s_wire *) * switch_box->num_starting_wires);
	for (i = 0; i < switch_box->num_starting_wires; i++) {
		wires[i] = &switch_box->starting_wires[i];
	}
	track_offset = malloc(sizeof(int *) * 16);
	for (i = 0; i < 16; i++) {
		track_offset[i] = calloc(16, sizeof(int));
	}

	for (instance = 0; instance < block->capacity; instance++) {
		for (port = 0; port < block->pb[instance].type->num_output_ports; port++) {
			for (pin = 0; pin < block->pb[instance].type->output_ports[port].num_pins; pin++) {
				assert(block->pb[instance].output_pins[port][pin].base.type == OPIN);
				block->pb[instance].output_pins[port][pin].base.id = (*global_routing_node_id)++;
				block->pb[instance].output_pins[port][pin].base.children = NULL;

				select_wire_uniformly(wires, switch_box->num_starting_wires, switch_box->num_starting_wires_by_type,
						switch_box->starting_wire_directions, switch_box->starting_direction_to_index,
						switch_box->starting_wire_shapes, switch_box->starting_shape_to_index,
						NULL, actual_fc_out, pin, shape_offset, track_offset);

				for (i = 0; i < actual_fc_out; i++) {
					block->pb[instance].output_pins[port][pin].base.children =
							g_slist_prepend(block->pb[instance].output_pins[port][pin].base.children, wires[i]);
				}
			}
		}
	}

	free(wires);
	for (i = 0; i < 16; i++) {
		 free(track_offset[i]);
	}
	free(track_offset);
}

void init_clb_input_pins(t_block *block, float fc_in, int *global_routing_node_id)
{
	int actual_fc_in;
	int i;
	s_switch_box *switch_box;
	int instance, port, pin;
	int shape_offset[NUM_WIRE_DIRECTIONS] = { 0 };
	int **track_offset;
	s_wire **wires;

	track_offset = malloc(sizeof(int *) * 16);
	for (i = 0; i < 16; i++) {
		track_offset[i] = calloc(16, sizeof(int));
	}

	switch_box = block->switch_box;
	actual_fc_in = switch_box->num_ending_wires * fc_in;

	wires = malloc(sizeof(s_wire *) * switch_box->num_ending_wires);
	for (i = 0; i < switch_box->num_ending_wires; i++) {
		wires[i] = switch_box->ending_wires[i];
	}

	for (instance = 0; instance < block->capacity; instance++) {
		for (port = 0; port < block->pb[instance].type->num_input_ports; port++) {
			for (pin = 0; pin < block->pb[instance].type->input_ports[port].num_pins; pin++) {
				assert(block->pb[instance].input_pins[port][pin].base.type == IPIN);
				block->pb[instance].input_pins[port][pin].base.id = (*global_routing_node_id)++;
				block->pb[instance].input_pins[port][pin].base.children = NULL;

				select_wire_uniformly(wires, switch_box->num_ending_wires, switch_box->num_ending_wires_by_type,
						switch_box->ending_wire_directions, switch_box->ending_direction_to_index,
						switch_box->ending_wire_shapes, switch_box->ending_shape_to_index,
						NULL, actual_fc_in, pin, shape_offset, track_offset);

				for (i = 0; i < actual_fc_in; i++) {
					wires[i]->base.children =
							g_slist_prepend(wires[i]->base.children, &block->pb[instance].input_pins[port][pin]);
				}
			}
		}
	}

	free(wires);
	for (i = 0; i < 16; i++) {
		 free(track_offset[i]);
	}
	free(track_offset);
}

void init_inter_switch_box(s_switch_box *switch_box, int fs)
{
	s_wire *ending_wire;
	s_wire_type *ending_wire_type;
	int i, j;
	GHashTable *possible_directions;
	s_wire **wires;
	int shape_offset[NUM_WIRE_DIRECTIONS] = { 0 };
	int **track_offset;

	track_offset = malloc(sizeof(int *) * 16);
	for (i = 0; i < 16; i++) {
		track_offset[i] = calloc(16, sizeof(int));
	}

	wires = malloc(sizeof(s_wire *) * switch_box->num_starting_wires);
	for (i = 0; i < switch_box->num_starting_wires; i++) {
		wires[i] = &switch_box->starting_wires[i];
	}

	possible_directions = g_hash_table_new(g_direct_hash, g_direct_equal);

	/* loop for each ending wire */
	for (i = 0; i < switch_box->num_ending_wires; i++) {
		ending_wire = switch_box->ending_wires[i];
		ending_wire_type = ending_wire->type;

		switch (ending_wire_type->direction) {
		case WIRE_E:
			g_hash_table_add(possible_directions, WIRE_N);
			g_hash_table_add(possible_directions, WIRE_S);
			g_hash_table_add(possible_directions, WIRE_E);
			break;
		case WIRE_W:
			g_hash_table_add(possible_directions, WIRE_N);
			g_hash_table_add(possible_directions, WIRE_S);
			g_hash_table_add(possible_directions, WIRE_W);
			break;
		case WIRE_N:
			g_hash_table_add(possible_directions, WIRE_E);
			g_hash_table_add(possible_directions, WIRE_W);
			g_hash_table_add(possible_directions, WIRE_N);
			break;
		case WIRE_S:
			g_hash_table_add(possible_directions, WIRE_E);
			g_hash_table_add(possible_directions, WIRE_W);
			g_hash_table_add(possible_directions, WIRE_S);
			break;
		default:
			/* not handling bent wires for now */
			assert(false);
			break;
		}

		select_wire_uniformly(wires, switch_box->num_starting_wires, switch_box->num_starting_wires_by_type,
				switch_box->starting_wire_directions, switch_box->starting_direction_to_index,
				switch_box->starting_wire_shapes, switch_box->starting_shape_to_index,
				possible_directions, fs, i, shape_offset, track_offset);

		for (j = 0; j < fs; j++) {
			ending_wire->base.children = g_slist_prepend(ending_wire->base.children, wires[j]);
		}

		g_hash_table_remove_all(possible_directions);
	}

	for (i = 0; i < 16; i++) {
		 free(track_offset[i]);
	}
	free(track_offset);
	free(wires);
	g_hash_table_destroy(possible_directions);
}

void dump_pin(s_pb_graph_pin *pin, FILE *file)
{
	fprintf(file, "%s.%s[%d] ", pin->pb->type->name, pin->port->name, pin->pin_number);
}

void dump_wire(s_wire *wire, FILE *file)
{
	assert(wire->base.type == WIRE);
	fprintf(file, "%10s[track=%3d,id=%4d,x=%3d,y=%3d,dir=%2d,shape=%3d,rel_x=%3d,rel_y=%3d] ", wire->type->name,
			wire->track, wire->base.id, wire->base.x, wire->base.y,
			wire->type->direction, wire->type->shape, wire->type->relative_x, wire->type->relative_y);
}

void dump_clb(t_block *block, FILE *file)
{
	GSList *item;
	s_wire *wire;
	s_routing_node *node;
	int i, j, k;
	int count;
	int instance, port, pin;

	fprintf(file, "CLB [%d,%d]\n", block->x, block->y);

	for (instance = 0; instance < block->capacity; instance++) {
		for (port = 0; port < block->pb[instance].type->num_output_ports; port++) {
			for (pin = 0; pin < block->pb[instance].type->output_ports[port].num_pins; pin++) {
				item = block->pb[instance].output_pins[port][pin].base.children;
				fprintf(file, "OPIN [%d,%d,%d] -> \n", instance, port, pin);
				while (item) {
					wire = (s_wire *)item->data;
					fprintf(file, "\t"); dump_wire(wire, file); fprintf(file, "\n");
					item = item->next;
				}
				fprintf(file, "\n");
			}
		}
	}
	fprintf(file, "\n");

//	printf("Starting wires\n");
//	for (i = 0; i < block->switch_box->num_starting_wires; i++) {
//		item = block->switch_box->starting_wires[i].base.children.head;
//		dump_wire(&block->switch_box->starting_wires[pin]); printf(" ->\n");
//		while (item) {
//			wire = (s_wire *)item->data;
//			printf("\t"); dump_wire(wire); printf("\n");
//			item = item->next;
//		}
//		printf("\n");
//	}
//	printf("\n");

//	printf("Starting wires by type\n");
//	count = 0;
//	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
//		for (j = 0; j < block->switch_box->num_wire_details; j++) {
//			for (k = 0; k < block->switch_box->num_starting_wires_by_direction_and_type[i][j]; k++) {
//				wire = block->switch_box->starting_wires_by_direction_and_type[i][j][k];
//				assert(wire);
//				dump_wire(wire);
//				printf("\n");
//				count++;
//			}
//		}
//	}
//	printf("\n");
//	assert(count == block->switch_box->num_starting_wires);
//
//	for (instance = 0; instance < block->capacity; instance++) {
//		for (port = 0; port < block->pb[instance].type->num_input_ports; port++) {
//			for (pin = 0; pin < block->pb[instance].type->input_ports[port].num_pins; pin++) {
//				item = block->pb[instance].input_pins[port][pin].base.children;
//				printf("IPIN [%d,%d,%d] -> \n", instance, port, pin);
//				while (item) {
//					wire = (s_wire *)item->data;
//					printf("\t"); dump_wire(wire); printf("\n");
//					item = item->next;
//				}
//				printf("\n");
//			}
//		}
//	}
//	printf("\n");
//
	fprintf(file, "Ending wires\n");
	for (pin = 0; pin < block->switch_box->num_ending_wires; pin++) {
		item = block->switch_box->ending_wires[pin]->base.children;
		dump_wire(block->switch_box->ending_wires[pin], file); fprintf(file, " ->\n");
		while (item) {
			node = (s_routing_node *)item->data;
			if (node->type == IPIN) {
				fprintf(file, "\t"); dump_pin(node, file); fprintf(file, "\n");
			} else if (node->type == WIRE) {
				fprintf(file, "\t"); dump_wire(node, file); fprintf(file, "\n");
			} else {
				assert(false);
			}
			item = item->next;
		}
		fprintf(file, "\n");
	}
	fprintf(file, "\n");

//	printf("Ending wires by type\n");
//	count = 0;
//	for (i = 0; i < NUM_WIRE_DIRECTIONS; i++) {
//		for (j = 0; j < block->switch_box->num_wire_details; j++) {
//			for (k = 0; k < block->switch_box->num_ending_wires_by_direction_and_type[i][j]; k++) {
//				wire = block->switch_box->ending_wires_by_direction_and_type[i][j][k];
//				assert(wire);
//				dump_wire(wire);
//				printf("\n");
//				count++;
//			}
//		}
//	}
//	printf("\n");
//	assert(count == block->switch_box->num_ending_wires);
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

void init_block_wires(t_block **grid, int nx, int ny, s_wire_type *wire_types, int num_wire_types, int num_wires_per_clb, int *global_routing_node_id)
{
	const float fc_out = 1;
	const float fc_in = 1;
	const int fs = 3;
	int x, y;
	FILE *file;

	file = fopen("rr_graph.txt", "w");

	*global_routing_node_id = 0;
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			grid[x][y].switch_box = calloc(1, sizeof(s_switch_box));
			init_starting_wires(&grid[x][y], nx, ny, wire_types, num_wire_types, num_wires_per_clb, global_routing_node_id);
		}
	}

	init_ending_wires(grid, nx, ny);
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			init_clb_output_pins(&grid[x][y], fc_out, wire_types, num_wire_types, global_routing_node_id);
			init_clb_input_pins(&grid[x][y], fc_in, global_routing_node_id);
			init_inter_switch_box(grid[x][y].switch_box, fs);
		}
	}

	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			dump_clb(&grid[x][y], file);
		}
	}

	fclose(file);
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
