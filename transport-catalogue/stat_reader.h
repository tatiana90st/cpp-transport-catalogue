#pragma once
#include <string_view>
#include "transport_catalogue.h"

namespace tr_cat {

namespace out_query {

namespace detail {

void PrintBusRequestResult(const std::string_view& bus, TransportCatalogue& cat);

void PrintStopRequestResult(const std::string_view& stop, TransportCatalogue& cat);

}//namespace detail

void Process(TransportCatalogue& cat);

}//namespace out_query

}//namespace tr_cat