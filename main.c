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
#include <assert.h>
#include <string.h>
#include "list.h"
#include "vpr_types.h"
#include "rr_graph.h"
#include "heap.h"
#include "arch.h"
#include "netlist.h"
#include "pb_graph.h"
#include "placement.h"
#include "route.h"
#include "quadtree.h"
#include "vpr_interface.h"
#include "vpr/base/globals.h"

int main(int argc, char **argv)
{
	s_wire_type wire_types[4];
	int num_wires = 200;
	s_track *tracks;
	int track, channel, segment;
	int start;
	int nx, ny;
	int i, j;
	char *names[] = { "SINGLE", "DOUBLE", "SINGLE_R", "DOUBLE_R" };
	//s_block clb;
	//s_rr_node *****rr_node_lookup;
	s_switch_box *sb;
	s_heap heap;
	s_heap_item item;
	char netlist_filename[256];
	char placement_filename[256];

	s_pb_top_type *pb_top_types;
	int num_pb_top_types;
	GHashTable *block_positions;
	GHashTableIter iter;
	gpointer key, value;
	struct _s_block_position *block_pos;
	s_block **grid;
	int x, y;
	GHashTable *external_nets;
	s_net *net;
	GSList *litem;
	s_pb_graph_pin *sink_pin;
	FILE *dot_file;
//	clb.num_output_pins = 10;
//	clb.output_pins = malloc(10*sizeof(s_list));
	int num_wire_types;
	int global_routing_node_id;
	int previous;
	int *node_usage;
	int *first_level_node_usage;
	int used_nodes;
	int overused_nodes;
	int max_overuse;
	float average_overuse;
	int **num_opins;
	int max_num_opins;
	GList **node_requests;
	GHashTable *id_to_node;
	s_net **grant;
	GHashTable *congested_nets;
	GList *node_request_item;
	s_node_requester *requester;
	int num_large_nets;
	s_pb *pbs;
	int num_pbs;

	t_options options;
	t_arch Arch;
	struct s_router_opts RouterOpts;
	struct s_det_routing_arch RoutingArch;
	t_segment_inf *Segments;
	t_timing_inf Timing;

	wire_types[0].name = names[0];
	wire_types[0].freq = 1;
	wire_types[0].relative_x = 1;
	wire_types[0].relative_y = 0;
	wire_types[0].shape = 0;
	//wire_types[0].num_shapes = 1;
	wire_types[0].direction = WIRE_E;

	wire_types[1].name = names[0];
	wire_types[1].freq = 1;
	wire_types[1].relative_x = 0;
	wire_types[1].relative_y = 1;
	wire_types[1].shape = 0;
	//wire_types[1].num_shapes = 1;
	wire_types[1].direction = WIRE_N;

	wire_types[2].name = names[2];
	wire_types[2].freq = 1;
	wire_types[2].relative_x = -1;
	wire_types[2].relative_y = 0;
	wire_types[2].shape = 0;
	//wire_types[2].num_shapes = 1;
	wire_types[2].direction = WIRE_W;

	wire_types[3].name = names[2];
	wire_types[3].freq = 1;
	wire_types[3].relative_x = 0;
	wire_types[3].relative_y = -1;
	wire_types[3].shape = 0;
	//wire_types[3].num_shapes = 1;
	wire_types[3].direction = WIRE_S;

	//quadtree_test();

	assert(argc == 3);

	memset(&options, 0, sizeof(options));
	options.ArchFile = strdup("sample_arch.xml");
	options.CircuitName = strdup("tseng");
	vpr_init(options, &Arch, &RouterOpts, &RoutingArch, &Segments, &Timing);

	num_wire_types = sizeof(wire_types)/sizeof(s_wire_type);

	parse_arch(argv[1], &pb_top_types, &num_pb_top_types);

	sprintf(placement_filename, "%s.place", argv[2]);
	sprintf(netlist_filename, "%s.net", argv[2]);
//	for (x = 0; x < nx; x++) {
//		for (y = 0; y < ny; y++) {
//			if (grid[x][y].name) {
//				printf("grid[%d][%d] x=%d y=%d name=%s\n", x, y, grid[x][y].x, grid[x][y].y, grid[x][y].name);
//			}
//		}
//	}
	parse_netlist(netlist_filename, pb_top_types, num_pb_top_types, &pbs, &num_pbs, &external_nets);

	read_netlist(netlist_filename, &Arch, &num_blocks, &block, &num_nets, &clb_net);
	/* This is done so that all blocks have subblocks and can be treated the same */
	check_netlist();

	/* have to read the netlist before calling this before it requires num_block to be initialized */
	InitArch(Arch);

	parse_placement(placement_filename, &nx, &ny, &block_positions);

	read_place(placement_filename, netlist_filename, argv[1], nx, ny, num_blocks, block);

	alloc_and_init_grid(&grid, nx+2, ny+2, pb_top_types, num_pb_top_types, pbs, num_pbs, block_positions);

	vpr_build_rr_graph(grid, 34, Arch.Chans, RoutingArch, RouterOpts, Segments, Timing);

	return 0;

//	heap_init(&heap);


//	for (i = 0; i < 100000; i++) {
//		j = rand() % 100000 + 1;
//		//printf("%5d ", j);
//		heap_push(&heap, j, j);
//	}
//	printf("\n");
//	//print_heap(&heap);
//
//	previous = -1;
//		while (i = (int)heap_pop(&heap)) {
//			assert(i >= previous);
//			//printf("item: %5d\n", i);
//			previous = i;
//		}

	update_wire_count(wire_types, num_wire_types, &num_wires);
	init_block_wires(grid, nx, ny, wire_types, num_wire_types, num_wires, &global_routing_node_id, &id_to_node);

	//printf("num global routing nodes: %d\n", global_routing_node_id);

	//dot_file = fopen("graph.dot", "w");

	node_usage = calloc(global_routing_node_id, sizeof(int));
	first_level_node_usage = calloc(global_routing_node_id, sizeof(int));
	node_requests = calloc(global_routing_node_id, sizeof(GList *));
	grant = calloc(global_routing_node_id, sizeof(s_net *));

	num_opins = malloc(sizeof(int *) * nx);

	for (i = 0; i < nx; i++) {
		num_opins[i] = calloc(ny, sizeof(int));
	}
	max_num_opins = 0;

	num_nets = g_hash_table_size(external_nets);

	num_large_nets = 0;
	g_hash_table_iter_init(&iter, external_nets);
	printf("area\n");
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		net = value;

		num_opins[net->source_pin->base.x][net->source_pin->base.y]++;
		if (num_opins[net->source_pin->base.x][net->source_pin->base.y] > max_num_opins) {
			max_num_opins = num_opins[net->source_pin->base.x][net->source_pin->base.y];
		}

		init_net_bounding_box(net);

		if (net->bounding_box.area > nx*ny*0.01) {
			num_large_nets++;
		}
	}

	congested_nets = g_hash_table_new(g_direct_hash, g_direct_equal);

	//for (i = 0; i < 10; i++) {
		for (j = 0; j < global_routing_node_id; j++) {
			node_usage[j] = 0;
			first_level_node_usage[j] = 0;
		}
		g_hash_table_iter_init(&iter, external_nets);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			net = value;

			//printf("net: %s\n", key);

			//printf("%s.%s.%s[%d]\n", net->source_pin->pb->name, net->source_pin->port->pb_type->name, net->source_pin->port->name, net->source_pin->pin_number);
			litem = net->sink_pins;
			while (litem) {
				sink_pin = litem->data;
				//printf("%s.%s.%s[%d] ", sink_pin->pb->name, sink_pin->port->pb_type->name, sink_pin->port->name, sink_pin->pin_number);
				litem = litem->next;
			}
			//printf("\n\n");

			//create_dot_file(&net->source_pin->base, dot_file, 1);
			//fclose(dot_file);
			route_net(net, global_routing_node_id, node_usage, first_level_node_usage, node_requests, grant, false);
		}

