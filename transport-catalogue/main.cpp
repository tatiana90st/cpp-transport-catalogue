#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"


int main() {
	tr_cat::TransportCatalogue cat;
	tr_cat::in_query::Process(cat);
	tr_cat::out_query::Process(cat);
}