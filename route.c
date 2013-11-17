#include <stdio.h>
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
	 printf("WIRE id=%d cost=%f e_cost=%f [%d,%d] -> [%d,%d]", wire->base.id, cost, expected_cost, wire->base.x, wire->base.y, wire->base.x+wire->type->relative_x, wire->base.y+wire->type->relative_y);
}

//#define VERBOSE

void route_net(s_net *net, int num_routing_nodes, int *node_usage, int *first_level_node_usage, GList **node_requests, s_net **grant, bool enhanced)
{
	s_heap heap;
	s_routing_node *current, *neighbour, *sink;
	s_route_details *route_details;
	GSList *sink_list_item, *children_list_item;
	s_routing_node *node;
	bool found;
	float cost;
	float expected_cost;
	int i;
	int count;
	GHashTable *route_tree;
	GHashTableIter route_tree_iter;
	int node_id;
	s_node_requester *node_requester;

	heap_init(&heap);

	route_details = calloc(num_routing_nodes, sizeof(s_route_details));
	for (i = 0; i < num_routing_nodes; i++) {
		route_details[i].min_cost = FLT_MAX;
		route_details[i].expected_cost = FLT_MAX;
		route_details[i].visited = false;
	}

	//printf("source: x=%d y=%d type=%d\n", net->source_pin->base.x, net->source_pin->base.y, net->source_pin->base.type);
	route_details[net->source_pin->base.id].min_cost = 0;

	route_tree = g_hash_table_new(g_direct_hash, g_direct_equal);
	g_hash_table_insert(route_tree, net->source_pin->base.id, net->source_pin);

	count = 0;
	sink_list_item = net->sink_pins;
	while (sink_list_item) {
		sink = sink_list_item->data;
		//printf("sink: id=%d x=%d y=%d type=%d\n", sink->id, sink->x, sink->y, sink->type);

		g_hash_table_iter_init(&route_tree_iter, route_tree);
		while (g_hash_table_iter_next (&route_tree_iter, &node_id, &node)) {
			assert(node_id == node->id);
			/* important to allow nodes in the current route tree to be reused */
			route_details[node->id].expected_cost = /*route_details[node->id].min_cost + */get_expected_cost(node, sink);
#ifdef VERBOSE
			if (node->type == WIRE) { printf("rt: "); print_wire(node, route_details[node->id].min_cost, route_details[node->id].expected_cost); printf("\n"); }
			else if (node->type == IPIN) { printf("rt: IPIN id=%d [%d,%d]\n", node->id, node->x, node->y); }
			else if (node->type == OPIN) { printf("rt: OPIN id=%d [%d,%d] cost=%2f e_cost=%2f\n", node->id, node->x, node->y, route_details[node->id].min_cost, route_details[node->id].expected_cost); }
#endif
			heap_push(&heap, route_details[node->id].expected_cost, node);
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
			else if (current->type == OPIN) { printf("current: OPIN id=%d [%d,%d] cost=%f e_cost=%f\n", current->id, current->x, current->y, route_details[current->id].min_cost, route_details[current->id].expected_cost); }
#endif
			if (current == sink) {
				found = true;

				node = current;
				while (node) {
					if (!g_hash_table_contains(route_tree, node->id)) {
						g_hash_table_insert(route_tree, node->id, node);
					}
#if 0
					if (node->type == WIRE) {
						printf("trace: "); print_wire(node, route_details[node->id].min_cost, route_details[node->id].expected_cost); printf("\n");
					} else if (node->type == IPIN) {
						printf("trace: sink id=%d cost: %f e_cost: %f [%d,%d]\n", node->id, route_details[node->id].min_cost, route_details[node->id].expected_cost, node->x, node->y);
					} else {
						assert(node->type == OPIN);
						printf("trace: source id=%d cost: %f e_cost: %f [%d,%d]\n", node->id, route_details[node->id].min_cost, route_details[node->id].expected_cost, node->x, node->y);
					}
#endif
					node = route_details[node->id].prev_node;
				}
			} else {
				/* for all neighbour */
				children_list_item = current->children;
				while (children_list_item) {
					neighbour = children_list_item->data;
					assert(neighbour);

					cost = route_details[current->id].min_cost + get_cost(current, neighbour);
					expected_cost = cost + get_expected_cost(neighbour, sink);

#ifdef VERBOSE
					if (neighbour->type == WIRE) { printf("\t neighbour: "); print_wire(neighbour, cost, expected_cost); printf(" "); }
					else if (neighbour->type == IPIN) { printf("\t neighbour: IPIN id=%d [%d,%d] cost=%f e_cost=%f ", neighbour->id, neighbour->x, neighbour->y, cost, expected_cost); }
					printf("min_cost: %f ", route_details[neighbour->id].min_cost);
#endif

					if (cost < route_details[neighbour->id].min_cost && !route_details[neighbour->id].visited  && (!enhanced || (current != net->source_pin || first_level_node_usage[neighbour->id] == 0))
						/*(!grant[neighbour->id] || grant[neighbour->id] == net)*/	/*&& node_usage[neighbour->id] == 0*/) {
						route_details[neighbour->id].min_cost = cost;
						route_details[neighbour->id].expected_cost = expected_cost;
						route_details[neighbour->id].prev_node = current;
						heap_push(&heap, expected_cost, neighbour);
#ifdef VERBOSE
						printf("ADDED");
#endif
					}
#ifdef VERBOSE
					printf("\n");
#endif

					children_list_item = children_list_item->next;
				}
			}
		}
		assert(found);

		for (i = 0; i < num_routing_nodes; i++) {
			route_details[i].visited = false;
			if (!g_hash_table_contains(route_tree, i)) {
				route_details[i].min_cost = FLT_MAX;
				route_details[i].expected_cost = FLT_MAX;
			}
		}

		heap_clear(&heap);

		sink_list_item = sink_list_item->next;
	}

	//printf("\n");
//#define PRINT_ROUTE_TREE
	g_hash_table_iter_init(&route_tree_iter, route_tree);
	while (g_hash_table_iter_next (&route_tree_iter, &node_id, &node)) {
		assert(node_id == node->id);
		/* important to allow nodes in the current route tree to be reused */
#ifdef PRINT_ROUTE_TREE
		if (node->type == WIRE) { printf("rt: "); print_wire(node, route_details[node->id].min_cost, route_details[node->id].expected_cost); printf(" "); }
		else if (node->type == IPIN) { printf("rt: IPIN id=%d [%d,%d] ", node->id, node->x, node->y); }
		else if (node->type == OPIN) { printf("rt: OPIN id=%d [%d,%d] cost=%f e_cost=%f ", node->id, node->x, node->y, route_details[node->id].min_cost, route_details[node->id].expected_cost); }

		if (route_details[node->id].prev_node) {
			printf("prev_node id=%d", route_details[node->id].prev_node->id);
		} else {
			assert(node->type == OPIN);
		}
		printf("\n");
#endif
		if (route_details[node->id].prev_node == net->source_pin) {
			first_level_node_usage[node->id]++;
		}

		node_usage[node->id]++;
		node_requester = malloc(sizeof(s_node_requester));
		node_requester->net = net;
		node_requester->node = route_details[node->id].prev_node;

		node_requests[node->id] = g_list_prepend(node_requests[node->id], node_requester);
	}

	heap_free(&heap);
	free(route_details);
	g_hash_table_destroy(route_tree);
}

