/*
 * route.h
 *
 *  Created on: Oct 12, 2013
 *      Author: chinhau5
 */

#ifndef ROUTE_H_
#define ROUTE_H_

void route_net(s_net *net, int num_routing_nodes, int *node_usage, int *first_level_node_usage, GList **node_requests, s_net **grant, bool enhanced);
void reserve_route_resource(GList **node_requests, int *node_usage, int num_routing_nodes, GHashTable *id_to_node, s_net **grant, float total_area);
void init_net_bounding_box(s_net *net);

#endif /* ROUTE_H_ */
