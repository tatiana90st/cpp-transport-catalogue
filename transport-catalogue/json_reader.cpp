#include "json_reader.h"
#include "json.h"
#include "json_builder.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "domain.h"
#include "transport_router.h"
#include "serialization.h"
//#include "log_duration.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <algorithm>
#include <filesystem>

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;


JSONReader::JSONReader(tr_cat::TransportCatalogue& transport_catalogue)
    :transport_catalogue_(transport_catalogue)
    , rh_(transport_catalogue_) {
}

//без сериализации
json::Document JSONReader::ProcessJSON(std::istream& input) {
    json::Document doc = json::Load(input);
    doc_ = &doc;
    CreateAndAddStops();
    AddDistances();
    CreateAndAddBuses();
    ReadRenderSettings();
    ReadRouterSettings();
    ReadSerializationSettings();
    json::Document result = StatRequestsHandler();
    return result;
}

json::Document ProcessRequests(std::istream& input) {
    tr_cat::TransportCatalogue tr_cat;
    JSONReader j_read(tr_cat);
    json::Document result = j_read.ProcessRequests(input);
    return result;
}

json::Document JSONReader::ProcessRequests(std::istream& input) {
   // LOG_DURATION("Process requests"s);
    json::Document doc = json::Load(input);
    doc_ = &doc;
    ReadSerializationSettings();
    FillBase();
    json::Document result = StatRequestsHandler();
    return result;
}

void JSONReader::MakeBase(std::istream& input) {
    //LOG_DURATION("Make base"s);
    json::Document doc = json::Load(input);
    doc_ = &doc;
    CreateAndAddStops();
    AddDistances();
    CreateAndAddBuses();
    ReadRenderSettings();
    ReadRouterSettings();
    ReadSerializationSettings();
    TransportRouter tr_router(transport_catalogue_, r_set_, rh_);
    SerializeBase(tr_router);
}

void JSONReader::FillBase() {
    std::filesystem::path in_file = serialization_set_.file_name;
    std::ifstream in(in_file, std::ios::binary);
    tr_router_ = serial::DeserializeTrCatalogue(in, transport_catalogue_, settings_, r_set_, rh_); //logic lost to time
}

void JSONReader::SerializeBase(const TransportRouter& tr_router) {
    std::filesystem::path out_file = serialization_set_.file_name;
    std::ofstream out(out_file, std::ios::binary);
    serial::SerializeTrCatalogue(out, rh_, settings_, tr_router);
}

void JSONReader::CreateAndAddStops() {
    std::string req_format = "base_requests"s;
    if (!CheckReqFormat(req_format)) {
        return;
    }
    for (const json::Node& request : doc_->GetRoot().AsDict().at(req_format).AsArray()) {
        if (request.IsDict()) {
            json::Dict request_map = request.AsDict();
            if (!request_map.empty() && request_map.count("type"s) && request_map.at("type"s) == "Stop"s) {
                
                if (!request_map.count("name"s) || !request_map.count("latitude"s) || !request_map.count("latitude"s)
                    || !request_map.count("road_distances"s)) {
                    throw ReadJSONError("Unexpected format of AddStop request");
                }
                Stop stop = domain::MakeStop(request_map.at("name"s).AsString(), request_map.at("latitude"s).AsDouble(), request_map.at("longitude"s).AsDouble());
                Stop* stop_ptr = transport_catalogue_.AddStop(std::move(stop));
                distances_to_process_[stop_ptr] = ParseDistances(request_map.at("road_distances"s).AsDict());
            }
        }
        else {
            throw ReadJSONError("Unexpected format of request");
        }
    }
}

void JSONReader::AddDistances() {
    for (const auto& [stop1, dist] : distances_to_process_) {
        for (const auto& [st2, dm] : dist) {
            auto st = transport_catalogue_.FindStop(st2);
            if (st.has_value()) {
                Stop* stop2 = st.value();
                transport_catalogue_.AddDistances(std::make_pair((std::make_pair(stop1, stop2)), dm));
            }
        }
    }
}

void JSONReader::CreateAndAddBuses() {
    std::string req_format = "base_requests"s;
    if (!CheckReqFormat(req_format)) {
        return;
    }
    for (const json::Node& request : doc_->GetRoot().AsDict().at(req_format).AsArray()) {
        json::Dict request_map = request.AsDict();
        if (!request_map.empty() && request_map.count("type"s) && request_map.at("type"s).AsString() == "Bus"s) {
            Bus bus;
            if (!request_map.count("name"s) || !request_map.count("is_roundtrip"s) || !request_map.count("stops"s)) {
                throw ReadJSONError("Unexpected format of AddBus request");
            }
            bus.name = request_map.at("name"s).AsString();
            bus.is_round = request_map.at("is_roundtrip"s).AsBool();
            ProcessRoute(bus, request_map.at("stops"s));
            transport_catalogue_.AddBus(std::move(bus));
        }
    }
}

