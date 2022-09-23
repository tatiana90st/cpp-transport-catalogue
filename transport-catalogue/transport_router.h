#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include <unordered_map>
#include <string_view>
#include <string>
#include <vector>
#include <optional>

struct RouterSettings {
	double bus_wait_time_ = 0.0;
	double bus_velocity_ = 0.0;
};

struct Item {
	std::string type;
	std::string_view name;
	double time = 0.;
	int span_count = 0;//для типа Wait не заполняем
};

class TransportRouter {
public:
	TransportRouter(tr_cat::TransportCatalogue& catalogue, RouterSettings rout_set);

	std::optional<std::vector<Item>> FindRoute(std::string_view stop1, std::string_view stop2);

private:
	tr_cat::TransportCatalogue& catalogue_;
	RouterSettings rout_set_;
	graph::DirectedWeightedGraph<double>* graph_ptr_ = nullptr;
	graph::Router<double>* router_ptr_ = nullptr;

	//common case: even(.first) = from, uneven(.second) = to; wait edges: even = to, uneven = from
	std::unordered_map<std::string_view, std::pair<size_t, size_t>> stops_to_vertexes_;
	std::unordered_map<size_t, Item> edges_index_;

	void AddWaitEdges();
	void ProcessRoute(std::vector<Stop*>& route, std::string_view bus_name);
	void AddRouteEdges();
	void BuildGraph(size_t size);

};