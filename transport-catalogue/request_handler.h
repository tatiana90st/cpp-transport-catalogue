#pragma once
#include "transport_catalogue.h"
#include "domain.h"
#include "svg.h"
#include <unordered_set>
#include <optional>
#include <string>


class RequestHandler {
public:
    RequestHandler(const tr_cat::TransportCatalogue& db);

    // ���������� ���������� � �������� (������ Bus)
    std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

    // ���������� ��������, ���������� �����
    const std::unordered_set<Bus*> GetBusesByStop(const std::string_view& stop_name) const;


    const std::map<std::string_view, Bus*> GetAllBusesWithRoutesAndSorted() const;

    const std::map<std::string_view, Stop*> GetAllStopsWithBusesAndSorted() const;

private:
    // RequestHandler ���������� ��������� �������� "������������ ����������" � "������������ �����"
    const tr_cat::TransportCatalogue& db_;
};
