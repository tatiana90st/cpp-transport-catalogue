#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */
using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;

class ReadJSONError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

std::map<std::string, int> ParseDistances(const json::Dict& distances);

bool CheckBaseReqFormat(const json::Document& doc);

void CreateAndAddStops(tr_cat::TransportCatalogue& transport_catalogue, json::Document& doc,
    std::unordered_map<tr_cat::Stop*, std::map<std::string, int>>& distances_to_process);

void AddDistances(std::unordered_map<tr_cat::Stop*, std::map<std::string, int>>&& distances_to_process, tr_cat::TransportCatalogue& cat);

void ProcessRoute(tr_cat::Bus& bus, json::Node& node, tr_cat::TransportCatalogue& transport_catalogue);

void CreateAndAddBuses(tr_cat::TransportCatalogue& transport_catalogue, json::Document& doc);

bool CheckStatReqFormat(const json::Document& doc);

json::Dict ErrorResult(int id);

json::Document StatRequestsHandler(json::Document& doc, tr_cat::TransportCatalogue& transport_catalogue,
    RequestHandler& rh, RenderSettings& settings);

bool CheckSettingsReqFormat(const json::Document& doc);

svg::Color ParseColor(const json::Node& node);

void ReadSettings(json::Document& doc, RenderSettings& settings);

json::Document ProcessJSON(tr_cat::TransportCatalogue& transport_catalogue, std::istream& input);