//		for (j = 0; j < global_routing_node_id; j++) {
//			if (node_usage[j] > 1) {
//				node_request_item = node_requests[j];
//				while (node_request_item) {
//					requester = node_request_item->data;
//					if (!g_hash_table_contains(congested_nets, requester->net)) {
//						g_hash_table_add(congested_nets, requester->net);
//					}
//					node_request_item = node_request_item->next;
//				}
//			}
//		}

		used_nodes = 0;
		overused_nodes = 0;
		max_overuse = 0;
		average_overuse = 0;
		printf("before\n");
		for (j = 0; j < global_routing_node_id; j++) {
			if (node_usage[j] > 0) {
				used_nodes++;
			}
			if (node_usage[j] > 1) {
				if (node_usage[j] > max_overuse) {
					max_overuse = node_usage[j];
				}
				average_overuse += node_usage[j];
				overused_nodes++;
				//printf("%d\n", node_usage[j]);
			}
		}
		average_overuse /= overused_nodes;
		printf("used: %d overused: %d average_overuse: %f\n", used_nodes, overused_nodes, average_overuse);

		for (j = 0; j < global_routing_node_id; j++) {
			node_usage[j] = 0;
			first_level_node_usage[j] = 0;
		}

		g_hash_table_iter_init(&iter, external_nets);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			net = value;

			//printf("net: %s\n", key);

			//printf("%s.%s.%s[%d]\n", net->source_pin->pb->name, net->source_pin->port->pb_type->name, net->source_pin->port->name, net->source_pin->pin_number);
			litem = net->sink_pins;
			while (litem) {
				sink_pin = litem->data;
				//printf("%s.%s.%s[%d] ", sink_pin->pb->name, sink_pin->port->pb_type->name, sink_pin->port->name, sink_pin->pin_number);
				litem = litem->next;
			}
			//printf("\n\n");

			//create_dot_file(&net->source_pin->base, dot_file, 1);
			//fclose(dot_file);
			route_net(net, global_routing_node_id, node_usage, first_level_node_usage, node_requests, grant, true);
		}

