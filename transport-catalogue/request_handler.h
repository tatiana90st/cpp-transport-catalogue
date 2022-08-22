#pragma once
#include "transport_catalogue.h"
#include "svg.h"
#include <unordered_set>
#include <optional>
#include <string>

struct BusStat {
    double curvature = 0.0;
    int route_length = 0;
    int stop_count = 0;
    int unique_stop_count = 0;
};

 // ����� RequestHandler ������ ���� ������, ����������� �������������� JSON reader-�
 // � ������� ������������ ����������.
 // ��. ������� �������������� �����: https://ru.wikipedia.org/wiki/�����_(������_��������������)
 class RequestHandler {
 public:
     // MapRenderer ����������� � ��������� ����� ��������� �������
     //RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);
     RequestHandler(const tr_cat::TransportCatalogue& db);

     // ���������� ���������� � �������� (������ Bus)
     std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

     // ���������� ��������, ���������� �����
     //const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;
     const std::unordered_set<tr_cat::Bus*> GetBusesByStop(const std::string_view& stop_name) const;

     // ���� ����� ����� ����� � ��������� ����� ��������� �������
     //svg::Document RenderMap() const;

     const std::map<std::string_view, tr_cat::Bus*> GetAllBusesWithRoutesAndSorted() const;

     const std::map<std::string_view, tr_cat::Stop*> GetAllStopsWithBusesAndSorted() const;

 private:
     // RequestHandler ���������� ��������� �������� "������������ ����������" � "������������ �����"
     const tr_cat::TransportCatalogue& db_;
     //const renderer::MapRenderer& renderer_;
 };
