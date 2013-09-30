/*
 * rr_graph.c
 *
 *  Created on: 23 Aug, 2013
 *      Author: chinhau5
 */

typedef struct _cluster_info {
	int num_luts;
	int num_inputs;
	int num_outputs;
} t_cluster_info;

typedef struct _track_info {
	int length;
	int *fs;
	//int **
	int num_outputs;
} t_track_info;

typedef struct _rr_node {
	int num_children;
	struct _rr_node **children;
} s_rr_node;

typedef struct _list {
	void *data;
	struct _list *next;
	struct _list *prev;
} s_list;

enum { BLOCK_INPUT, BLOCK_OUTPUT } e_block_pin_type;

enum { TOP, RIGHT, BOTTOM, LEFT, SIDE_END } e_side;

enum { CLB, X_CHANNEL, Y_CHANNEL, SWITCH_BOX } e_block_type;

typedef struct _s_block_pin {
	e_block_pin_type type;
	s_list *fanout;
} s_block_pin;

typedef struct _s_block {
	e_block_type type;
	s_block_pin *pins[4]; /* [side][pin] */
} s_block;

s_list *alloc_list_node()
{
	s_list *list = malloc(sizeof(s_list));
	list->data = NULL;
	list->next = NULL;
	list->prev = NULL;
	return list;
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

/* need to consider pass-throughs for long tracks */
void setup_disjoint_switch_box_internal_connection(int num_track)
{
	int itrack;
	int *starting_track[4];

	get_starting_track()
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

t_rr_node *alloc_rr_node()
{
	malloc(sizeof)
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


