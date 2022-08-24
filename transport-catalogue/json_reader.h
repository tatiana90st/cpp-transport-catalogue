#pragma once
#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "domain.h"
#include <iostream>
#include <map>
#include <string>


class JSONReader {
public:
    JSONReader(tr_cat::TransportCatalogue& transport_catalogue);

    json::Document ProcessJSON(std::istream& input);

    void CreateAndAddStops(json::Document& doc);

    void AddDistances();

    void CreateAndAddBuses(json::Document& doc);

    void ReadSettings(json::Document& doc);

    json::Document StatRequestsHandler(json::Document& doc);

private:
    tr_cat::TransportCatalogue& transport_catalogue_;
    RequestHandler rh_;
    std::unordered_map<Stop*, std::map<std::string, int>> distances_to_process_;
    RenderSettings settings_;

    bool CheckBaseReqFormat(const json::Document& doc);

    std::map<std::string, int> ParseDistances(const json::Dict& distances);

    void ProcessRoute(Bus& bus, json::Node& node);

    bool CheckSettingsReqFormat(const json::Document& doc);

    svg::Color ParseColor(const json::Node& node);

    bool CheckStatReqFormat(const json::Document& doc);

    json::Dict ErrorResult(int id);

};

class ReadJSONError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};
