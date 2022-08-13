#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <iostream>

#include "transport_catalogue.h"
#include "geo.h"

namespace tr_cat {

namespace query_sort {

enum class QueryType {
    EMPTY,
    BUS,
    STOP,
};

struct Query {
    QueryType type = QueryType::EMPTY;
    std::string text;
};

Query Sorter(std::string&& query);

std::vector<std::string> ReadAndPutIntoBuffer(std::istream& input);

}//namespace query_sort

namespace in_query {

namespace buses {

namespace detail {

std::pair< std::vector<std::string_view>, bool> SplitIntoStops(std::string_view str);

Bus Parser(std::string& bus, const TransportCatalogue& cat);

}//namespace detail

void CreateAndAdd(std::vector<std::string>&& buses, TransportCatalogue& cat);

}//namespace buses

namespace stops {

namespace detail {

std::vector<std::pair<std::string, int>> ExtractDistances(std::string_view s);

std::pair<Stop, std::vector<std::pair<std::string, int>>> Parser(std::string& stop);

}//namespace detail

void CreateAndAdd(std::vector<std::string>&& stops,
    std::unordered_map<Stop*, std::vector<std::pair<std::string, int>>>& temp_insane, TransportCatalogue& cat);

void AddDistances(std::unordered_map<Stop*, std::vector<std::pair<std::string, int>>>&& temp_insane, TransportCatalogue& cat);

}//namespace stops

void Process(TransportCatalogue& cat, std::istream& input);

}//namespace in_query

}//namespace tr_cat