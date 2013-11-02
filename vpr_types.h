/*
 * vpr_types.h
 *
 *  Created on: Oct 12, 2013
 *      Author: chinhau5
 */

#ifndef VPR_TYPES_H_
#define VPR_TYPES_H_

#include <stdbool.h>
#include "list.h"

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
	char *name;
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

typedef enum _e_interconnect_type { UNKNOWN = -1, DIRECT, COMPLETE, MUX  } e_interconnect_type;

typedef struct _s_interconnect {
	e_interconnect_type type;
	char *name;
	char *input_string;
	char *output_string;
} s_interconnect;

typedef struct _s_mode {
	char *name;
	struct _s_pb_type *children;
	int num_children;

	struct _s_interconnect *interconnects;
	int num_interconnects;
} s_mode;

typedef struct _s_port {
	char *name;
	int num_pins;
} s_port;

typedef struct _s_pb_type {
	char *name;
	char *blif_model;
	int num_pbs;

	struct _s_port *input_ports; /* [port_index][pin_index] */
	int num_input_ports;

	struct _s_port *output_ports; /* [port_index][pin_index] */
	int num_output_ports;

	struct _s_pb_type *parent;
	struct _s_mode *modes;
	int num_modes;
} s_pb_type;

typedef struct _s_pb_top_type {
	struct _s_pb_type pb;
	int height;
	int capacity;

	struct _s_pb_graph_node *pb_graph_head;
} s_pb_top_type;

typedef struct _s_pb_graph_pin {
	struct _s_pb_graph_pin *edges;
} s_pb_graph_pin;

typedef struct _s_pb_graph_node {
	struct _s_pb_type *type;

	struct _s_pb_graph_pin **input_pins; /* [0..num_input_ports-1] [0..num_port_pins-1]*/
	struct _s_pb_graph_pin **output_pins; /* [0..num_output_ports-1] [0..num_port_pins-1]*/
	struct _s_pb_graph_pin **clock_pins; /* [0..num_clock_ports-1] [0..num_port_pins-1]*/

	struct _s_pb_graph_node ***children; /* [0..num_modes-1][0..num_pb_type_in_mode-1][0..num_pb-1] */
	struct _s_pb_graph_node *parent;
} s_pb_graph_node;

typedef struct _s_pb {
	struct _s_pb_type *type;
	int mode;

	struct _s_pb *parent;
	struct _s_pb **children; /* [pb_type][pb_type_instance] */
	int *num_children;
} s_pb;

typedef struct _t_block {
	int x;
	int y;

	struct _s_pb *pb;
} t_block;

typedef struct _s_physical_block_instance {
	int x;
	int y;
	struct _s_pin *input_pins;
	int num_input_pins;
	struct _s_pin *output_pins;
	int num_output_pins;
	struct _s_switch_box *switch_box;
} s_block;

#endif /* VPR_TYPES_H_ */
