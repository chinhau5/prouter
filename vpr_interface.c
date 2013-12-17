/*
 * vpr_interface.c
 *
 *  Created on: Dec 14, 2013
 *      Author: chinhau5
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "vpr_interface.h"
#include "util.h"
#include "vpr/base/vpr_types.h"
#include "globals.h"
#include "graphics.h"
#include "read_netlist.h"
#include "check_netlist.h"
#include "print_netlist.h"
#include "draw.h"
#include "place_and_route.h"
#include "pack.h"
#include "SetupGrid.h"
#include "stats.h"
#include "path_delay.h"
//#include "OptionTokens.h"
//#include "ReadOptions.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"
#include "vpr/route/rr_graph.h"
#include "pb_type_graph.h"

void vpr_init(
		t_options Options,
		t_arch *Arch,
		struct s_router_opts *RouterOpts,
		struct s_det_routing_arch *RoutingArch,
		t_segment_inf **Segments,
		t_timing_inf *Timing)
{

	t_model *user_models, *library_models;

	enum e_operation Operation;
	struct s_file_name_opts FileNameOpts;
	struct s_packer_opts PackerOpts;
	struct s_placer_opts PlacerOpts;
	struct s_annealing_sched AnnealSched;


	boolean ShowGraphics;
	boolean TimingEnabled;
	int GraphPause;

	TimingEnabled = IsTimingEnabled(Options);

	SetupVPR(Options, TimingEnabled, &FileNameOpts, Arch, &Operation, &user_models,
		 &library_models, &PackerOpts, &PlacerOpts,
		 &AnnealSched, RouterOpts, RoutingArch, Segments,
		 Timing, &ShowGraphics, &GraphPause);

	/* Check inputs are reasonable */
	CheckOptions(Options, TimingEnabled);
	CheckArch(*Arch, TimingEnabled);

	/* Verify settings don't conflict or otherwise not make sense */
	CheckSetup(Operation, PlacerOpts, AnnealSched, *RouterOpts,
		   *RoutingArch, *Segments, *Timing, Arch->Chans);
	fflush(stdout);
}

s_pb_graph_pin *vpr_pin_to_vprx_pin(s_block *block, int sub_block, int node_block_pin)
{
	int i, j;
	s_pb_type *pb_type;
	s_pb_graph_pin *pin;
	int offset;

	assert(sub_block < block->capacity);
	pb_type = block->pb[sub_block].type;
	offset = -1;
	/* order of ports is important (input -> output -> clock) */
	for (i = 0; i < pb_type->num_input_ports && offset != node_block_pin; i++) {
		for (j = 0; j < pb_type->input_ports[i].num_pins && offset != node_block_pin; j++) {
			pin = &block->pb[sub_block].input_pins[i][j];
			offset++;
		}
	}
	for (i = 0; i < pb_type->num_output_ports && offset != node_block_pin; i++) {
		for (j = 0; j < pb_type->output_ports[i].num_pins && offset != node_block_pin; j++) {
			pin = &block->pb[sub_block].output_pins[i][j];
			offset++;
		}
	}
	for (i = 0; i < pb_type->num_clock_ports && offset != node_block_pin; i++) {
		for (j = 0; j < pb_type->clock_ports[i].num_pins && offset != node_block_pin; j++) {
			pin = &block->pb[sub_block].clock_pins[i][j];
			offset++;
		}
	}
	return pin;
}

void translate_rr_graph(s_block **grid)
{
	int i;
	for (i = 0; i < num_rr_nodes; i++) {
		switch (rr_node[i].type) {
		case IPIN:
			break;
		case OPIN:
			printf("%s\n", rr_node[i].pb_graph_pin->port->name);
			break;
		case CHANX:
			break;
		case CHANY:
			break;
		case SOURCE:
		case SINK:
			/* not handling them */
			break;
		default:
			assert(0);
			break;
		}
	}
}

void test(s_block **vprx_grid)
{
	int i, j;
	int blk;
	s_pb_graph_pin *pin;

	for (i = 0; i < num_nets; i++) {
		for (j = 0; j <= clb_net[i].num_sinks; j++) {
			blk = clb_net[i].node_block[j];

			pin = vpr_pin_to_vprx_pin(&vprx_grid[block[blk].x][block[blk].y], block[blk].z, clb_net[i].node_block_pin[j]);
			printf("%s.%s[%d] ", pin->pb->name, pin->port->name, pin->pin_number);
			fflush(stdout);
		}
		printf("\n");
//		blk = clb_net[i].node_block[0];
//		if (!strcmp(block[blk].type->name, "io")/*block[blk].x == 0*/) {
//			assert(!strcmp(block[blk].type->name, "io"));
//			assert(clb_net[i].node_block_pin[0] < 24);
//			printf("pin[%d,%d,%d,%s,%s]: %d\n", block[blk].x, block[blk].y, block[blk].z, block[blk].name, block[blk].type->name, clb_net[i].node_block_pin[0]);
//		}
	}
}

void test2()
{
	int i, j;
	int base;
	for (i = 0; i < )
}

void vpr_build_rr_graph(
		s_block **vprx_grid,
		int width_fac,
		t_chan_width_dist chan_width_dist,
		struct s_det_routing_arch det_routing_arch,
		struct s_router_opts router_opts,
		t_segment_inf * segment_inf,
		t_timing_inf timing_inf)
{
	int warnings;

	init_chan(width_fac, chan_width_dist);

	/* Free any old routing graph, if one exists. */

	free_rr_graph();


	/* Set up the routing resource graph defined by this FPGA architecture. */

	build_rr_graph(GRAPH_UNIDIR,
		   num_types, type_descriptors, nx, ny, grid,
		   chan_width_x[0], NULL,
		   det_routing_arch.switch_block_type, det_routing_arch.Fs,
		   det_routing_arch.num_segment, det_routing_arch.num_switch, segment_inf,
		   det_routing_arch.global_route_switch,
		   det_routing_arch.delayless_switch, timing_inf,
		   det_routing_arch.wire_to_ipin_switch,
		   router_opts.base_cost_type, &warnings);

	test(vprx_grid);

	translate_rr_graph(vprx_grid);
}
