/*
 * rr_graph.c
 *
 *  Created on: 23 Aug, 2013
 *      Author: chinhau5
 */

enum { BLOCK_INPUT, BLOCK_OUTPUT } e_block_pin_type;

enum { TOP, RIGHT, BOTTOM, LEFT, SIDE_END } e_side;

enum { CLB, X_CHANNEL, Y_CHANNEL, SWITCH_BOX } e_block_type;

enum { INC_DIRECTION, DEC_DIRECTION } e_track_direction;


typedef struct _cluster_info {
	int num_luts;
	int num_inputs;
	int num_outputs;
} t_cluster_info;

typedef struct _track_info {
	int length;
	float *frac;
	int *fs;
	//int **
	int num_outputs;
} s_track_info;

typedef struct _s_segment_info {
	int start;
	e_track_direction direction;
} s_segment_info;

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

void connect_channel_to_switch_box(s_block *channel, s_block *switch_box)
{
}

void setup_block_connection(s_block *from_block, s_block *to_block, s_list ***lookup) /* lookup[from_side][from_pin][to_side] */
{
	e_side from_side, to_side;
	int from_pin, to_pin;
	for (from_side = TOP; from_side < SIDE_END; from_side++) {
		to_pin = lookup[from_side][from_pin][to_side];

		if (from_block == to_block) {
			assert(from_block->pins[from_side][from_pin].type == BLOCK_INPUT && to_block->pins[to_side][to_pin].type == BLOCK_OUTPUT);
		} else {
			assert(from_block->pins[from_side][from_pin].type == BLOCK_OUTPUT && to_block->pins[to_side][to_pin].type == BLOCK_INPUT);
		}

		insert_into_list(from_block->pins[from_side][from_pin].fanout, &to_block->pins[to_side][to_pin]);
	}
}

void get_starting_tracks(s_block **grid, int x, int y, int side, int *track_start, int num_tracks, int *starting_tracks)
{
	int itrack;
	assert(grid[x][y].type == SWITCH_BOX);
	for (itrack = 0; itrack <  num_tracks; itrack++) {
		track_start[itrack] grid[x][y].pin[]
	}
}

void load_track_start(s_track_info *track_info, int num_track_types, int *num_tracks, int *track_start)
{
	int itype;
	int *
	for (itype = 0; itype < num_track_types; itype++) {
		num_tracks
	}
}

/* need to consider pass-throughs for long tracks */
void setup_disjoint_switch_box_internal_connection(s_block **grid, int x, int y, int num_track)
{
	int itrack, iside;
	int *starting_tracks[4];
	char is_core;

	assert(grid[x][y].type == SWITCH_BOX);

	is_core = (x >= 3 && y >= 3);

	for (iside = 0; iside < SIDE_END; iside++) {

	}



	get_starting_tracks();
}

void create_channels(s_block **grid, int nx, int ny, s_segment_info *seg_info, int num_tracks)
{
	int x, y;
	int itrack;

	/* x channels */
	for (x = 1; x < nx; x += 2) {
		for (y = 1; y < ny; y += 2) {
			grid[x][y].pins[LEFT] = malloc(num_tracks*sizeof(s_block_pin));
			grid[x][y].pins[RIGHT] = malloc(num_tracks*sizeof(s_block_pin));
			grid[x][y].pins[TOP] = NULL;
			grid[x][y].pins[BOTTOM] = NULL;

			for (itrack = 0; itrack < num_tracks; itrack++) {
				grid[x][y].type = X_CHANNEL;
				if (seg_info[itrack].direction == INC_DIRECTION) {
					grid[x][y].pins[LEFT][itrack].type = BLOCK_INPUT;
					grid[x][y].pins[LEFT][itrack].fanout = alloc_list_node();
					insert_into_list(grid[x][y].pins[LEFT][itrack].fanout, &grid[x][y].pins[RIGHT][itrack]);

					grid[x][y].pins[RIGHT][itrack].type = BLOCK_OUTPUT;
				} else {
					assert(seg_info[itrack].direction == DEC_DIRECTION);
					grid[x][y].pins[RIGHT][itrack].type = BLOCK_INPUT;
					grid[x][y].pins[RIGHT][itrack].fanout = alloc_list_node();
					insert_into_list(grid[x][y].pins[RIGHT][itrack].fanout, &grid[x][y].pins[LEFT][itrack]);

					grid[x][y].pins[LEFT][itrack].type = BLOCK_OUTPUT;
				}
			}
		}
	}

	/* y channels */
	for (x = 1; x < nx; x += 2) {
		for (y = 1; y < ny; y += 2) {
			grid[x][y].pins[TOP] = malloc(num_tracks*sizeof(s_block_pin));
			grid[x][y].pins[BOTTOM] = malloc(num_tracks*sizeof(s_block_pin));
			grid[x][y].pins[LEFT] = NULL;
			grid[x][y].pins[RIGHT] = NULL;

			for (itrack = 0; itrack < num_tracks; itrack++) {
				grid[x][y].type = Y_CHANNEL;
				if (seg_info[itrack].direction == INC_DIRECTION) {
					grid[x][y].pins[BOTTOM][itrack].type = BLOCK_INPUT;
					grid[x][y].pins[BOTTOM][itrack].fanout = alloc_list_node();
					insert_into_list(grid[x][y].pins[BOTTOM][itrack].fanout, &grid[x][y].pins[TOP][itrack]);

					grid[x][y].pins[TOP][itrack].type = BLOCK_OUTPUT;
				} else {
					assert(seg_info[itrack].direction == DEC_DIRECTION);
					grid[x][y].pins[TOP][itrack].type = BLOCK_INPUT;
					grid[x][y].pins[TOP][itrack].fanout = alloc_list_node();
					insert_into_list(grid[x][y].pins[TOP][itrack].fanout, &grid[x][y].pins[BOTTOM][itrack]);

					grid[x][y].pins[BOTTOM][itrack].type = BLOCK_OUTPUT;
				}
			}
		}
	}
}

