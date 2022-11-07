#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include "request_handler.h"
#include <graph.pb.h>
#include <transport_router.pb.h>
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

	TransportRouter(tr_cat::TransportCatalogue& catalogue, RouterSettings rout_set, RequestHandler& rh);

	TransportRouter(tr_cat::TransportCatalogue& catalogue, RouterSettings rout_set, RequestHandler& rh,
		graph::DirectedWeightedGraph<double>* graph_ptr, graph::Router<double>* router_ptr);

	std::optional<std::vector<Item>> FindRoute(std::string_view stop1, std::string_view stop2);

	tc_serialize::Graph GetSerializedGraph() const;
	tc_serialize::Router GetSerializedRouter() const;
	tc_serialize::TransportRouter GetSerializedTransportRouter() const;

private:
	tr_cat::TransportCatalogue& catalogue_;
	RouterSettings rout_set_;
	RequestHandler& rh_;
	graph::DirectedWeightedGraph<double>* graph_ptr_ = nullptr;
	graph::Router<double>* router_ptr_ = nullptr;

	size_t des_index_ = 0;

	//common case: even(.first) = from, odd(.second) = to; wait edges: even = to, odd = from
	std::unordered_map<std::string_view, std::pair<size_t, size_t>> stops_to_vertexes_;
	std::unordered_map<size_t, Item> edges_index_;

	void AddWaitEdges();
	void ProcessRoute(std::vector<Stop*>& route, std::string_view bus_name);
	void AddRouteEdges();
	void BuildGraph(size_t size);
	
	void BuildRouter();
	void AddWaitEdgesPhase2();
	void AddRouteEdgesPhase2();
	void ProcessRoutePhase2(std::vector<Stop*>& route, std::string_view bus_name);

	tc_serialize::RouterSettings SerializeSettings() const;

};

