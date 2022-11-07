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

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::set<Bus*> GetBusesByStop(const std::string_view& stop_name) const;

    const std::map<std::string_view, Bus*> GetAllBusesWithRoutesAndSorted() const;

    const std::map<std::string_view, Stop*> GetAllStopsWithBusesAndSorted() const;

    const std::map<std::string_view, Stop*>& GetStopsIndex() const;

    const std::map<std::string_view, Bus*> GetAllBuses() const;

    int GetDistanceBtwStops(std::string_view first_name, std::string_view second_name) const;

    const std::unordered_map<std::pair<Stop*, Stop*>, int, tr_cat::detail::PairPtrHasher>& GetDistances() const;

private:

    const tr_cat::TransportCatalogue& db_;
};
