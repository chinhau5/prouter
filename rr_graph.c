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


typedef enum e_block_pin_type { BLOCK_INPUT, BLOCK_OUTPUT } e_block_pin_type;

typedef enum e_side { TOP, RIGHT, BOTTOM, LEFT, SIDE_END } e_side;

typedef enum e_block_type { CLB, X_CHANNEL, Y_CHANNEL, SWITCH_BOX } e_block_type;

typedef enum e_rr_type { CHANX, CHANY, RR_TYPE_END } e_rr_type;


typedef struct cluster_info {
	int num_luts;
	int num_inputs;
	int num_outputs;
} t_cluster_info;

typedef struct s_track_specs {
	int length;
	int freq;
	int *fs;
	//int **
	int num_outputs;
} s_track_specs;

typedef struct s_track {
	int start;
	int length;
	bool is_increasing;
} s_track;

typedef struct s_list {
	void *data;
	struct s_list *next;
	struct s_list *prev;
} s_list;

typedef struct s_rr_node {
	int index;
	e_rr_type type;
	bool is_increasing;
	int xlow;
	int xhigh;
	int ylow;
	int yhigh;
	int ptc_number;
	s_list *children;
} s_rr_node;


typedef struct _s_block_pin {
	e_block_pin_type type;
	struct _s_block *block;
	int side;
	int index;
	s_list *fanout;
	s_list *switch_type;
} s_block_pin;

typedef struct s_block {
	e_block_type type;
	int x;
	int y;
	s_block_pin *pins[4]; /* [side][pin] */
	int num_pins[4];
} s_block;

s_list *alloc_list_node()
{
	s_list *list = malloc(sizeof(s_list));
	list->data = NULL;
	list->next = NULL;
	list->prev = NULL;
	return list;
}

s_list *create_list(void *data)
{
	s_list *node;

	node = alloc_list_node();
	node->data = data;

	return node;
}

