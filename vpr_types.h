/*
 * vpr_types.h
 *
 *  Created on: Oct 12, 2013
 *      Author: chinhau5
 */

#ifndef VPR_TYPES_H_
#define VPR_TYPES_H_

typedef enum e_block_pin_type { BLOCK_INPUT, BLOCK_OUTPUT } e_block_pin_type;

typedef enum e_side { TOP, RIGHT, BOTTOM, LEFT, SIDE_END } e_side;

typedef enum e_block_type { CLB, X_CHANNEL, Y_CHANNEL, SWITCH_BOX } e_block_type;

typedef enum e_rr_type { CHANX, CHANY, RR_TYPE_END } e_rr_type;

typedef enum _e_routing_node_type {	OPIN, IPIN, WIRE } e_routing_node_type;

typedef enum _e_wire_node_type {
	WIRE_E, WIRE_W, WIRE_N, WIRE_S,
	WIRE_EN, WIRE_ES, WIRE_WN, WIRE_WS,
	WIRE_NE, WIRE_NW, WIRE_SE, WIRE_SW,
	NUM_WIRE_DIRECTIONS
} e_wire_direction;

enum { INC_DIRECTION, DEC_DIRECTION, NUM_DIRECTIONS };

typedef struct _s_wire_details {
	int id;
	char *name;
	int freq;
	int num_wires; /* determined at runtime */
	bool is_horizontal;
	int relative_x;
	int relative_y;
	e_wire_direction direction; /* determined at runtime */
} s_wire_details;

typedef struct _s_track {
	int start;
	int length;
	bool is_increasing;
} s_track;

typedef struct _s_rr_node {
	int index;
	e_rr_type type;
	bool is_increasing;
	int xlow;
	int xhigh;
	int ylow;
	int yhigh;
	int ptc_number;
	s_list children;
} s_rr_node;

typedef struct _s_routing_node {
	e_routing_node_type type;
	int id;
} s_routing_node;

typedef struct _s_wire {
	s_routing_node super;
	struct _s_wire_details *details;
	int sb_x;
	int sb_y;
	struct _s_list fanin;
	struct _s_list fanout;
} s_wire;

typedef struct _s_pin {
	s_routing_node super;
	struct _s_list connections;
} s_pin;

typedef struct _s_switch_box {
	struct _s_wire *starting_wires;
	int num_starting_wires;
	struct _s_wire ***starting_wires_by_direction_and_type[NUM_WIRE_DIRECTIONS];
	int num_starting_wire_types_by_direction[NUM_WIRE_DIRECTIONS];
	int *num_starting_wires_by_direction_and_type[NUM_WIRE_DIRECTIONS]; /* [direction][type] */

	struct _s_wire **ending_wires;
	int num_ending_wires;
	struct _s_wire ***ending_wires_by_direction_and_type[NUM_WIRE_DIRECTIONS];
	int num_ending_wire_types_by_direction[NUM_WIRE_DIRECTIONS];
	int *num_ending_wires_by_direction_and_type[NUM_WIRE_DIRECTIONS]; /* [direction][type] */

	s_wire_details *wire_details;
	int num_wire_details;
} s_switch_box;

typedef struct _s_block {
	int x;
	int y;
	struct _s_pin *input_pins;
	int num_input_pins;
	struct _s_pin *output_pins;
	int num_output_pins;
	struct _s_switch_box *switch_box;
} s_block;

#endif /* VPR_TYPES_H_ */
