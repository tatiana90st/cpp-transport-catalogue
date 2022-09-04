#include <vector>
#include "svg.h"
#include "geo.h"
#include "request_handler.h"
#include "map_renderer.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <string_view>
#include <sstream>

using namespace std::literals::string_literals;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}


// Проецирует широту и долготу в координаты внутри SVG-изображения
svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

MapRenderer::MapRenderer(RenderSettings& settings, RequestHandler& rh)
    :settings_(settings)
    , rh_(rh) {
    buses_ = rh_.GetAllBusesWithRoutesAndSorted();
    geo_coords_ = CollectCoordinates();
    stops_ = rh_.GetAllStopsWithBusesAndSorted();

}
void MapRenderer::DrawBusRouteLines() {
    SphereProjector proj{
    geo_coords_.begin(), geo_coords_.end(), settings_.width, settings_.height, settings_.padding
    };
    int bus_index = 0;
    for (const auto& [name, bus_ptr] : buses_) {
        svg::Polyline bus_route_line;
        for (const auto& stop_ptr : bus_ptr->route) {
            bus_route_line.AddPoint(proj(stop_ptr->place));
        }
        int c_pal_index = bus_index % settings_.color_palette.size();
        bus_route_line.SetStrokeColor(settings_.color_palette[c_pal_index]).SetStrokeWidth(settings_.line_width).SetFillColor("none"s);
        bus_route_line.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        ++bus_index;
        image_.Add(bus_route_line);
    }
}

void MapRenderer::AddBusNames() {
    SphereProjector proj{
    geo_coords_.begin(), geo_coords_.end(), settings_.width, settings_.height, settings_.padding
    };
    int bus_index = 0;
    for (const auto& [name, bus_ptr] : buses_) {
        const svg::Text base_text =
            svg::Text()
            .SetPosition(proj(bus_ptr->route[0]->place))
            .SetOffset({ settings_.bus_label_offset })
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold"s)
            .SetData(std::string(name));
        //подложка
        image_.Add(svg::Text{ base_text }
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND));
        //надпись
        int c_pal_index = bus_index % settings_.color_palette.size();
        image_.Add(svg::Text{ base_text }
        .SetFillColor(settings_.color_palette[c_pal_index]));
        if (!bus_ptr->is_round) {
            int end_route_index = bus_ptr->route.size() / 2;
            if (bus_ptr->route[end_route_index]->name == bus_ptr->route[0]->name) {
                //отрисовывать дважды одно название в одном и том же месте не нужно, даже если маршрут некольцевой
                ++bus_index;
                continue;
            }
            const svg::Text base_text =
                svg::Text()
                .SetPosition(proj(bus_ptr->route[end_route_index]->place))
                .SetOffset({ settings_.bus_label_offset })
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFontWeight("bold"s)
                .SetData(std::string(name));
            //подложка
            image_.Add(svg::Text{ base_text }
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND));
            //надпись
            image_.Add(svg::Text{ base_text }
            .SetFillColor(settings_.color_palette[c_pal_index]));
        }
        ++bus_index;
    }

}

void MapRenderer::DrawStopCircles() {
    //const std::map<std::string_view, tr_cat::Stop*> stops = rh_.GetAllStopsWithBusesAndSorted();
    SphereProjector proj{
    geo_coords_.begin(), geo_coords_.end(), settings_.width, settings_.height, settings_.padding
    };
    for (const auto& [name, stop_ptr] : stops_) {
        svg::Circle c;
        c.SetCenter(proj(stop_ptr->place)).SetRadius(settings_.stop_radius).SetFillColor("white"s);
        image_.Add(c);
    }
}

void MapRenderer::AddStopNames() {
    SphereProjector proj{
        geo_coords_.begin(), geo_coords_.end(), settings_.width, settings_.height, settings_.padding
    };
    for (const auto& [name, stop_ptr] : stops_) {
        const svg::Text base_text =
            svg::Text()
            .SetPosition(proj(stop_ptr->place))
            .SetOffset({ settings_.stop_label_offset })
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetData(std::string(name));
        //подложка
        image_.Add(svg::Text{ base_text }
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND));
        //надпись
        image_.Add(svg::Text{ base_text }
        .SetFillColor("black"s));
    }
}

std::string MapRenderer::DrawMap() {
    DrawBusRouteLines();
    AddBusNames();
    DrawStopCircles();
    AddStopNames();
    std::ostringstream ss;
    image_.Render(ss);
    return ss.str();
}

std::vector<geo::Coordinates> MapRenderer::CollectCoordinates() {
    std::vector<geo::Coordinates> geo_coords;
    for (const auto& [name, bus_ptr] : buses_) {
        for (const auto& stop_ptr : bus_ptr->route) {
            geo_coords.push_back(stop_ptr->place);
        }
    }
    return geo_coords;
}
