#include "request_handler.h"


RequestHandler::RequestHandler(const tr_cat::TransportCatalogue& db)
	:db_(db)
{
}

std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
	std::optional<Bus*> ref = db_.FindBus(bus_name);
	if (ref) {
		BusStat result;
		double geo_l = 0.0;
		std::vector<Stop*> route = ref.value()->route;
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

const std::set<Bus*> RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
	std::optional<Stop*> ref = db_.FindStop(stop_name);
	if (ref) {
		return db_.GetBusesForStop(ref.value());
	}
	else {
		std::set<Bus*> empty;
		return empty;
	}
}

const std::map<std::string_view, Bus*> RequestHandler::GetAllBusesWithRoutesAndSorted() const {
	const std::map<std::string_view, Bus*>& sorted = db_.GetAllBuses();
	std::map<std::string_view, Bus*> sorted_not_empty;
	for (auto it = sorted.begin(); it != sorted.end(); ++it) {
		if (!it->second->route.empty()) {
			sorted_not_empty.insert(*it);
		}
	}
	return sorted_not_empty;
}

const std::map<std::string_view, Stop*> RequestHandler::GetAllStopsWithBusesAndSorted() const {
	const std::map<Stop*, std::set<Bus*>>& sorted = db_.GetStopsWithBuses();
	std::map<std::string_view, Stop*> sorted_not_empty;
	for (auto it = sorted.begin(); it != sorted.end(); ++it) {
		if (!it->second.empty()) {
			std::string_view name = it->first->name;
			Stop* st = it->first;
			sorted_not_empty.insert({ name, st });
		}
	}
	return sorted_not_empty;
}

const std::map<std::string_view, Stop*>& RequestHandler::GetStopsIndex() const {
	return db_.GetStopsIndex();
}

const std::map<std::string_view, Bus*> RequestHandler::GetAllBuses() const {
	return db_.GetAllBuses();
}

int RequestHandler::GetDistanceBtwStops(std::string_view first_name, std::string_view second_name) const {
	Stop* first_stop = nullptr;
	Stop* second_stop = nullptr;
	std::optional<Stop*> first = db_.FindStop(first_name);
	if (first) {
		first_stop = first.value();
	}
	std::optional<Stop*> second = db_.FindStop(second_name);
	if (second) {
		second_stop = second.value();
	}
	return db_.GetDistanceBtwStops(first_stop, second_stop);
}

const std::unordered_map<std::pair<Stop*, Stop*>, int, tr_cat::detail::PairPtrHasher>& RequestHandler::GetDistances() const {
	return db_.GetDistances();
}