void JSONReader::ReadRenderSettings() {
    std::string settings_type = "render_settings"s;
    if (!CheckSettingsReqFormat(settings_type)) {
        return;
    }
    std::map<std::string, json::Node> map_set = doc_->GetRoot().AsDict().at(settings_type).AsDict();
    settings_.width = map_set.at("width"s).AsDouble();
    settings_.height = map_set.at("height"s).AsDouble();
    settings_.padding = map_set.at("padding"s).AsDouble();
    settings_.stop_radius = map_set.at("stop_radius"s).AsDouble();
    settings_.line_width = map_set.at("line_width"s).AsDouble();
    settings_.bus_label_font_size = map_set.at("bus_label_font_size"s).AsInt();

    json::Array arr = map_set.at("bus_label_offset"s).AsArray();
    settings_.bus_label_offset.x = arr[0].AsDouble();
    settings_.bus_label_offset.y = arr[1].AsDouble();
    settings_.stop_label_font_size = map_set.at("stop_label_font_size"s).AsInt();
    settings_.bus_label_font_size = map_set.at("bus_label_font_size"s).AsInt();
    arr = map_set.at("stop_label_offset"s).AsArray();
    settings_.stop_label_offset.x = arr[0].AsDouble();
    settings_.stop_label_offset.y = arr[1].AsDouble();

    settings_.underlayer_color = ParseColor(map_set.at("underlayer_color"s));
    settings_.underlayer_width = map_set.at("underlayer_width"s).AsDouble();
    arr = map_set.at("color_palette"s).AsArray();
    for (const auto& j : arr) {
        settings_.color_palette.push_back(ParseColor(j));
    }
}

void JSONReader::ReadRouterSettings() {
    std::string settings_type = "routing_settings"s;
    if (!CheckSettingsReqFormat(settings_type)) {
        return;
    }
    std::map<std::string, json::Node> map_set = doc_->GetRoot().AsDict().at(settings_type).AsDict();
    r_set_.bus_velocity_ =  map_set.at("bus_velocity"s).AsDouble();
    r_set_.bus_wait_time_ = map_set.at("bus_wait_time"s).AsInt();
}

void JSONReader::ReadSerializationSettings() {
    std::string settings_type = "serialization_settings"s;
    if (!CheckSettingsReqFormat(settings_type)) {
        return;
    }
    std::map<std::string, json::Node> map_set = doc_->GetRoot().AsDict().at(settings_type).AsDict();
    serialization_set_.file_name = map_set.at("file"s).AsString();
}

json::Document JSONReader::StatRequestsHandler() {

    json::Array reply{};
    std::string req_format = "stat_requests"s;
    if (!CheckReqFormat(req_format)) {
        return json::Document{ reply };
    }

    for (const json::Node& request : doc_->GetRoot().AsDict().at(req_format).AsArray()) {
        json::Dict request_map = request.AsDict();

        if (request_map.empty()) {
            continue;
        }
        if (!request_map.count("type"s) || !request_map.count("id"s)) {
            throw ReadJSONError("Unexpected format of stat request");
        }

        if (request_map.at("type"s).AsString() == "Stop"s) {
            bool found = transport_catalogue_.FindStop(request_map.at("name").AsString()).has_value();
            if (!found) {
                reply.emplace_back(ErrorResult(request_map.at("id"s).AsInt()));
                continue;
            }
            json::Array bus_names = BusNames(request_map);
            reply.emplace_back(json::Builder{}.StartDict()
                                                .Key("request_id"s).Value(request_map.at("id"s).AsInt())
                                                .Key("buses"s).Value(bus_names)
                                              .EndDict().Build().AsDict());                               
        }

        if (request_map.at("type"s).AsString() == "Bus"s) {
            std::optional<BusStat> bus_info = rh_.GetBusStat(request_map.at("name").AsString());
            if (!bus_info) {
                reply.emplace_back(ErrorResult(request_map.at("id"s).AsInt()));
                continue;
            }
            reply.emplace_back(json::Builder{}.StartDict()
                                                .Key("request_id"s).Value(request_map.at("id"s).AsInt())
                                                .Key("curvature"s).Value(bus_info.value().curvature)
                                                .Key("route_length"s).Value(bus_info.value().route_length)
                                                .Key("stop_count"s).Value(bus_info.value().stop_count)
                                                .Key("unique_stop_count").Value(bus_info.value().unique_stop_count)
                                              .EndDict().Build().AsDict());

        }

        if (request_map.at("type"s).AsString() == "Map"s) {
            MapRenderer m_rend(settings_, rh_);
            std::string map_s = m_rend.DrawMap();
            reply.emplace_back(json::Builder{}.StartDict()
                                                .Key("request_id"s).Value(request_map.at("id"s).AsInt())
                                                .Key("map"s).Value(map_s)
                                              .EndDict().Build().AsDict());
        }

        if (request_map.at("type"s).AsString() == "Route"s) {
             
            std::optional<std::vector<Item>> items = tr_router_->FindRoute(request_map.at("from"s).AsString(), request_map.at("to"s).AsString());
            if (!items) {
                reply.emplace_back(ErrorResult(request_map.at("id"s).AsInt()));
                continue;
            }
            std::pair<json::Array, double> route_inf = RouteItems(items.value());

            reply.emplace_back(json::Builder{}.StartDict()
                                                .Key("request_id"s).Value(request_map.at("id"s).AsInt())
                                                .Key("total_time"s).Value(route_inf.second)
                                                .Key("items"s).Value(route_inf.first)
                                              .EndDict().Build().AsDict());
        }


    }
    return json::Document{ reply };
}

