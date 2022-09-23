#include "transport_router.h"

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include <unordered_map>
#include <string_view>
#include <string>
#include <vector>
#include <optional>

using namespace std::string_literals;

TransportRouter::TransportRouter(tr_cat::TransportCatalogue& catalogue, RouterSettings rout_set)
	:catalogue_(catalogue)
	, rout_set_(rout_set)
{
	BuildGraph(catalogue_.CountStops() * 2);
}

std::optional<std::vector<Item>> TransportRouter::FindRoute(std::string_view stop1, std::string_view stop2) {

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

void TransportRouter::AddWaitEdges() {
	const std::unordered_map<std::string_view, Stop*>& stops = catalogue_.GetStopsIndex();
	graph::VertexId from = 1;
	graph::VertexId to = 0;
	for (const auto& [name, ptr] : stops) {
		graph::Edge<double> edge{ from, to, rout_set_.bus_wait_time_ };
		stops_to_vertexes_.insert({ name, {to, from} });
		Item item{ "Wait"s, name, rout_set_.bus_wait_time_ , 0 };
		size_t index = graph_ptr_->AddEdge(edge);
		edges_index_[index] = std::move(item);
		from += 2;
		to += 2;
	}
}

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
			Item item{ "Bus"s, bus_name, adding_time, j - i };
			edges_index_[index] = std::move(item);
			prev_stop = route[j];
		}
	}
}

void TransportRouter::AddRouteEdges() {
	const std::unordered_map<std::string_view, Bus*>& buses = catalogue_.GetAllBuses();
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

