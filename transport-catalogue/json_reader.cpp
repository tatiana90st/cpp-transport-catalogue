#include "json_reader.h"
#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "domain.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <algorithm>

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;


JSONReader::JSONReader(tr_cat::TransportCatalogue& transport_catalogue)
    :transport_catalogue_(transport_catalogue)
    ,rh_(transport_catalogue_) {
}
json::Document JSONReader::ProcessJSON(std::istream& input) {
    json::Document doc = json::Load(input);
    CreateAndAddStops(doc);
    AddDistances();
    CreateAndAddBuses(doc);
    ReadSettings(doc);
    json::Document result = StatRequestsHandler(doc);
    return result;
}

void JSONReader::CreateAndAddStops(json::Document& doc) {
    if (!CheckBaseReqFormat(doc)) {
        return;
    }
    for (const json::Node& request : doc.GetRoot().AsMap().at("base_requests"s).AsArray()) {
        if (request.IsMap()) {
            json::Dict request_map = request.AsMap();
            if (!request_map.empty() && request_map.count("type"s) && request_map.at("type"s) == "Stop"s) {
                Stop stop;
                if (!request_map.count("name"s) || !request_map.count("latitude"s) || !request_map.count("latitude"s)
                    || !request_map.count("road_distances"s)) {
                    throw ReadJSONError("Unexpected format of AddStop request");
                }
                stop.name = request_map.at("name"s).AsString();
                std::string name_copy = stop.name;
                stop.place.lat = request_map.at("latitude"s).AsDouble();
                stop.place.lng = request_map.at("longitude"s).AsDouble();
                transport_catalogue_.AddStop(std::move(stop));
                Stop* stop_ptr = transport_catalogue_.FindStop(std::move(name_copy)).value();
                distances_to_process_[stop_ptr] = ParseDistances(request_map.at("road_distances"s).AsMap());
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

void JSONReader::CreateAndAddBuses(json::Document& doc) {
    if (!CheckBaseReqFormat(doc)) {
        return;
    }
    for (const json::Node& request : doc.GetRoot().AsMap().at("base_requests"s).AsArray()) {
        json::Dict request_map = request.AsMap();
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

void JSONReader::ReadSettings(json::Document& doc) {
    if (!CheckSettingsReqFormat(doc)) {
        return;
    }
    std::map<std::string, json::Node> map_set = doc.GetRoot().AsMap().at("render_settings"s).AsMap();
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

json::Document JSONReader::StatRequestsHandler(json::Document& doc) {
    json::Array reply{};
    if (!CheckStatReqFormat(doc)) {
        return json::Document{ reply };
    }
    for (const json::Node& request : doc.GetRoot().AsMap().at("stat_requests"s).AsArray()) {
        json::Dict request_map = request.AsMap();
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
            const std::unordered_set<Bus*> buses_for_stop = rh_.GetBusesByStop(request_map.at("name").AsString());

            json::Array bus_names;
            for (const auto& bus : buses_for_stop) {
                bus_names.push_back(bus->name);
            }
            std::sort(bus_names.begin(), bus_names.end(),
                [](const json::Node& node_lhs, const json::Node& node_rhs)
                { return node_lhs.AsString() < node_rhs.AsString(); });
            reply.emplace_back(json::Dict{
                {"request_id"s, request_map.at("id"s).AsInt()},
                {"buses"s, bus_names}
                });
        }
        if (request_map.at("type"s).AsString() == "Bus"s) {
            std::optional<BusStat> bus_info = rh_.GetBusStat(request_map.at("name").AsString());
            if (!bus_info) {
                reply.emplace_back(ErrorResult(request_map.at("id"s).AsInt()));
                continue;
            }
            reply.emplace_back(json::Dict{
                {"request_id"s, request_map.at("id"s).AsInt()},
                {"curvature"s, bus_info.value().curvature},
                {"route_length"s, bus_info.value().route_length},
                {"stop_count"s, bus_info.value().stop_count},
                {"unique_stop_count", bus_info.value().unique_stop_count}
                });
        }
        if (request_map.at("type"s).AsString() == "Map"s) {
            MapRenderer m_rend(settings_, rh_);
            std::string map_s = m_rend.DrawMap();
            reply.emplace_back(json::Dict{
                {"request_id"s, request_map.at("id"s).AsInt()},
                {"map"s, map_s}
                });
        }
    }
    return json::Document{ reply };
}

bool JSONReader::CheckBaseReqFormat(const json::Document& doc) {
    if (!doc.GetRoot().IsMap() || doc.GetRoot().AsMap().empty()) {
        return false;
    }
    if (!doc.GetRoot().AsMap().count("base_requests"s)) {
        return false;
    }
    if (!doc.GetRoot().AsMap().at("base_requests"s).IsArray() || doc.GetRoot().AsMap().at("base_requests"s).AsArray().empty()) {
        return false;
    }
    return true;
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
        if (transport_catalogue_.FindStop(stop_on_route.AsString()).has_value()) {
            Stop* stop = transport_catalogue_.FindStop(stop_on_route.AsString()).value();
            bus.route.push_back(stop);
            bus.unique_stops.insert(stop);
        }
    }
    if (!bus.is_round) {
        bus.route.reserve(bus.route.size() * 2);
        for (int i = bus.route.size() - 2; i > -1; --i) {
            bus.route.push_back(bus.route[i]);
        }
    }
}

bool JSONReader::CheckSettingsReqFormat(const json::Document& doc) {
    if (!doc.GetRoot().IsMap() || doc.GetRoot().AsMap().empty()) {
        return false;
    }
    if (!doc.GetRoot().AsMap().count("render_settings"s)) {
        return false;
    }
    if (!doc.GetRoot().AsMap().at("render_settings"s).IsMap() || doc.GetRoot().AsMap().at("render_settings"s).AsMap().empty()) {
        return false;
    }
    return true;
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

bool JSONReader::CheckStatReqFormat(const json::Document& doc) {
    if (!doc.GetRoot().IsMap() || doc.GetRoot().AsMap().empty()) {
        return false;
    }
    if (!doc.GetRoot().AsMap().count("stat_requests"s)) {
        return false;
    }
    if (!doc.GetRoot().AsMap().at("stat_requests"s).IsArray() || doc.GetRoot().AsMap().at("stat_requests"s).AsArray().empty()) {
        return false;
    }
    return true;
}

json::Dict JSONReader::ErrorResult(int id) {
    return {
                {"request_id"s, id},
                {"error_message"s, "not found"s}
    };
}
