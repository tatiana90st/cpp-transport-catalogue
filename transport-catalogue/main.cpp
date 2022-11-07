#include "json_reader.h"
#include "transport_catalogue.h"
//#include "log_duration.h"
#include <fstream>
#include <iostream>
#include <string_view>
#include <filesystem>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        tr_cat::TransportCatalogue transport_catalogue;
        JSONReader j_read(transport_catalogue);
        j_read.MakeBase(std::cin);

    }
    else if (mode == "process_requests"sv) {

        json::Document to_print = ProcessRequests(std::cin);
        json::Print(to_print, std::cout);

    }
    else {
        PrintUsage();
        return 1;
    }
}
