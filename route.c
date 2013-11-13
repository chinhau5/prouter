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
	float expected_cost;
	bool visited;
} s_route_details;

//float get_expected_cost(s_routing_node *current_node, GSList *sinks)
//{
//	float cost;
//	GSList *sinks_item;
//	s_routing_node *sink;
//
//	cost = 0;
//	sinks_item = sinks;
//	//while (sinks_item) {
//		sink = sinks_item->data;
//		cost += abs(current_node->x - sink->x) + abs(current_node->y - sink->y);
//		//sinks_item = sinks_item->next;
//	//}
//
//	return cost;
//}

float get_expected_cost(s_routing_node *current_node, s_routing_node *sink_node)
{
	s_wire *wire;
	float cost;

	assert(sink_node->type == IPIN);

	switch (current_node->type) {
	case IPIN:
		if (current_node == sink_node) {
			cost = 0;
		} else {
			cost = FLT_MAX; /* we can't reach other IPIN from a IPIN */
		}
		break;
	case WIRE:
		wire = current_node;
		cost = abs((wire->base.x + wire->type->relative_x) - sink_node->x) + abs((wire->base.y + wire->type->relative_y) - sink_node->y) +
				abs(wire->type->relative_x) + abs(wire->type->relative_y);
		break;
	case OPIN:
		cost = abs(current_node->x - sink_node->x) + abs(current_node->y - sink_node->y);
		break;
	default:
		assert(0);
		break;
	}

	return cost;
}

float get_cost(s_routing_node *current, s_routing_node *neighbour)
{
	float cost;
	s_wire *wire;

	switch (current->type) {
	case IPIN:
		assert(0); /* we would have finished dijkstra by this time */
		break;
	case WIRE:
		assert(neighbour->type == WIRE || neighbour->type == IPIN);
		cost = abs(current->x - neighbour->x) + abs(current->y - neighbour->y); /* manhattan distance */
		wire = current;
		assert(cost == abs(wire->type->relative_x) + abs(wire->type->relative_y));
		break;
	case OPIN:
		assert(neighbour->type == WIRE);
		cost = 0;
		break;
	default:
		assert(0); /* unknown routing node type */
		break;
	}
	return cost;
}

void print_wire(s_wire *wire, float cost, float expected_cost)
{
	 printf("WIRE id=%d cost=%2f e_cost=%2f [%d,%d] -> [%d,%d]", wire->base.id, cost, expected_cost, wire->base.x, wire->base.y, wire->base.x+wire->type->relative_x, wire->base.y+wire->type->relative_y);
}

