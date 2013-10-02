/*
 * rr_graph.c
 *
 *  Created on: 23 Aug, 2013
 *      Author: chinhau5
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


typedef enum { BLOCK_INPUT, BLOCK_OUTPUT } e_block_pin_type;

typedef enum { TOP, RIGHT, BOTTOM, LEFT, SIDE_END } e_side;

typedef enum { CLB, X_CHANNEL, Y_CHANNEL, SWITCH_BOX } e_block_type;

typedef enum { INC_DIRECTION, DEC_DIRECTION } e_track_direction;


typedef struct _cluster_info {
	int num_luts;
	int num_inputs;
	int num_outputs;
} t_cluster_info;

typedef struct _track_info {
	int length;
	int freq;
	int *fs;
	//int **
	int num_outputs;
} s_track_info;

typedef struct _s_track_instance {
	int start;
	int length;
	e_track_direction direction;
} s_track_instance;

typedef struct _list {
	void *data;
	struct _list *next;
	struct _list *prev;
} s_list;

typedef struct _rr_node {
	s_list *children;
} s_rr_node;



//enum

typedef struct _s_block_pin {
	e_block_pin_type type;
	struct _s_block *block;
	int side;
	int index;
	s_list *fanout;
	s_list *switch_type;
} s_block_pin;

typedef struct _s_block {
	e_block_type type;
	int x;
	int y;
	s_block_pin *pins[4]; /* [side][pin] */
	int num_pins[4];
} s_block;

//s_list *alloc_list_node()
//{
//	s_list *list = malloc(sizeof(s_list));
//	list->data = NULL;
//	list->next = NULL;
//	list->prev = NULL;
//	return list;
//}
//
//s_list *create_list(void *data)
//{
//	s_list *node;
//
//	node = alloc_list_node();
//	node->data = data;
//
//	return node;
//}
//
//void insert_into_list(s_list *list, void *data)
//{
//	s_list *node;
//
//	assert(list->next == NULL);
//
//	node = alloc_list_node();
//	node->data = data;
//	node->prev = list;
//	list->next = node;
//}
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

int get_segment_start(s_track_instance *track_instance, int channel, int track, int segment, int seg_max)
{
	int seg_start;

	if (track_instance[track].direction == INC_DIRECTION) {
		if (segment < track_instance[track].start) {
			seg_start = 2;
		} else {
			seg_start = (segment - track_instance[track].start) / track_instance[track].length;
			seg_start = track_instance[track].start + (seg_start) * track_instance[track].length;
		}
	} else {
		if (segment < track_instance[track].start) {
			seg_start = segment;
		} else {
			seg_start = (segment - track_instance[track].start) / track_instance[track].length;
			seg_start = track_instance[track].start + seg_start * track_instance[track].length;
		}
	}

	return seg_start;
}

int *get_starting_tracks(int channel, int segment, e_track_direction direction, s_track_instance *track_instances, int num_tracks, int *num_starting_tracks)
{
	int itrack;
	int *starting_tracks;
	assert(channel >= 1 && channel%2 == 1);
	assert(segment >= 2 && segment%2 == 0);

	*num_starting_tracks = 0;

	for (itrack = 0; itrack < num_tracks; itrack++) {
		if (track_instances[itrack].direction == direction && segment == get_segment_start(track_instances, channel, itrack, segment, 100)) {
			(*num_starting_tracks)++;
		}
	}
	starting_tracks = malloc(*num_starting_tracks * sizeof(int));
	*num_starting_tracks = 0;
	for (itrack = 0; itrack < num_tracks; itrack++) {
		if (track_instances[itrack].direction == direction && segment == get_segment_start(track_instances, channel, itrack, segment, 100)) {
			starting_tracks[(*num_starting_tracks)++] = itrack;
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
//void build_channel(int channel, int segment)
//{
//	for all starting tracks {
//		alloc_rr_node();
//
//		add rr node to global array for lookup
//	}
//}
//
//
//void build_channels()
//{
//	for all channel
//}
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
//s_rr_node *alloc_rr_node()
//{
//	s_rr_node *node;
//	node = malloc(sizeof(s_rr_node));
//	node->children = NULL;
//	return node;
//}
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

int *get_number_of_tracks_by_type(s_track_info *track_info, int num_track_info, int *num_tracks)
{
	int *num_tracks_by_type;
	int total_freq;
	int type;
	int remainder;
	int num_sets;

	num_tracks_by_type = malloc(num_track_info * sizeof(int));
	num_sets = *num_tracks / 2;

	total_freq = 0;
	for (type = 0; type < num_track_info; type++) {
		total_freq += track_info[type].freq;
	}

	remainder = 0;
	for (type = 0; type < num_track_info; type++) {
		num_tracks_by_type[type] = (num_sets * track_info[type].freq) / total_freq * 2;
		remainder += (num_sets * track_info[type].freq) % total_freq;
	}

	type = 0;
	/* distribute remaining tracks evenly across different track types */
	remainder = remainder / total_freq * 2;
	while (remainder > 0) {
		num_tracks_by_type[type] += 2;
		type = (type + 1) % num_track_info;
		remainder -= 2; //unidirectional routing have tracks in pairs
	}

	/* recalculate total tracks */
	*num_tracks = 0;
	for (type = 0; type < num_track_info; type++) {
		*num_tracks += num_tracks_by_type[type];
	}

	return num_tracks_by_type;
}

s_track_instance *alloc_and_init_track_instances(s_track_info *track_info, int num_track_info, int *num_tracks)
{
	int info;

	int *num_tracks_by_type;
	int track;
	s_track_instance *track_instances;
	int start;
	e_track_direction direction;
	int ntrack;

	num_tracks_by_type = get_number_of_tracks_by_type(track_info, num_track_info, num_tracks);
	track_instances = malloc(*num_tracks * sizeof(s_track_instance));

	ntrack = 0;
	for (info = 0; info < num_track_info; info++) {
		start = 0;
		direction = INC_DIRECTION;
		for (track = 0; track < num_tracks_by_type[info]; track += 2) {
			for (direction = 0; direction < 2; direction++) {
				track_instances[ntrack].start = 2 + start;
				track_instances[ntrack].length = track_info[info].length;
				track_instances[ntrack].direction = direction;

				ntrack++;
			}
			start = (start + 2) % track_info[info].length;
		}
	}

	return track_instances;
}

void dump_track_instances(s_track_instance *track_instances, int num_tracks)
{
	int track;
	for (track = 0; track < num_tracks; track++) {
		printf("Track: %d Start: %d Length: %d\n", track, track_instances[track].start, track_instances[track].length);
	}
}

int main()
{
	s_track_info track_info[2];
	int num_tracks = 39;
	s_track_instance *track_instances;
	int track, channel, segment;
	track_info[0].freq = 1;
	track_info[0].length = 8;
	track_info[1].freq = 2;
	track_info[1].length = 4;

	track_instances = alloc_and_init_track_instances(track_info, 2, &num_tracks);
	dump_track_instances(track_instances, num_tracks);
	//get_starting_tracks(0, 2, INC_DIRECTION, track_instances,num_tracks, NULL);
	for (channel = 0; channel < 3; channel += 2) {
		for (track = 0; track < 10; track++) {
			for (segment = 2; segment < 20; segment+=2) {
				printf("channel: %3d track: %3d segment: %3d start: %3d\n", channel, track, segment, get_segment_start(track_instances, channel, track, segment, 50));
			}
		}
	}
	return 0;
}



