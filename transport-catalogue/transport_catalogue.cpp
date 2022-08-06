#include "geo.h"
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <deque>
#include <optional>

#include "transport_catalogue.h"
namespace tr_cat {

namespace detail {
size_t PairPtrHasher::operator()(const std::pair<Stop*, Stop*> p) const {
	size_t h1 = hasher_(p.first);
	size_t h2 = hasher_(p.second);
	return h1 + 37 * h2;
}
}//namespace detail

void TransportCatalogue::AddStop(const Stop&& stop) {
	stops_.emplace_back(stop);
	stops_index_[stops_.back().name] = &stops_.back();
	stops_with_buses_[&stops_.back()];
}

void TransportCatalogue::AddBus(const Bus&& bus) {
	buses_.emplace_back(bus);
	buses_index_[buses_.back().name] = &buses_.back();
	for (const auto stop : bus.unique_stops) {
		stops_with_buses_[stop].insert(&buses_.back());
	}
}

std::optional<Bus*> TransportCatalogue::FindBus(const std::string_view name) const {
	if (!buses_index_.count(name)) {
		return std::nullopt;
	}
	else {
		return buses_index_.at(name);
	}
}

std::optional<Stop*> TransportCatalogue::FindStop(const std::string_view name) const {
	if (!stops_index_.count(name)) {
		return std::nullopt;
	}
	else {
		return stops_index_.at(name);
	}
}

const std::unordered_set<Bus*> TransportCatalogue::GetBusesForStop(Stop* stop) const {
	return stops_with_buses_.at(stop);
}

void TransportCatalogue::AddDistances(std::pair<std::pair<Stop*, Stop*>, int>&& p) {
	distances_[p.first] = p.second;
}

int TransportCatalogue::GetDistanceBtwStops(Stop* stop1, Stop* stop2) const {
	std::pair<Stop*, Stop*> p = std::make_pair(stop1, stop2);
	if (distances_.count(p)) {
		return distances_.at(p);
	}
	else {
		p = std::make_pair(stop2, stop1);
		return distances_.at(p);
	}
}
}