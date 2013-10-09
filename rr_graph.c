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
#include <gtk/gtk.h>

#define IN
#define OUT
#define INOUT

typedef enum e_block_pin_type { BLOCK_INPUT, BLOCK_OUTPUT } e_block_pin_type;

typedef enum e_side { TOP, RIGHT, BOTTOM, LEFT, SIDE_END } e_side;

typedef enum e_block_type { CLB, X_CHANNEL, Y_CHANNEL, SWITCH_BOX } e_block_type;

typedef enum e_rr_type { CHANX, CHANY, RR_TYPE_END } e_rr_type;
enum { INC_DIRECTION, DEC_DIRECTION, NUM_DIRECTIONS };


typedef struct _s_cluster_info {
	int num_luts;
	int num_inputs;
	int num_outputs;
} s_cluster_info;

typedef struct _s_wire_specs {
	int id;
	char *name;
	int freq;
	int num_wires;
	int relative_x;
	int relative_y;
} s_wire_specs;

typedef struct _s_track {
	int start;
	int length;
	bool is_increasing;
} s_track;



typedef struct _s_list_item {
	void *data;
	struct _s_list_item *next;
	struct _s_list_item *prev;
} s_list_item;

typedef struct _s_list {
	s_list_item *head;
	s_list_item *current;
} s_list;

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

typedef struct _s_wire {
	int id;
	s_wire_specs *spec;
	s_list fanin;
	s_list fanout;
} s_wire;

typedef struct _s_block {
	e_block_type type;
	int x;
	int y;
	s_list *output_pins;
	int num_output_pins;
} s_block;

typedef struct _s_switch_box {
	int total_num_wires;
	int *num_wires_by_type;
	int num_wire_types;
	s_wire *wires;
} s_switch_box;

s_list_item *alloc_list_item()
{
	s_list_item *item = malloc(sizeof(s_list_item));
	item->data = NULL;
	item->next = NULL;
	item->prev = NULL;
	return item;
}

//s_list *create_list(void *data)
//{
//	s_list *node;
//
//	node = alloc_list_item();
//	node->data = data;
//
//	return node;
//}

void insert_into_list(s_list *list, void *data)
{
	s_list_item *item;

	item = alloc_list_item();
	item->data = data;

	if (!list->head) {
		list->head = item;
		list->current = item;
	} else {
		item->prev = list->current;
		list->current->next = item;
		list->current = item;
	}
}

void update_wire_spec_with_wire_count(INOUT s_wire_specs *wire_specs, IN int num_wire_specs, INOUT int *num_wires)
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

/* we assume all the wire_specs are unique */
s_switch_box *create_switch_box(s_wire_specs *wire_specs, int num_wire_specs, int num_wires_per_clb, int x, int y)
{
	s_switch_box *switch_box;
	int wire_spec;
	int wire;
	int count;

	switch_box = malloc(sizeof(s_switch_box));
	switch_box->wires = malloc(sizeof(s_wire) * num_wires_per_clb);
	switch_box->num_wires_by_type = malloc(sizeof(int) * num_wire_specs);
	switch_box->total_num_wires = num_wires_per_clb;
	switch_box->num_wire_types = num_wire_specs;
	count = 0;
	for (wire_spec = 0; wire_spec < num_wire_specs; wire_spec++) {
		switch_box->num_wires_by_type[wire_spec] = wire_specs[wire_spec].num_wires;
		for (wire = 0; wire < wire_specs[wire_spec].num_wires; wire++) {
			switch_box->wires[count].id = count;
			switch_box->wires[count].spec = &wire_specs[wire_spec];
			count++;
		}
	}
	assert(count == num_wires_per_clb);
	return switch_box;
}

void add_fanout_to_clb_output_pin(s_block *clb, int pin_index, s_wire *fanout)
{
	insert_into_list(&(clb->output_pins[pin_index]), fanout);
}

void connect_clb_to_switch_box(s_block *clb, s_switch_box *switch_box, float fc_in, float fc_out)
{
	int output_pin;
	int actual_fc_out;
	int id;
	bool *connected;
	int num_connected_wires;
	int i;

	actual_fc_out = switch_box->total_num_wires * fc_out;
	connected = malloc(sizeof(bool) * switch_box->total_num_wires);

	id = 0;
	for (output_pin = 0; output_pin < clb->num_output_pins; output_pin++) {
		num_connected_wires = 0;
		for (i = 0; i < switch_box->total_num_wires; i++) {
			connected[i] = false;
		}
		while (num_connected_wires < actual_fc_out) {
			for (i = 0; i < switch_box->total_num_wires; i++) {
				if (id == switch_box->wires[i].spec->id && !connected[i]) {
					add_fanout_to_clb_output_pin(clb, output_pin, &(switch_box->wires[i]));
					connected[i] = true;

					num_connected_wires++;
					break;
				}
			}
			id = (id+1) % switch_box->num_wire_types;
		}
	}
}

