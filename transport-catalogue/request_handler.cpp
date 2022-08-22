#include "request_handler.h"


RequestHandler::RequestHandler(const tr_cat::TransportCatalogue& db) 
	:db_(db)
{
}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
	std::optional<tr_cat::Bus*> ref = db_.FindBus(bus_name);
	if (ref) {
		BusStat result;
		double geo_l = 0.0;
		std::vector<tr_cat::Stop*> route = ref.value()->route;
		result.stop_count = route.size();
		for (size_t i = 0; i + 1 < route.size(); ++i) {
			geo::Coordinates from, to;
			from = route[i]->place;
			to = route[i + 1]->place;
			geo_l += ComputeDistance(from, to);
			result.route_length += db_.GetDistanceBtwStops(route[i], route[i + 1]);
		}
		result.curvature = result.route_length / geo_l;
		result.unique_stop_count = ref.value()->unique_stops.size();
		return result;
	}
	else {
		return std::nullopt;
	}
}

// Возвращает маршруты, проходящие через
//const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;
const std::unordered_set<tr_cat::Bus*> RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
	std::optional<tr_cat::Stop*> ref = db_.FindStop(stop_name);
	return db_.GetBusesForStop(ref.value());
}
/*
svg::Document RequestHandler::RenderMap() const {

}
*/
const std::map<std::string_view, tr_cat::Bus*> RequestHandler::GetAllBusesWithRoutesAndSorted() const {
	std::unordered_map<std::string_view, tr_cat::Bus*> unsorted = db_.GetAllBuses();
	std::map<std::string_view, tr_cat::Bus*> sorted_not_empty;
	for (const auto& [name, bus_ptr] : unsorted) {
		if (!bus_ptr->route.empty()) {
			sorted_not_empty.insert({ name, bus_ptr });
		}
	}
	return sorted_not_empty;
}

const std::map<std::string_view, tr_cat::Stop*> RequestHandler::GetAllStopsWithBusesAndSorted() const {
	const std::unordered_map<tr_cat::Stop*, std::unordered_set<tr_cat::Bus*>> unsorted = db_.GetStopsWithBuses();
	std::map<std::string_view, tr_cat::Stop*> sorted_not_empty;
	for (const auto& [stop_ptr, bus_list] : unsorted) {
		if (!bus_list.empty()) {
			sorted_not_empty.insert({ stop_ptr->name, stop_ptr });
		}
	}
	return sorted_not_empty;
}