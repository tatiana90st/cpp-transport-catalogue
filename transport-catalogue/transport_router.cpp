#include "transport_router.h"

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include <transport_router.pb.h>
#include <unordered_map>
#include <string_view>
#include <string>
#include <vector>
#include <optional>

using namespace std::string_literals;

//phase process_requests
TransportRouter::TransportRouter(tr_cat::TransportCatalogue& catalogue, RouterSettings rout_set, RequestHandler& rh, 
	graph::DirectedWeightedGraph<double>* graph_ptr, graph::Router<double>* router_ptr)
	: catalogue_(catalogue)
	, rout_set_(rout_set)
	, rh_(rh )
	, graph_ptr_(graph_ptr)
	, router_ptr_(router_ptr)

{
	BuildRouter();
}

//phase make_base
TransportRouter::TransportRouter(tr_cat::TransportCatalogue& catalogue, RouterSettings rout_set, RequestHandler& rh)
	: catalogue_(catalogue)
	, rout_set_(rout_set)
	, rh_(rh) {

	BuildGraph(catalogue_.CountStops() * 2);
}

std::optional<std::vector<Item>> TransportRouter::FindRoute(std::string_view stop1, std::string_view stop2) {
	if (!stops_to_vertexes_.count(stop1) || !stops_to_vertexes_.count(stop2)) {
		return std::nullopt;
	}
	std::optional<graph::Router<double>::RouteInfo> best_route =
		router_ptr_->BuildRoute(stops_to_vertexes_.at(stop1).second, stops_to_vertexes_.at(stop2).second);
	if (!best_route) {
		return std::nullopt;
	}
	std::vector<Item> result;
	for (const auto edge_id : best_route.value().edges) {
		result.push_back(edges_index_.at(edge_id));
	}
	return result;
}

tc_serialize::Graph TransportRouter::GetSerializedGraph() const {
	return graph_ptr_->SerializeGraph();
}

tc_serialize::Router TransportRouter::GetSerializedRouter() const {
	return router_ptr_->SerializeRouter();
}

//phase make_base
void TransportRouter::AddWaitEdges() {
	const std::map <std::string_view, Stop*> stops = rh_.GetAllStopsWithBusesAndSorted();
	graph::VertexId from = 1;
	graph::VertexId to = 0;
	for (const auto& [name, ptr] : stops) {
		graph::Edge<double> edge{ from, to, rout_set_.bus_wait_time_ };
		stops_to_vertexes_.insert({ name, {to, from} });
		size_t index = graph_ptr_->AddEdge(edge);
		from += 2;
		to += 2;
	}
}

//phase make_base
void TransportRouter::ProcessRoute(std::vector<Stop*>& route, std::string_view bus_name) {
	for (int i = 0; i + 1 < route.size(); ++i) {
		graph::VertexId from = stops_to_vertexes_.at(route[i]->name).first;
		Stop* prev_stop = route[i];
		double adding_time = 0.;
		for (int j = i + 1; j < route.size(); ++j) {
			graph::VertexId to = stops_to_vertexes_.at(route[j]->name).second;
			adding_time += (catalogue_.GetDistanceBtwStops(prev_stop, route[j]) * 60) / (rout_set_.bus_velocity_ * 1000);
			graph::Edge<double> edge{ from, to, adding_time };
			size_t index = graph_ptr_->AddEdge(edge);
			prev_stop = route[j];
		}
	}
}

//phase make_base
void TransportRouter::AddRouteEdges() {
	const std::map<std::string_view, Bus*> buses = rh_.GetAllBusesWithRoutesAndSorted();
	for (const auto& [name, ptr] : buses) {
		if (ptr->is_round) {
			ProcessRoute(ptr->route, name);
		}
		else {
			ProcessRoute(ptr->half_route, name);
			std::vector<Stop*> second_half = { ptr->half_route.rbegin(), ptr->half_route.rend() };
			ProcessRoute(second_half, name);
		}
	}
}

void TransportRouter::BuildGraph(size_t size) {
	graph_ptr_ = new graph::DirectedWeightedGraph<double>(size);
	AddWaitEdges();
	AddRouteEdges();
	router_ptr_ = new graph::Router<double>(*graph_ptr_);
}

void TransportRouter::BuildRouter() {
	AddWaitEdgesPhase2();
	AddRouteEdgesPhase2();
	
}

//phase process_requests
void TransportRouter::AddWaitEdgesPhase2() {
	const std::map <std::string_view, Stop*> stops = rh_.GetAllStopsWithBusesAndSorted();
	graph::VertexId from = 1;
	graph::VertexId to = 0;
	for (const auto& [name, ptr] : stops) {
		stops_to_vertexes_.insert({ name, {to, from} });
		Item item{ "Wait"s, name, rout_set_.bus_wait_time_ , 0 };
		edges_index_[des_index_++] = std::move(item);
		from += 2;
		to += 2;
	}
}

//phase process_requests
void TransportRouter::AddRouteEdgesPhase2() {
	const std::map<std::string_view, Bus*> buses = rh_.GetAllBusesWithRoutesAndSorted();
	for (const auto& [name, ptr] : buses) {
		if (ptr->is_round) {
			ProcessRoutePhase2(ptr->route, name);
		}
		else {
			ProcessRoutePhase2(ptr->half_route, name);
			std::vector<Stop*> second_half = { ptr->half_route.rbegin(), ptr->half_route.rend() };
			ProcessRoutePhase2(second_half, name);
		}
	}
}

const int M_PER_KM = 1000;
const int MIN_PER_HOUR = 60;

//phase process_requests
void TransportRouter::ProcessRoutePhase2(std::vector<Stop*>& route, std::string_view bus_name) {
	for (int i = 0; i + 1 < route.size(); ++i) {
		Stop* prev_stop = route[i];
		double adding_time = 0.;
		for (int j = i + 1; j < route.size(); ++j) {
			adding_time += (catalogue_.GetDistanceBtwStops(prev_stop, route[j]) * MIN_PER_HOUR) / (rout_set_.bus_velocity_ * M_PER_KM);
			Item item{ "Bus"s, bus_name, adding_time, j - i };
			edges_index_[des_index_++] = std::move(item);
			prev_stop = route[j];
		}
	}
}

tc_serialize::TransportRouter TransportRouter::GetSerializedTransportRouter() const {
	tc_serialize::TransportRouter tr_router;

	*tr_router.mutable_rout_set() = SerializeSettings();

	return tr_router;
}

tc_serialize::RouterSettings TransportRouter::SerializeSettings() const {
	tc_serialize::RouterSettings settings;
	settings.set_bus_velocity(rout_set_.bus_velocity_);
	settings.set_bus_wait_time(rout_set_.bus_wait_time_);
	return settings;
}