void dump_clb_output_pin(s_block *clb)
{
	int pin;
	s_list_item *item;
	s_wire *wire;
	for (pin = 0; pin < clb->num_output_pins; pin++) {
		item = clb->output_pins[pin].head;
		printf("Pin %d ->\n", pin);
		while (item) {
			wire = (s_wire *)item->data;
			printf("\t %s[%d]\n", wire->spec->name, wire->id);
			item = item->next;
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
s_rr_node *****alloc_rr_node_lookup(int nx, int ny)
{
	int x, y, type, ptc;
	s_rr_node *****lookup;

	lookup = malloc(sizeof(void *) * nx);
	for (x = 0; x < nx; x++) {
		lookup[x] = malloc(sizeof(void *) * ny);
		for (y = 0; y < ny; y++) {
			lookup[x][y] = malloc(sizeof(void *) * RR_TYPE_END);
			for (type = 0; type < RR_TYPE_END; type++) {
				lookup[x][y][type] = malloc(sizeof(void *) * 200);
				for (ptc = 0; ptc < 200; ptc++) {
					lookup[x][y][type][ptc] = NULL;
				}
			}
		}
	}

	return lookup;
}

void add_rr_node_to_lookup(s_rr_node *node, s_rr_node *****rr_node_lookup, int *num_rr_nodes)
{
	int x, y;

	switch (node->type) {
	case CHANX:
		assert (node->ylow == node->yhigh);

		y = node->ylow;

		for (x = node->xlow; x <= node->xhigh; x++) {
			rr_node_lookup[x][y][node->type][node->ptc_number] = node;
		}

		break;
	case CHANY:
		assert (node->xlow == node->xhigh);

		x = node->xlow;

		for (y = node->ylow; y <= node->yhigh; y++) {
			rr_node_lookup[x][y][node->type][node->ptc_number] = node;
		}

		break;
	default:
		break;
	}

}

s_rr_node *alloc_rr_node()
{
	s_rr_node *node;
	node = malloc(sizeof(s_rr_node));
	//node->children = NULL;
	return node;
}

bool is_valid_channel(int x, int y) {
	return true;
}

int get_seg_max(bool is_horizontal, int nx, int ny)
{
	if (is_horizontal) {
		return nx - 1 - 1;
	} else {
		return ny - 1 - 1;
	}
}

void build_block_pins(int x, int y)
{

}

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

s_rr_node *alloc_and_load_rr_node_array(s_rr_node *****rr_node_lookup, int *num_rr_nodes)
{
	s_rr_node *rr_nodes;
	rr_nodes = malloc(sizeof(s_rr_node) * *num_rr_nodes);

	//for
	return rr_nodes;
}
void build_channels()
{
	//for ()
}

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

void connect_channel(int channel, int segment, bool is_increasing, bool is_horizontal,
		int nx, int ny, int num_tracks, s_track *tracks, s_rr_node *****rr_node_lookup)
{
	int seg;
	int num_starting_tracks;
	int *starting_tracks;
	int track;
	int i;
	int low_seg, high_seg;
	int direction;
	s_list *fanouts, *fanout;
	s_rr_node *dst_node, *src_node;

	//starting_tracks = get_starting_tracks(channel, segment, is_increasing, tracks, num_tracks, &num_starting_tracks);
	for (i = 0; i < num_starting_tracks; i++) {
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
	}
}
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

void dump_tracks(s_track *tracks, int num_tracks)
{
	int track;
	for (track = 0; track < num_tracks; track++) {
		printf("Track: %d Start: %d Length: %d is_increasing: %d\n", track, tracks[track].start, tracks[track].length, tracks[track].is_increasing);
	}
}

void dump_rr_nodes(s_rr_node *rr_nodes, int num_rr_nodes)
{
	int i;
	char *rr_type_name[] = { "CHANX", "CHANY" };
	for (i = 0; i < num_rr_nodes; i++) {
		printf("Node %d: %s (%d,%d) -> (%d,%d) is_increasing: %d\n",
				rr_nodes[i].index, rr_type_name[rr_nodes[i].type], rr_nodes[i].xlow, rr_nodes[i].ylow, rr_nodes[i].xhigh, rr_nodes[i].yhigh,
				rr_nodes[i].is_increasing);
	}
}

void clb_array_size_to_grid_size(int clb_nx, int clb_ny, int *grid_nx, int *grid_ny)
{
	int num_connection_box;
	const int num_io = 2;

	num_connection_box = clb_nx + 1;
	*grid_nx = clb_nx + num_connection_box + num_io;

	num_connection_box = clb_ny + 1;
	*grid_ny = clb_ny + num_connection_box + num_io;
}

static void
close_window (GtkWidget *widget, gpointer lol)
{
//  if (surface)
//    cairo_surface_destroy (surface);
	printf("%X\n", lol);

  //gtk_main_quit ();
}

int main2(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *da;
	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "prouter");
	g_signal_connect(window, "destroy", G_CALLBACK(close_window), 0xdeadbeef);
	da = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), da);

	gtk_widget_show(window);
	gtk_main();
}

int main()
{
	s_wire_specs wire_specs[2];
	int num_wires = 40;
	s_track *tracks;
	int track, channel, segment;
	int start;
	int nx, ny;
	char *names[] = { "SINGLE", "QUAD" };
	s_block clb;
	s_rr_node *****rr_node_lookup;
	s_switch_box *sb;
	clb.num_output_pins = 10;
	clb.output_pins = malloc(10*sizeof(s_list));
	wire_specs[0].name = names[0];
	wire_specs[0].id = 0;
	wire_specs[0].freq = 1;
	wire_specs[0].relative_x = 1;
	wire_specs[0].relative_y = 0;
	wire_specs[1].id = 1;
	wire_specs[1].name = names[1];
	wire_specs[1].freq = 1;
	wire_specs[1].relative_x = 4;
	wire_specs[1].relative_y = 0;

	//tracks = alloc_and_init_tracks(track_info, 1, &num_tracks);
	//dump_tracks(tracks, num_tracks);
	//clb_array_size_to_grid_size(2, 2, &nx, &ny);
	//rr_node_lookup = alloc_rr_node_lookup(nx, ny);
	//build_channel(1, 2, true, true, nx, ny, num_tracks, tracks, rr_node_lookup);
	update_wire_spec_with_wire_count(wire_specs, 2, &num_wires);
	sb = create_switch_box(wire_specs, 2, num_wires, 0, 0);
	connect_clb_to_switch_box(&clb, sb, 0.5, 0.5);
	dump_clb_output_pin(&clb);
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



