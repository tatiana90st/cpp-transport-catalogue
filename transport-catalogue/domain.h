#pragma once
#include "geo.h"
#include <string>
#include <vector>
#include <unordered_set>


struct Stop {
	std::string name;
	geo::Coordinates place{ 0.0, 0.0 };
};

struct Bus {
	std::string name;
	std::vector<Stop*> route;
	std::unordered_set<Stop*> unique_stops;
	bool is_round = false;
};


struct BusStat {
	double curvature = 0.0;
	int route_length = 0;
	int stop_count = 0;
	int unique_stop_count = 0;
};