/*
 * main.c
 *
 *  Created on: Oct 19, 2013
 *      Author: chinhau5
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glib.h>
#include "list.h"
#include "vpr_types.h"
#include "rr_graph.h"
#include "heap.h"
#include "arch.h"
#include "netlist.h"
#include "pb_graph.h"
#include "placement.h"

void alloc_and_init_block_grid_positions(t_block ***grid, int nx, int ny, GHashTable *block_positions)
{
	int x, y;
	GHashTableIter iter;
	gpointer key, value;
	s_block_position *block_position;

	*grid = malloc(sizeof(t_block *) * nx);
	for (x = 0; x < nx; x++) {
		(*grid)[x] = malloc(sizeof(t_block) * ny);
	}

	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			(*grid)[x][y].name = NULL;
			(*grid)[x][y].pb = NULL;
		}
	}

	g_hash_table_iter_init (&iter, block_positions);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		block_position = value;
		(*grid)[block_position->x][block_position->y].x = block_position->x;
		(*grid)[block_position->x][block_position->y].y = block_position->y;
		(*grid)[block_position->x][block_position->y].name = key;
	}
}

int main()
{
	s_wire_details wire_specs[4];
	int num_wires = 20;
	s_track *tracks;
	int track, channel, segment;
	int start;
	int nx, ny;
	int i;
	char *names[] = { "SINGLE", "DOUBLE", "SINGLE_R", "DOUBLE_R" };
	s_block clb;
	s_rr_node *****rr_node_lookup;
	s_switch_box *sb;
	s_heap heap;
	s_heap_item item;
	int num_blocks;

	s_pb_top_type *pb_top_types;
	int num_pb_top_types;
	s_pb_graph_node pb_graph_head;
	GHashTable *block_positions;
	GHashTableIter iter;
	gpointer key, value;
	struct _s_block_position *block_pos;
	t_block **grid;
	int x, y;
	GHashTable *external_nets;
//	clb.num_output_pins = 10;
//	clb.output_pins = malloc(10*sizeof(s_list));
//	wire_specs[0].name = names[0];
//	wire_specs[0].id = 0;
//	wire_specs[0].freq = 1;
//	wire_specs[0].relative_x = 1;
//	wire_specs[0].relative_y = 0;
//
//	wire_specs[1].name = names[0];
//	wire_specs[1].id = 1;
//	wire_specs[1].freq = 1;
//	wire_specs[1].relative_x = 0;
//	wire_specs[1].relative_y = 1;
//
//	wire_specs[2].name = names[2];
//	wire_specs[2].id = 2;
//	wire_specs[2].freq = 1;
//	wire_specs[2].relative_x = -1;
//	wire_specs[2].relative_y = 0;
//
//	wire_specs[3].name = names[2];
//	wire_specs[3].id = 3;
//	wire_specs[3].freq = 1;
//	wire_specs[3].relative_x = 0;
//	wire_specs[3].relative_y = -1;

	pb_top_types = parse_arch("sample_arch.xml", &num_pb_top_types);

	parse_placement("tseng.place", &nx, &ny, &block_positions);
	alloc_and_init_block_grid_positions(&grid, nx, ny, block_positions);
	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			if (grid[x][y].name) {
				printf("grid[%d][%d] x=%d y=%d name=%s\n", x, y, grid[x][y].x, grid[x][y].y, grid[x][y].name);
			}
		}
	}
	parse_netlist("tseng.net", grid, block_positions, pb_top_types, num_pb_top_types, &num_blocks, &external_nets);

//	init_heap(&heap);
//	insert_to_heap(&heap, 10, NULL);
//	insert_to_heap(&heap, 9, NULL);
//	insert_to_heap(&heap, 8, NULL);
//
//	insert_to_heap(&heap, 7, NULL);
//	insert_to_heap(&heap, 6, NULL);
//
//	while (get_heap_head(&heap, &item)) {
//		printf("item: %f\n", item.cost);
//	}

//	update_wire_count(wire_specs, 4, &num_wires);
//	update_wire_type(wire_specs, 4);
//	init(wire_specs, 4, num_wires, 10, 5);

	//tracks = alloc_and_init_tracks(track_info, 1, &num_tracks);
	//dump_tracks(tracks, num_tracks);
	//clb_array_size_to_grid_size(2, 2, &nx, &ny);
	//rr_node_lookup = alloc_rr_node_lookup(nx, ny);
	//build_channel(1, 2, true, true, nx, ny, num_tracks, tracks, rr_node_lookup);
//	update_wire_count(wire_specs, 2, &num_wires);
//	sb = create_switch_box(wire_specs, 2, num_wires);
//	connect_clb_to_switch_box(&clb, sb, 0.5);
//	dump_clb_output_pin(&clb);
	//get_starting_tracks(0, 2, INC_DIRECTION, track_instances,num_tracks, NULL);
//	for (channel = 1; channel < 3; channel += 2) {
//		for (track = 0; track < 10; track++) {
//			for (segment = 2; segment < 20; segment+=2) {
//				if (tracks[track].is_increasing) {
//					printf("channel: %3d track: %3d segment: %3d start: %3d\n", channel, track, segment, get_track_low_segment(tracks, channel, track, segment));
//				} else {
//					start = get_track_low_segment(tracks, channel, track, segment);
//					printf("channel: %3d track: %3d segment: %3d start: %3d\n", channel, track, segment, get_track_high_segment(tracks, channel, track, segment, start, 50));
//				}
//
//			}
//		}
//	}
	return 0;
}
