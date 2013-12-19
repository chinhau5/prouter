/*
 * placement.c
 *
 *  Created on: Nov 6, 2013
 *      Author: chinhau5
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vpr_types.h"
#include "placement.h"
#include "pb_graph.h"

void alloc_and_init_grid(s_block ***grid, int nx, int ny, s_pb_top_type *pb_top_types, int num_pb_top_types, s_pb *pbs, int num_pbs, GHashTable *block_positions)
{
	int x, y;
	int i;
	s_pb_top_type *io_type;
	s_pb_top_type *clb_type;
	s_pb *pb;
	s_block_position *pos;

	*grid = malloc(sizeof(s_block *) * nx);
	for (x = 0; x < nx; x++) {
		(*grid)[x] = calloc(ny, sizeof(s_block));
	}

	io_type = NULL;
	for (i = 0; i < num_pb_top_types && !io_type; i++) {
		if (!strcmp(pb_top_types[i].base.name, "io")) {
			io_type = &pb_top_types[i];
		}
	}
	clb_type = NULL;
	for (i = 0; i < num_pb_top_types && !clb_type; i++) {
		if (!strcmp(pb_top_types[i].base.name, "clb")) {
			clb_type = &pb_top_types[i];
		}
	}
	assert(clb_type && io_type);

	for (x = 0; x < nx; x++) {
		for (y = 0; y < ny; y++) {
			(*grid)[x][y].x = x;
			(*grid)[x][y].y = y;

			if (x == 0 || x == nx-1 || y == 0 || y == ny-1) { /* IO tiles */
				(*grid)[x][y].pb = calloc(io_type->capacity, sizeof(s_pb));
//				for (i = 0; i < io_type->capacity; i++) {
//					pb = &(*grid)[x][y].pb[i];
//					pb->type = &io_type->base;
//					pb->block = &(*grid)[x][y];
//					alloc_and_init_pb_pins(pb);
//				}
				(*grid)[x][y].capacity = io_type->capacity;
			} else {
				(*grid)[x][y].pb = calloc(clb_type->capacity, sizeof(s_pb));
//				for (i = 0; i < clb_type->capacity; i++) {
//					pb = &(*grid)[x][y].pb[i];
//					pb->type = &clb_type->base;
//					pb->block = &(*grid)[x][y];
//					alloc_and_init_pb_pins(pb);
//				}
				(*grid)[x][y].capacity = clb_type->capacity;
			}
		}
	}

	assert(g_hash_table_size(block_positions) == num_pbs);
	for (i = 0; i < num_pbs; i++) {
		assert(g_hash_table_contains(block_positions, pbs[i].name));
		pos = g_hash_table_lookup(block_positions, pbs[i].name);
		assert(pos->x < nx && pos->y < ny && pos->z < (*grid)[pos->x][pos->y].capacity);
		(*grid)[pos->x][pos->y].pb[pos->z] = pbs[i];
		pbs[i].block = &(*grid)[pos->x][pos->y];
	}
}

void parse_placement(const char *filename, int *nx, int *ny, GHashTable **block_positions)
{
	FILE *file;
	char buffer[256];
	char block_name[256];
	int block_number;
	s_block_position *block_position;
	int x, y, z;

	*block_positions = g_hash_table_new(g_str_hash, g_str_equal);

	file = fopen(filename, "r");

	fgets(buffer, sizeof(buffer), file);

	fscanf(file, "Array size : %d x %d logic blocks", nx, ny);

	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);

	while (fscanf(file, "%s %d %d %d #%d ", block_name, &x, &y, &z, &block_number) == 5) {
		block_position = malloc(sizeof(s_block_position));
		block_position->x = x;
		block_position->y = y;
		block_position->z = z;

		//printf("%d %s %d %d %d %d\n", val, block_name, block_position->x, block_position->y, block_position->z, block_number);
		assert(!g_hash_table_contains(*block_positions, block_name));
		g_hash_table_insert(*block_positions, strdup(block_name), block_position);
	}

	fclose(file);

	//alloc_and_init_block_grid_positions(grid, *nx, *ny, *block_positions);
}