void route_net(s_net *net, int num_routing_nodes)
{
	s_heap heap;
	s_routing_node *current, *neighbour, *sink;
	s_route_details *route_details;
	GSList *sink_list_item, *children_list_item;
	s_routing_node *prev_node;
	s_routing_node *node;
	s_wire *wire;
	bool found;
	float cost;
	float expected_cost;
	int i;
	int count;
	GList *route_tree;
	GList *route_tree_item;

	heap_init(&heap);

	route_details = calloc(num_routing_nodes, sizeof(s_route_details));
	for (i = 0; i < num_routing_nodes; i++) {
		route_details[i].min_cost = FLT_MAX;
		route_details[i].expected_cost = FLT_MAX;
		route_details[i].visited = false;
	}

	printf("source: x=%d y=%d type=%d\n", net->source_pin->base.x, net->source_pin->base.y, net->source_pin->base.type);
	route_details[net->source_pin->base.id].min_cost = 0;

	route_tree = g_list_prepend(NULL, net->source_pin);

	count = 0;
	sink_list_item = net->sink_pins;
	while (sink_list_item) {
		sink = sink_list_item->data;
		printf("sink: id=%d x=%d y=%d type=%d\n", sink->id, sink->x, sink->y, sink->type);

		route_tree_item = route_tree;
		while (route_tree_item) {
			node = route_tree_item->data;
			route_details[node->id].expected_cost = get_expected_cost(node, sink);
			heap_push(&heap, route_details[node->id].expected_cost, node);
			route_tree_item = route_tree_item->next;
		}

		found = false;
		while (!heap_is_empty(&heap) && !found) {
			count++;

			current = heap_pop(&heap);
			route_details[current->id].visited = true;

			/* DEBUG */
#ifdef VERBOSE
			if (current->type == WIRE) { printf("current: "); print_wire(current, route_details[current->id].min_cost, route_details[current->id].expected_cost); printf("\n"); }
			else if (current->type == IPIN) { printf("current: IPIN id=%d [%d,%d]\n", current->id, current->x, current->y); }
			else if (current->type == OPIN) { printf("current: OPIN id=%d [%d,%d] e_cost=%2f\n", current->id, current->x, current->y, route_details[current->id].expected_cost); }
#endif
			if (current == sink) {
				found = true;

				/* DEBUG */
				printf("trace: sink id=%d x=%d y=%d type=%d\n", current->id, current->x, current->y, current->type);

				route_tree = g_list_prepend(route_tree, current);
				prev_node = route_details[current->id].prev_node;
				while (prev_node) {
					route_tree = g_list_prepend(route_tree, prev_node);
					if (prev_node->type == WIRE) {
						printf("trace: "); print_wire(prev_node, route_details[prev_node->id].min_cost, route_details[prev_node->id].expected_cost); printf("\n");
					} else {
						printf("trace: source id=%d [%d,%d]\n", prev_node->id, prev_node->x, prev_node->y);
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
					expected_cost = get_expected_cost(neighbour, sink);

					route_details[neighbour->id].expected_cost = expected_cost;

					if (cost < route_details[neighbour->id].min_cost) {
						route_details[neighbour->id].min_cost = cost;
						route_details[neighbour->id].prev_node = current;
					}

					/* DEBUG */
#ifdef VERBOSE
					if (neighbour->type == WIRE) { printf("\t neighbour: "); print_wire(neighbour, cost, expected_cost); printf("\n"); }
					else if (neighbour->type == IPIN) { printf("\t neighbour: IPIN id=%d [%d,%d] cost=%2f e_cost=%2f\n", neighbour->id, neighbour->x, neighbour->y, cost, expected_cost); }
#endif

					if (!route_details[neighbour->id].visited) {
						heap_push(&heap, expected_cost, neighbour);
					}

					children_list_item = children_list_item->next;
				}
			}
		}
		assert(found);

		for (i = 0; i < num_routing_nodes; i++) {
			route_details[i].visited = false;
		}

		sink_list_item = sink_list_item->next;
	}

	printf("\n");

	free(route_details);
}

//void route_net(s_net *net, int num_routing_nodes)
//{
//	s_heap heap;
//	s_routing_node *current, *neighbour, *sink;
//	s_route_details *route_details;
//	GSList *sink_list_item, *children_list_item;
//	s_routing_node *prev_node;
//	s_wire *wire;
//	bool found;
//	float cost;
//	float predicted_cost;
//	int i;
//	GHashTable *sinks_to_reach;
//	int count;
//
//	route_details = calloc(num_routing_nodes, sizeof(s_route_details));
//	for (i = 0; i < num_routing_nodes; i++) {
//		route_details[i].min_cost = FLT_MAX;
//		route_details[i].visited = false;
//	}
//
//	sinks_to_reach = g_hash_table_new(g_direct_hash, g_direct_equal);
//
//	heap_init(&heap);
//	heap_push(&heap, 0, net->source_pin);
//	route_details[net->source_pin->base.id].min_cost = 0;
//	printf("source: x=%d y=%d type=%d\n", net->source_pin->base.x, net->source_pin->base.y, net->source_pin->base.type);
//
//	sink_list_item = net->sink_pins;
//	while (sink_list_item) {
//		sink = sink_list_item->data;
//		printf("sink: id=%d x=%d y=%d type=%d\n", sink->id, sink->x, sink->y, sink->type);
//		g_hash_table_add(sinks_to_reach, sink);
//		sink_list_item = sink_list_item->next;
//	}
//
//	count = 0;
//	while (!heap_is_empty(&heap) && g_hash_table_size(sinks_to_reach) > 0) {
//		count++;
//		//sink = sink_list_item->data;
//		//printf("sink: id=%d x=%d y=%d type=%d\n", sink->id, sink->x, sink->y, sink->type);
//
//		//found = false;
//		//while () {
//			current = heap_pop(&heap);
//			route_details[current->id].visited = true;
//
//			if (current->type == WIRE) { printf("current: "); print_wire(current, route_details[current->id].min_cost); printf("\n"); }
//			else if (current->type == IPIN) { printf("current: IPIN id=%d [%d,%d]\n", current->id, current->x, current->y); }
//
//			if (g_hash_table_contains(sinks_to_reach, current)) {
//				//found = true;
//
//				g_hash_table_remove(sinks_to_reach, current);
//				printf("trace: sink id=%d x=%d y=%d type=%d\n", current->id, current->x, current->y, current->type);
//				prev_node = route_details[current->id].prev_node;
//				while (prev_node) {
//					if (prev_node->type == WIRE) {
//						printf("trace: "); print_wire(prev_node, route_details[prev_node->id].min_cost); printf("\n");
//					} else {
//						printf("trace: source id=%d [%d,%d]\n", prev_node->id, prev_node->x, prev_node->y);
//					}
//					prev_node = route_details[prev_node->id].prev_node;
//				}
//			} else {
//				/* for all neighbour */
//				children_list_item = current->children;
//				while (children_list_item) {
//					neighbour = children_list_item->data;
//					assert(neighbour);
//
//					cost = route_details[current->id].min_cost + get_cost(current, neighbour);
//					predicted_cost = cost + get_expected_cost(neighbour, net->sink_pins);
//					if (cost < route_details[neighbour->id].min_cost) {
//						route_details[neighbour->id].min_cost = cost;
//						route_details[neighbour->id].prev_node = current;
//					}
//					//if (!route_details[neighbour->id].visited) {
//					if (neighbour->type == WIRE) { printf("\t neighbour: "); print_wire(neighbour, predicted_cost); printf("\n"); }
//					else if (neighbour->type == IPIN) { printf("\t neighbour: IPIN id=%d [%d,%d] cost=%f\n", neighbour->id, neighbour->x, neighbour->y, predicted_cost); }
//					if (!route_details[neighbour->id].visited) {
//						heap_push(&heap, predicted_cost, neighbour);
//					}
//					//}
//
//					children_list_item = children_list_item->next;
//				}
//			}
//		//}
//		//assert(found);
//		//sink_list_item = sink_list_item->next;
//	}
//
//	free(route_details);
//}
