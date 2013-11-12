#include <stdlib.h>
#include <stdbool.h>
#include <glib.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
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
	bool visited;
} s_route_details;

float get_cost(s_routing_node *source, s_routing_node *destination)
{
	float cost;

	switch (source->type) {
	case IPIN:
		assert(0); /* we would have finished dijkstra by this time */
		break;
	case WIRE:
		assert(destination->type == WIRE || destination->type == IPIN);
		cost = abs(source->x - destination->x) + abs(source->y - destination->y); /* manhattan distance */
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

void print_wire(s_wire *wire, float cost)
{
	 printf("WIRE id=%d cost=%2f [%d,%d] -> [%d,%d]", wire->base.id, cost, wire->base.x, wire->base.y, wire->base.x+wire->type->relative_x, wire->base.y+wire->type->relative_y);
}

void route_net(s_net *net, int num_routing_nodes)
{
	s_heap heap;
	s_routing_node *current, *neighbour, *sink;
	s_route_details *route_details;
	GSList *sink_list_item, *children_list_item;
	s_routing_node *prev_node;
	s_wire *wire;
	bool found;
	float cost;
	int i;

	route_details = calloc(num_routing_nodes, sizeof(s_route_details));
	for (i = 0; i < num_routing_nodes; i++) {
		route_details[i].min_cost = FLT_MAX;
		route_details[i].visited = false;
	}

	heap_init(&heap);
	heap_push(&heap, 0, net->source_pin);
	route_details[net->source_pin->base.id].min_cost = 0;
	printf("source: x=%d y=%d type=%d\n", net->source_pin->base.x, net->source_pin->base.y, net->source_pin->base.type);

	sink_list_item = net->sink_pins;
	while (sink_list_item) {
		sink = sink_list_item->data;
		printf("sink: id=%d x=%d y=%d type=%d\n", sink->id, sink->x, sink->y, sink->type);

		found = false;
		while (!heap_is_empty(&heap) && !found) {
			current = heap_pop(&heap);
			//route_details[current->id].visited = true;

			if (current->type == WIRE) { printf("current: "); print_wire(current, route_details[current->id].min_cost); printf("\n"); }
			//else if (current->type == IPIN) { printf("current: IPIN id=%d [%d,%d]\n", current->id, current->x, current->y); }

			if (current == sink) {
				found = true;

				prev_node = route_details[current->id].prev_node;
				while (prev_node) {
					if (prev_node->type == WIRE) {
						printf("trace: "); print_wire(prev_node, route_details[prev_node->id].min_cost); printf("\n");
					} else {
						printf("trace: PIN id=%d [%d,%d]\n", prev_node->id, prev_node->x, prev_node->y);
					}
					prev_node = route_details[prev_node->id].prev_node;
				}
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
					}
					//if (!route_details[neighbour->id].visited) {
					//if (neighbour->type == WIRE) { printf("\t neighbour: "); print_wire(neighbour, cost); printf("\n"); }
					//else if (neighbour->type == IPIN) { printf("\t neighbour: IPIN id=%d [%d,%d] cost=%f\n", neighbour->id, neighbour->x, neighbour->y, cost); }
					if (!route_details[neighbour->id].visited) {
						heap_push(&heap, cost, neighbour);
					}
					//}

					children_list_item = children_list_item->next;
				}
			}
		}
		assert(found);
		sink_list_item = sink_list_item->next;
	}

	free(route_details);
}