float get_num_alternative_routes(s_routing_node *node, s_routing_node *requested_node, int *node_usage)
{
	GSList *child_item;
	s_routing_node *child_node;
	int num_alternative_routes;
	int num_children;

	num_alternative_routes = 0;
	num_children = 0;
	child_item = node->children;
	while (child_item) {
		child_node = child_item->data;
		if (child_node != requested_node && node_usage[child_node->id] == 0) {
			num_alternative_routes++;
		}
		num_children++;
		child_item = child_item->next;
	}

	return (float)num_alternative_routes/num_children;
}

void init_net_bounding_box(s_net *net)
{
	int left, bottom, right, top;
	GSList *sink_item;
	s_pb_graph_pin *sink;

	right = top = 0;
	left = bottom = INT_MAX;

	if (net->source_pin->base.x < left) {
		left = net->source_pin->base.x;
	}
	if (net->source_pin->base.x > right) {
		right = net->source_pin->base.x;
	}
	if (net->source_pin->base.y < bottom) {
		bottom = net->source_pin->base.y;
	}
	if (net->source_pin->base.y > top) {
		top = net->source_pin->base.y;
	}

	sink_item = net->sink_pins;
	while (sink_item) {
		sink = sink_item->data;
		if (sink->base.x < left) {
			left = sink->base.x;
		}
		if (sink->base.x > right) {
			right = sink->base.x;
		}
		if (sink->base.y < bottom) {
			bottom = sink->base.y;
		}
		if (sink->base.y > top) {
			top = sink->base.y;
		}
		sink_item = sink_item->next;
	}

	net->bounding_box.left = left;
	net->bounding_box.right = right;
	net->bounding_box.top = top;
	net->bounding_box.bottom = bottom;
	assert(right >= left && top >= bottom);
	net->bounding_box.area = abs(left-right) * abs(bottom-top);
}

void reserve_route_resource(GList **node_requests, int *node_usage, int num_routing_nodes, GHashTable *id_to_node, s_net **grant, float total_area)
{
	int i;
	s_heap heap;
	GList *node_requests_item;
	s_node_requester *requester;
	s_routing_node *requested_node;
	float num_alternative_routes;
	float cost;
	s_node_requester *granted;

	heap_init(&heap);
	for (i = 0; i < num_routing_nodes; i++) {
		if (node_requests[i] && node_usage[i] > 1) {
			assert(g_list_length(node_requests[i]) == node_usage[i]);
			assert(g_hash_table_contains(id_to_node, i));
			requested_node = g_hash_table_lookup(id_to_node, i);
			assert(requested_node->id == i);
			assert(requested_node->type == WIRE);
			printf("overused: id=%d [%d,%d]\n", requested_node->id, requested_node->x, requested_node->y);

			node_requests_item = node_requests[i];
			while (node_requests_item) {
				requester = node_requests_item->data;

				if (requester->node->type == WIRE) {
					printf("requester: WIRE id=%d type=%d [%d,%d]->[%d,%d]\n", requester->node->id, requester->node->type, requester->node->x, requester->node->y, requested_node->x, requested_node->y);
				} else {
					assert(requester->node->type == OPIN);
					printf("requester: OPIN id=%d type=%d [%d,%d]\n", requester->node->id, requester->node->type, requester->node->x, requester->node->y);
				}

				num_alternative_routes = get_num_alternative_routes(requester->node, requested_node, node_usage);

				/* higher area + small num_alternative_routes = smaller cost */
				cost = /*num_alternative_routes +*/ 1.0/(requester->net->bounding_box.area/total_area);
				heap_push(&heap, cost, requester);

				node_requests_item = node_requests_item->next;
			}

			if (!grant[i]) {
				granted = heap_pop(&heap);
				heap_clear(&heap);
				grant[i] = granted->net;
			}
			printf("\n");
		}
	}
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