//		for (j = 0; j < global_routing_node_id; j++) {
//			if (node_usage[j] > 1) {
//				node_request_item = node_requests[j];
//				while (node_request_item) {
//					requester = node_request_item->data;
//					if (!g_hash_table_contains(congested_nets, requester->net)) {
//						g_hash_table_add(congested_nets, requester->net);
//					}
//					node_request_item = node_request_item->next;
//				}
//			}
//		}

		used_nodes = 0;
		overused_nodes = 0;
		max_overuse = 0;
		average_overuse = 0;
		printf("after\n");
		for (j = 0; j < global_routing_node_id; j++) {
			if (node_usage[j] > 0) {
				used_nodes++;
			}
			if (node_usage[j] > 1) {
				if (node_usage[j] > max_overuse) {
					max_overuse = node_usage[j];
				}
				average_overuse += node_usage[j];
				overused_nodes++;
				//printf("%d\n", node_usage[j]);
			}
		}
		average_overuse /= overused_nodes;

		printf("used: %d overused: %d average_overuse: %f\n", used_nodes, overused_nodes, average_overuse);

//		for (j = 0; j < global_routing_node_id; j++) {
//			grant[j] = NULL;
//		}
		//reserve_route_resource(node_requests, node_usage, global_routing_node_id, id_to_node, grant, nx*ny);

		for (j = 0; j < global_routing_node_id; j++) {
			node_usage[j] = 0;
			g_list_free(node_requests[j]);
			node_requests[j] = NULL;
		}

		//return 0;
	//}





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
