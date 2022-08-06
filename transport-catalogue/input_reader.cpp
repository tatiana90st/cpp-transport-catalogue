#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "input_reader.h"
#include "transport_catalogue.h"
#include "geo.h"

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;

namespace tr_cat {

namespace query_sort {

Query Sorter(std::string&& query) {
    Query q;
    auto space = query.find(' ');
    if (space == query.npos) {
        return q;
    }
    std::string query_type = query.substr(0, space);
    if (query_type == "Bus"s) {
        q.type = QueryType::BUS;
    }
    if (query_type == "Stop"s) {
        q.type = QueryType::STOP;
    }
    q.text = query.substr(space + 1);
    return q;
}

std::vector<std::string> ReadAndPutIntoBuffer() {
    int q_amount = 0;
    std::string s;
    getline(std::cin, s);
    q_amount = std::stoi(s);
    std::vector<std::string> buffer(q_amount);
    for (int i = 0; i < q_amount; ++i) {
        getline(std::cin, s);
        buffer[i] = std::move(s);
    }
    return buffer;
}
}//namespace query_sort

namespace in_query {

namespace buses {

namespace detail {

std::pair< std::vector<std::string_view>, bool> SplitIntoStops(std::string_view str) {
    std::vector<std::string_view> result;
    bool circle = false;
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    auto space = str.find(" - ");
    if (space == str.npos) {
        circle = true;
    }

    if (circle) {
        while (!str.empty()) {
            auto space = str.find(" > ");
            result.push_back(str.substr(0, space));
            if (space != str.npos) {
                str.remove_prefix(std::min(space + 2, str.size()));
            }
            else {
                str.remove_prefix(str.size());
            }
            str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
        }
    }
    else {
        while (!str.empty()) {
            auto space = str.find(" - ");
            result.push_back(str.substr(0, space));
            if (space != str.npos) {
                str.remove_prefix(std::min(space + 2, str.size()));
            }
            else {
                str.remove_prefix(str.size());
            }
            str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
        }
    }
    return std::make_pair(std::move(result), circle);
}

Bus Parser(std::string& bus, const TransportCatalogue& cat) {
    std::string name;
    std::vector<Stop*> route;
    std::unordered_set<Stop*> unique_stops;
    auto n = bus.find(':');
    name = bus.substr(0, n);
    bus = bus.substr(n + 1);
    std::pair<std::vector<std::string_view>, bool> splitted = SplitIntoStops(bus);
    route.reserve(splitted.first.size());
    for (std::string_view s : splitted.first) {
        std::optional<Stop*> stop = cat.FindStop(s);
        if (!stop) {
            continue;
        }
        unique_stops.insert(stop.value());
        route.push_back(stop.value());
    }
    if (!splitted.second) {
        route.reserve(route.size() * 2);
        for (int i = route.size() - 2; i > -1; --i) {
            route.push_back(route[i]);
        }
    }
    return { std::move(name), std::move(route), std::move(unique_stops) };
}

}//namespace detail

void CreateAndAdd(std::vector<std::string>&& buses, TransportCatalogue& cat) {
for (std::string bus : buses) {
    Bus b = detail::Parser(bus, cat);
    cat.AddBus(std::move(b));
}
}

}//namespace buses

namespace stops {

namespace detail {

std::vector<std::pair<std::string, int>> ExtractDistances(std::string_view s) {
    std::vector<std::pair<std::string, int>> result;
    s.remove_prefix(std::min(s.find_first_not_of(" "), s.size()));
    while (!s.empty()) {
        auto to = s.find(" to ");
        if (to == s.npos) {
            break;
        }
        auto comma = s.find(",");
        if (comma != s.npos) {
            //ooohoooo midnight algebra
            result.push_back(std::make_pair(std::string(s.substr(to + 4, comma - to - 4)), std::stoi(std::string(s.substr(0, to - 1)))));
            s.remove_prefix(std::min(comma + 2, s.size()));
        }
        else {
            result.push_back(std::make_pair(std::string(s.substr(to + 4)), std::stoi(std::string(s.substr(0, to - 1)))));
            s.remove_prefix(s.size());
        }
        s.remove_prefix(std::min(s.find_first_not_of(" "), s.size()));
    }
    return result;
}

//Stop X : latitude, longitude, D1m to stop1, D2m to stop2, ...
std::pair<Stop, std::vector<std::pair<std::string, int>>> Parser(std::string& stop) {
    std::string name;
    Coordinates place;
    auto n = stop.find(':');
    name = stop.substr(0, n);
    auto comma = stop.find(',');
    place.lat = std::stod(stop.substr(n + 2, comma));
    stop = stop.substr(comma + 1);
    std::vector<std::pair<std::string, int>> dist;
    auto comma2 = stop.find(',');
    if (comma2 != stop.npos) {
        place.lng = std::stod(stop.substr(0, comma2));
        stop = stop.substr(comma2 + 1);
        dist = ExtractDistances(stop);
    }
    else {
        place.lng = std::stod(stop);
    }
    Stop s;
    s.name = std::move(name);
    s.place = place;
    return std::make_pair(std::move(s), std::move(dist));
}

}//namespace detail

void CreateAndAdd(std::vector<std::string>&& stops,
    std::unordered_map<Stop*, std::vector<std::pair<std::string, int>>>& temp_insane, TransportCatalogue& cat) {

    for (std::string stop : stops) {
        std::pair<Stop, std::vector<std::pair<std::string, int>>> parsed = detail::Parser(stop);
        std::string name = parsed.first.name;
        cat.AddStop(std::move(parsed.first));
        if (!parsed.second.empty()) {
            temp_insane[cat.FindStop(name).value()] = parsed.second;
        }
    }
}

void AddDistances(std::unordered_map<Stop*, std::vector<std::pair<std::string, int>>>&& temp_insane, TransportCatalogue& cat) {
    for (const auto [stop1, dist] : temp_insane) {
        for (const auto [st2, dm] : dist) {
            Stop* stop2 = cat.FindStop(st2).value();
            cat.AddDistances(std::make_pair((std::make_pair(stop1, stop2)), dm));
        }
    }
}

}//namespace stops

void Process(TransportCatalogue& cat) {

    std::vector<std::string> buffer = query_sort::ReadAndPutIntoBuffer();

    int q_amount = buffer.size();
    std::vector<std::string> buses;
    std::vector<std::string> stops;
    buses.reserve(q_amount);
    stops.reserve(q_amount);
    for (int i = 0; i < q_amount; ++i) {
        query_sort::Query q = query_sort::Sorter(std::move(buffer[i]));
        switch (q.type) {
        case query_sort::QueryType::BUS:
            buses.push_back(std::move(q.text));
            break;
        case query_sort::QueryType::STOP:
            stops.push_back(std::move(q.text));
            break;
        case query_sort::QueryType::EMPTY:
            break;
        }
    }

    std::unordered_map<Stop*, std::vector<std::pair<std::string, int>>> temp_insane; //vector stops, vector of vectors of pairs

    stops::CreateAndAdd(std::move(stops), temp_insane, cat);

    stops::AddDistances(std::move(temp_insane), cat); //можно в async

    buses::CreateAndAdd(std::move(buses), cat);
}

}//namespace in_query

}//namespace tr_cat