void create_switch_boxes(s_block **grid, int nx, int ny)
{
	int x, y;

	for (x = 1; x < nx; x += 2) {
		for (y = 1; y < ny; y += 2) {
			setup_disjoint_switch_box_internal_connection(grid, x, y)
		}
	}
}

void connect_channels_and_switch_boxes()
{

}

/* CLB OPIN -> CHAN IPIN -> CHAN OPIN -> SB IPIN -> SB OPIN -> CHAN IPIN -> CHAN OPIN -> CLB IPIN */
void build_rr_from_source(s_block_pin *pin)
{
	s_list *current_fanout;
	s_block_pin *current_pin;
	s_rr_node *rr_node;
	switch (pin->block->type) {
	case CLB:
		current_fanout = pin->fanout;
		while (current_fanout) {
			current_pin = current_fanout->data;
			assert(current_pin->block->type == X_CHANNEL || current_pin->block->type == Y_CHANNEL);



			/* find existing rr_node, if not found, alloc node */
			rr_node = find_rr_node();
			if (rr_node == NULL) {
				rr_node = alloc_rr_node();
			}

			if (rr_node->children) {
				insert_into_list(rr_node->children, data);
			} else {
				rr_node->children = create_list(data);
			}

			build_rr_from_source(current_pin);
			current_fanout = current_fanout->next;
		}
		break;
	case SWITCH_BOX:
		current_fanout = pin->fanout;
		while (current_fanout) {
			current_pin = current_fanout->data;

			if (current_pin->block->type == X_CHANNEL || current_pin->block->type == Y_CHANNEL) {
				/* check whether the chanx or chany rr node exists */
				/* connect current
			}

			/* find existing rr_node, if not found, alloc node */
			rr_node = find_rr_node();
			if (rr_node == NULL) {
				rr_node = alloc_rr_node();
			}

			if (rr_node->children) {
				insert_into_list(rr_node->children, data);
			} else {
				rr_node->children = create_list(data);
			}

			build_rr_from_source(current_pin);
			current_fanout = current_fanout->next;
		}
	case X_CHANNEL:
	case Y_CHANNEL:
	default:
		printf("Unexpected block type");
		break
	}
}

void build_actual_rr(s_block **grid, int nx, int ny)
{
	int x, y;
	int side;
	int pin;

	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			if (grid[x][y].type == CLB) {
				for (side = 0; side < SIDE_END; side++) {
					for (pin = 0; pin < grid[x][y].num_pins[side]; pin++) {
						if (grid[x][y].pins[side][pin].type == BLOCK_OUTPUT) {
							build_rr_from_source(&grid[x][y].pins[side][pin]);
						}
					}
				}
			}
		}
	}
}

void build_main()
{
	s_grid **grid;
	alloc_grid(&grid
	create_channels(grid, nx, ny);
	create_clbs(grid, nx, ny);
	connect_channels_and_switch_boxes();
	connect_clbs_and_switch_boxes()
}

void load_channel_lookup(s_block)



void get_tile_segment_info()
{
	get_tile_hori
}

void connect_track_to_track(switch_box_topology)
{
	//connection based on architectural specs
}

void connect_clb_output_to_track(fc_out)
{
	//connection based on architectural specs
}

void connect_drivers_to_track()
{
	connect_track_to_track(rr_node);
	connect_clb_output_to_track(rr_node);
}

void connect_track_to_clb_input(fc_in)
{
	//connection based on architectural specs
}

void add_track_to_lookup()
{
	tile_track_rr_node[x][y][track] = rr_node;
}

s_rr_node *alloc_rr_node()
{
	s_rr_node *node;
	node = malloc(sizeof(s_rr_node));
	node->children = NULL;
	return node;
}

void free_rr_node_array(t_rr_node **array, int array_size)
{
	int i;
	for (i = 0; i < array_size; i++) {
		free(array[i]);
	}
}

void connect_clb_and_channel(int x, int y, char is_horizontal)
{
	int i;
	int num_tracks;
	t_rr_node **tracks;
	t_rr_node *rr_node;

	tracks = get_tracks_starting_at_tile(x, y, &num_tracks);
	for (i = 0; i < num_tracks; i++) {
		connect_drivers_to_track(x, y, rr_node);
	}
	free_rr_node_array(tracks, num_tracks);

	tracks = get_tracks_passing_or_ending_at_tile(x, y, &num_tracks);
	for (i = 0; i < num_tracks; i++) {
		connect_track_to_clb_input(tracks[i], x, y);
	}
	free_rr_node_array(tracks, num_tracks);
}

void build_tile_rr_graph(se)
{
	build_tile_channel(x, y, TRUE);
	build_tile_channel(x, y, FALSE);
}

//assume only one track type for now
void build_channel(t_track_info *track_info, int num_track_info, char is_horizontal)
{
	int i;

	for (i = 0; i < num_track_info; i++) {

	}
	rr_node = alloc_rr_node();

			add_track_to_lookup(rr_node);
}
l
void build_channels(int num_rows, int num_columns)
{
	int i
	for all rows {
		build_channel()
	}

	for all cols {

	}
}

/* required inputs
 * FPGA array size
 * cluster info
 * types of segments
 *
 */
void build_rr_graph(int nx, int ny,
		t_cluster_info *cluster_info, int num_cluster_types,
		t_segment_info *segment_info, int num_segment_types
		)
{
	build_channels();

	for all tiles {
		build_tile_rr_graph(tile_x, tile_y, segment_info);
	}

}


