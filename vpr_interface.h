/*
 * vpr_interface.h
 *
 *  Created on: Dec 14, 2013
 *      Author: chinhau5
 */

#ifndef VPR_INTERFACE_H_
#define VPR_INTERFACE_H_

#include "vpr/base/vpr_types.h"
#include "OptionTokens.h"
#include "ReadOptions.h"

void vpr_init(
		t_options Options,
		t_arch *Arch,
		struct s_router_opts *RouterOpts,
		struct s_det_routing_arch *RoutingArch,
		t_segment_inf **Segments,
		t_timing_inf *Timing);

void vpr_build_rr_graph(
		int width_fac,
		t_chan_width_dist chan_width_dist,
		struct s_det_routing_arch det_routing_arch,
		struct s_router_opts router_opts,
		t_segment_inf * segment_inf,
		t_timing_inf timing_inf);

#endif /* VPR_INTERFACE_H_ */
