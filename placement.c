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

void parse_placement(const char *filename, int *nx, int *ny, GHashTable **block_positions)
{
	FILE *file;
	char buffer[256];
	char block_name[256];
	int block_number;
	s_block_position *block_position;
	int x, y, z;
	int val;
	int count;

	*block_positions = g_hash_table_new(g_str_hash, g_str_equal);

	file = fopen(filename, "r");

	fgets(buffer, sizeof(buffer), file);

	fscanf(file, "Array size : %d x %d logic blocks", nx, ny);
	*nx += 2;
	*ny += 2;

	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);
	fgets(buffer, sizeof(buffer), file);

	count = 0;
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
}
