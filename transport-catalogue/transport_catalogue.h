#pragma once
#include "geo.h"
#include "domain.h"
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <deque>
#include <optional>
#include <vector>

namespace tr_cat {

namespace detail {
struct PairPtrHasher {
	size_t operator()(const std::pair<Stop*, Stop*> p) const;
private:
	std::hash<const void*> hasher_;
};
}//namespace detail

class TransportCatalogue {
public:
	void AddStop(const Stop&& stop);

	void AddBus(const Bus&& bus);

	std::optional<Bus*> FindBus(const std::string_view name) const;

	std::optional<Stop*> FindStop(const std::string_view name) const;

	const std::unordered_set<Bus*> GetBusesForStop(Stop* stop) const;

	void AddDistances(std::pair<std::pair<Stop*, Stop*>, int>&& p);

	int GetDistanceBtwStops(Stop* stop1, Stop* stop2) const;

	const std::unordered_map<std::string_view, Bus*> GetAllBuses() const;

	const std::unordered_map<Stop*, std::unordered_set<Bus*>> GetStopsWithBuses() const;

	const std::unordered_map<std::string_view, Stop*>& GetStopsIndex() const;

	int CountStops() const;

private:
	std::deque<Stop> stops_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, Bus*> buses_index_;
	std::unordered_map<std::string_view, Stop*> stops_index_;
	std::unordered_map<Stop*, std::unordered_set<Bus*>> stops_with_buses_;
	std::unordered_map<std::pair<Stop*, Stop*>, int, detail::PairPtrHasher> distances_;
};

}//namespace tr_cat