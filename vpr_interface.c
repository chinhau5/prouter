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

void translate_rr_graph()
{
	int i;
	for (i = 0; i < num_rr_nodes; i++) {
		switch (rr_node[i].type) {
		case IPIN:
			break;
		case OPIN:
			break;
		case CHANX:
			break;
		case CHANY:
			break;

		default:
			assert(0);
			break;
		}
	}
}

void vpr_build_rr_graph(
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

	translate_rr_graph();
}
