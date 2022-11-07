#pragma once
#include "transport_catalogue.h"
#include "domain.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
#include <string>
#include <transport_catalogue.pb.h>
#include <fstream>
#include <unordered_map>
#include <map>

namespace serial {
struct SerializationSettings {
	std::string file_name;
};

class Serializator {
public:
	Serializator() = default;
	Serializator(tc_serialize::TransportBase db_);

	void BuildStops(const std::map<std::string_view, Stop*>& stops);
	void BuildDistances(const std::unordered_map<std::pair<Stop*, Stop*>, int, tr_cat::detail::PairPtrHasher>& distances);
	void BuildBuses(const std::map<std::string_view, Bus*> buses);
	void BuildRenderSettings(const RenderSettings& rend_set);
	void BuildRouter(const TransportRouter& tr_router);
	void SaveBaseToFile(std::ofstream& out_file);

	void BuildCatalogue(tr_cat::TransportCatalogue& tr_cat);
	void AddSettings(RenderSettings& rend_set);

	graph::DirectedWeightedGraph<double>* EctractGraph();
	graph::Router<double>* ExtractRouter(const graph::DirectedWeightedGraph<double>& graph);
	void SetRouterSettings(RouterSettings& r_set);
	TransportRouter* ExtractTrRouter(tr_cat::TransportCatalogue& tr_cat, RouterSettings r_set, 
		graph::DirectedWeightedGraph<double>* graph, graph::Router<double>* router, RequestHandler& rh);

private:
	tc_serialize::TransportBase db_;
	std::unordered_map<std::string, int> stops_map_;
	std::unordered_map<int32_t, Stop*> id_to_stop_;

	std::unordered_map<std::string, int> buses_map_;
	std::unordered_map<int32_t, Bus*> id_to_bus_;

	void AddStops(tr_cat::TransportCatalogue& tr_cat);
	void AddDistances(tr_cat::TransportCatalogue& tr_cat);
	void AddBuses(tr_cat::TransportCatalogue& tr_cat);
};

void SerializeTrCatalogue(std::ofstream& out_file, const RequestHandler& rh, const RenderSettings& rend_set, const TransportRouter& tr_router);

TransportRouter* DeserializeTrCatalogue(std::ifstream& in, tr_cat::TransportCatalogue& tr_cat, RenderSettings& rend_set, RouterSettings& r_set, RequestHandler& rh);

tc_serialize::Color FormatColor(svg::Color svg_color);

svg::Color TransformColorToSvg(const tc_serialize::Color& serial_color);

}