void insert_into_list(s_list *list, void *data)
{
	s_list *node;

	assert(list->next == NULL);

	node = alloc_list_node();
	node->data = data;
	node->prev = list;
	list->next = node;
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

int get_track_low_segment(s_track *tracks, int channel, int track, int segment)
{
	int low_seg;
	int staggering_offset;

	staggering_offset = channel - 1;

	low_seg = segment - (segment + tracks[track].length - (tracks[track].start - staggering_offset)) % tracks[track].length;
	if (low_seg < 2) {
		low_seg = 2;
	}

	return low_seg;
}

int get_track_high_segment(s_track *tracks, int channel, int track, int segment, int low_seg, int seg_max)
{
	int high_seg;
	int first_full;

	int staggering_offset;

	staggering_offset = channel - 1;

	high_seg = low_seg + tracks[track].length - 2;

	if (low_seg == 2) {
		first_full = 2 + (tracks[track].start + tracks[track].length - 2 - (staggering_offset % tracks[track].length)) % tracks[track].length;
		if(first_full > 2)
		{
			/* then we stop just before the first full seg */
			high_seg = first_full - 2;
		}
	}

	if (high_seg > seg_max) {
		high_seg = seg_max;
	}

	return high_seg;
}

int *get_starting_tracks(int channel, int segment, bool is_increasing, int seg_max, s_track *tracks, int num_tracks, int *num_starting_tracks)
{
	int itrack;
	int *starting_tracks;
	int low_seg, high_seg;

	assert(channel >= 1 && channel%2 == 1 && segment >= 2 && segment%2 == 0);

	*num_starting_tracks = 0;
	for (itrack = 0; itrack < num_tracks; itrack++) {
		if (tracks[itrack].is_increasing == is_increasing) {
			low_seg = get_track_low_segment(tracks, channel, itrack, segment);
			high_seg = get_track_high_segment(tracks, channel, itrack, segment, low_seg, seg_max);
			if ((is_increasing && segment == low_seg) || (!is_increasing && segment == high_seg)) {
				(*num_starting_tracks)++;
			}
		}
	}
	starting_tracks = malloc(*num_starting_tracks * sizeof(int));
	*num_starting_tracks = 0;
	for (itrack = 0; itrack < num_tracks; itrack++) {
		if (tracks[itrack].is_increasing == is_increasing) {
			low_seg = get_track_low_segment(tracks, channel, itrack, segment);
			high_seg = get_track_high_segment(tracks, channel, itrack, segment, low_seg, seg_max);
			if ((is_increasing && segment == low_seg) || (!is_increasing && segment == high_seg)) {
				starting_tracks[(*num_starting_tracks)++] = itrack;
			}
		}
	}

	return starting_tracks;
}

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

void add_rr_node_to_lookup(s_rr_node *node, s_rr_node *****rr_node_lookup, s_rr_node **rr_nodes, int *num_rr_nodes)
{
	int x, y;

	switch (node->type) {
	case CHANX:
		assert (node->ylow == node->yhigh);

		y = node->ylow;

		for (x = node->xlow; x <= node->xhigh; x++) {
			rr_node_lookup[x][y][node->type][node->ptc_number] = node;
		}

		rr_nodes[(*num_rr_nodes)++] = node;

		break;
	case CHANY:
		assert (node->xlow == node->xhigh);

		x = node->xlow;

		for (y = node->ylow; y <= node->yhigh; y++) {
			rr_node_lookup[x][y][node->type][node->ptc_number] = node;
		}

		rr_nodes[(*num_rr_nodes)++] = node;

		break;
	default:
		break;
	}

}

s_rr_node *alloc_rr_node()
{
	s_rr_node *node;
	node = malloc(sizeof(s_rr_node));
	node->children = NULL;
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

void build_channel(int channel, int segment, bool is_increasing, bool is_horizontal,
		int nx, int ny, int num_tracks, s_track *tracks,
		s_rr_node *****rr_node_lookup, s_rr_node *rr_nodes, int *num_rr_nodes)
{
	int num_starting_tracks;
	int *starting_tracks;
	int i;
	int track;
	s_rr_node *node;

	starting_tracks = get_starting_tracks(channel, segment, is_increasing, get_seg_max(is_horizontal, nx, ny), tracks, num_tracks, &num_starting_tracks);

	for (i = 0; i < num_starting_tracks; i++) {
		track = starting_tracks[i];

		assert(tracks[track].is_increasing == is_increasing);

		node = alloc_rr_node();
		node->index = *num_rr_nodes;
		node->ptc_number = track;
		node->is_increasing = tracks[track].is_increasing;
		if (is_horizontal) {
			node->type = CHANX;
			node->xlow = get_track_low_segment(tracks, channel, track, segment);
			node->xhigh = get_track_high_segment(tracks, channel, track, segment, node->xlow, get_seg_max(is_horizontal, nx, ny));
			node->ylow = node->yhigh = channel;
		} else {
			node->type = CHANY;
			node->ylow = get_track_low_segment(tracks, channel, track, segment);
			node->yhigh = get_track_high_segment(tracks, channel, track, segment, node->ylow, get_seg_max(is_horizontal, nx, ny));
			node->xlow = node->xhigh = channel;
		}

		add_rr_node_to_lookup(node, rr_node_lookup, rr_nodes, num_rr_nodes);
	}
}
//
//
void build_channels()
{
	//for ()
}

s_rr_node *get_rr_node(int x, int y, e_rr_type type, int ptc_number, s_rr_node *****rr_node_lookup)
{
	return rr_node_lookup[x][y][type][ptc_number];
}

void add_rr_node_fanout(s_rr_node *src_node, s_rr_node *dst_node)
{
	if (src_node->children) {
		insert_into_list(src_node->children, dst_node);
	} else {
		src_node->children = create_list(dst_node);
	}
}

void connect_channel(int channel, int segment, bool is_increasing, bool is_horizontal, int nx, int ny, int num_tracks, s_track *tracks, s_rr_node *****rr_node_lookup)
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

int *get_number_of_tracks_by_type(s_track_specs *track_specs, int num_track_specs, int *num_tracks)
{
	int *num_tracks_by_type;
	int total_freq;
	int type;
	int remainder;
	int num_sets;

	num_tracks_by_type = malloc(num_track_specs * sizeof(int));
	num_sets = *num_tracks / 2;

	total_freq = 0;
	for (type = 0; type < num_track_specs; type++) {
		total_freq += track_specs[type].freq;
	}

	remainder = 0;
	for (type = 0; type < num_track_specs; type++) {
		num_tracks_by_type[type] = (num_sets * track_specs[type].freq) / total_freq * 2;
		remainder += (num_sets * track_specs[type].freq) % total_freq;
	}

	type = 0;
	/* distribute remaining tracks evenly across different track types */
	remainder = remainder / total_freq * 2;
	while (remainder > 0) {
		num_tracks_by_type[type] += 2;
		type = (type + 1) % num_track_specs;
		remainder -= 2; //unidirectional routing have tracks in pairs
	}

	/* recalculate total tracks */
	*num_tracks = 0;
	for (type = 0; type < num_track_specs; type++) {
		*num_tracks += num_tracks_by_type[type];
	}

	return num_tracks_by_type;
}

s_track *alloc_and_init_tracks(s_track_specs *track_specs, int num_track_specs, int *num_tracks)
{
	int info;

	int *num_tracks_by_type;
	int track;
	s_track *track_instances;
	int start;
	int direction;
	bool is_increasing;
	int ntrack;

	num_tracks_by_type = get_number_of_tracks_by_type(track_specs, num_track_specs, num_tracks);
	track_instances = malloc(*num_tracks * sizeof(s_track));

	ntrack = 0;
	for (info = 0; info < num_track_specs; info++) {
		start = 0;
		for (track = 0; track < num_tracks_by_type[info]; track += 2) {
			is_increasing = true;
			for (direction = 0; direction < 2; direction++) {
				track_instances[ntrack].start = 2 + start;
				track_instances[ntrack].length = track_specs[info].length;
				track_instances[ntrack].is_increasing = is_increasing;

				ntrack++;
				is_increasing = !is_increasing;
			}
			start = (start + 2) % track_specs[info].length;
		}
	}

	return track_instances;
}

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

int main()
{
	s_track_specs track_info[2];
	int num_tracks = 40;
	s_track *tracks;
	int track, channel, segment;
	int start;
	int nx, ny;
	s_rr_node *****rr_node_lookup;
	track_info[0].freq = 1;
	track_info[0].length = 8;
	track_info[1].freq = 2;
	track_info[1].length = 4;

	tracks = alloc_and_init_tracks(track_info, 2, &num_tracks);
	dump_tracks(tracks, num_tracks);
	clb_array_size_to_grid_size(2, 2, &nx, &ny);
	rr_node_lookup = alloc_rr_node_lookup(nx, ny);
	build_channel(1, 2, true, true, nx, ny, num_tracks, tracks, rr_node_lookup);

	//get_starting_tracks(0, 2, INC_DIRECTION, track_instances,num_tracks, NULL);
//	for (channel = 0; channel < 3; channel += 2) {
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



