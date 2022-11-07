#pragma once
#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "domain.h"
#include "serialization.h"
#include <iostream>
#include <map>
#include <string>


class JSONReader {
public:
    JSONReader(tr_cat::TransportCatalogue& transport_catalogue);

    void MakeBase(std::istream& input);
    json::Document ProcessRequests(std::istream& input);

    json::Document ProcessJSON(std::istream& input);

private:

    tr_cat::TransportCatalogue& transport_catalogue_;
    RequestHandler rh_;
    json::Document* doc_ = nullptr;
    TransportRouter* tr_router_ = nullptr;
    std::unordered_map<Stop*, std::map<std::string, int>> distances_to_process_;
    RenderSettings settings_;
    RouterSettings r_set_;
    serial::SerializationSettings serialization_set_;

    void CreateAndAddStops();
    void AddDistances();
    void CreateAndAddBuses();
    void ReadRenderSettings();
    void ReadRouterSettings();
    void ReadSerializationSettings();
    void SerializeBase(const TransportRouter& tr_router);

    void FillBase();

    json::Document StatRequestsHandler();

    std::map<std::string, int> ParseDistances(const json::Dict& distances);
    void ProcessRoute(Bus& bus, json::Node& node);
    svg::Color ParseColor(const json::Node& node);
    json::Array BusNames(json::Dict& request_map);
    std::pair<json::Array, double> RouteItems(std::vector<Item> items);

    bool CheckReqFormat(std::string& req_type);
    bool CheckSettingsReqFormat(std::string& settings_type);

    json::Dict ErrorResult(int id);
};

class ReadJSONError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

json::Document ProcessRequests(std::istream& input);