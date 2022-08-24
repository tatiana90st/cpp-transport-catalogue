#pragma once
#include <vector>
#include "svg.h"
#include "geo.h"
#include "request_handler.h"
#include "domain.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include<set>
#include <string_view>

using namespace std::literals::string_literals;
struct RenderSettings {
    //width � height � ������ � ������ ����������� � ��������. ������������ ����� � ��������� �� 0 �� 100000.
    double width = 600.0;
    double height = 400.0;
    
    //padding � ������ ���� ����� �� ������ SVG-���������. ������������ ����� �� ������ 0 � ������ min(width, height)/2.
    double padding = 50.0;
    
    //line_width � ������� �����, �������� �������� ���������� ��������. ������������ ����� � ��������� �� 0 �� 100000.
    double line_width = 14.0;
    
    //stop_radius � ������ �����������, �������� ������������ ���������. ������������ ����� � ��������� �� 0 �� 100000.
    double stop_radius = 5.0;
    
    //bus_label_font_size � ������ ������, ������� �������� �������� ���������� ���������. ����� ����� � ��������� �� 0 �� 100000.
    int bus_label_font_size = 20;
    
    //bus_label_offset � �������� ������� � ��������� �������� ������������ ��������� �������� ��������� �� �����.
    svg::Point bus_label_offset;
    
    //stop_label_font_size � ������ ������, ������� ������������ �������� ���������.����� ����� � ��������� �� 0 �� 100000.
    int stop_label_font_size = 20;
    
    //stop_label_offset � �������� �������� ��������� ������������ � ��������� �� �����. ������ �� ���� ��������� ���� double.
    svg::Point stop_label_offset;
    
    //underlayer_color � ���� �������� ��� ���������� ��������� � ���������
    svg::Color underlayer_color = svg::Rgba{ 255, 255, 255, 0.85 };
    
    //underlayer_width � ������� �������� ��� ���������� ��������� � ���������. ����� �������� �������� stroke-width �������� <text>.
    double underlayer_width = 3.0;
    
    //color_palette � �������� �������. �������� ������.
    std::vector<svg::Color> color_palette;
};

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin � points_end ������ ������ � ����� ��������� ��������� geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // ���� ����� ����������� ����� �� ������, ��������� ������
        if (points_begin == points_end) {
            return;
        }

        // ������� ����� � ����������� � ������������ ��������
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // ������� ����� � ����������� � ������������ �������
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // ��������� ����������� ��������������� ����� ���������� x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // ��������� ����������� ��������������� ����� ���������� y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // ������������ ��������������� �� ������ � ������ ���������,
            // ���� ����������� �� ���
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            // ����������� ��������������� �� ������ ���������, ���������� ���
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            // ����������� ��������������� �� ������ ���������, ���������� ���
            zoom_coeff_ = *height_zoom;
        }
    }

    // ���������� ������ � ������� � ���������� ������ SVG-�����������
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    explicit MapRenderer(RenderSettings& settings, RequestHandler& rh);

    void DrawBusRouteLines();

    void AddBusNames();

    void DrawStopCircles();

    void AddStopNames();

    std::string DrawMap();

private:
    const RenderSettings& settings_;
    const RequestHandler& rh_;
    svg::Document image_;
    std::map<std::string_view, Bus*> buses_;
    std::map<std::string_view, Stop*> stops_;

    std::vector<geo::Coordinates> geo_coords_;
    std::vector<geo::Coordinates> CollectCoordinates();

};

