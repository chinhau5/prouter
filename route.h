/*
 * route.h
 *
 *  Created on: Oct 12, 2013
 *      Author: chinhau5
 */

#ifndef ROUTE_H_
#define ROUTE_H_

typedef struct _s_bounding_box {

};

typedef struct _s_net {
	int src_clb_x;
	int src_clb_y;
	int output_pin;

	int dst_clb_x;
	int dst_clb_y;
	int input_pin;

	struct _s_pin *source_pin;
	struct _s_pin *sink_pin;
} s_net;

#endif /* ROUTE_H_ */
