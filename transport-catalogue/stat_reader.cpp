#include <iostream>
#include <iomanip>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <string_view>
#include "transport_catalogue.h"
#include "input_reader.h"
#include "geo.h"

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;

namespace tr_cat {

namespace out_query {

namespace detail {

void PrintBusRequestResult(const std::string_view& bus, TransportCatalogue& cat) {
	std::optional<const Bus*> ref = cat.FindBus(bus);
	if (ref) {
		double geo_l = 0.0;
		int r_length = 0;
		std::vector<Stop*> route = ref.value()->route;
		for (size_t i = 0; i + 1 < route.size(); ++i) {
			Coordinates from, to;
			from = route[i]->place;
			to = route[i + 1]->place;
			geo_l += ComputeDistance(from, to);
			r_length += cat.GetDistanceBtwStops(route[i], route[i + 1]);
		}
		double curve = r_length / geo_l;
		std::cout << "Bus "s << ref.value()->name << ": "s << route.size() << " stops on route, "s <<
			ref.value()->unique_stops.size() << " unique stops, "s << std::setprecision(6) << r_length
			<< " route length, "s << curve << " curvature\n"sv;
	}
	else {
		std::cout << "Bus "sv << bus << ": not found\n"sv;
	}
}

void PrintStopRequestResult(const std::string_view& stop, TransportCatalogue& cat) {
	std::optional<Stop*> ref = cat.FindStop(stop);
	if (!ref) {
		std::cout << "Stop "sv << stop << ": not found\n"sv;
		return;
	}
	const std::unordered_set<Bus*> buses = cat.GetBusesForStop(ref.value());
	if (buses.empty()) {
		std::cout << "Stop "sv << stop << ": no buses\n"sv;
		return;
	}
	std::set<std::string_view> sorted_buses;
	for (const auto bus : buses) {
		sorted_buses.insert(bus->name);
	}
	std::cout << "Stop "sv << stop << ": buses"sv;
	for (std::string_view bus : sorted_buses) {
		std::cout << " "sv << bus;
	}
	std::cout << "\n"sv;
}

}//namespace detail

void Process(TransportCatalogue& cat) {
	std::vector<std::string> buffer = query_sort::ReadAndPutIntoBuffer();

	int q_amount = buffer.size();
	for (int i = 0; i < q_amount; ++i) {
		query_sort::Query q = query_sort::Sorter(move(buffer[i]));
		switch (q.type) {
		case query_sort::QueryType::BUS:
			detail::PrintBusRequestResult(q.text, cat);
			std::cout << std::flush;
			break;
		case query_sort::QueryType::STOP:
			detail::PrintStopRequestResult(q.text, cat);
			std::cout << std::flush;
			break;
		case query_sort::QueryType::EMPTY:
			break;
		}
	}
}

}//namespace out_query

}//namespace tr_cat