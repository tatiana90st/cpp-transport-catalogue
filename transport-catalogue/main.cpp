#include "json_reader.h"
#include "transport_catalogue.h"
//#include "log_duration.h"


int main() {
    //{
      // LOG_DURATION("whole time"s);
        tr_cat::TransportCatalogue transport_catalogue;
        JSONReader j_read(transport_catalogue);

        json::Document to_print = j_read.ProcessJSON(std::cin);

        json::Print(to_print, std::cout);
   // }
}