json::Array JSONReader::BusNames(json::Dict& request_map) {
    const std::set<Bus*> buses_for_stop = rh_.GetBusesByStop(request_map.at("name").AsString());
    json::Array bus_names;
    if (!buses_for_stop.empty()) {
        bus_names.reserve(buses_for_stop.size());
        for (const auto& bus : buses_for_stop) {
            bus_names.push_back(bus->name);
        }

        std::sort(bus_names.begin(), bus_names.end(),
            [](const json::Node& node_lhs, const json::Node& node_rhs)
            { return node_lhs.AsString() < node_rhs.AsString(); });
    }
    return bus_names;
}

std::pair<json::Array, double> JSONReader::RouteItems(std::vector<Item> items) {
    json::Array items_json;
    double total_time = 0.;
    for (const Item& item : items) {
        total_time += item.time;
        json::Dict reply_item;
        reply_item["type"s] = item.type;
        reply_item["time"s] = item.time;
        if (item.type == "Wait"s) {
            reply_item["stop_name"s] = std::string(item.name);
        }
        else {
            reply_item["bus"s] = std::string(item.name);
            reply_item["span_count"s] = item.span_count;
        }

        items_json.emplace_back(reply_item);
    }
    return { std::move(items_json), total_time };
}

std::map<std::string, int> JSONReader::ParseDistances(const json::Dict& distances) {
    std::map<std::string, int>result;
    for (const auto& [key, value] : distances) {
        result[key] = value.AsInt();
    }
    return result;
}

void JSONReader::ProcessRoute(Bus& bus, json::Node& node) {
    for (const json::Node& stop_on_route : node.AsArray()) {
        std::optional<Stop*> st = transport_catalogue_.FindStop(stop_on_route.AsString());
        if (st.has_value()) {
            Stop* stop = st.value();
            bus.route.push_back(stop);
            bus.unique_stops.insert(stop);
        }
    }
    if (!bus.is_round) {
        bus.half_route = bus.route;
        bus.route.reserve(bus.route.size() * 2);
        for (int i = bus.route.size() - 2; i > -1; --i) {
            bus.route.push_back(bus.route[i]);
        }
    }
}

svg::Color JSONReader::ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();
    }
    const auto& array = node.AsArray();
    uint8_t red = array.at(0).AsInt();
    uint8_t green = array.at(1).AsInt();
    uint8_t blue = array.at(2).AsInt();
    if (array.size() == 3) {
        return svg::Rgb(red, green, blue);
    }
    double alpha = array.at(3).AsDouble();
    return svg::Rgba(red, green, blue, alpha);
}

bool JSONReader::CheckReqFormat(std::string& req_type) {
    if (!doc_->GetRoot().IsDict() || doc_->GetRoot().AsDict().empty()) {
        return false;
    }
    if (!doc_->GetRoot().AsDict().count(req_type)) {
        return false;
    }
    if (!doc_->GetRoot().AsDict().at(req_type).IsArray() || doc_->GetRoot().AsDict().at(req_type).AsArray().empty()) {
        return false;
    }
    return true;
}


bool JSONReader::CheckSettingsReqFormat(std::string& settings_type) {
    if (!doc_->GetRoot().IsDict() || doc_->GetRoot().AsDict().empty()) {
        return false;
    }
    if (!doc_->GetRoot().AsDict().count(settings_type)) {
        return false;
    }
    if (!doc_->GetRoot().AsDict().at(settings_type).IsDict() || doc_->GetRoot().AsDict().at(settings_type).AsDict().empty()) {
        return false;
    }
    return true;
}

json::Dict JSONReader::ErrorResult(int id) {
    return json::Builder{}.StartDict()
                            .Key("request_id"s).Value(id)
                            .Key("error_message"s).Value("not found"s)
                            .EndDict()
                          .Build().AsDict();
}
    

