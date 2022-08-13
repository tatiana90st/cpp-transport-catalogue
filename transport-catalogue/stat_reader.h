#pragma once
#include <string_view>
#include <iostream>
#include "transport_catalogue.h"

namespace tr_cat {

namespace out_query {

namespace detail {

void PrintBusRequestResult(const std::string_view& bus, TransportCatalogue& cat, std::ostream& out);

void PrintStopRequestResult(const std::string_view& stop, TransportCatalogue& cat, std::ostream& out);

}//namespace detail

void Process(TransportCatalogue& cat, std::istream& input, std::ostream& out);

}//namespace out_query

}//namespace tr_cat