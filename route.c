#include <stdlib.h>
#include <stdbool.h>
#include <glib.h>
#include <assert.h>
#include <limits.h>
#include "list.h"
#include "vpr_types.h"
#include "route.h"
#include "heap.h"

//void init_net_pin_pointers(s_block *clbs, int clb_nx, int clb_ny, s_net *net)
//{
//	assert()
//}
//
//void get_mst(s_block *clbs, s_net *net)
//{
//
//}
//
typedef struct _s_route_details {
	struct _s_routing_node *prev_node;
	float min_cost;
} s_route_details;

float get_cost(s_routing_node *source, s_routing_node *destination)
{
	float cost;
	s_wire *source_wire, *destination_wire;

	switch (source->type) {
	case IPIN:
		assert(0); /* we would have finished dijkstra by this time */
		break;
	case WIRE:
		source_wire = source;
		destination_wire = destination;
		cost = abs(source_wire->x - destination_wire->x) + abs(source_wire->y - destination_wire->y); /* manhattan distance */
		break;
	case OPIN:
		assert(destination->type == WIRE);
		cost = 0;
		break;
	default:
		assert(0); /* unknown routing node type */
		break;
	}
	return cost;
}

void route_net(s_net *net, int num_routing_nodes)
{
	s_heap heap;
	s_routing_node *current, *neighbour, *sink;
	s_route_details *route_details;
	GSList *sink_list_item, *children_list_item;
	bool found;
	float cost;
	int i;

	route_details = calloc(num_routing_nodes, sizeof(s_route_details));
	for (i = 0; i < num_routing_nodes; i++) {
		route_details[i].min_cost = INT_MAX;
	}

	heap_init(&heap);
	heap_push(&heap, 0, net->source_pin);
	route_details[net->source_pin->base.id].min_cost = 0;

	sink_list_item = net->sink_pins;
	while (sink_list_item) {
		sink = sink_list_item->data;

		found = false;
		while (!heap_is_empty(&heap) && !found) {
			current = heap_pop(&heap);

			if (current == sink) {
				found = true;
			} else {
				/* for all neighbour */
				children_list_item = current->children;
				while (children_list_item) {
					neighbour = children_list_item->data;
					assert(neighbour);

					cost = route_details[current->id].min_cost + get_cost(current, neighbour);
					if (cost < route_details[neighbour->id].min_cost) {
						route_details[neighbour->id].min_cost = cost;
						route_details[neighbour->id].prev_node = current;
						heap_push(&heap, cost, neighbour);
					}

					children_list_item = children_list_item->next;
				}
			}
		}
		assert(found);
		sink_list_item = sink_list_item->next;
	}

	free(route_details);
}
