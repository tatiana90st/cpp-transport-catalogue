#include "json_reader.h"
#include "transport_catalogue.h"


int main() {

    tr_cat::TransportCatalogue transport_catalogue;

    json::Document to_print = ProcessJSON(transport_catalogue, std::cin);
    
    json::Print(to_print, std::cout);
}